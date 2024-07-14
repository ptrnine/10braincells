#pragma once

#include <string_view>

#include <core/array.hpp>
#include <core/ranges/range.hpp>
#include <core/traits/declval.hpp>
#include <core/traits/remove_ref.hpp>

namespace core::views
{
template <typename T>
struct subst_entry {
    subst_entry() = default;
    subst_entry(const T* iwhat, const T* ireplace_with): what(iwhat), replace_with(ireplace_with) {}
    subst_entry(const std::basic_string_view<T>& iwhat, const std::basic_string_view<T>& ireplace_with):
        what(iwhat), replace_with(ireplace_with) {}

    std::basic_string_view<T> what;
    std::basic_string_view<T> replace_with;
};

template <typename T>
subst_entry(const T*, const T*) -> subst_entry<T>;

template <typename T>
subst_entry(const std::basic_string_view<T>&, const std::basic_string_view<T>&) -> subst_entry<T>;

struct subst_iterator_sentinel {};

template <typename CharT, size_t S, size_t MaxRecursionDepth = 4>
class subst_iterator {
public:
    static inline constexpr size_t npos = std::basic_string_view<CharT>::npos;

    constexpr subst_iterator(const random_access_range auto& input,
                             array<CharT, 3>                 subst_symbols,
                             array<subst_entry<CharT>, S>    subst_entries):
        st(subst_symbol_found),
        states{saved_pos{
            .restore_pos = 0,
            .end         = 0,
            .data        = std::basic_string_view<CharT>(input.data(), input.size()),
        }},
        states_count(1),
        symbols(subst_symbols),
        entries(subst_entries) {
        find_subst_symbol();
    }

    constexpr subst_iterator& operator++() {
        if (state().end == npos) {
            exit_subst();
            find_subst_symbol();
        }
        else {
            make_subst();
            find_subst_symbol();
        }

        return *this;
    }

    constexpr subst_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    constexpr auto operator*() const {
        const auto& s = state();
        return s.data.substr(0, s.end);
    }

    constexpr bool operator==(subst_iterator_sentinel) const {
        return is_end();
    }

    constexpr bool operator!=(subst_iterator_sentinel s) const {
        return !(*this == s);
    }

private:
    constexpr void find_subst_symbol() {
        if (states_count == 0)
            return;

        auto& [_, e, data] = this->state();
        while (e != npos) {
            e = data.find(symbols[0], e);
            if (e != npos) {
                if (e + 1 < data.size() && data[e + 1] == symbols[1])
                    break;
                if (e + 1 < data.size())
                    ++e;
            }
        }
    }

    constexpr void make_subst() {
        auto& [_, e, data] = this->state();
        if (e == npos)
            return;

        e += 2;
        auto start = e;
        while (e < data.size() && data[e] != symbols[2])
            ++e;

        auto end = e;
        if (e < data.size())
            ++e;

        if (states_count == states.size())
            return;

        states[states_count++] = saved_pos{
            .restore_pos = e,
            .end         = 0,
            .data        = get_value(data.substr(start, end - start)),
        };
    }

    constexpr void exit_subst() {
        if (states_count == 0)
            return;

        --states_count;
        if (states_count == 0)
            return;

        auto& [_, e, data] = state();
        e = 0;
        data = data.substr(states[states_count].restore_pos);
    }

    constexpr std::basic_string_view<CharT> get_value(const std::basic_string_view<CharT>& key) const {
        for (auto&& [k, v] : entries)
            if (k == key)
                return v;
        return {};
    }

    constexpr bool is_end() const {
        return states_count == 0;
    }

private:
    enum state_e {
        subst_symbol_found = 0,
        after_subst,
    };

    struct saved_pos {
        size_t                        restore_pos;
        size_t                        end;
        std::basic_string_view<CharT> data;
    };

    constexpr auto&& state(this auto&& it) {
        return it.states[it.states_count - 1];
    }

private:
    state_e                                 st;
    array<saved_pos, MaxRecursionDepth + 1> states;
    size_t                                  states_count;
    array<CharT, 3>                         symbols;
    array<subst_entry<CharT>, S>            entries;
};

template <typename CharT, size_t S, size_t MaxRecursionDepth = 4>
class subst {
public:
    constexpr subst(const auto&... subst_entries): symbols{'$', '{', '}'}, entries{subst_entries...} {}

    constexpr subst(array<CharT, 3> subst_symbols, const auto&... subst_entries):
        symbols{subst_symbols}, entries{subst_entries...} {}

    constexpr friend auto operator|(std::basic_string_view<CharT> data, const subst& subst) {
        return range_holder{
            subst_iterator<CharT, S, MaxRecursionDepth>{data, subst.symbols, subst.entries},
            subst_iterator_sentinel{},
        };
    }

    constexpr friend auto operator|(const CharT* data, const subst& subst) {
        return range_holder{
            subst_iterator<CharT, S, MaxRecursionDepth>{
                std::basic_string_view<CharT>(data), subst.symbols, subst.entries},
            subst_iterator_sentinel{},
        };
    }

private:
    array<CharT, 3>              symbols;
    array<subst_entry<CharT>, S> entries;
};

template <typename T, typename... Ts>
subst(const subst_entry<T>& entry, const subst_entry<Ts>&... entries) -> subst<T, sizeof...(entries) + 1>;

template <typename T, typename... Ts>
subst(array<T, 3>, const subst_entry<T>& entry, const subst_entry<Ts>&... entries) -> subst<T, sizeof...(entries) + 1>;
} // namespace core
