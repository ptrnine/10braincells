#pragma once

#include <bit>
#include <ctime>

#include <core/array.hpp>
#include <core/io/mmap.hpp>
#include <util/basic_types.hpp>
#include <util/vec.hpp>
#include <util/nbt.hpp>
#include <util/zlib.hpp>

namespace util::mc {
struct mca_header_chunk_info {
    u32 data;

    u32 get_sector_count() const {
        return data >> 24;
    }

    u32 get_offset() const {
        return std::byteswap(data) >> 8;
    }
};

struct mca_header_time {
    u32 data;

    std::time_t get() const {
        return std::byteswap(data);
    }
};

struct mca_header {
    static inline constexpr size_t max_chunks  = 1024;
    static inline constexpr size_t sector_size = 4096;

    core::array<mca_header_chunk_info, max_chunks> chunk_info;
    core::array<mca_header_time, max_chunks>       update_time;
};

class unsaved_chunk_changes : public std::exception {
public:
    unsaved_chunk_changes(const vec2i& pos):
        msg(std::format(
            "Chunk {} {} in region r.{}.{}.mca has unsaved changes. Save the region or try with force=true", pos.x(), pos.y(), pos.x() / 32, pos.y() / 32
        )) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

class section {
public:
    section(class chunk* ichunk, nbt::compound* inbt_data): _chunk(ichunk), nbt_data(inbt_data) {}

    int get_y() const {
        return nbt_data->at("Y").get<i8>();
    }

    auto& nbt(this auto&& it) {
        return *it.nbt_data;
    }

private:
    void reload() const;

private:
    class chunk*   _chunk;
    nbt::compound* nbt_data;

    nbt::compound* block_states = nullptr;
    bool           dirty = false;
};

class chunk {
public:
    chunk(class region* iregion, size_t position): _region(iregion), pos(position) {}

    bool is_loaded() const {
        return loaded;
    }

    vec2i position_in_region() const {
        return {int(pos % 32), int(pos / 32)};
    }

    const nbt::compound& nbt() const {
        load_if_not_loaded();
        return *nbt_data;
    }

    nbt::compound& nbt() {
        load_if_not_loaded();
        return *nbt_data;
    }

    vec2i position_global() const;

    const mca_header_chunk_info& chunk_info() const;

    auto& sections(this auto&& it) {
        return it._sections;
    }

    auto& get_section(this auto&& it, int y) {
        return it._sections.at(y);
    }

private:
    void reload(bool force = false) const;

    void load_if_not_loaded() const {
        if (!is_loaded()) {
            reload();
        }
    }

private:
    class region*                    _region;
    size_t                           pos;
    mutable std::vector<core::byte>  decompressed;
    mutable core::box<nbt::compound> nbt_data;
    mutable std::map<int, section>   _sections;
    mutable bool                     loaded = false;

    bool dirty = false;
};

class unsaved_region_changes : public std::exception {
public:
    unsaved_region_changes(const vec2i& pos):
        msg(std::format("Region r.{}.{}.mca has unsaved changes. Save the region or try with force=true", pos.x(), pos.y())) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

class region {
public:
    region(std::string region_directory, vec2i position, bool load = false): region_dir(core::mov(region_directory)), pos(position) {
        if (load) {
            reload();
        }
    }

    bool is_loaded() const {
        return anvil.has_value();
    }

    std::string file_path() const {
        auto res = region_dir;
        auto file_name = std::format("r.{}.{}.mca", pos.x(), pos.y());
        if (res.ends_with('/')) {
            return res + file_name;
        } else {
            return res + '/' + file_name;
        }
    }

    std::span<core::byte> bytes() {
        load_if_not_loaded();
        return anvil->span();
    }

    std::span<const core::byte> bytes() const {
        load_if_not_loaded();
        return anvil->span();
    }

    const vec2i& position() const {
        return pos;
    }

    const mca_header& header() const {
        load_if_not_loaded();
        return *_header;
    }

    auto&& get_chunk(this auto&& it, size_t pos) {
        it.load_if_not_loaded();
        return it.chunks.at(pos);
    }

    auto&& get_chunk(this auto&& it, vec2u pos) {
        it.load_if_not_loaded();
        return it.chunks.at(pos.y() * 32 + pos.x());
    }

private:
    void reload(bool force = false) const {
        if (!force && dirty && anvil) {
            throw unsaved_region_changes{pos};
        }

        anvil =
            core::io::mmap{core::io::file::open(file_path(), sys::openflags::read_write), sys::map_flags::priv, sys::map_prots::read | sys::map_prots::write};
        _header = &anvil->from_byteview<mca_header>();

        chunks.clear();
        for (size_t i = 0; i < 1024; ++i) {
            chunks.emplace_back((region*)this, i);
        }
    }

    void load_if_not_loaded() const {
        if (!is_loaded()) {
            reload();
        }
    }

private:
    std::string                                       region_dir;
    vec2i                                             pos;
    mutable core::opt<core::io::mmap<core::io::file>> anvil;
    mutable mca_header*                               _header;
    mutable std::vector<chunk>                        chunks;

    bool dirty = false;
};

vec2i chunk::position_global() const {
    return _region->position() * 32 + position_in_region();
}

const mca_header_chunk_info& chunk::chunk_info() const {
    auto&& header = _region->header();
    // TODO: implement and use at()
    return header.chunk_info[pos];
}

void chunk::reload(bool force) const {
    if (!force && dirty && loaded) {
        throw unsaved_chunk_changes{position_global()};
    }

    auto&& info   = chunk_info();
    auto   offset = info.get_offset() * mca_header::sector_size;
    auto   size   = info.get_sector_count() * mca_header::sector_size;

    if (offset + size > _region->bytes().size()) {
        glog().error("Invalid offset {} and size for chunk {}", offset, size, position_global());
        loaded = true;
        return;
    }

    if (size == 0) {
        loaded = true;
        return;
    }

    std::span<const core::byte> compressed_data = _region->bytes().subspan(offset, size);

    auto comp_size = nbt::details::read_num<u32>(compressed_data);
    auto comp_type = nbt::details::read_num<u8>(compressed_data);

    if (comp_type != 2) {
        glog().error("Unsupported compression type: {}", int(comp_type));
        loaded = true;
        return;
    }

    if (comp_size == 0) {
        glog().error("Invalid compressed size: {}", comp_size);
        loaded = true;
        return;
    }

    decompressed = zlib::decompress(compressed_data);
    nbt_data     = nbt::parse(decompressed);

    auto&& sect_list = nbt_data->at("").at("sections").as_list<nbt::types::compound>();
    for (auto& sect : sect_list.data) {
        _sections.emplace(int(sect->at("Y").get<i8>()), section{(chunk*)this, sect.get()});
    }

    loaded = true;
}
} // namespace util::mc
