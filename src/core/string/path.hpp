#pragma once

#include <filesystem>

#include <core/ranges/zip.hpp>
#include <util/log.hpp>

namespace core {
bool is_relative_to(std::filesystem::path parent, const std::filesystem::path& child) {
    if (!parent.empty() && parent.filename().empty()) {
        parent = parent.parent_path();
    }

    auto p_it = parent.begin();
    auto c_it = child.begin();

    while (p_it != parent.end() && c_it != child.end()) {
        if (*p_it != *c_it) {
            return false;
        }
        ++p_it;
        ++c_it;
    }

    return p_it == parent.end();
}
} // namespace core
