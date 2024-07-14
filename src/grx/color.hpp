#pragma once

#include <core/concepts/same_as.hpp>
#include <core/meta/type.hpp>
#include <grx/basic_types.hpp>

namespace grx
{
using clr8_g    = u8;
using clr8_ga   = vec<u8, 2>;
using clr8_rgb  = vec<u8, 3>;
using clr8_rgba = vec<u8, 4>;
using clrf_g    = float;
using clrf_ga   = vec<float, 2>;
using clrf_rgb  = vec<float, 3>;
using clrf_rgba = vec<float, 4>;

template <typename T>
concept color_component = core::same_as<T, u8> || core::same_as<T, float>;

template <typename T>
constexpr size_t components_count(core::type_t<T> = {}) {
    if constexpr (util::math_vector<T>)
        return T::size();
    else
        return 1;
}

template <typename T>
constexpr T grayscale(auto value) {
    if constexpr (util::math_vector<T>)
        return T::filled_with(value);
    else
        return T{value};
}

namespace details
{
    template <typename NewComponentType, typename T>
    constexpr auto _component_type_cast() {
        if constexpr (util::math_vector<T>)
            return core::type<vec<NewComponentType, T::size()>>;
        else
            return core::type<NewComponentType>;
    }
} // namespace details

template <typename NewComponentType, typename ColorType>
using component_type_cast = core::decl_type<details::_component_type_cast<NewComponentType, ColorType>()>;

namespace details
{
    template <typename T>
    struct component_type_traits {
        using type = T;
    };

    template <typename T, size_t S>
    struct component_type_traits<vec<T, S>> {
        using type = T;
    };
} // namespace details

template <typename T, typename ComponentType>
concept have_component_type = core::is_same<ComponentType, typename details::component_type_traits<T>::type>;
} // namespace grx
