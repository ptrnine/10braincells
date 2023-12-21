#pragma once

namespace core::details {
template <typename>
struct false_on_eval {
    static const bool false_ = false;
};

} // namespace core::details
