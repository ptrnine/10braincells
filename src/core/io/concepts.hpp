#pragma once

#include <core/io/helpers.hpp>
#include <core/io/types.hpp>
#include <core/nostd/concepts/same_as.hpp>

namespace core {
template <typename T>
concept Outfd = requires(T o) {
    {o.write(details::autocast{}, size_t{})} -> same_as<T&>;
    {o.flatcopy(0)} -> same_as<T&>;
    {o.flush()} -> same_as<bool>;
    {o.seek(size_t{})} -> same_as<T&>;
    {o.put(details::autocast{})} -> same_as<T&>;
    {o.is_fifo()} -> same_as<bool>;
    {o.descriptor()};
};

template <typename T>
concept Infd = requires(T i) {
    {i.read(details::autocast{}, size_t{})} -> same_as<io_read_res>;
    {i.nonblock_read(details::autocast{}, size_t{})} -> same_as<io_read_res>;
    {i.is_blocking()} -> same_as<bool>;
    {i.read_wait_timeout()};
    {i.read_wait_timeout(details::autocast{})};
    {i.can_be_blocked()} -> same_as<bool>;
    {i.descriptor()};
};
} // namespace core
