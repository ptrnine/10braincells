#pragma once

#include <span>
#include <stdexcept>
#include <vector>
#include <zlib.h>

#include <core/basic_types.hpp>

namespace util::zlib {
std::vector<core::byte> decompress(std::span<const core::byte> compressed_data) {
    // TODO: dont include zlib
    z_stream zs;
    zs.zalloc   = Z_NULL;
    zs.zfree    = Z_NULL;
    zs.opaque   = Z_NULL;
    zs.avail_in = core::uint(compressed_data.size());
    zs.next_in  = (Bytef*)(compressed_data.data());

    // Инициализация декомпрессора
    int ret = inflateInit(&zs);
    if (ret != Z_OK) {
        throw std::runtime_error("zlib error: " + std::string(zError(ret)));
    }

    std::vector<core::byte> decompressed_data;
    const int               CHUNK_SIZE = 16384;

    do {
        size_t oldSize = decompressed_data.size();
        decompressed_data.resize(oldSize + CHUNK_SIZE);

        zs.avail_out = CHUNK_SIZE;
        zs.next_out  = (Bytef*)decompressed_data.data() + oldSize;

        ret = inflate(&zs, Z_NO_FLUSH);

        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&zs);
            throw std::runtime_error("zlib inflate error: " + std::string(zError(ret)));
        }

        decompressed_data.resize(oldSize + CHUNK_SIZE - zs.avail_out);

    } while (ret != Z_STREAM_END);

    inflateEnd(&zs);
    return decompressed_data;
}
} // namespace util::zlib
