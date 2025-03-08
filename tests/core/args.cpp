#include <catch2/catch_test_macros.hpp>

#include <iostream>

#include <util/arg_parse.hpp>


/* TODO: write test */

tbc_cmd(log) {
    util::arg::triopt enable;
};

tbc_cmd(trace) {
    tbc_arg(enable, bool);
};


tbc_cmd(main) {
    tbc_arg(test, bool);
    tbc_arg(lol, core::u64);
    tbc_sub(sub, log_cmd<>, trace_cmd<>);
};

TEST_CASE("args") {

    /*
    const char* as[] = {
        "./programname",
        "--test",
        "lol=0644",
        "trace",
    };

    auto a = util::parse_args<main_cmd<>>(int(std::size(as)), as);

    visit(a.sub,
          core::overloaded{
              [](const log_cmd<>& log) {
                  if (log.enable) {
                      std::cout << "Enable set" << std::endl;
                  }
                  else {
                      std::cout << "enable not set" << std::endl;
                  }
              },
              [](auto) {
                std::cout << __PRETTY_FUNCTION__ << std::endl;
              },
          });
    std::cout << "LOL: " << a.lol.get() << std::endl;
    */
}
