#include <catch2/catch_test_macros.hpp>

#include <iostream>



#include <core/string_builder.hpp>

#include <core/io/file.hpp>
#include <core/io/in.hpp>
#include <core/io/mmap.hpp>
#include <core/io/out.hpp>
#include <core/utility/as_const.hpp>
#include <sys/event.hpp>
#include <sys/lseek.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

using namespace core;

TEST_CASE("io_common") {
    SECTION("buffer_capacity") {
        io::in in{io::invalid_fd, size_c<1337>};
        static_assert(in.buffer_capacity() == 1337);

        io::out out{io::invalid_fd, size_c<1337>};
        static_assert(out.buffer_capacity() == 1337);

        io::in in2{io::invalid_fd};
        static_assert(in2.buffer_capacity() == 8192);

        io::out out2{io::invalid_fd};
        static_assert(out2.buffer_capacity() == 8192);
    }

    SECTION("base_type") {
        io::file file;

        io::in in1{io::invalid_fd};
        static_assert(type<decltype(in1.fd())> == type<const io::fd_t&>);

        io::in in2{mov(file)};
        static_assert(type<decltype(in2.fd())> == type<const io::file&>);

        io::out out1{io::invalid_fd};
        static_assert(type<decltype(out1.fd())> == type<const io::fd_t&>);

        io::out out2{mov(file)};
        static_assert(type<decltype(out2.fd())> == type<const io::file&>);

        io::in in3{std::vector<u8>{}};
        static_assert(type<decltype(in3.base_buff())> == type<std::vector<u8>&>);
        static_assert(type<decltype(as_const(in3).base_buff())> == type<const std::vector<u8>&>);

        io::out out3{std::vector<u8>{}};
        static_assert(type<decltype(out3.base_buff())> == type<std::vector<u8>&>);
        static_assert(type<decltype(as_const(out3).base_buff())> == type<const std::vector<u8>&>);

        std::vector<u8> buff;

        io::in in4{as_const(buff)};
        static_assert(type<decltype(in4.base_buff())> == type<io::rw_impl_buff_ref<const std::vector<u8>>&>);
        static_assert(type<decltype(as_const(in4).base_buff())> ==
                      type<const io::rw_impl_buff_ref<const std::vector<u8>>&>);

        io::out out4{buff};
        static_assert(type<decltype(out4.base_buff())> == type<io::rw_impl_buff_ref<std::vector<u8>>&>);
        static_assert(type<decltype(as_const(out4).base_buff())> == type<const io::rw_impl_buff_ref<std::vector<u8>>&>);

        io::in in5{file};
        static_assert(type<decltype(in5.fd())> == type<const io::fd_t&>);

        io::out out5{file};
        static_assert(type<decltype(out5.fd())> == type<const io::fd_t&>);
    }
}

#include <core/int_const.hpp>
#include <sys/ftruncate.hpp>

using namespace std::string_view_literals;
using namespace core::int_const_literals;

io::file memfd_with(const trivial_span_like auto& data, bool seals = false) {
    auto f = io::file::memfd("test", seals ? io::memfd_flags::allow_sealing : io::memfd_flags{});
    if (data.size()) {
        io::out o{f};
        o.write(data);
        o.seek(-data.size(), io::seek_whence::cur);
    }
    return f;
}

auto pipe_with(const trivial_span_like auto& data) {
    auto p = io::file::pipe(io::pipeflags::nonblock);
    io::out o{p.out};
    o.write(data);
    return p;
}

TEST_CASE("file") {
    SECTION("dtor_close") {
        io::fd_t fd;
        {
            auto f = io::file::memfd("test");
            fd     = f.fd();
            char c[1];
            REQUIRE(sys::read(fd, c, 1).ok());

            auto f2 = mov(f);
            REQUIRE(sys::read(fd, c, 1).ok());
        }

        char c[1];
        REQUIRE(sys::read(fd, c, 1).error());
    }
}

TEST_CASE("in") {
    auto test_read_bypass_buff = [](auto&& base) {
        io::in in{fwd(base), 0_zuc};

        bool ok = false;
        char b[8] = {};
        try {
            in.read(b);
        } catch (const io::partial_read_error& e) {
            ok = true;
            in.seek(-e.size, io::seek_whence::cur);
        }
        REQUIRE(ok);

        char c[7];
        auto sz = in.read(c);
        REQUIRE(sz == 7);
        REQUIRE(strcmp(c, "hello!") == 0);
    };

    auto test_read_buff2 = [](auto&& base) {
        io::in in{fwd(base), 0_zuc};

        bool ok = false;
        char b[2][5] = {};
        try {
            in.read(std::span{&b[0], 2});
        } catch (const io::partial_read_error& e) {
            ok = true;
            in.seek(-e.size, io::seek_whence::cur);
        }
        REQUIRE(ok);

        char c[2][4] = {};
        auto sz = in.read(std::span{&c[0], 2});
        REQUIRE(sz == 8);
        REQUIRE(strncmp(c[0], "hell", 4) == 0);
        REQUIRE(strncmp(c[1], "o!!!", 4) == 0);
    };

    auto test_read_buff = [](auto&& base) {
        io::in in{fwd(base), 5_zuc};

        bool ok = false;
        char b[2][5] = {};
        try {
            in.read(std::span{&b[0], 2});
        } catch (const io::partial_read_error& e) {
            ok = true;
            in.seek(-e.size, io::seek_whence::cur);
        }
        REQUIRE(ok);

        char c[2][4] = {};
        auto sz = in.read(std::span{&c[0], 2});
        REQUIRE(sz == 8);
        REQUIRE(strncmp(c[0], "hell", 4) == 0);
        REQUIRE(strncmp(c[1], "o!!!", 4) == 0);
    };

    auto test_seek = [](auto&& base) {
        io::in in{fwd(base), 5_zuc};
        char a[2], b[4], c[6], d[8], e[10];

        in.read(a);
        REQUIRE(strncmp(a, "he", 2) == 0);

        in.seek(0, io::seek_whence::set);
        in.read(b);
        REQUIRE(strncmp(b, "hell", 4) == 0);

        in.seek(-4, io::seek_whence::cur);
        in.read(c);
        REQUIRE(strncmp(c, "helloo", 6) == 0);

        in.seek(0, io::seek_whence::set);
        in.read(d);
        REQUIRE(strncmp(d, "helloo!!", 8) == 0);

        in.seek(-8, io::seek_whence::cur);
        in.read(e);
        REQUIRE(strncmp(e, "helloo!!??", 10) == 0);
    };

    SECTION("read_bypass") {
        test_read_bypass_buff(std::vector{'h', 'e', 'l', 'l', 'o', '!', '\0'});
        test_read_bypass_buff(memfd_with(std::vector{'h', 'e', 'l', 'l', 'o', '!', '\0'}));
    }

    SECTION("read") {
        test_read_buff(std::vector{'h', 'e', 'l', 'l', 'o', '!', '!', '!'});
        test_read_buff(memfd_with(std::vector{'h', 'e', 'l', 'l', 'o', '!', '!', '!'}));
    }

    SECTION("read2") {
        test_read_buff2(std::vector{'h', 'e', 'l', 'l', 'o', '!', '!', '!'});
        test_read_buff2(memfd_with(std::vector{'h', 'e', 'l', 'l', 'o', '!', '!', '!'}));
    }

    SECTION("seek") {
        std::string b = "helloo!!??";
        test_seek(b);
        test_seek(memfd_with(b));
    }

    SECTION("read_pipe_bypass") {
        std::vector data{'h', 'e', 'l', 'l', 'o', '!', '\0'};
        auto base = pipe_with(data);
        io::in in{base.in, 0_zuc};

        bool ok = false;
        char b[8] = {};
        try {
            in.read(b);
        } catch (const io::partial_read_error&) {
            ok = true;
            io::out{base.out}.write(data);
        }
        REQUIRE(ok);

        char c[7];
        auto sz = in.read(c);
        REQUIRE(sz == 7);
        REQUIRE(strcmp(c, "hello!") == 0);
    }

    SECTION("read_pipe_bypass2") {
        std::vector data{'h', 'e', 'l', 'l', 'o', '!', '!', '!'};
        auto base = pipe_with(data);
        io::in in{base.in, 0_zuc};

        bool ok = false;
        char b[2][5] = {};
        try {
            in.read(std::span{b});
        } catch (const io::partial_read_error&) {
            ok = true;
            io::out{base.out}.write(data);
        }
        REQUIRE(ok);

        char c[2][4] = {};
        auto sz = in.read(std::span{c});
        REQUIRE(sz == 8);
        REQUIRE(strncmp(c[0], "hell", 4) == 0);
        REQUIRE(strncmp(c[1], "o!!!", 4) == 0);
    };

    SECTION("read_pipe") {
        std::vector data{'h', 'e', 'l', 'l', 'o', '!', '!', '!'};
        auto base = pipe_with(data);
        io::in in{base.in, 5_zuc};

        bool ok = false;
        char b[2][5] = {};
        try {
            in.read(std::span{b});
        } catch (const io::partial_read_error&) {
            ok = true;
            io::out{base.out}.write(data);
        }
        REQUIRE(ok);

        char c[2][4] = {};
        auto sz = in.read(std::span{c});
        REQUIRE(sz == 8);
        REQUIRE(strncmp(c[0], "hell", 4) == 0);
        REQUIRE(strncmp(c[1], "o!!!", 4) == 0);
    };

    SECTION("ctors") {
        static_assert(default_ctor<decltype(io::in{type<io::file>})>);
        static_assert(default_ctor<decltype(io::in{type<io::file&>})>);
        static_assert(default_ctor<decltype(io::in{type<io::fd_t>})>);
        static_assert(default_ctor<decltype(io::in{type<io::fd_t&>})>);
        static_assert(default_ctor<decltype(io::in{type<std::vector<char>>})>);
        static_assert(default_ctor<decltype(io::in{type<std::vector<char>&>})>);

        static_assert(move_ctor<decltype(io::in{type<io::file>})>);
        static_assert(!copy_ctor<decltype(io::in{type<io::file>})>);
        static_assert(move_ctor<decltype(io::in{type<io::file&>})>);
        static_assert(!copy_ctor<decltype(io::in{type<io::file&>})>);

        static_assert(move_ctor<decltype(io::in{type<io::fd_t>})>);
        static_assert(!copy_ctor<decltype(io::in{type<io::fd_t>})>);
        static_assert(move_ctor<decltype(io::in{type<io::fd_t&>})>);
        static_assert(!copy_ctor<decltype(io::in{type<io::fd_t&>})>);

        static_assert(move_ctor<decltype(io::in{type<std::vector<char>>})>);
        static_assert(copy_ctor<decltype(io::in{type<std::vector<char>>})>);
        static_assert(move_ctor<decltype(io::in{type<std::vector<char>&>})>);
        static_assert(!copy_ctor<decltype(io::in{type<std::vector<char>&>})>);

        static_assert(type<decltype(io::in{io::file{}})> == type<io::in<io::file>>);
        static_assert(type<decltype(io::in{declval<const io::file&>()})> == type<io::in<io::fd_t>>);
        static_assert(type<decltype(io::in{io::fd_t{}})> == type<io::in<io::fd_t>>);
        static_assert(type<decltype(io::in{declval<const io::fd_t&>()})> == type<io::in<io::fd_t>>);
        static_assert(type<decltype(io::in{std::vector<char>{}})> == type<io::in<std::vector<char>>>);
        static_assert(type<decltype(io::in{declval<std::vector<char>&>()})> ==
                      type<io::in<io::rw_impl_buff_ref<const std::vector<char>>>>);
        static_assert(type<decltype(io::in{declval<const std::vector<char>&>()})> ==
                      type<io::in<io::rw_impl_buff_ref<const std::vector<char>>>>);
    }
}

TEST_CASE("out") {
    SECTION("ctors") {
        static_assert(default_ctor<decltype(io::out{type<io::file>})>);
        static_assert(default_ctor<decltype(io::out{type<io::file&>})>);
        static_assert(default_ctor<decltype(io::out{type<io::fd_t>})>);
        static_assert(default_ctor<decltype(io::out{type<io::fd_t&>})>);
        static_assert(default_ctor<decltype(io::out{type<std::vector<char>>})>);
        static_assert(default_ctor<decltype(io::out{type<std::vector<char>&>})>);

        static_assert(move_ctor<decltype( io::out{type<io::file>})>);
        static_assert(!copy_ctor<decltype(io::out{type<io::file>})>);
        static_assert(move_ctor<decltype( io::out{type<io::file&>})>);
        static_assert(!copy_ctor<decltype(io::out{type<io::file&>})>);

        static_assert(move_ctor<decltype( io::out{type<io::fd_t>})>);
        static_assert(!copy_ctor<decltype(io::out{type<io::fd_t>})>);
        static_assert(move_ctor<decltype( io::out{type<io::fd_t&>})>);
        static_assert(!copy_ctor<decltype(io::out{type<io::fd_t&>})>);

        static_assert(move_ctor<decltype(io::out{type<std::vector<char>>})>);
        static_assert(copy_ctor<decltype(io::out{type<std::vector<char>>})>);
        static_assert(move_ctor<decltype(io::out{type<std::vector<char>&>})>);
        static_assert(!copy_ctor<decltype(io::out{type<std::vector<char>&>})>);

        static_assert(type<decltype(io::out{io::file{}})> == type<io::out<io::file>>);
        static_assert(type<decltype(io::out{declval<const io::file&>()})> == type<io::out<io::fd_t>>);
        static_assert(type<decltype(io::out{io::fd_t{}})> == type<io::out<io::fd_t>>);
        static_assert(type<decltype(io::out{declval<const io::fd_t&>()})> == type<io::out<io::fd_t>>);
        static_assert(type<decltype(io::out{std::vector<char>{}})> == type<io::out<std::vector<char>>>);
        static_assert(type<decltype(io::out{declval<std::vector<char>&>()})> ==
                      type<io::out<io::rw_impl_buff_ref<std::vector<char>>>>);
        static_assert(type<decltype(io::out{declval<const std::vector<char>&>()})> ==
                      type<io::out<io::rw_impl_buff_ref<std::vector<char>>>>);
    }

    auto write_bypass_buff = [](auto&& base) {
        io::out o{base, 0_zuc};
        o.write("hello_123"sv);

        std::string s1;
        s1.resize(9);
        io::in i{base, 0_zuc};
        i.seek(0, io::seek_whence::set);
        i.read(s1);
        REQUIRE(s1 == "hello_123");
    };

    auto write_buff = [](auto&& base) {
        {
            io::out o{base, 4_zuc};
            o.write("hel"sv);
            o.write("l"sv);
            o.write("o_"sv);
            o.write("123"sv);
        }

        std::string s1;
        s1.resize(9);
        io::in i{base, 4_zuc};
        i.seek(0, io::seek_whence::set);
        i.read(std::span{s1.data(), 5});
        i.read(std::span{s1.data() + 5, 4});
        REQUIRE(s1 == "hello_123");
    };

    SECTION("write_bypass_buff") {
        write_bypass_buff(std::string{});
        write_bypass_buff(memfd_with(std::string{}));
    }

    SECTION("write_buff") {
        write_buff(std::string{});
        write_buff(memfd_with(std::string{}));
    }

    SECTION("partial_write") {
        class rw_impl_di : public io::rw_impl<io::fd_t> {
        public:
            using io::rw_impl<io::fd_t>::rw_impl;
            constexpr size_t handle_write(const void* data, size_t size) {
                if (size > 5)
                    size = 5;
                return io::rw_impl<io::fd_t>::handle_write(data, size);
            }
        };

        auto memf = io::file::memfd("test2");

        io::out_base<sys::fd_t, 10, rw_impl_di> o{memf.fd()};
        o.write("012"sv);
        o.write("345678"sv);

        bool ok = false;
        try {
            o.write("che");
        }
        catch (const io::partial_write_error& e) {
            ok = true;
            o.clear_buffer();
            o.seek(-off_t(e.size), io::seek_whence::cur);
        }
        o.write("qweqw"sv);
        o.flush();

        REQUIRE(ok);

        io::in i{memf};
        i.seek(0, io::seek_whence::set);

        std::string s(20, ' ');
        auto sz = i.read(s);
        s.resize(sz);
        REQUIRE(s.size() == 5);
        REQUIRE(s == "qweqw");
    }
}

TEST_CASE("mmap") {
    SECTION("ctors") {
        static_assert(is_same<decltype(io::mmap(io::file{})), io::mmap<io::file>>);
        static_assert(is_same<decltype(io::mmap(declval<io::file&>())), io::mmap<io::fd_t>>);
        static_assert(is_same<decltype(io::mmap(declval<const io::file&>())), io::mmap<io::fd_t>>);
        static_assert(is_same<decltype(io::mmap(declval<const io::file&&>())), io::mmap<io::fd_t>>);
        static_assert(is_same<decltype(io::mmap(io::invalid_fd)), io::mmap<io::fd_t>>);

        static_assert(!copy_ctor<io::mmap<io::file>>);
        static_assert(!copy_assign<io::mmap<io::file>>);
        static_assert(move_ctor<io::mmap<io::file>>);
        static_assert(move_assign<io::mmap<io::file>>);

        static_assert(!copy_ctor<io::mmap<io::fd_t>>);
        static_assert(!copy_assign<io::mmap<io::fd_t>>);
        static_assert(move_ctor<io::mmap<io::fd_t>>);
        static_assert(move_assign<io::mmap<io::fd_t>>);
    }

    SECTION("mmap_anon") {
        io::mmap map{io::map_flags::shared | io::map_flags::anon, io::map_prots::read | io::map_prots::write, 10};
        REQUIRE(map.size() == 10);
        std::memcpy(map.data(), "123456789", 10);
        REQUIRE("123456789"sv == map.data<char>());
    }

    SECTION("mmap_zero_size") {
        bool ok = false;
        try {
            io::mmap map{io::file::memfd("test"), io::map_flags::priv, io::map_prots::read};
        } catch (const errc_exception& e) {
            if (e.error() == errc::einval)
                ok = true;
        }
        REQUIRE(ok);
    }

    SECTION("mmap_no_auto_truncate_ro") {
        auto f = io::file::memfd("test");
        io::mmap map{f, io::map_flags::priv, io::map_prots::read, 4};
        REQUIRE(map.size() == 4);
        REQUIRE(sys::statx(f, sys::statx_mask::size).get().size == 0);
    }

    SECTION("mmap_auto_truncate_rw") {
        auto f = io::file::memfd("test");
        io::mmap map{f, io::map_flags::shared, io::map_prots::read | io::map_prots::write, 4};
        REQUIRE(map.size() == 4);
        REQUIRE(sys::statx(f, sys::statx_mask::size).get().size == 4);
    }

    SECTION("mmap_no_auto_truncate_rw") {
        auto f = io::file::memfd("test");
        sys::ftruncate(f, 8).throw_if_error();
        io::mmap map{f, io::map_flags::shared, io::map_prots::read | io::map_prots::write, 4};
        REQUIRE(map.size() == 4);
        REQUIRE(sys::statx(f, sys::statx_mask::size).get().size == 8);
    }

    SECTION("mmap_remap") {
        auto f = io::file::memfd("test");
        std::string s = "start";
        s.resize(4096, ' ');
        s += "end";

        io::out{f}.write(s);
        io::mmap map{f, io::map_flags::priv, io::map_prots::read, 5};
        REQUIRE(std::string_view(map.data<char>(), map.size()) == "start");

        map.remap(4099);
        REQUIRE(map.size() == 4099);
        REQUIRE(std::string_view(map.data<char>() + 4096, 3) == "end");
    }

    SECTION("mmap_truncate") {
        auto f = io::file::memfd("test");
        io::mmap map{f, io::map_flags::shared, io::map_prots::read, 4};
        REQUIRE(map.size() == 4);
        REQUIRE(sys::statx(f, sys::statx_mask::size).get().size == 0);
        map.truncate(4);
        REQUIRE(sys::statx(f, sys::statx_mask::size).get().size == 4);
    }

    SECTION("mmap_byteview") {
        auto f = io::file::memfd("test");
        {
            io::mmap map{f, io::map_flags::shared, io::map_prots::read | io::map_prots::write, 16};

            bool ok = false;
            try {
                map.from_byteview<u64>(1);
            } catch (const invalid_byteview_alignment&) {
                ok = true;
            }
            REQUIRE(ok);

            ok = false;
            try {
                map.from_byteview<u64>(16);
            } catch (const invalid_byteview_size&) {
                ok = true;
            }
            REQUIRE(ok);

            auto&& i = map.from_byteview<u64>(8);
            i = 0xdeadbeefdeadface;
        }
        {
            io::mmap map{f, io::map_flags::priv, io::map_prots::read, 16};
            REQUIRE(map.from_byteview<u64>(8) == 0xdeadbeefdeadface);
        }
    }
}
