#include <cstring>
#include <sys/readdir.hpp>
#include <util/arg_parse.hpp>
#include <core/io/mmap.hpp>
#include <core/ranges/zip.hpp>
#include <util/log.hpp>
#include <util/ranges/dimensional_seq.hpp>
#include <util/zlib.hpp>
#include <util/nbt.hpp>
#include <util/mc_block_data.hpp>
#include <sys/dirent.hpp>
#include <util/mca.hpp>

tbc_cmd(main) {
    tbc_arg(world, std::string, "path to world directory"_ctstr);
};

using namespace core;
namespace nbt = util::nbt;

void tbc_main(main_cmd<> args) {
    util::mc::region region{args.world.get() + "/dimensions/minecraft/overworld/region", {0, 0}};

    for (auto pos : util::dimensional_seq(util::vec2u{32, 32})) {
        auto&& chunk = region.get_chunk(pos);

        auto&& nbt   = chunk.nbt();

        auto block_entities = nbt.at("").at("block_entities").try_as_list<nbt::types::compound>();
        for (auto&& [section_y, sect] : chunk.sections()) {
            auto block_states = sect.nbt().find("block_states");
            if (!block_states) {
                continue;
            }

            if (block_entities) {
                for (auto&& ent : block_entities->data) {
                    auto items_p = ent->find("Items");
                    if (items_p) {
                        auto items = items_p->try_as_list<nbt::types::compound>();
                        if (items) {
                            glog().info("Items:");
                            for (auto&& item : items->data) {
                                glog().info("  {}", item->at("id").get<std::string>());
                            }
                        }
                    }
                }
            }
        }
    }
}

#if 0
void tbc_main2(main_cmd<> args) {
    // auto anvil = io::mmap(io::file::open(args.world.get() + "/region/r.0.0.mca", io::openflags::read_only), io::map_flags::priv, io::map_prots::read);
    // glog().error("path: {}", path);
    auto anvil = io::mmap(io::file::open(path, io::openflags::read_only), io::map_flags::priv, io::map_prots::read);

    auto&& header = anvil.from_byteview<mca_header>();
    //glog().info("chunks count: {} {}", header.chunk_info.size(), header.update_time.size());

    size_t chunk_num = 0;
    for (auto&& [info, update_time] : zip_view(header.chunk_info, header.update_time)) {
        //glog().warn("sectors: {} offset: {} update_time: {}", info.get_sector_count(), info.get_offset(), update_time.get());
        if (info.get_sector_count() == 0) {
            //glog().error("invalid chunk: {}", chunk_num);
            ++chunk_num;
            continue;
        }
        ++chunk_num;


        auto offset     = info.get_offset() * mca_header::sector_size;
        auto chunk_size = info.get_sector_count() * mca_header::sector_size;

        if (offset >= anvil.size()) {
            glog().error("Invalid chunk: offset {} > file size {}", offset, anvil.size() / sizeof(u32));
            continue;
        }

        auto compressed_chunk_data = anvil.span().subspan(offset, chunk_size);

        u32 comp_size = 0;
        std::memcpy(&comp_size, compressed_chunk_data.data(), sizeof(comp_size));
        comp_size = std::byteswap(comp_size);

        compressed_chunk_data = compressed_chunk_data.subspan(sizeof(comp_size));

        auto comp_type = u8(compressed_chunk_data.front());
        compressed_chunk_data = compressed_chunk_data.subspan(sizeof(comp_type));

        //glog().warn("compressed size: {} type: {}", comp_size, u32(comp_type));
        if (comp_size == 0) {
            continue;
        }

        std::vector<core::byte> decompressed_data;
        if (comp_type == 2) {
            decompressed_data = util::zlib::decompress(compressed_chunk_data);
        } else {
            glog().warn("Invalid comp type: {}", u32(comp_type));
            continue;
        }
        //glog().warn("decompressed size: {}", decompressed_data.size());

        auto nbt = util::nbt::parse(decompressed_data);

        auto&& sections       = nbt->at("").at("sections").as_list<nbt::types::compound>();
        auto block_entities = nbt->at("").at("block_entities").try_as_list<nbt::types::compound>();
        for (auto&& sect : sections.data) {
            //glog().warn("keys {}", sect->keys());
            auto block_states = sect->find("block_states");
            if (!block_states) {
                continue;
            }

            //auto data = block_states->find("data");
            //if (!data) {
            //    continue;
            //}

            //glog().error("data: {}", data->get_tag_type());

            //auto&& palette = block_states->at("palette").as_list<nbt::types::compound>();
            //for (auto&& p : palette.data) {
            //    //glog().warn("palette e: {}", p->at("Name").get<std::string>());
            //}

            if (block_entities) {
                for (auto&& ent : block_entities->data) {
                    auto items_p = ent->find("Items");
                    if (items_p) {
                        auto items = items_p->try_as_list<nbt::types::compound>();
                        if (items) {
                            glog().info("Items:");
                            for (auto&& item : items->data) {
                                glog().info("  {}", item->at("id").get<std::string>());
                            }
                        }
                    }
                }
            }

            //util::mc::block_data bd{*block_states};
            //for (size_t i = 0; i < 4096; ++i) {
            //    if (bd.at(i).name() == "minecraft:chest") {
            //        for (auto&& p : palette.data) {
            //            if (p->at("Name").get<std::string>() != "minecraft:chest")
            //                continue;
            //            if (block_entities) {
            //                for (auto&& ent : block_entities->data) {
            //                    auto items_p = ent->find("Items");
            //                    if (items_p) {
            //                        auto items = items_p->try_as_list<nbt::types::compound>();
            //                        if (items) {
            //                            for (auto&& item : items->data) {
            //                                nbt::print(item);
            //                            }
            //                        }
            //                    }
            //                }
            //            }
            //        }
            //        glog().warn("block index: {}", bd.at(i).name());
            //    }
            //}
        }


        //util::nbt::print(nbt);
        //break;

        //break;
    }
}
#endif

#include <util/tbc_main.hpp>
