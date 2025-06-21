#pragma once

#include <grx/vk/extension_defines.hpp>

#include <grx/vk/device.cg.hpp>

namespace vk {
namespace arg {
    struct device_queue {
        u32 family_index;
        u32 count = 1;
    };

    constexpr struct _queue_create_info {
        constexpr auto operator=(const std::vector<arg::device_queue>& values) const {
            struct _result {
                constexpr _result(const std::vector<arg::device_queue>& ivalues) {
                    std::map<u32, u32> uniq;
                    for (auto&& v : ivalues) {
                        if (v.count > priorities.size()) {
                            priorities.resize(v.count, 1.f);
                        }
                        uniq.emplace(v.family_index, v.count);
                    }
                    for (auto&& [family, count] : uniq) {
                        values.emplace_back(device_queue_create_info{
                            .queue_family_index = family,
                            .queue_count        = count,
                            .queue_priorities   = priorities.data(),
                        });
                    }
                }

                constexpr void visit(device_create_info& info) {
                    info.queue_create_info_count = u32(values.size());
                    info.queue_create_infos = values.data();
                }

                std::vector<float> priorities = {};
                std::vector<device_queue_create_info> values;
            };
            return _result{values};
        }
    } queue_create_info;
} // namespace arg
} // namespace vk
