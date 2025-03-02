#pragma once

#include <pugixml.hpp>

#include <map>

#include <codegen/helpers.hpp>
#include <codegen/struct.hpp>

namespace cg::vk
{
inline void trim_space(std::string& str) {
    auto pos1 = str.find_first_not_of(' ');
    if (pos1 != str.npos)
        str = str.substr(pos1);
    while (str.ends_with(' '))
        str.resize(str.size() - 1);
}

inline std::string transform_type(const std::string& type, bool use_namespace = false) {
    static std::map<std::string, std::string> type_map = {
        {"VkBool32", "bool32_t"},
        {"uint8_t", "u8"},
        {"uint16_t", "u16"},
        {"uint32_t", "u32"},
        {"uint64_t", "u64"},
        {"int8_t", "i8"},
        {"int16_t", "i16"},
        {"int32_t", "i32"},
        {"int64_t", "i64"},
        {"VkDeviceSize", "device_size_t"},
        {"VkDeviceAddress", "device_address_t"},
        {"VkFlags", "flags_t"},
        {"VkFlags64", "flags64_t"},
        {"VkSampleMask", "sample_mask_t"},
        {"HINSTANCE", "HINSTANCE"},
        {"HWND", "HWND"},
        {"HANDLE", "HANDLE"},
        {"DWORD", "DWORD"},
        {"SECURITY_ATTRIBUTES", "SECURITY_ATTRIBUTES"},
        {"LPCWSTR", "LPCWSTR"},
        {"Display", "Display"},
        {"Window", "Window"},
        {"IDirectFB", "IDirectFB"},
        {"IDirectFBSurface", "IDirectFBSurface"},
        {"GgpStreamDescriptor", "GgpStreamDescriptor"},
        {"NvSciSyncAttrList", "NvSciSyncAttrList"},
        {"CAMetalLayer", "CAMetalLayer"},
    };

    auto found = type_map.find(type);
    if (found != type_map.end())
        return found->second;

    auto t = to_snake_case(type);
    drop_prefix(t, "pfn_");
    if (use_namespace) {
        if (t.starts_with("vk_"))
            t = "vk::" + t.substr(3);
    }
    else {
        drop_prefix(t, "vk_");
    }
    trim_space(t);

    if (t.ends_with("_d")) {
        t.resize(t.size() - 2);
        t.push_back('d');
    }

    if (t.ends_with("_flags2")) {
        t.resize(t.size() - (sizeof("_flags2") - 1));
        t += "2_flags";
    }

    if (t.ends_with("_flag_bits")) {
        t.resize(t.size() - (sizeof("_flag_bits") - 1));
        t += "_flags";
    }
    if (t.ends_with("_flag_bits_khr")) {
        t.resize(t.size() - (sizeof("_flag_bits_khr") - 1));
        t += "_flags_khr";
    }
    if (t.ends_with("_flag_bits_ext")) {
        t.resize(t.size() - (sizeof("_flag_bits_ext") - 1));
        t += "_flags_ext";
    }

    return t;
}

inline std::string drop_vk_pref_suff(std::string value) {
    drop_postfixes(value, "_bit");
    drop_prefixes(value, "vk_");
    return value;
}

inline field_gen parse_field(const pugi::xml_node& xml_field) {
    field_gen field;

    if (xml_field.attribute("optional"))
        field.optional = true;

    for (auto child : xml_field.children()) {
        if (child.type() == pugi::xml_node_type::node_pcdata) {
            std::string token = child.text().as_string();
            if (token == "[") {
                field.is_array = true;
            }
            else if (token.starts_with('[') && token.ends_with(']')) {
                field.is_array = true;
                field.size     = token.substr(1, token.size() - 2);
            }
            else if (token.starts_with(':')) {
                field.bitfield = token.substr(1);
            }
            else if (token != "]") {
                field.type.append(token);
            }
        }
        else if (child.name() == std::string_view("type")) {
            field.type.append(transform_type(child.text().as_string()));
        }
        else if (child.name() == std::string_view("enum")) {
            field.size = to_lower_case(child.text().as_string());
            drop_prefix(field.size, "vk_");
            field.size = "constants::" + field.size;
        }
        else if (child.name() == std::string_view("name")) {
            field.name = child.text().as_string();
        }
        else if (child.name() == std::string_view("comment")) {
            field.comment = child.text().as_string();
        }
    }

    trim_space(field.name);
    field.name = to_snake_case(field.name);
    trim_space(field.type);

    if (field.name.starts_with("p_"))
        field.name = field.name.substr(2);
    if (field.name.starts_with("pp_"))
        field.name = field.name.substr(3);
    if (field.name.starts_with("pfn_"))
        field.name = field.name.substr(4);
    if (field.name == "s_type")
        field.name = "_stype";

    if (auto values = xml_field.attribute("values")) {
        std::string val = to_lower_case(values.value());
        if (val.starts_with("vk_structure_type_")) {
            drop_prefix(val, "vk_structure_type_");
            field.value = "vk::structure_type::";
            field.value += val;
        }
    }

    return field;
}
} // namespace cg::vk
