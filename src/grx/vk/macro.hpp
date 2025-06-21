#pragma once

#define TBC_VK_DEF_INFO_ARG(INFO, bind_to, name) \
    constexpr struct _##name { \
        /* NOLINTNEXTLINE */ \
        constexpr auto operator=(decltype(INFO::bind_to) value) const { \
            struct _result { \
                constexpr void visit(INFO& info) { \
                    info.bind_to = value; \
                } \
                decltype(INFO::bind_to) value; \
            }; \
            return _result{value}; \
        } \
    } name
