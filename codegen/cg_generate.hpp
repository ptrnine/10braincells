#include <string>

#include <core/io/file.hpp>
#include <core/io/std.hpp>
#include <core/io/mmap.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

inline void
cg_generate(std::string_view name, auto&& args, const core::opt<std::string>& output_path, auto&& gen_func) {
    namespace io = core::io;

    if (output_path) {
        std::string guard_line = "/* Generated with " + std::string(name) + " */\n";
        auto        file =
            io::file::open(*output_path, io::openflags::read_write | io::openflags::create, io::file_perms::o644);
        if (sys::statx(file, sys::statx_mask::size).get().size == 0 ||
            io::mmap{file, io::map_flags::priv, io::map_prots::read}.as_sv().starts_with(guard_line)) {
            io::out out{file};
            out.write(guard_line);
            fwd(gen_func)(out, args);
        }
        else {
            throw std::runtime_error("Cannot generate at path " + *output_path);
        }
    }
    else {
        fwd(gen_func)(io::std_out, args);
    }
}

#undef fwd
