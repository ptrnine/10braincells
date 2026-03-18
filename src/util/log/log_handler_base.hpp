#pragma once

#include <atomic>
#include <string_view>

#include <core/limits.hpp>

#include <util/basic_types.hpp>

namespace util {
enum class log_level { debug = 0, detail, info, warn, error };

class log_handler_base {
public:
    enum class write_type { new_record = 0, write_same, update };

    virtual ~log_handler_base() = default;

    virtual void write_handler(log_level level, write_type wt, std::string_view time, std::string_view msg, u64 times) = 0;

    void write(log_level level, std::string_view time, std::string_view msg, u64 msg_hash) {
        auto comphash   = u32((msg_hash & 0x00000000ffffffff) ^ (msg_hash >> 32));
        u64  d          = data.load(std::memory_order_acquire);
        auto prev_hash  = unpack_hash(d);
        auto same_times = unpack_times(d);

        auto wt = write_type::new_record;

        if (comphash != prev_hash)
            same_times = 0;
        else
            wt = write_type::write_same;
        if (same_times != core::limits<u16>::max())
            ++same_times;

        data.store(pack_data(comphash, same_times, 0), std::memory_order_release);
        write_handler(level, wt, time, msg, same_times);
    }

    void write_update(u16 update_id, log_level level, std::string_view time, std::string_view msg) {
        u64  d              = data.load(std::memory_order_acquire);
        auto hash           = unpack_hash(d);
        auto same_times     = unpack_times(d);
        auto prev_update_id = unpack_update_id(d);

        auto wt = write_type::update;
        if (update_id != prev_update_id) {
            same_times = 0;
            wt         = write_type::new_record;
        }
        if (same_times != core::limits<u16>::max())
            ++same_times;

        data.store(pack_data(hash, same_times, update_id), std::memory_order_release);
        write_handler(level, wt, time, msg, same_times);
    }

private:
    static u32 unpack_hash(u64 v) {
        return u32(v >> 32);
    }

    static u16 unpack_times(u64 v) {
        return u16((v & 0x00000000ffff0000) >> 16);
    }

    static u16 unpack_update_id(u64 v) {
        return u16(v & 0x000000000000ffff);
    }

    static u64 pack_data(u32 comphash, u16 times, u16 update_id) {
        return (u64(comphash) << 32) | (u64(times) << 16) | u64(update_id);
    }

private:
    std::atomic<u64> data = 0;
};
} // namespace util
