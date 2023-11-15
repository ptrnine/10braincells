#pragma once

#include <concepts>

#include <core/io/helpers.hpp>
#include <core/io/types.hpp>

namespace core {
template <typename T>
concept Outfd = requires(T o) {
    {o.write(details::autocast{}, size_t{})} -> std::same_as<T&>;
    {o.flatcopy(0)} -> std::same_as<T&>;
    {o.flush()} -> std::same_as<bool>;
    {o.seek(size_t{})} -> std::same_as<T&>;
    {o.put(details::autocast{})} -> std::same_as<T&>;
    {o.is_fifo()} -> std::same_as<bool>;
    {o.descriptor()};
};

template <typename T>
concept Infd = requires(T i) {
    {i.read(details::autocast{}, size_t{})} -> std::same_as<io_read_res>;
    {i.nonblock_read(details::autocast{}, size_t{})} -> std::same_as<io_read_res>;
    {i.is_blocking()} -> std::same_as<bool>;
    {i.read_wait_timeout()};
    {i.read_wait_timeout(details::autocast{})};
    {i.can_be_blocked()} -> std::same_as<bool>;
    {i.descriptor()};
};
} // namespace core
