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

// One bone's constant animation values, either from the gun pack's
// "static_idle" animation or sampled from a keyframe animation (see
// sample_animation). Values are in the bedrock (y-up, 1/16) convention:
//   * position: delta from the bone's rest pose, 1/16 units, applied in the
//               parent frame (pre-multiplied into the base transform)
//   * rotation: additional rotation, degrees (Rz*Ry*Rx, same order as bones),
//               applied in the bone frame (post-multiplied)
//   * scale:    scale multipliers (post-multiplied)
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
    // Vertex count of each face: a face is a convex CCW polygon (4 for an
    // unclipped quad, 3 or more for a clipped piece) whose fan triangles are the
    // (v-2) index triples (base, base+k, base+k+1), k = 1..v-2, stored in
    // order, 3 per face. (See the de-fighting clip pass in build_mesh.)
    std::vector<u32> face_sizes;
};

// Per-face budget of the exact clip pass: a face whose clipped pieces would
// need more than this many fan triangles is emitted unclipped (the level
// scheme still resolves it; the pack's worst face needs 111). 3 verts per
// triangle bounds the per-face vertex/index count.
inline constexpr u32 kMaxClipTriangles = 128;

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

// Bone local transform with an animation applied (BedrockPart +
// AnimationListeners):
//   T(pos/16) * ( T(pivot/16) * Rz * Ry * Rx ) * R(rot) * S(scale)
// The position delta is a bedrock y-up offset, converted to y-down blocks
// (d.x, -d.y, d.z) / 16 and applied in the parent frame; rotation and scale
// are applied in the bone frame.
inline glm::mat4 bone_local(const bone_pose& p, const bone_anim* a) {
    glm::mat4 m = bone_local(p);
    if (a) {
        if (a->position)
            m = trans(glm::vec3{a->position->x, -a->position->y, a->position->z} / 16.f) * m;
        if (a->rotation)
            m = m * rot_z(glm::radians(a->rotation->z)) * rot_y(glm::radians(a->rotation->y)) * rot_x(glm::radians(a->rotation->x));
        if (a->scale)
            m = m * glm::scale(glm::mat4(1.f), a->scale.value());
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

// A point on a face's own plane in its 2D frame (s, t along frame axes u, v)
// plus the face uv carried along for interpolation at clip intersections.
struct p2 {
    f32 s{}, t{}, u{}, v{};
};

// Sutherland-Hodgman clip of a CCW convex polygon by ONE half-plane: the
// "outside" side (f <= 0) of the directed line a -> b, where a, b are two
// consecutive corners of a CCW clipper quad (the clipper's interior is f >=
// 0). Reversing (a, b) keeps the inside instead. The de-fighting pass uses
// it to build the disjoint convex partition of P \ C described in
// build_mesh.
inline std::vector<p2> half_clip(const std::vector<p2>& poly, const p2& a, const p2& b) {
    auto F = [&](const p2& p) -> f32 { return (b.s - a.s) * (p.t - a.t) - (b.t - a.t) * (p.s - a.s); };
    std::vector<p2> out;
    out.reserve(poly.size() + 1);
    const u32 n = u32(poly.size());
    for (u32 i = 0; i < n; ++i) {
        const p2& pa = poly[i];
        const p2& pb = poly[(i + 1) % n];
        const f32 fa = F(pa), fb = F(pb);
        const bool ina = fa <= 0.f, inb = fb <= 0.f;
        if (ina)
            out.push_back(pa);
        if (ina != inb) {
            const f32 tt = fa / (fa - fb);
            out.push_back(p2{pa.s + tt * (pb.s - pa.s), pa.t + tt * (pb.t - pa.t), pa.u + tt * (pb.u - pa.u), pa.v + tt * (pb.v - pa.v)});
        }
        if (inb)
            out.push_back(pb);
    }
    // drop consecutive duplicates (including wrap-around)
    std::vector<p2> cl;
    cl.reserve(out.size());
    auto dist2 = [](const p2& a, const p2& b) -> f32 { const f32 dx = a.s - b.s, dy = a.t - b.t; return dx * dx + dy * dy; };
    for (const auto& p : out)
        if (cl.empty() || dist2(p, cl.back()) > 1e-12f)
            cl.push_back(p);
    if (cl.size() > 2 && dist2(cl.front(), cl.back()) <= 1e-12f)
        cl.pop_back();
    if (cl.size() < 3)
        return {};
    // drop degenerate slivers
    f32 area = 0.f;
    for (u32 i = 0; i < u32(cl.size()); ++i) {
        const p2& pa = cl[i], &pb = cl[(i + 1) % u32(cl.size())];
        area += pa.s * pb.t - pb.s * pa.t;
    }
    return glm::abs(area) < 1e-12f ? std::vector<p2>{} : std::move(cl);
}
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
    // A constant value in any of the channel forms the pack uses:
    //   [x, y, z] | N (uniform scale) | {"0.0": [x, y, z]} |
    //   {"0.0": {"post"|"data": [x, y, z]}}
    auto read_v3_const = [&read_v3](const nlohmann::json& v) -> std::optional<glm::vec3> {
        if (v.is_number())
            return glm::vec3{v.get<f32>()};
        if (v.is_array())
            return read_v3(v);
        if (!v.is_object())
            return std::nullopt;
        if (v.size() == 1) {
            for (auto& [k, val] : v.items()) {
                if (val.is_array())
                    return read_v3(val);
                if (val.is_object()) {
                    for (auto& key : {"post", "data"})
                        if (val.contains(key) && val.at(key).is_array())
                            return read_v3(val.at(key));
                }
            }
        }
        return std::nullopt;
    };
    auto read_v3_at = [&read_v3_const](const nlohmann::json& obj, std::string_view key) -> std::optional<glm::vec3> {
        auto fit = obj.find(key);
        if (fit == obj.end())
            return std::nullopt;
        return read_v3_const(*fit);
    };
    for (auto& [name, b] : bones->items()) {
        bone_anim a{
            .position = read_v3_at(b, "position"),
            .rotation = read_v3_at(b, "rotation"),
            .scale    = read_v3_at(b, "scale"),
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

// Keyframe animation data (bedrock format_version 1.8.0).
//
// A channel is a map of time (seconds, as a string) to a keyframe value:
//   * a bare [x, y, z] array
//   * {"pre": [...], "post": [...]}, optionally with "lerp_mode" / "spline"
// The gun pack only ever uses lerp_mode "catmullrom" (the bedrock default).
//
// "pre" is the value at the END of the incoming segment (just before the
// keyframe time); "post" is the value at the START of the outgoing segment
// (just after). A bare array / an object without "pre" means pre == post.
// A keyframe with pre != post is an instantaneous jump at that time (the gun
// pack uses this for parts appearing/disappearing: scale 0 <-> 1, and for
// hands teleporting during fast reloads).
struct keyframe {
    f32     time{};   // seconds
    glm::vec3 pre{};  // incoming-segment end value
    glm::vec3 post{}; // outgoing-segment start value
};

struct anim_channel {
    std::vector<keyframe> keys; // sorted by time
};

struct bone_track {
    anim_channel position;
    anim_channel rotation;
    anim_channel scale;
};

struct keyframe_anim {
    std::string name;
    f32         length = 0.f;   // "animation_length", seconds
    bool        loop = false;   // "loop": true (the pack also uses false / "hold_on_last_frame")
    f32         end_time = 0.f; // max(length, last keyframe time): playback stops here when not looping
    std::map<std::string, bone_track> bones;
};

namespace details
{
inline void parse_keyframes(const nlohmann::json& j, anim_channel& ch)
{
    auto read_v3 = [](const nlohmann::json& v) {
        return glm::vec3{v[0].get<f32>(), v[1].get<f32>(), v[2].get<f32>()};
    };
    for (auto& [time, value] : j.items()) {
        const f32 t = f32(std::stof(time));
        if (value.is_array()) {
            const auto v = read_v3(value);
            ch.keys.push_back(keyframe{t, v, v});
            continue;
        }
        if (!value.is_object())
            continue;
        keyframe kf{t, {}, {}};
        bool has_pre = false, has_post = false;
        if (value.contains("pre") && value.at("pre").is_array()) {
            kf.pre = read_v3(value.at("pre"));
            has_pre = true;
        }
        if (value.contains("post") && value.at("post").is_array()) {
            kf.post = read_v3(value.at("post"));
            has_post = true;
        }
        if (!has_pre && value.contains("data") && value.at("data").is_array()) {
            kf.post = read_v3(value.at("data"));
            has_post = true;
        }
        if (has_pre && !has_post)
            kf.post = kf.pre;
        if (!has_pre)
            kf.pre = kf.post;
        if (has_pre || has_post)
            ch.keys.push_back(kf);
    }
    std::sort(ch.keys.begin(), ch.keys.end(),
              [](const keyframe& a, const keyframe& b) { return a.time < b.time; });
}
}

// Parse every animation of a gun pack animation file (in file order).
// Throws if the "animations" object is missing.
inline std::vector<keyframe_anim> parse_animations(const nlohmann::json& j)
{
    std::vector<keyframe_anim> out;
    auto ait = j.find("animations");
    if (ait == j.end() || !ait->is_object())
        throw std::runtime_error("animation file has no 'animations' object");

    auto read_v3 = [](const nlohmann::json& v) {
        return glm::vec3{v[0].get<f32>(), v[1].get<f32>(), v[2].get<f32>()};
    };
    for (auto& [name, a] : ait->items()) {
        if (!a.is_object())
            continue;
        keyframe_anim anim;
        anim.name = name;
        if (a.contains("animation_length") && a.at("animation_length").is_number())
            anim.length = a.at("animation_length").get<f32>();
        if (a.contains("loop")) {
            const auto& l = a.at("loop");
            anim.loop = l.is_boolean() ? l.get<bool>() : (l.is_string() && l.get<std::string>() == "loop");
        }
        if (a.contains("bones") && a.at("bones").is_object()) {
            for (auto& [bn, b] : a.at("bones").items()) {
                if (!b.is_object())
                    continue;
                bone_track track;
                for (auto& [prop, v] : b.items()) {
                    anim_channel* ch = nullptr;
                    if (prop == "position")
                        ch = &track.position;
                    else if (prop == "rotation")
                        ch = &track.rotation;
                    else if (prop == "scale")
                        ch = &track.scale;
                    if (!ch)
                        continue;
                    if (v.is_array()) {
                        // Constant channel: a bare [x, y, z] holds for the
                        // whole animation.
                        const auto val = read_v3(v);
                        ch->keys.push_back(keyframe{0.f, val, val});
                    } else if (v.is_number()) {
                        // Scalar scale (e.g. 0 to hide a part): a uniform
                        // scale by that number.
                        const auto s = glm::vec3{v.get<f32>()};
                        ch->keys.push_back(keyframe{0.f, s, s});
                    } else if (v.is_object()) {
                        details::parse_keyframes(v, *ch);
                    }
                }
                const bool empty = track.position.keys.empty() && track.rotation.keys.empty() && track.scale.keys.empty();
                if (!empty)
                    anim.bones[bn] = track;
            }
        }
        f32 end = anim.length;
        for (auto& [bn, track] : anim.bones)
            for (auto& ch : {track.position, track.rotation, track.scale})
                if (!ch.keys.empty())
                    end = std::max(end, ch.keys.back().time);
        anim.end_time = end;
        out.push_back(std::move(anim));
    }
    return out;
}

// Load the gun pack animation file and return all its animations (throws if
// the file is missing).
inline std::vector<keyframe_anim> load_animations(std::string_view path)
{
    auto file = core::io::file::open(std::string{path}, sys::openflag::read_only);
    auto map  = core::io::mmap{file, sys::map_prot::read, sys::map_flag::priv};
    auto data = map.data<char>();
    return parse_animations(nlohmann::json::parse(data, data + map.size()));
}

// Sample one animation channel at time t. The segment (i -> i + 1)
// interpolates keys[i].post to keys[i + 1].pre as a catmullrom spline:
// bedrock's uniform-parameter Catmull-Rom (cubic Hermite with unit-segment
// tangents m = 0.5 * (next - prev); time enters only through the normalized
// in-segment parameter, so key spacing does NOT rescale the tangents). A
// keyframe with pre != post is an instantaneous jump:
// the channel holds pre up to that time, then jumps to post. Times before
// the first keyframe sample keys[0].pre, times after the last one
// keys.back().post (looped channels wrap instead).
inline glm::vec3 sample_track(const std::vector<keyframe>& keys, f32 t, bool loop)
{
    const size_t n = keys.size();
    if (n == 0)
        return {};
    if (n == 1)
        return t < keys[0].time ? keys[0].pre : keys[0].post;
    const f32 first = keys.front().time;
    const f32 last  = keys.back().time;
    if (last <= first)
        return t < first ? keys.front().pre : keys.back().post;
    const f32 period = last - first;

    // Locate the segment i with keys[i].time <= tt < keys[i + 1].time; for
    // looped channels tt is shifted into [first, last + period) and
    // i == n - 1 is the wrap segment into the next loop.
    f32 tt;
    size_t i;
    if (loop) {
        if (t >= first && t < last) {
            tt = t; // no wrap needed: bit-exact for keyframe times
        } else {
            tt = first + std::fmod(t - first, period);
            if (tt < first)
                tt += period;
        }
        if (tt >= last) {
            i = n - 1;
        } else {
            i = 0;
            while (keys[i + 1].time <= tt)
                ++i;
        }
    } else {
        if (t < first)
            return keys.front().pre;
        if (t >= last)
            return keys.back().post;
        tt = t;
        i = 0;
        while (keys[i + 1].time <= tt)
            ++i;
    }

    const bool wrap = loop && i == n - 1;
    const size_t i1 = wrap ? n : i + 1; // segment end key (n = first key, next loop)
    const glm::vec3 p1 = keys[i].post;
    const glm::vec3 p2 = wrap ? keys[0].pre : keys[i1].pre;
    const f32 t1 = keys[i].time;
    const f32 t2 = wrap ? first + period : keys[i1].time;
    const f32 a  = t2 > t1 ? (tt - t1) / (t2 - t1) : 0.f;
    if (a >= 1.f) // exactly on the segment end key: its outgoing value
        return wrap ? keys[0].post : keys[i1].post;

    // Neighbouring spline points: p0 is the previous segment's end value,
    // p3 the next segment's start value (the post values of keys i - 1 /
    // i + 1), clamped at the track ends. Uniform Catmull-Rom uses only the
    // values, not their times.
    glm::vec3 p0 = p1, p3 = p2;
    if (i > 0) {
        p0 = keys[i - 1].post;
    } else if (loop) {
        p0 = keys[n - 1].post;
    }
    if (wrap) {
        p0 = keys[n - 2].post;
        p3 = keys[0].post;
    } else {
        p3 = keys[i1].post;
    }
    const glm::vec3 m1 = 0.5f * (p2 - p0);
    const glm::vec3 m2 = 0.5f * (p3 - p1);
    const f32 a2 = a * a;
    const f32 a3 = a2 * a;
    return (2.f * a3 - 3.f * a2 + 1.f) * p1 + (a3 - 2.f * a2 + a) * m1 +
           (-2.f * a3 + 3.f * a2) * p2 + (a3 - a2) * m2;
}

// Sample a keyframe animation at time t into per-bone values in the same
// form as a "static_idle" map, so the result can be passed to build_mesh().
inline std::map<std::string, bone_anim> sample_animation(const keyframe_anim& a, f32 t)
{
    std::map<std::string, bone_anim> out;
    for (auto& [name, track] : a.bones) {
        bone_anim ba;
        if (!track.position.keys.empty())
            ba.position = sample_track(track.position.keys, t, a.loop);
        if (!track.rotation.keys.empty())
            ba.rotation = sample_track(track.rotation.keys, t, a.loop);
        if (!track.scale.keys.empty())
            ba.scale = sample_track(track.scale.keys, t, a.loop);
        out[name] = ba;
    }
    return out;
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
        std::string bone; // owning bone (face_dbg only)
    };
    std::vector<raw_face> faces;

    auto emit_cube = [&](const char* bone_name, const glm::mat4& M, const glm::vec3& local_min, const glm::vec3& local_max, const cube& c) {
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
            rf.bone = bone_name;
            const glm::vec3 cr = glm::cross(rf.p[1] - rf.p[0], rf.p[2] - rf.p[0]);
            if (glm::dot(cr, cr) < 1e-12f)
                continue; // degenerate face (e.g. a scale-0 cube): emit nothing
            const glm::vec3 n = glm::normalize(cr);
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
                emit_cube(b.name.c_str(), sub, min16 - c.inflate, min16 + c.size + c.inflate, c);
            } else {
                const glm::vec3 min16 = {c.origin.x - b.pivot.x, b.pivot.y - c.origin.y - c.size.y, c.origin.z - b.pivot.z};
                emit_cube(b.name.c_str(), M, min16 - c.inflate, min16 + c.size + c.inflate, c);
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
            *face_dbg << "face " << i << " bone=" << faces[i].bone << " nrm=" << faces[i].nrm.x << "," << faces[i].nrm.y << "," << faces[i].nrm.z << " d=" << faces[i].d << " p0=" << p0.x << "," << p0.y << "," << p0.z << "\n";
        }

    // ------------------------------------------------------------------
    // De-fighting pass, part 1: EXACT CLIPPING. The level offset below is a
    // depth-buffer TIE-BREAKER: it only decides the winner between two faces
    // that still overlap on screen, and its per-level nudge can eat into
    // (partially invert) the true depth gap of near-coplanar faces. A
    // stronger guarantee is to make the faces not overlap at all: every
    // farther face is clipped exactly against the nearer (near-)coplanar
    // same-orientation faces in 2D on the shared plane, so no pixel is ever
    // covered by two faces of the same clip class and the depth test is
    // never asked to break a tie within the class.
    //
    // The difference P \ C of two convex polygons is not convex (a notch),
    // so it is computed from the clipper's CCW edge half-planes
    // (C = intersect_e H_e) with the DISJOINT partition
    //     P \ C = union_k (P \ H_k \cap H_k^c),   H_k^c = intersect_{m < k} H_m
    // i.e. each point of P \ C is assigned to the piece of its FIRST
    // violated edge index. Every piece is still a convex polygon
    // (an intersection of half-planes) and fan-triangulates trivially, and
    // the pieces are mutually disjoint (they touch only at boundaries), so
    // no two faces - not even two pieces of one face - share a pixel.
    // A fully covered face is
    // dropped entirely (nearer faces hide it from every view: same outward
    // normal => both face the eye or neither does).
    //
    // Pair eligibility (normals agree to 0.29 deg, |d_a - d_b| < kClipGap):
    //   * coplanar (|d_a - d_b| < 1e-5): the earlier face in draw order is
    //     in front (gl_LESS: first drawn wins the exact depth tie), so the
    //     later face is clipped against the earlier one.
    //   * distinct planes: the face in front everywhere clips the other.
    //     The plane separation is linear over each quad, so the four
    //     corners of each give the gap's min/max; a is in front of b
    //     everywhere iff every corner of b is behind a's plane AND every
    //     corner of a is in front of b's plane (both directions must hold:
    //     a crossing pair also has some corners behind on each side). If
    //     the planes CROSS within the extents, the truly closer surface
    //     flips pixel-by-pixel: that is real geometry, not z-fighting, and
    //     the depth test resolves it - the pair is skipped.
    //
    // Clipping is view-independent and EXACT for parallel planes: along
    // any eye ray through the overlap the nearer plane is hit first, at
    // every camera angle (mouse-look included), so removing the farther
    // face's covered region cannot change the visible image. The clipped
    // pieces keep the face's own plane, its uv mapping and its CCW winding
    // (inherited from the quad), so invariants 1-4 of the level pass
    // (winding, per-face fresh vertices, vertex order, monotone levels)
    // all hold unchanged: pieces are emitted where the original quad would
    // be, with the original face's level.
    constexpr f32 kClipGap = 0.05f; // max plane offset (blocks) for clip pairs

    // Per-face 2D frame on its own plane (u, v orthonormal, u x v = nrm),
    // built lazily. Faces are CCW from the outside by construction, so the
    // signed (s, t) area of the quad is positive for a valid frame.
    struct frame2d {
        glm::vec3 u{}, v{};
        bool ok{};
    };
    std::vector<frame2d> frames(faces.size());
    auto make_frame = [&](u32 i) -> frame2d& {
        frame2d& fr = frames[i];
        if (fr.ok)
            return fr;
        const glm::vec3& n = faces[i].nrm;
        const f32 ax = glm::abs(n.x), ay = glm::abs(n.y), az = glm::abs(n.z);
        const glm::vec3 ref = (ax <= ay && ax <= az) ? glm::vec3{1.f, 0.f, 0.f}
                            : (ay <= az                 ? glm::vec3{0.f, 1.f, 0.f}
                                                       : glm::vec3{0.f, 0.f, 1.f});
        fr.u = glm::normalize(glm::cross(n, ref));
        fr.v = glm::cross(n, fr.u);
        f32 area = 0.f;
        for (u32 k = 0; k < 4; ++k) {
            const glm::vec3& pa = faces[i].p[k];
            const glm::vec3& pb = faces[i].p[(k + 1) % 4];
            area += glm::dot(pa, fr.u) * glm::dot(pb, fr.v) - glm::dot(pb, fr.u) * glm::dot(pa, fr.v);
        }
        fr.ok = area > 1e-9f;
        return fr;
    };

    struct aabb {
        glm::vec3 mn{}, mx{};
    };
    std::vector<aabb> bbs(faces.size());
    for (u32 i = 0; i < faces.size(); ++i) {
        bbs[i].mn = bbs[i].mx = faces[i].p[0];
        for (u32 k = 1; k < 4; ++k) {
            bbs[i].mn = glm::min(bbs[i].mn, faces[i].p[k]);
            bbs[i].mx = glm::max(bbs[i].mx, faces[i].p[k]);
        }
    }

    // Clip classes: same outward normal (0.29 deg) AND plane offset within
    // kClipGap of the class representative (no quantized key: f32 noise in
    // per-face normals straddles any quantization boundary, as for the
    // subgroup pass below). Faces are only clipped within their class.
    struct clip_class {
        glm::vec3 nrm{};
        f32 d{};
        std::vector<u32> idx;
    };
    std::vector<clip_class> classes;
    classes.reserve(faces.size());
    for (u32 i = 0; i < faces.size(); ++i) {
        clip_class* cc = nullptr;
        for (auto& c : classes) {
            if (glm::dot(c.nrm, faces[i].nrm) > 0.99999f && glm::abs(c.d - faces[i].d) < kClipGap) {
                cc = &c;
                break;
            }
        }
        if (!cc) {
            classes.push_back(clip_class{});
            cc     = &classes.back();
            cc->nrm = faces[i].nrm;
            cc->d   = faces[i].d;
        }
        cc->idx.push_back(i);
    }

    // Which faces clip which: for every eligible pair, the in-front face is
    // recorded as a clipper of the farther one.
    std::vector<std::vector<u32>> clippers(faces.size());
    auto d_rank = [&](u32 i) -> sys::i32 { return sys::i32(glm::floor(faces[i].d / 1e-6f + 0.5f)); };
    auto earlier = [&](u32 a, u32 b) {
        const sys::i32 ka = d_rank(a), kb = d_rank(b);
        return ka != kb ? ka > kb : a < b; // (d descending, draw order ascending)
    };
    for (auto& c : classes) {
        auto& idxs = c.idx;
        std::sort(idxs.begin(), idxs.end(), [&](u32 a, u32 b) { return earlier(a, b); });
        for (size_t ia = 0; ia < idxs.size(); ++ia) {
            const u32 a = idxs[ia];
            for (size_t ib = ia + 1; ib < idxs.size(); ++ib) {
                const u32 b  = idxs[ib];
                if (faces[a].d - faces[b].d > kClipGap)
                    break; // sorted by d descending: later ones are farther apart
                const aabb& A = bbs[a], &B = bbs[b];
                // 3D AABB overlap (slacked: coplanar faces touch in the
                // normal axis) is necessary for 2D overlap on the plane.
                if (glm::min(A.mx.x, B.mx.x) - glm::max(A.mn.x, B.mn.x) <= -1e-9f ||
                    glm::min(A.mx.y, B.mx.y) - glm::max(A.mn.y, B.mn.y) <= -1e-9f ||
                    glm::min(A.mx.z, B.mx.z) - glm::max(A.mn.z, B.mn.z) <= -1e-9f)
                    continue;
                if (faces[a].d - faces[b].d < 1e-5f) {
                    clippers[b].push_back(a); // coplanar: a precedes b in draw order
                    continue;
                }
                // Which face is in front? f_a(x) = dot(n_a, x) - d_a > 0 means
                // x is on the eye side of a's plane (closer to the eye). The
                // gap is linear over each quad, so the corners suffice. All
                // four extrema are needed: a pair whose corners all lie on
                // the "correct" side of both planes can still be a crossing
                // pair (the corner min/max of each side must be checked).
                f32 smin = 1e30f, smax = -1e30f; // f_a at b's corners
                for (u32 k = 0; k < 4; ++k) {
                    const f32 f = glm::dot(faces[a].nrm, faces[b].p[k]) - faces[a].d;
                    smin        = glm::min(smin, f);
                    smax        = glm::max(smax, f);
                }
                f32 tmin = 1e30f, tmax = -1e30f; // f_b at a's corners
                for (u32 k = 0; k < 4; ++k) {
                    const f32 f = glm::dot(faces[b].nrm, faces[a].p[k]) - faces[b].d;
                    tmin        = glm::min(tmin, f);
                    tmax        = glm::max(tmax, f);
                }
                if (smax < 1e-6f && tmin > -1e-6f)
                    clippers[b].push_back(a); // a in front of b everywhere
                else if (smin > -1e-6f && tmax < 1e-6f)
                    clippers[a].push_back(b); // b in front of a everywhere
                // else the planes cross within the extents: real geometry,
                // left to the depth test (never clipped).
            }
        }
    }

    // Clip each face that has clippers. Result: convex pieces (2D, on the
    // face's own plane) in draw-safe order; empty optional = unclipped,
    // empty piece list = fully hidden face.
    std::vector<std::optional<std::vector<std::vector<details::p2>>>> clipped(faces.size());
    for (u32 i = 0; i < faces.size(); ++i) {
        if (clippers[i].empty())
            continue;
        frame2d& fr = make_frame(i);
        if (!fr.ok)
            continue; // broken winding: leave the face untouched
        auto to2d = [&](const raw_face& rf) -> std::vector<details::p2> {
            std::vector<details::p2> q;
            q.reserve(4);
            for (u32 k = 0; k < 4; ++k)
                q.push_back(details::p2{glm::dot(rf.p[k], fr.u), glm::dot(rf.p[k], fr.v), rf.uv[k].x, rf.uv[k].y});
            return q;
        };
        // Clipper quads projected onto i's plane along the eye rays (the
        // eye is at the origin): exact for any pair of near-parallel planes,
        // so the 2D shadow has the exact pixel coverage of the clipper at
        // every camera angle. The eye is on the outward side of both planes,
        // so the projection scale d/(n . x) stays positive and the CCW
        // winding is preserved.
        auto order = clippers[i];
        std::sort(order.begin(), order.end(), [&](u32 a, u32 b) { return earlier(a, b); });
        std::vector<std::vector<details::p2>> cps;
        cps.reserve(order.size());
        for (u32 j : order) {
            const raw_face& rf = faces[j];
            std::vector<details::p2> q;
            q.reserve(4);
            for (u32 k = 0; k < 4; ++k) {
                const glm::vec3& B  = rf.p[k];
                const glm::vec3     pp = (faces[i].d / glm::dot(faces[i].nrm, B)) * B; // eye ray through B hits plane i
                q.push_back(details::p2{glm::dot(pp, fr.u), glm::dot(pp, fr.v), 0.f, 0.f});
            }
            cps.push_back(std::move(q));
        }

        std::vector<std::vector<details::p2>> pieces = {to2d(faces[i])};
        bool over = false;
        for (const auto& cp : cps) {
            std::vector<std::vector<details::p2>> next;
            for (auto& piece : pieces) {
                // disjoint partition of piece \ clipper: point -> piece of its
                // first violated edge index (see the pass comment above)
                for (u32 k = 0; k < 4; ++k) {
                    auto q = details::half_clip(piece, cp[k], cp[(k + 1) % 4]); // \ H_k
                    for (u32 m = 0; m < k && !q.empty(); ++m)
                        q = details::half_clip(q, cp[(m + 1) % 4], cp[m]); //   ∩ H_m
                    if (!q.empty())
                        next.push_back(std::move(q));
                }
            }
            pieces = std::move(next);
            if (pieces.empty())
                break;
            u32 tris = 0;
            for (const auto& piece : pieces)
                tris += u32(piece.size() - 2);
            if (tris > kMaxClipTriangles) {
                over = true; // too many pieces: keep the face whole
                break;
            }
        }
        if (!over)
            clipped[i] = std::move(pieces);
    }
    if (face_dbg)
        for (u32 i = 0; i < faces.size(); ++i)
            if (clipped[i]) {
                u32 nv2 = 0;
                for (const auto& p : *clipped[i])
                    nv2 += u32(p.size());
                *face_dbg << "  clip face " << i << " bone=" << faces[i].bone << " clippers=" << clippers[i].size() << " pieces=" << clipped[i]->size() << " verts=" << nv2
                          << (clipped[i]->empty() ? " (fully hidden)" : "") << "\n";
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
            if (clipped[idxs[rank]]) {
                // Emit the clip pieces where the quad would go, fan
                // triangulated, with the original face's level and normal.
                // 3D positions are reconstructed on the face's own plane
                // (d * n + s * u + t * v), which snaps f32 wobble off the
                // plane; unwound pieces are CCW, so culling is unchanged.
                const frame2d& fr = frames[idxs[rank]];
                for (const auto& piece : *clipped[idxs[rank]]) {
                    const u32 pb = u32(out.vertices.size());
                    for (const auto& pt : piece)
                        out.vertices.push_back(mesh_vertex{rf.d * rf.nrm + pt.s * fr.u + pt.t * fr.v, glm::vec2{pt.u, pt.v}, lvl, s.nrm});
                    for (u32 k = 1; k + 1 < u32(piece.size()); ++k)
                        out.indices.insert(out.indices.end(), {pb, pb + k, pb + k + 1});
                    out.face_sizes.push_back(u32(piece.size()));
                }
            } else {
                const u32 base = u32(out.vertices.size());
                for (u32 q = 0; q < 4; ++q)
                    out.vertices.push_back(mesh_vertex{rf.p[q], rf.uv[q], lvl, s.nrm});
                out.indices.insert(out.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
                out.face_sizes.push_back(4);
            }
        }
    }
    return out;
}

// Upper bound on the face count of build_mesh's output for this geometry:
// every UV-mapped face of every bone not in a hidden-by-default subtree. No
// pose can exceed it, so vertex/index buffers sized to it never need to
// grow (animated frames with fewer faces, e.g. scale-0 parts, are smaller).
inline u32 max_face_count(const geometry& geo) {
    std::map<std::string, const bone*> by_name;
    for (auto& b : geo.bones)
        by_name.try_emplace(b.name, &b);

    u32 n = 0;
    for (auto& b : geo.bones) {
        const bone* p = &b;
        bool hidden = false;
        while (p) {
            if (hidden_by_default(p->name)) {
                hidden = true;
                break;
            }
            p = p->parent ? by_name.find(*p->parent)->second : nullptr;
        }
        if (hidden)
            continue;
        for (auto& c : b.cubes)
            n += u32(c.uv.size());
    }
    return n;
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
