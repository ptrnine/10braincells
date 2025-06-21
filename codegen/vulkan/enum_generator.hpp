#pragma once

#include <pugixml.hpp>

#include <string>
#include <map>

#include <core/io/std.hpp>

#include <codegen/enum_class.hpp>
#include <codegen/helpers.hpp>
#include <codegen/vulkan/helpers.hpp>
#include <util/log.hpp>

using namespace std::string_view_literals;

namespace cg::vk {
inline void add_underscore_before_digits(std::string& str) {
    if (!isdigit(str.back()))
        return;

    auto i = str.size() - 1;
    for (; isdigit(str[i]) && i < str.size(); --i);

    str.insert(i + 1, "_");
}

static inline const std::vector<std::string_view> vk_postfixes = {
    "_khr",
    "_ext",
    "_nv",
    "_android",
    "_fuchsia",
    "_qnx",
    "_qcom",
    "_arm",
    "_huawei",
    "_coreavi",
    "_amd",
    "_nvx",
    "_intel",
    "_amdx",
    "_img",
};

inline void drop_bit_suffix(std::string& str) {
    if (str.ends_with("_bit")) {
        drop_suffix(str, "_bit");
        return;
    }

    for (auto&& postfix : vk_postfixes) {
        std::string p = "_bit" + std::string(postfix);
        if (str.ends_with(p)) {
            drop_suffix(str, "_bit");
            return;
        }
    }
}

inline void drop_vk_postfix(std::string& str) {
    for (auto&& postfix : vk_postfixes)
        if (drop_postfix(str, postfix))
            return;
}

inline enum_class_value
parse_flag_record(const pugi::xml_node& xml_record, const std::string& record_prefix, const std::string& ifdef = {}) {
    enum_class_value record;

    record.ifdef = ifdef;

    record.key = to_lower_case(xml_record.attribute("name").value());
    drop_prefix(record.key, record_prefix);
    drop_bit_suffix(record.key);
    add_underscore_if_invalid(record.key);

    if (auto value = xml_record.attribute("value")) {
        record.value = value.value();
    }
    else if (auto bitpos = xml_record.attribute("bitpos")) {
        record.value = "1UL << " + std::string(bitpos.value());
    }
    else if (auto alias = xml_record.attribute("alias")) {
        record.value = to_lower_case(alias.value());
        drop_prefix(record.value, record_prefix);
        drop_bit_suffix(record.value);
        add_underscore_if_invalid(record.value);
        record.is_alias = true;
    }

    if (auto comment = xml_record.attribute("comment"))
        record.comment = comment.value();

    if (auto protect = xml_record.attribute("protect")) {
        if (record.ifdef.empty())
            record.ifdef = std::string("defined(") + protect.value() + ")";
        else
            record.ifdef += std::string(" && defined(") + protect.value() + ")";
    }

    return record;
}

inline enum_class parse_flags_from_enum_node(const std::string& native_name, const pugi::xml_node& xml_enum) {
    enum_class enum_;

    enum_.name = to_snake_case(native_name);
    drop_suffix(enum_.name, "_flag_bits");

    auto record_prefix = enum_.name;
    drop_vk_postfix(record_prefix);
    add_underscore_before_digits(record_prefix);
    record_prefix += '_';

    drop_prefix(enum_.name, "vk_");
    enum_.name += "_flag";

    enum_.underlying_type = "core::u32";
    if (auto bitwidth = xml_enum.attribute("bitwidth")) {
        auto bw = std::string(bitwidth.value());
        if (bw == "64")
            enum_.underlying_type = "core::u64";
    }

    if (auto comment = xml_enum.attribute("comment"))
        enum_.comment = comment.value();

    for (auto xml_record : xml_enum.children("enum"))
        enum_.append(parse_flag_record(xml_record, record_prefix));

    return enum_;
}

auto parse_flags(const pugi::xml_node& registry) {
    std::map<std::string, cg::enum_class> enums;

    /* Parse enums */
    for (auto e : registry.children("enums"))
        if (e.attribute("type").value() == "bitmask"sv)
            if (auto native_name = e.attribute("name"))
                enums.emplace(native_name.value(), parse_flags_from_enum_node(native_name.value(), e));

    auto parse_from_require = [&enums](const pugi::xml_node& require, const std::string& ifdef = {}) {
        for (auto e : require.children("enum")) {
            if (!(e.attribute("value") || e.attribute("bitpos")))
                continue;

            auto extends = e.attribute("extends");
            if (!extends)
                continue;

            auto found = enums.find(extends.value());
            if (found == enums.end())
                continue;
            auto& enum_ = found->second;

            std::string record_prefix = to_snake_case(extends.value());
            drop_suffix(record_prefix, "_flag_bits");
            drop_vk_postfix(record_prefix);
            add_underscore_before_digits(record_prefix);
            record_prefix += '_';

            enum_.append(parse_flag_record(e, record_prefix, ifdef));
        }
    };

    /* Parse flags from features */
    for (auto feature : registry.children("feature"))
        for (auto require : feature.children("require"))
            parse_from_require(require);

    /* Parse flags from extensions */
    for (auto extension : registry.child("extensions").children("extension")) {
        auto ifdef = std::string("defined(") + extension.attribute("name").value() + ")";
        for (auto require : extension.children("require"))
            parse_from_require(require, ifdef);
    }

    /* Remove enums without keys */
    for (auto it = enums.begin(); it != enums.end();) {
        if (it->second.values.empty())
            enums.erase(it++);
        else
            ++it;
    }

    /* Set underlying type */
    for (auto t : registry.child("types").children()) {
        if (t.attribute("category").value() == "bitmask"sv) {
            std::string type = t.child("type").text().as_string();
            std::string name = t.attribute("requires").value();
            if (name.empty())
                name = t.attribute("bitvalues").value();

            std::string alias = to_snake_case(type) + "_t";
            drop_prefix(alias, "vk_");

            auto enum_i = enums.find(name);
            if (enum_i != enums.end())
                enum_i->second.underlying_type = alias;
        }
    }

    return enums;
}

auto generate_flags_header(const pugi::xml_node& registry, auto&& out) {
    out.write("#pragma once\n"
              "\n"
              "#include <string>\n"
              "\n"
              "#include <core/basic_types.hpp>\n"
              "#include <core/flags.hpp>\n"
              "#include <core/tuple.hpp>\n"
              "#include <core/array.hpp>\n"
              "\n"
              "#include <grx/vk/types.cg.hpp>\n"
              "\n"
              "namespace vk {\n"
              "namespace details {\n"
              "    template <typename>\n"
              "    struct _using_enum;\n"
              "}\n"
              "\n");

    auto flags = cg::vk::parse_flags(registry);
    for (auto&& [_, f] : flags) {
        f.generate_as_flags(out);
        out.write("\n");
    }

    out.write("} /* namespace vk */\n");

    return flags;
}

inline enum_class_value parse_enum_record(const pugi::xml_node& xml_record,
                                          const std::string&    record_prefix,
                                          const std::string&    ifdef      = {},
                                          core::opt<int>        ext_number = {}) {
    enum_class_value record;

    record.ifdef = ifdef;

    record.key = to_lower_case(xml_record.attribute("name").value());
    drop_prefix(record.key, record_prefix);
    add_underscore_if_invalid(record.key);
    drop_prefix(record.key, "vk_");

    if (auto value = xml_record.attribute("value")) {
        record.value = value.value();
    }
    else if (auto alias = xml_record.attribute("alias")) {
        record.value = to_lower_case(alias.value());
        drop_prefix(record.value, record_prefix);
        drop_prefix(record.value, "vk_");
        add_underscore_if_invalid(record.value);
        record.is_alias = true;
    }
    else if (auto offset = xml_record.attribute("offset"); offset) {
        if (auto ext_number_attr = xml_record.attribute("extnumber"))
            ext_number = ext_number_attr.as_int();
        std::string dir;
        if (auto dir_attr = xml_record.attribute("dir"))
            dir = dir_attr.value();
        record.value = dir + std::to_string(1000000000 + ((ext_number.value() - 1) * 1000) + offset.as_int());
    }

    if (auto comment = xml_record.attribute("comment"))
        record.comment = comment.value();

    if (auto protect = xml_record.attribute("protect")) {
        if (record.ifdef.empty())
            record.ifdef = std::string("defined(") + protect.value() + ")";
        else
            record.ifdef += std::string(" && defined(") + protect.value() + ")";
    }

    return record;
}

inline enum_class parse_enums_from_enum_node(const std::string& native_name, const pugi::xml_node& xml_enum) {
    enum_class enum_;

    enum_.name = to_snake_case(native_name);
    auto record_prefix = enum_.name;
    drop_vk_postfix(record_prefix);
    record_prefix += '_';
    drop_prefix(enum_.name, "vk_");

    if (auto comment = xml_enum.attribute("comment"))
        enum_.comment = comment.value();

    for (auto xml_record : xml_enum.children("enum"))
        if (!xml_record.attribute("deprecated"))
            enum_.append(parse_enum_record(xml_record, record_prefix));

    return enum_;
}

auto parse_enums(const pugi::xml_node& registry) {
    std::map<std::string, cg::enum_class> enums;

    /* Parse enums */
    for (auto e : registry.children("enums"))
        if (e.attribute("type").value() == "enum"sv)
            if (auto native_name = e.attribute("name"))
                enums.emplace(native_name.value(), parse_enums_from_enum_node(native_name.value(), e));

    auto parse_from_require =
        [&enums](const pugi::xml_node& require, const std::string& ifdef = {}, core::opt<int> ext_number = {}) {
            for (auto e : require.children("enum")) {
                auto extends = e.attribute("extends");
                if (!extends)
                    continue;

                auto found = enums.find(extends.value());
                if (found == enums.end())
                    continue;
                auto& enum_ = found->second;

                std::string record_prefix = to_snake_case(extends.value());
                drop_vk_postfix(record_prefix);
                add_underscore_before_digits(record_prefix);
                record_prefix += '_';

                enum_.append(parse_enum_record(e, record_prefix, {}, ext_number));
            }
        };

    /* Parse enums from features */
    for (auto feature : registry.children("feature"))
        for (auto require : feature.children("require"))
            parse_from_require(require);

    /* Parse enums from extensions */
    for (auto extension : registry.child("extensions").children("extension")) {
        auto ifdef = std::string("defined(") + extension.attribute("name").value() + ")";
        core::opt<int> ext_number;
        if (auto number = extension.attribute("number"))
            ext_number = number.as_int();
        for (auto require : extension.children("require"))
            parse_from_require(require, ifdef, ext_number);
    }

    /* Sort aliases */
    for (auto&& [_, e] : enums) {
        std::set<std::string> keys;
        for (auto it = e.values.begin(); it != e.values.end();) {
            keys.emplace(it->key);
            if (it->is_alias && !keys.contains(it->value)) {
                auto enum_v = core::mov(*it);
                e.values.erase(it++);
                auto pos = std::find_if(e.values.begin(), e.values.end(), [&](auto&& value) {
                    return value.key == enum_v.value;
                });
                e.values.insert(std::next(pos), core::mov(enum_v));
            }
            else
                ++it;
        }
    }

    return enums;
}

auto generate_enums_header(const pugi::xml_node& registry, auto&& out) {
    out.write("#pragma once\n"
              "\n"
              "#include <string_view>\n"
              "\n"
              "#include <core/robin_map.hpp>\n"
              "\n"
              "namespace vk\n"
              "{\n");

    for (auto&& [_, f] : parse_enums(registry)) {
        f.generate_as_enum(out);
        out.write("\n");
    }

    out.write("} // namespace vk");
}
} // namespace cg::vk
