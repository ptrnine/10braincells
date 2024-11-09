#pragma once

#include <core/basic_types.hpp>

namespace core::io
{
template <typename T>
class rw_impl_buff_ref {
public:
    rw_impl_buff_ref(T& ibuff): buff(&ibuff) {}

    rw_impl_buff_ref(rw_impl_buff_ref&&) = default;
    rw_impl_buff_ref& operator=(rw_impl_buff_ref&&) = default;

    constexpr size_t size() const {
        return buff->size();
    }

    constexpr auto data(this auto&& it) {
        return it.buff->data();
    }

    constexpr void resize(size_t size) {
        buff->resize(size);
    }

protected:
    T* buff;
};
} // namespace core::io
