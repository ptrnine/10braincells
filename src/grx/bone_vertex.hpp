#pragma once

#include <cstring>

#include <core/array.hpp>
#include <util/assert.hpp>
#include <grx/basic_types.hpp>

namespace grx
{
template <size_t BonesPerVertex>
struct bone_vertex {
    [[nodiscard]]
    static constexpr size_t size() {
        return BonesPerVertex;
    }

    void append(uint bone_id, float weight) {
        float zero = 0.0f;
        for (size_t i = 0; i < size(); ++i) {
            if (!std::memcmp(&weights[i], &zero, sizeof(float))) {
                ids[i]     = bone_id;
                weights[i] = weight;
                return;
            }
        }

        ASSERT(false, "Not enough bone size. Weights: {}", weights);
    }

    core::array<u32, BonesPerVertex>   ids     = {0};
    core::array<float, BonesPerVertex> weights = {0};
};

using bone_vertex4 = bone_vertex<4>;
} // namespace grx
