#pragma once
//
// Parser for the Minecraft Bedrock geometry format ("minecraft:geometry")
// as used by the TACZ gun pack. build_mesh() replicates the TACZ
// (Timeless) rendering pipeline for first-person view, so the resulting
// mesh matches exactly what the mod draws in-game.
//
// Conventions (from TACZ source, com.tacz.guns.client.*):
//
//   * Pose space: bedrock coordinates are converted to a y-DOWN space with
//     y' = 24 - y (BedrockModel.convertPivot). A root bone translates by
//     (p.x, 24 - p.y, p.z) / 16; a child bone translates by
//     ((c.x - p.x), (p.y - c.y), (c.z - p.z)) / 16 relative to its parent.
//
//   * Bone local transform (BedrockPart.translateAndRotateAndScale), applied
//     as post-multiplication in this order: T(pivot) then Rz, then Ry, then
//     Rx (i.e. R = Rz * Ry * Rx, x applied first in the local frame).
//
//   * Cube local frame: "origin" is the cube's MIN corner in absolute
//     bedrock coordinates (y up). In the bone's y-down frame the min corner
//     is (o.x - p.x, p.y - o.y - s.y, o.z - p.z) (BedrockModel.convertOrigin).
//     "inflate" grows the box by that amount in every direction.
//
//   * A cube with a "rotation" field is a sub-bone (BedrockModel): it
//     translates by ((cp.x - p.x), (p.y - cp.y), (cp.z - p.z)) / 16 and
//     rotates about its own "pivot"; the cube origin is measured from that
//     pivot.
//
//   * Faces and UVs (BedrockCubePerFace + BedrockPolygon + FaceUVsItem.getFace).
//     With (x, y, z) the inflated min corner and (X, Y, Z) the max corner in
//     the y-down local frame:
//         v1=(x,y,z)  v2=(X,y,z)  v3=(X,Y,z)  v4=(x,Y,z)
//         v5=(x,y,Z)  v6=(X,y,Z)  v7=(X,Y,Z) v8=(x,Y,Z)
//     and the quads (in vertex order) are:
//         down   [v6, v5, v1, v2]  <- JSON "up" uv
//         up     [v3, v4, v8, v7]  <- JSON "down" uv
//         west   [v1, v5, v8, v4]  <- JSON "east" uv
//         north  [v2, v1, v4, v3]  <- JSON "north" uv
//         east   [v6, v2, v3, v7]  <- JSON "west" uv
//         south  [v5, v6, v7, v8]  <- JSON "south" uv
//     ("down"/"up" are MC Direction names in the y-down pose space, so the
//     geometric top face is labelled "down" and takes the JSON "up" uv, etc.)
//     For a face uv (u, v) with size (w, h): u2 = u + w, v2 = v + h and the
//     four quad vertices map to
//         v[0] -> (u2/texW, v1/texH)   v[1] -> (u1/texW, v1/texH)
//         v[2] -> (u1/texW, v2/texH)   v[3] -> (u2/texW, v2/texH)
//     v = 0 is the TOP of the texture (MC convention), so in Vulkan the uv
//     is used as-is (no vertical flip). A negative uv size flips that face
//     automatically.
//
//   * First-person camera chain (GunItemRendererWrapper.renderFirstPerson +
//     FirstPersonRenderGunEvent.applyFirstPersonPositioningTransform), in
//     block units, post-multiplication:
//         M_cam = T(0, 1.5, 0) * Rz(180) * T(0, 1.5, 0) * M_inv * T(0, -1.5, 0)
//     where M_inv is the inverse of the idle camera node path
//     ("views" -> "idle_view"), built like FirstPersonRenderGunEvent.
//     getPositioningNodeInverse: for the path root-to-leaf, processed from
//     the leaf up:
//         M <- M * Rx(-rx) * Ry(-ry) * Rz(-rz)
//         M <- M * T(-p.x/16, -p.y/16, -p.z/16)          (child nodes)
//         M <- M * T(-p.x/16, 1.5 - p.y/16, -p.z/16)    (root nodes)
//     (p in 1/16 converted-pivot units).
//
// The output mesh is in first-person "eye space": the eye sits at the
// origin, x is right, y is up, -z is forward, units are blocks. The camera
// chain above is baked into the vertices; callers just apply a uniform
// scale (and should disable face culling, since MC draws these quads
// unculled).

#include <nlohmann/json.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <core/basic_types.hpp>
#include <core/io/file.hpp>
#include <core/io/mmap.hpp>
#include <sys/open_flags.hpp>

#include <array>
#include <map>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tacz
{
using sys::f32;
using sys::u32;

struct face_uv {
    f32 u{}; // top-left corner in texture pixels
    f32 v{};
    f32 w{}; // size in pixels; negative flips the mapping on that face
    f32 h{};
};

struct cube {
    glm::vec3 origin{};
    glm::vec3 size{};
    f32 inflate = 0.f;
    std::optional<glm::vec3> pivot;
    std::optional<glm::vec3> rotation; // degrees
    std::map<std::string, face_uv> uv; // keys: north/south/east/west/up/down
};

struct bone {
    std::string name;
    std::optional<std::string> parent;
    glm::vec3 pivot{};
    glm::vec3 rotation{}; // degrees
    std::vector<cube> cubes;
};

struct geometry {
    u32 texture_width  = 64;
    u32 texture_height = 64;
    std::vector<bone> bones;
};

// One bone's constant (non-keyframed) animation values from the gun pack's
// "static_idle" animation (format_version 1.8.0): a per-bone
// {position, rotation, scale} in the bedrock (y-up, 1/16) convention.
//   * position: delta from the bone's rest pose, 1/16 units
//   * rotation: additional rotation, degrees (Rz*Ry*Rx, same order as bones)
//   * scale:    scale multipliers (bedrock AnimationKeyframes, applied last)
struct bone_anim {
    std::optional<glm::vec3> position;
    std::optional<glm::vec3> rotation;
    std::optional<glm::vec3> scale;
};

struct mesh_vertex {
    glm::vec3 pos;   // eye space, blocks
    glm::vec2 uv;    // normalized; v = 0 is the top of the texture
    f32       level; // depth-fighting level: the vertex shader shifts the face
                     // away from the eye, along the face normal (view space),
                     // by level * depthBias (eye units)
    glm::vec3 nrm;   // face normal (model space), for the shader-side shift
};

struct mesh {
    std::vector<mesh_vertex> vertices;
    std::vector<u32> indices;
};

namespace details
{
inline glm::mat4 rot_x(f32 rad) { return glm::rotate(glm::mat4(1.f), rad, glm::vec3{1.f, 0.f, 0.f}); }
inline glm::mat4 rot_y(f32 rad) { return glm::rotate(glm::mat4(1.f), rad, glm::vec3{0.f, 1.f, 0.f}); }
inline glm::mat4 rot_z(f32 rad) { return glm::rotate(glm::mat4(1.f), rad, glm::vec3{0.f, 0.f, 1.f}); }
inline glm::mat4 trans(const glm::vec3& v) { return glm::translate(glm::mat4(1.f), v); }

// Converted bone data (BedrockModel.convertPivot / convertRotation)
struct bone_pose {
    glm::vec3 pivot16{}; // converted pivot, 1/16 units (y-down convention included)
    glm::vec3 rot{};     // radians (x, y, z)
};

inline bone_pose convert_bone(const bone& b, const std::map<std::string, const bone*>& by_name) {
    bone_pose p;
    p.rot = glm::radians(b.rotation);
    if (b.parent) {
        auto it = by_name.find(*b.parent);
        if (it == by_name.end())
            throw std::runtime_error("bone '" + b.name + "' has unknown parent '" + *b.parent + "'");
        const glm::vec3& pp = it->second->pivot;
        p.pivot16 = {b.pivot.x - pp.x, pp.y - b.pivot.y, b.pivot.z - pp.z};
    } else {
        p.pivot16 = {b.pivot.x, 24.f - b.pivot.y, b.pivot.z};
    }
    return p;
}

// Bone local transform in block units: T(pivot/16) * Rz * Ry * Rx
inline glm::mat4 bone_local(const bone_pose& p) {
    return trans(p.pivot16 / 16.f) * rot_z(p.rot.z) * rot_y(p.rot.y) * rot_x(p.rot.x);
}

// Bone local transform with a static animation applied
// (BedrockPart.translateAndRotateAndScale + AnimationListeners):
//   T(offset) * T(pivot/16) * Rz * Ry * Rx * Q_anim * Scale
// where offset = (d.x, -d.y, d.z) / 16 (bedrock y-up delta -> y-down blocks).
inline glm::mat4 bone_local(const bone_pose& p, const bone_anim* a) {
    glm::mat4 m = bone_local(p);
    if (a) {
        if (a->rotation)
            m = m * rot_z(glm::radians(a->rotation->z)) * rot_y(glm::radians(a->rotation->y)) * rot_x(glm::radians(a->rotation->x));
        if (a->scale)
            m = m * glm::scale(glm::mat4(1.f), a->scale.value());
        if (a->position)
            m = trans(glm::vec3{a->position->x, -a->position->y, a->position->z} / 16.f) * m;
    }
    return m;
}

// FirstPersonRenderGunEvent.getPositioningNodeInverse for a path from the
// root bone to the camera node (path[0] = root ... path.back() = leaf).
inline glm::mat4 positioning_inverse(std::span<const bone_pose> path, const std::vector<bool>& has_parent) {
    glm::mat4 m{1.f};
    for (size_t i = path.size(); i-- > 0;) {
        const bone_pose& p = path[i];
        m = m * rot_x(-p.rot.x) * rot_y(-p.rot.y) * rot_z(-p.rot.z);
        if (has_parent[i])
            m = m * trans(-p.pivot16 / 16.f);
        else
            m = m * trans(glm::vec3{-p.pivot16.x / 16.f, 1.5f - p.pivot16.y / 16.f, -p.pivot16.z / 16.f});
    }
    return m;
}

// GunItemRendererWrapper.renderFirstPerson idle chain (static, no view lag)
inline glm::mat4 camera_matrix(std::span<const bone_pose> path, const std::vector<bool>& has_parent) {
    return trans({0.f, 1.5f, 0.f}) * rot_z(glm::pi<f32>()) * trans({0.f, 1.5f, 0.f}) * positioning_inverse(path, has_parent) * trans({0.f, -1.5f, 0.f});
}

// Face quad: vertex indices into the 8-corner array (0 = v1 ... 7 = v8),
// the JSON uv key it uses, and the outward normal in the y-down frame.
struct face_def {
    std::array<u32, 4> verts;
    const char* uv_key;
    glm::vec3 normal;
};

inline constexpr std::array<face_def, 6> faces{
    face_def{std::array<u32, 4>{5u, 4u, 0u, 1u}, "up",    {0.f, -1.f, 0.f}}, // down
    face_def{std::array<u32, 4>{2u, 3u, 7u, 6u}, "down",  {0.f, 1.f, 0.f}},  // up
    face_def{std::array<u32, 4>{0u, 4u, 7u, 3u}, "east",  {-1.f, 0.f, 0.f}}, // west
    face_def{std::array<u32, 4>{1u, 0u, 3u, 2u}, "north", {0.f, 0.f, -1.f}}, // north
    face_def{std::array<u32, 4>{5u, 1u, 2u, 6u}, "west",  {1.f, 0.f, 0.f}},  // east
    face_def{std::array<u32, 4>{4u, 5u, 6u, 7u}, "south", {0.f, 0.f, 1.f}},  // south
};
} // namespace details

inline geometry parse_geometry(const nlohmann::json& j)
{
    geometry geo;
    auto     root = j.at("minecraft:geometry").at(0);

    if (root.contains("description")) {
        const auto& desc = root.at("description");
        if (desc.contains("texture_width"))
            geo.texture_width = desc.at("texture_width").get<u32>();
        if (desc.contains("texture_height"))
            geo.texture_height = desc.at("texture_height").get<u32>();
    }

    auto read_v3 = [](const nlohmann::json& v) {
        return glm::vec3{v[0].get<f32>(), v[1].get<f32>(), v[2].get<f32>()};
    };

    for (const auto& jb : root.at("bones")) {
        bone b;
        b.name = jb.at("name").get<std::string>();
        if (jb.contains("parent"))
            b.parent = jb.at("parent").get<std::string>();
        if (jb.contains("pivot"))
            b.pivot = read_v3(jb.at("pivot"));
        if (jb.contains("rotation"))
            b.rotation = read_v3(jb.at("rotation"));

        if (jb.contains("cubes")) {
            for (const auto& jc : jb.at("cubes")) {
                cube c;
                c.origin = read_v3(jc.at("origin"));
                c.size   = read_v3(jc.at("size"));
                if (jc.contains("inflate"))
                    c.inflate = jc.at("inflate").get<f32>();
                if (jc.contains("pivot"))
                    c.pivot = read_v3(jc.at("pivot"));
                if (jc.contains("rotation"))
                    c.rotation = read_v3(jc.at("rotation"));
                if (jc.contains("uv")) {
                    for (auto& [face, v] : jc.at("uv").items()) {
                        face_uv fuv;
                        fuv.u = v.at("uv")[0].get<f32>();
                        fuv.v = v.at("uv")[1].get<f32>();
                        fuv.w = v.at("uv_size")[0].get<f32>();
                        fuv.h = v.at("uv_size")[1].get<f32>();
                        c.uv[face] = fuv;
                    }
                }
                b.cubes.push_back(c);
            }
        }
        geo.bones.push_back(b);
    }
    return geo;
}

inline geometry load_geometry(std::string_view path)
{
    auto file = core::io::file::open(std::string{path}, sys::openflag::read_only);
    auto map  = core::io::mmap{file, sys::map_prot::read, sys::map_flag::priv};
    auto data = map.data<char>();
    return parse_geometry(nlohmann::json::parse(data, data + map.size()));
}

// TACZ hides these parts on a bare gun (no attachments, no extended mag):
//   * mag_extended_1/2/3      (BedrockGunModel: visible only at matching level)
//   * mount                   (scope mount rail, visible only with a scope)
//   * sight_folded            (folded iron sight, visible only with a scope)
//   * additional_magazine     (reload-animation second mag)
//   * handguard_tactical      (visible only with laser/grip installed)
//   * attachment_adapter      (adapter sockets, visible only when attached)
//   * oem_stock_*, *_stock_adapter (alternate stocks shipped in the model)
inline bool hidden_by_default(std::string_view name) {
    return name.starts_with("mag_extended") || name == "mount" || name == "sight_folded" || name == "additional_magazine" || name == "handguard_tactical" || name == "attachment_adapter" || name.starts_with("oem_stock") || name.ends_with("_stock_adapter");
}

// Find the idle camera path (root ... leaf); empty if the model has no
// "idle_view" bone (TACZ then uses an identity positioning matrix).
inline std::pair<std::vector<const bone*>, std::vector<bool>> camera_path(const geometry& geo) {
    std::map<std::string, const bone*> by_name;
    for (auto& b : geo.bones)
        by_name.try_emplace(b.name, &b);

    std::vector<const bone*> path;
    auto it = by_name.find("idle_view");
    if (it != by_name.end()) {
        const bone* b = it->second;
        while (b) {
            path.push_back(b);
            b = b->parent ? by_name.find(*b->parent)->second : nullptr;
        }
        std::reverse(path.begin(), path.end());
    }
    std::vector<bool> has_parent(path.size());
    for (size_t i = 0; i < path.size(); ++i)
        has_parent[i] = path[i]->parent.has_value();
    return {path, has_parent};
}

inline std::map<std::string, bone_anim> parse_idle_animation(const nlohmann::json& j)
{
    std::map<std::string, bone_anim> out;
    auto ait = j.find("animations");
    if (ait == j.end())
        return out;
    auto sit = ait->find("static_idle");
    if (sit == ait->end())
        return out;
    auto bones = sit->find("bones");
    if (bones == sit->end())
        return out;

    auto read_v3 = [](const nlohmann::json& v) {
        return glm::vec3{v[0].get<f32>(), v[1].get<f32>(), v[2].get<f32>()};
    };
    auto read_v3_at = [&read_v3](const nlohmann::json& obj, std::string_view key) -> std::optional<glm::vec3> {
        auto fit = obj.find(key);
        if (fit == obj.end())
            return std::nullopt;
        if (fit->is_array())
            return read_v3(*fit);
        if (fit->is_object() && fit->contains("0.0") && fit->at("0.0").is_object() && fit->at("0.0").contains("data"))
            return read_v3(fit->at("0.0").at("data"));
        return std::nullopt;
    };
    for (auto& [name, b] : bones->items()) {
        bone_anim a{
            read_v3_at(b, "position"),
            read_v3_at(b, "rotation"),
            read_v3_at(b, "scale"),
        };
        if (a.position || a.rotation || a.scale)
            out[name] = a;
    }
    return out;
}

// Load the gun pack animation file and return the per-bone constant values
// of the "static_idle" animation (throws if the file is missing).
inline std::map<std::string, bone_anim> load_idle_animation(std::string_view path)
{
    auto file = core::io::file::open(std::string{path}, sys::openflag::read_only);
    auto map  = core::io::mmap{file, sys::map_prot::read, sys::map_flag::priv};
    auto data = map.data<char>();
    return parse_idle_animation(nlohmann::json::parse(data, data + map.size()));
}

// face_dbg: if non-null, build_mesh writes one line per collected face:
// "face <draw order> nq=<...> dq=<...>"
inline mesh build_mesh(const geometry& geo, const std::map<std::string, bone_anim>& anims = {}, std::ostream* face_dbg = nullptr)
{
    mesh out;

    std::map<std::string, const bone*> by_name;
    for (auto& b : geo.bones)
        by_name.try_emplace(b.name, &b);

    // Camera chain, baked into the vertices
    auto [cam_path, cam_has_parent] = camera_path(geo);
    std::vector<details::bone_pose> cam_poses;
    for (auto* b : cam_path)
        cam_poses.push_back(details::convert_bone(*b, by_name));
    const glm::mat4 cam = cam_path.empty()
                              ? glm::mat4{1.f}
                              : details::camera_matrix(std::span<const details::bone_pose>{cam_poses.data(), cam_poses.size()}, cam_has_parent);

    const f32 tex_w = f32(geo.texture_width);
    const f32 tex_h = f32(geo.texture_height);

    // Faces are collected in draw order first; after the walk, faces sharing
    // an exact plane are made not to overlap (see the de-fighting pass).
    struct raw_face {
        glm::vec3 p[4];
        glm::vec2 uv[4];
        glm::vec3 nrm{}; // face normal (normalized)
        f32       d{};   // plane offset nrm . p
    };
    std::vector<raw_face> faces;

    auto emit_cube = [&](const glm::mat4& M, const glm::vec3& local_min, const glm::vec3& local_max, const cube& c) {
        const glm::vec3 v[8] = {
            local_min,
            {local_max.x, local_min.y, local_min.z},
            {local_max.x, local_max.y, local_min.z},
            {local_min.x, local_max.y, local_min.z},
            {local_min.x, local_min.y, local_max.z},
            {local_max.x, local_min.y, local_max.z},
            local_max,
            {local_min.x, local_max.y, local_max.z},
        };

        for (const auto& f : details::faces) {
            auto uit = c.uv.find(f.uv_key);
            if (uit == c.uv.end())
                continue;
            const face_uv& uv = uit->second;
            const f32 u2      = uv.u + uv.w;
            const f32 v2      = uv.v + uv.h;
            const glm::vec2 t[4] = {
                {u2 / tex_w, uv.v / tex_h},
                {uv.u / tex_w, uv.v / tex_h},
                {uv.u / tex_w, v2 / tex_h},
                {u2 / tex_w, v2 / tex_h},
            };

            raw_face rf;
            for (u32 i = 0; i < 4; ++i) {
                rf.p[i]  = glm::vec3(M * glm::vec4(v[f.verts[i]] / 16.f, 1.f));
                rf.uv[i] = t[i];
            }
            const glm::vec3 n = glm::normalize(glm::cross(rf.p[1] - rf.p[0], rf.p[2] - rf.p[0]));
            rf.nrm = n;
            rf.d   = glm::dot(n, rf.p[0]);
            faces.push_back(rf);
        }
    };

    auto render_bone = [&](const bone& b, const glm::mat4& parent_m, auto&& self) -> void {
        if (hidden_by_default(b.name)) // skip the whole subtree
            return;
        const auto pose  = details::convert_bone(b, by_name);
        const auto ait   = b.name == "constraint" ? anims.end() : anims.find(b.name); // "constraint" is a special node, not a plain bone
        const glm::mat4 M = parent_m * details::bone_local(pose, ait == anims.end() ? nullptr : &ait->second);

        for (const auto& c : b.cubes) {
            if (c.size == glm::vec3{0.f, 0.f, 0.f})
                continue;

            if (c.rotation) {
                // Rotated cube = sub-bone pivoted at c->pivot
                const glm::vec3& cp  = *c.pivot;
                const glm::mat4 sub = M * details::trans({(cp.x - b.pivot.x) / 16.f, (b.pivot.y - cp.y) / 16.f, (cp.z - b.pivot.z) / 16.f})
                                    * details::rot_z(glm::radians(c.rotation->z)) * details::rot_y(glm::radians(c.rotation->y)) * details::rot_x(glm::radians(c.rotation->x));
                const glm::vec3 min16 = {c.origin.x - cp.x, cp.y - c.origin.y - c.size.y, c.origin.z - cp.z};
                emit_cube(sub, min16 - c.inflate, min16 + c.size + c.inflate, c);
            } else {
                const glm::vec3 min16 = {c.origin.x - b.pivot.x, b.pivot.y - c.origin.y - c.size.y, c.origin.z - b.pivot.z};
                emit_cube(M, min16 - c.inflate, min16 + c.size + c.inflate, c);
            }
        }

        for (auto& b2 : geo.bones) {
            if (b2.parent && *b2.parent == b.name)
                self(b2, M, self);
        }
    };

    for (auto& b : geo.bones) {
        if (!b.parent)
            render_bone(b, cam, render_bone);
    }

    // De-fighting pass. The bedrock models contain many faces that sit
    // exactly on top of each other (adjacent cubes sharing a plane, inflated
    // faces). Two front-facing faces at identical depth z-fight in the depth
    // buffer, and no position-only offset can separate them: the depth of a
    // point on a plane is fixed by the eye ray through the pixel. The fix is
    // a per-face depth level: faces are grouped by plane (same outward
    // normal, same plane offset), and each face gets a level that the vertex
    // shader uses to nudge it away from the eye *along the face normal* in
    // view space by level * depthBias. Shifting along the normal changes the
    // depth of every pixel of the face by level * depthBias / cos(theta) (>
    // 0 for a front-facing face), so the order of coplanar faces is
    // deterministic at every camera angle, while faces on distinct planes
    // keep their true depth order (levels are assigned so that the face
    // nearest the eye gets the smallest level).
    //
    // The level order within a group is (distance to eye, draw order): a
    // nearer face beats a farther one regardless of draw order (as in the
    // depth buffer), and exact coplanar ties go to the earlier face in draw
    // order (gl_LESS: the first drawn wins an exact depth tie). The
    // displacement of at most max_level * depthBias is a fraction of a
    // pixel, so the model is visually unchanged.
    //
    // Opposite-orientation faces on a shared plane are handled by backface
    // culling: at any view exactly one of the two orientations faces the eye.
    // Plane identity is established by exact matching (normal agreement to
    // 0.29 deg + offset agreement to 1e-5) instead of a quantized key: f32
    // noise in the per-face normals is large enough to straddle a
    // quantization boundary, which would silently split a true plane group.
    if (face_dbg)
        for (u32 i = 0; i < faces.size(); ++i) {
            const auto& p0 = faces[i].p[0];
            *face_dbg << "face " << i << " nrm=" << faces[i].nrm.x << "," << faces[i].nrm.y << "," << faces[i].nrm.z << " d=" << faces[i].d << " p0=" << p0.x << "," << p0.y << "," << p0.z << "\n";
        }

    struct subgroup {
        glm::vec3     nrm{};
        f32           d{};
        std::vector<u32> faceIdx;
    };
    std::vector<subgroup> subgroups;
    subgroups.reserve(faces.size());
    for (u32 i = 0; i < faces.size(); ++i) {
        const raw_face& rf = faces[i];
        subgroup*        sg = nullptr;
        for (auto& s : subgroups) {
            if (glm::dot(s.nrm, rf.nrm) > 0.99999f && glm::abs(s.d - rf.d) < 1e-5f) {
                sg = &s;
                break;
            }
        }
        if (!sg) {
            subgroups.push_back(subgroup{});
            sg           = &subgroups.back();
            sg->nrm      = rf.nrm;
            sg->d        = rf.d;
        }
        sg->faceIdx.push_back(i);
    }

    for (auto& s : subgroups) {
        // Nearest face first (largest d = closest to the eye along the
        // outward normal), coplanar ties by draw order. d is quantized to
        // 1e-6 in the comparator: smaller differences are f32 noise of
        // true coplanar faces and must be treated as ties.
        auto& idxs = s.faceIdx;
        std::sort(idxs.begin(), idxs.end(), [&](u32 a, u32 b) {
            const sys::i32 ka = sys::i32(glm::floor(faces[a].d / 1e-6f + 0.5f));
            const sys::i32 kb = sys::i32(glm::floor(faces[b].d / 1e-6f + 0.5f));
            if (ka != kb)
                return ka > kb;
            return a < b;
        });
        for (u32 rank = 0; rank < idxs.size(); ++rank) {
            const raw_face& rf  = faces[idxs[rank]];
            const f32        lvl = f32(rank + 1);
            const u32        base = u32(out.vertices.size());
            for (u32 q = 0; q < 4; ++q)
                out.vertices.push_back(mesh_vertex{rf.p[q], rf.uv[q], lvl, s.nrm});
            out.indices.insert(out.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
        }
    }
    return out;
}

inline std::pair<glm::vec3, glm::vec3> bounding_box(const mesh& m)
{
    glm::vec3 mn{1e30f, 1e30f, 1e30f};
    glm::vec3 mx{-1e30f, -1e30f, -1e30f};
    for (auto& v : m.vertices) {
        mn = glm::min(mn, v.pos);
        mx = glm::max(mx, v.pos);
    }
    return {mn, mx};
}
} // namespace tacz
