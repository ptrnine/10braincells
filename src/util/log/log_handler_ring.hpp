#pragma once

#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

#include <core/box.hpp>
#include <core/ranges/split.hpp>
#include <core/ring_buffer.hpp>

#include <util/log/log_handler_base.hpp>

namespace util {
template <typename T>
class log_records {
public:
    template <typename I>
    log_records(I ibegin, I iend, size_t start_pos, size_t end_pos, size_t max_pos): mstart_pos(start_pos), mend_pos(end_pos), mmax_pos(max_pos) {
        records.resize(end_pos - start_pos);
        size_t i = 0;
        for (auto iter = ibegin; iter != iend; ++iter)
            records[i++] = *iter;
    }

    [[nodiscard]]
    auto begin() const {
        return records.begin();
    }

    [[nodiscard]]
    auto end() const {
        return records.end();
    }

    [[nodiscard]]
    size_t start_pos() const {
        return mstart_pos;
    }

    [[nodiscard]]
    size_t end_pos() const {
        return mend_pos;
    }

    [[nodiscard]]
    size_t max_pos() const {
        return mmax_pos;
    }

private:
    std::vector<T> records;
    size_t         mstart_pos;
    size_t         mend_pos;
    size_t         mmax_pos;
};

template <typename I>
log_records(I i, I, size_t, size_t, size_t) -> log_records<std::decay_t<decltype(*i)>>;

template <typename I>
class locked_log_range {
public:
    locked_log_range(I ibegin, I iend, size_t start_pos, size_t end_pos, size_t max_pos, std::shared_mutex& imtx):
        b(ibegin), e(iend), mstart_pos(start_pos), mend_pos(end_pos), mmax_pos(max_pos), mtx(imtx) {}

    ~locked_log_range() {
        mtx.unlock_shared();
    }

    [[nodiscard]]
    I begin() const {
        return b;
    }

    [[nodiscard]]
    I end() const {
        return e;
    }

    [[nodiscard]]
    size_t start_pos() const {
        return mstart_pos;
    }

    [[nodiscard]]
    size_t end_pos() const {
        return mend_pos;
    }

    [[nodiscard]]
    size_t max_pos() const {
        return mmax_pos;
    }

    [[nodiscard]]
    auto copy_records() const {
        return log_records(b, e, mstart_pos, mend_pos, mmax_pos);
    }

private:
    I                  b, e;
    size_t             mstart_pos;
    size_t             mend_pos;
    size_t             mmax_pos;
    std::shared_mutex& mtx;
};

class log_handler_ring : public log_handler_base {
public:
    struct record {
        std::string time;
        std::string msg;
        u64         times;
        log_level   lvl;
        write_type  wt;
    };

    log_handler_ring(size_t max_records): records(max_records) {}
    static core::box<log_handler_ring> create(size_t max_records) {
        return core::boxed<log_handler_ring>(max_records);
    }

    void write_handler(log_level level, write_type wt, std::string_view time, std::string_view msg, u64 times) final {
        std::unique_lock lock{mtx};

        if (wt == write_type::new_record || records.empty()) {
            std::string tm{time};
            last_lines_count = 0;
            for (auto line : msg | core::views::split('\n', '\r')) {
                /* Specify time in first line only */
                records.push({std::move(tm), {line.begin(), line.end()}, times, level, wt});
                ++last_lines_count;
            }
        } else if (last_lines_count <= records.size()) {
            auto offset = records.size() - last_lines_count;
            auto b      = records.begin() + ssize_t(offset);

            b->lvl   = level;
            b->time  = time;
            b->wt    = wt;
            b->times = times;

            if (wt == write_type::update) {
                size_t lines_count = 0;
                for (auto line : msg | core::views::split('\n', '\r')) {
                    if (b != records.end())
                        b->msg = std::string(line.begin(), line.end());
                    else
                        records.push({{}, {line.begin(), line.end()}, times, level, wt});
                    ++lines_count;
                    ++b;
                }

                while (lines_count < last_lines_count) {
                    records.pop();
                    --lines_count;
                }

                last_lines_count = lines_count;
            }
        }
    }

    auto read_lock(size_t preferred_start, size_t max_records) const {
        mtx.lock_shared();
        if (records.size() < max_records) {
            return locked_log_range(records.begin(), records.end(), 0, records.size(), records.size(), mtx);
        } else if (preferred_start + max_records > records.size()) {
            preferred_start = records.size() - max_records;
            return locked_log_range(records.begin() + ssize_t(preferred_start), records.end(), preferred_start, records.size(), records.size(), mtx);
        } else {
            return locked_log_range(
                records.begin() + ssize_t(preferred_start),
                records.begin() + ssize_t(preferred_start + max_records),
                preferred_start,
                preferred_start + max_records,
                records.size(),
                mtx
            );
        }
    }

    auto get_records(size_t preferred_start, size_t max_records) const {
        return read_lock(preferred_start, max_records).copy_records();
    }

    [[nodiscard]]
    size_t size() const {
        std::shared_lock lock{mtx};
        return records.size();
    }

    [[nodiscard]]
    size_t max_size() const {
        std::shared_lock lock{mtx};
        return records.max_size();
    }

    void max_size(size_t value) {
        std::unique_lock lock{mtx};
        records.resize(value);
    }

    void clear() {
        std::unique_lock lock{mtx};
        records.clear();
    }

private:
    core::ring_buffer<record> records;
    size_t                    last_lines_count = 0;
    mutable std::shared_mutex mtx;
};
} // namespace util
