#include <core/io/mmap.hpp>
#include <core/io/std.hpp>

#include <core/ranges/fold.hpp>
#include <core/ston.hpp>

using namespace core;

struct args_t {
    args_t(int argc, char** argv) {
        bool max_fields_init = false;
        for (size_t i = 0; i < size_t(argc); ++i) {
            std::string_view arg = argv[i];
            if (arg.starts_with("max_fields="sv)) {
                max_fields = ston<size_t>(arg.substr("max_fields="sv.size()));
                max_fields_init = true;
            }
            else if (arg == "--help") {
                help = true;
                io::std_err.write("cg_as_tuple - Generates code for casting structs into core::tuple\n"
                                  "\n"
                                  "Usage:\n"
                                  "    cg_as_tuple [OPTIONS]... [OUTPUT_FILE]\n"
                                  "\n"
                                  "Options:\n"
                                  "    max_fields=<uint> - maximum struct fields count\n"

                );
                return;
            }
            else {
                out_path = arg;
            }

            cmdline.push_back(arg);
        }

        if (!max_fields_init)
            throw std::runtime_error("Missing 'max_fields=' argument");
    }

    opt<std::string>              out_path;
    size_t                        max_fields;
    std::vector<std::string_view> cmdline;
    bool                          help;
};

void gen(auto&& out, const args_t& args) {
    out.write("/* Command: "sv, args.cmdline | views::fold{" "}, " */\n"sv);
    out.write("#pragma once\n"
              "\n"
              "#include <core/traits/fields_count.hpp>\n"
              "#include <core/traits/remove_cvref.hpp>\n"
              "#include <core/tuple.hpp>\n"
              "\n"
              "#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)\n"
              "\n"
              "namespace core\n"
              "{\n"
              "namespace details\n"
              "{\n"
              "    template <size_t>\n"
              "    struct as_tuple_impl;\n"
              "\n"
              "    template <>\n"
              "    struct as_tuple_impl<0> {\n"
              "        static constexpr auto ref(auto&) {\n"
              "            return tuple{};\n"
              "        }\n"
              "        static constexpr auto val(auto&&) {\n"
              "            return tuple{};\n"
              "        }\n"
              "    };\n"
              "\n"sv);

    std::string names = "a0";
    std::string fwds  = "fwd(a0)";

    for (size_t i = 1; i < args.max_fields + 1; ++i) {
        out.write("    template <>\n"sv,
                  "    struct as_tuple_impl<" + std::to_string(i) + "> {\n",
                  "        static constexpr auto ref(auto& s) {\n"sv,
                  "            auto&& [" + names + "] = s;\n",
                  "            return fwd_as_tuple(" + names + ");\n",
                  "        }\n"sv
                  "        static constexpr auto val(auto&& s) {\n"sv,
                  "            auto&& [" + names + "] = fwd(s);\n",
                  "            return fwd_as_tuple(" + fwds + ");\n",
                  "        }\n"
                  "    };\n"sv);
        auto next_name = "a" + std::to_string(i);
        names += ", " + next_name;
        fwds += ", fwd(" + next_name + ")";
    }

    out.write("} // namespace details\n"
              "\n"
              "template <typename T>\n"
              "constexpr auto as_tuple_ref(T&& structure) {\n"
              "    return details::as_tuple_impl<fields_count<remove_cvref<T>>>::ref(fwd(structure));\n"
              "}\n"
              "\n"
              "template <typename T>\n"
              "constexpr auto as_tuple(T&& structure) {\n"
              "    return details::as_tuple_impl<fields_count<remove_cvref<T>>>::val(fwd(structure));\n"
              "}\n"
              "} // namespace core\n"
              "\n"
              "#undef fwd\n"sv);
}

int main(int argc, char** argv) {
    args_t args{argc, argv};

    if (args.help)
        return 1;

    if (args.out_path) {
        std::string guard_line = "/* Generated with cg_as_tuple */\n";
        auto        file =
            io::file::open(*args.out_path, io::openflags::read_write | io::openflags::create, io::file_perms::o644);
        if (sys::statx(file, sys::statx_mask::size).get().size == 0 ||
            io::mmap{file, io::map_flags::priv, io::map_prots::read}.as_sv().starts_with(guard_line)) {
            io::out out{file};
            out.write(guard_line);
            gen(out, args);
        }
        else {
            throw std::runtime_error("Cannot generate at path " + *args.out_path);
        }
    }
    else {
        gen(io::std_out, args);
    }
}
