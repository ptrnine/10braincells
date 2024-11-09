#pragma once

#include <fcntl.h>

#include <core/array.hpp>
#include <core/tuple.hpp>
#include <sys/basic_types.hpp>
#include <sys/syscall.hpp>

namespace sys {
/*
 * [[[codegen start]]]
 * Generated with:
 * ./build/codegen/flags pipeflag pipeflags uint close_exec=O_CLOEXEC direct=O_DIRECT nonblock=O_NONBLOCK
 */
enum class pipeflag : uint {
    close_exec = O_CLOEXEC,
    direct = O_DIRECT,
    nonblock = O_NONBLOCK,
};

struct pipeflags {
    using enum pipeflag;

    constexpr pipeflags() = default;
    constexpr pipeflags(pipeflag ivalue): value(uint(ivalue)) {}

    constexpr pipeflags operator|(pipeflags flag) const {
        return pipeflag(uint(value | flag.value));
    }

    constexpr pipeflags operator&(pipeflags flag) const {
        return pipeflag(uint(value & flag.value));
    }

    constexpr pipeflags& operator|=(pipeflags flag) {
        value = uint(value | flag.value);
        return *this;
    }

    constexpr bool test(pipeflags flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr void unset(pipeflags flags) {
        value &= uint(~flags.value);
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{pipeflag::close_exec, std::string_view("close_exec")},
            core::tuple{pipeflag::direct, std::string_view("direct")},
            core::tuple{pipeflag::nonblock, std::string_view("nonblock")},
        };
        std::string res;
        for (auto&& [f, s] : flags) {
            if (uint(value) & uint(f)) {
                res.append(s);
                res.append(" | ");
            }
        }
        if (res.size() > 2)
            res.resize(res.size() - 2);
        return res;
    }

    uint value;
};

inline constexpr pipeflags operator|(pipeflag lhs, pipeflag rhs) {
    return pipeflag(uint(lhs) | uint(rhs));
}
/* [[[codegen end]]] */

struct pipe_result {
    fd_t in;
    fd_t out;
};

inline auto pipe(pipeflags flags = {}) {
    syscall_result<pipe_result> result{core::type<pipe_result>};
    auto res = syscall<void>(SYS_pipe2, &result.unsafe_get(), flags.value);
    if (!res.ok())
        result.set(res.unsafe_error());
    return result;
}
} // namespace sys
