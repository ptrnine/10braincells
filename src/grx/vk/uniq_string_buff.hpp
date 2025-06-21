#pragma once

#include <core/ranges/range.hpp>
#include <set>
#include <vector>

#include <grx/vk/extension_defines.hpp>

#include <grx/vk/structs.cg.hpp>

namespace vk::details {
class uniq_string_buff {
public:
    uniq_string_buff() = default;

    template <core::input_range T>
        requires requires(T v) {
            { *v.begin() } -> core::convertible_to<std::string>;
        }
    uniq_string_buff(const T& v): buff(v.begin(), v.end()) {
        buff_raw.reserve(buff.size());
        for (auto&& v : buff) {
            buff_raw.emplace_back(v.data());
        }
    }

    template <core::input_range T>
        requires requires(T v) {
            { *v.begin() } -> core::convertible_to<extension_properties>;
        }
    uniq_string_buff(const T& props) {
        for (auto&& prop : props) {
            buff.emplace(std::string(prop.extension_name));
        }

        buff_raw.reserve(buff.size());
        for (auto&& v : buff) {
            buff_raw.emplace_back(v.data());
        }
    }

    template <core::convertible_to<std::string> T>
    uniq_string_buff(std::initializer_list<T> args): uniq_string_buff(core::range_holder{args.begin(), args.end()}) {}

    void push_back(std::string value) {
        auto [pos, insert] = buff.emplace(core::mov(value));
        if (insert) {
            buff_raw.emplace_back(pos->data());
        }
    }

    auto size() const {
        return u32(buff_raw.size());
    }

    auto data() const {
        return buff_raw.data();
    }

private:
    std::set<std::string>    buff;
    std::vector<const char*> buff_raw;
};
} // namespace vk::details
