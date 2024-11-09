#pragma once

#include <core/io/rw_impl.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core::io
{
class partial_read_error : public std::exception {
public:
    partial_read_error(size_t isize): size(isize) {}

    const char* what() const noexcept override {
        return "partial read";
    }

    size_t size;
};

template <typename Fd, size_t BS = 8192, typename B = rw_impl<Fd>>
class in_base : public B {
public:
    constexpr in_base(): B(true) {}
    constexpr in_base(auto&& base): B(fwd(base), true) {}

    static constexpr size_t buffer_capacity() {
        return BS;
    }

    size_t read(trivial_span_like auto&& data) {
        constexpr auto element_size = sizeof(*data.data());
        size_t data_sz = data.size() * element_size;

        if constexpr (BS == 0) {
            auto read = this->handle_read(data.data(), data_sz);
            if (read % element_size)
                throw partial_read_error(read);
            return read;
        }

        if (size == 0)
            take_next();

        if (size == 0)
            return 0;

        if (data_sz > size) {
            auto read_from_buff    = relocate_to((u8*)data.data());
            auto reminder_sz       = data_sz - read_from_buff;
            auto read_from_backend = this->handle_read((u8*)data.data() + read_from_buff, reminder_sz);
            auto read              = read_from_buff + read_from_backend;
            auto len               = read / element_size;
            if (read % element_size)
                throw partial_read_error(read);
            return read;
        }
        else {
            __builtin_memcpy(data.data(), buff + pos, data_sz);
            size -= data_sz;
            pos  += data_sz;
            return data_sz;
        }
    }

    size_t read(trivial auto& data) {
        return read(std::span{&data, 1});
    }

    void seek(off_t offset, seek_whence whence = seek_whence::cur) {
        switch (whence) {
        case seek_whence::cur:
            if (offset > 0 && offset <= size) {
                pos += offset;
                size -= offset;
            }
            else {
                this->handle_seek(offset - size, whence);
                size = 0;
            }
            break;
        case seek_whence::set:
            this->handle_seek(offset, whence);
            size = 0;
            break;
        case seek_whence::end: throw errc_exception(errc::einval);
        }
    }

private:
    void take_next() {
        auto read = this->handle_read(buff, BS);
        size      = read;
        pos       = 0;
    }

    size_t relocate_to(u8* dst) noexcept {
        size_t actually_read = size;
        __builtin_memcpy(dst, buff + pos, size);
        size = 0;
        pos  = 0;
        return actually_read;
    }

private:
    size_t  size = 0;
    size_t  pos  = 0;
    u8      buff[BS];
};
} // namespace core::io

#undef fwd
