#pragma once

#include <core/io/rw_impl.hpp>
#include <core/ranges/range.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core::io
{
class partial_write_error : public std::exception {
public:
    partial_write_error(size_t isize): size(isize) {}

    const char* what() const noexcept override {
        return "partial write";
    }

    size_t size;
};

template <typename Fd, size_t BS = 8192, typename B = rw_impl<Fd>>
class out_base : public B {
public:
    constexpr out_base(): B(false) {}
    constexpr out_base(auto&& base): B(fwd(base), false) {}

    static constexpr size_t buffer_capacity() {
        return BS;
    }

    ~out_base() {
        try {
            flush();
        }
        catch (...) {
        }
    }

    constexpr size_t buffer_size() const {
        return pos;
    }

    void write(const trivial_span_like auto& data) {
        constexpr auto element_size = sizeof(*data.data());
        size_t data_sz = data.size() * element_size;

        if (data_sz >= BS) {
            flush();
            auto wrote = this->handle_write(data.data(), data_sz);
            if (wrote != data_sz)
                throw partial_write_error(wrote);
        }
        else {
            auto free_space = BS - pos;
            if (free_space < data_sz)
                flush();

            __builtin_memcpy(buff + pos, data.data(), data_sz);
            pos += data_sz;
        }
    }

    void write(const trivial auto& data) {
        write(std::span{&data, 1});
    }

    template <input_range R> requires (!trivial<R> && !trivial_span_like<R>)
    void write(R&& data) {
        for (auto&& v : data)
            write(v);
    }

    void write(const auto& data1, const auto& data2, const auto&... data) {
        write(data1);
        write(data2);
        (write(data), ...);
    }

    /* XXX: thats can be dangerous... */
    template <size_t N> requires (N >= 1)
    void write(const char(&data)[N]) {
        write(std::span{data, N - 1});
    }

#ifndef DISABLE_ASYNC
    task<> write_async(const trivial_span_like auto& data) {
        constexpr auto element_size = sizeof(*data.data());
        size_t data_sz = data.size() * element_size;

        if (data_sz >= BS) {
            co_await flush_async();
            auto wrote = co_await this->handle_write_async(data.data(), data_sz);
            if (wrote != data_sz)
                throw partial_write_error(wrote);
        }
        else {
            auto free_space = BS - pos;
            if (free_space < data_sz)
                co_await flush_async();

            __builtin_memcpy(buff + pos, data.data(), data_sz);
            pos += data_sz;
        }
    }

    auto write_async(const trivial auto& data) {
        return write_async(std::span{&data, 1});
    }

    template <input_range R> requires (!trivial<R> && !trivial_span_like<R>)
    task<> write_async(R&& data) {
        for (auto&& v : data)
            co_await write_async(v);
    }

    task<> write_async(const auto& data1, const auto& data2, const auto&... data) {
        co_await write_async(data1);
        co_await write_async(data2);
        (co_await write_async(data), ...);
    }

    /* XXX: thats can be dangerous... */
    template <size_t N> requires (N >= 1)
    auto write_async(const char(&data)[N]) {
        return write_async(std::span{data, N - 1});
    }
#endif

    void flush() {
        if (pos) {
            auto wrote = this->handle_write(buff, pos);
            if (wrote != pos) {
                __builtin_memmove(buff, buff + wrote, pos - wrote);
                throw partial_write_error(wrote);
            }
            pos = 0;
        }
    }

#ifndef DISABLE_ASYNC
    task<> flush_async() {
        if (pos) {
            auto wrote = co_await this->handle_write_async(buff, pos);
            if (wrote != pos) {
                __builtin_memmove(buff, buff + wrote, pos - wrote);
                throw partial_write_error(wrote);
            }
            pos = 0;
        }
    }
#endif

    void clear_buffer() {
        pos = 0;
    }

    void seek(off_t offset, sys::seek_whence whence = sys::seek_whence::cur) {
        flush();
        switch (whence) {
        case sys::seek_whence::cur:
        case sys::seek_whence::set:
            this->handle_seek(offset, whence);
            return;
        case sys::seek_whence::end:
            throw errc_exception(errc::einval);
        }
    }

private:
    void destroy() {
        flush();
    }

private:
    size_t pos = 0;
    u8     buff[BS];
};
} // namespace core::io

#undef fwd
