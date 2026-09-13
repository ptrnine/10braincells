#pragma once

#include <util/nbt.hpp>

namespace util::mc {
class block {
public:
    block(class block_data* idata, size_t ipos): data(idata), pos(ipos) {}

    u32 index() const;
    std::string name() const;

private:
    class block_data* data;
    size_t            pos;
};

class block_data {
public:
    friend class block;

    block_data(nbt::tag_value& block_states): bs(&block_states) {
        palette = &bs->at("palette").as_list<nbt::types::compound>();
        auto data_raw = bs->find("data");
        if (data_raw) {
            data = &data_raw->get<nbt::types::long_array>();
        }
    }

    bool empty() const {
        return data == nullptr;
    }

    size_t bits_per_index() {
        auto palette_size = size_t(palette->data.size() - 1);
        if (palette_size == 0) {
            return 0;
        }

        auto bits = size_t(std::bit_width(palette_size));
        auto result = (bits + 7) / 8;

        return std::max(result, size_t(4));
    }

    block at(size_t pos) {
        // TODO bounds check
        return {this, pos};
    }

private:
    nbt::tag_value*                  bs;
    nbt::list<nbt::types::compound>* palette;
    std::vector<i64>*                data = nullptr;
};

namespace details {
    u64 low_bit_filled(size_t bits) {
        return ~u64(0) >> (64 - bits);
    }
}

u32 block::index() const {
    if (data->empty()) {
        return 0;
    }

    auto bpi        = data->bits_per_index();
    auto bit_offset = pos * bpi;

    auto long_offset     = bit_offset / 64;
    auto long_bit_offset = bit_offset - long_offset * 64;
    auto shift           = (64 - long_bit_offset);

    if (shift >= bpi) {
        shift -= bpi;
        return u32((u64((*data->data)[long_offset]) >> shift) & details::low_bit_filled(bpi));
    } else {
        auto high = (u64((*data->data)[long_offset]) << (bpi - shift)) & details::low_bit_filled(bpi);
        auto low = (u64((*data->data)[long_offset + 1]) >> (64 - (bpi - shift)));
        return u32(high | low);
    }
}

std::string block::name() const {
    return data->palette->data[index()]->at("Name").get<std::string>();
}
} // namespace util::mc
