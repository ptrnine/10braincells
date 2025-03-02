#pragma once

#include <core/io/std.hpp>

namespace tbc::details {
    template <typename T>
    struct tbc_main_arg_type;

    template <typename R, typename T>
    struct tbc_main_arg_type<R(*)(T)> {
        using ret = R;
        using arg = core::remove_cvref<T>;
    };
}

template <typename R, typename Arg>
auto main_helper(int argc, const char** argv) {
    if constexpr (core::is_same<R, int>)
        return tbc_main(util::parse_args<Arg>(argc, argv));
    else {
        tbc_main(util::parse_args<Arg>(argc, argv));
        return 0;
    }
}

int main(int argc, const char** argv) {
    try {
        using traits = tbc::details::tbc_main_arg_type<decltype(&tbc_main)>;
        return main_helper<traits::ret, traits::arg>(argc, argv);
    } catch (const util::generate_help_interrupt& e) {
        core::io::std_out.write(e.text);
        return 1;
    }
}
