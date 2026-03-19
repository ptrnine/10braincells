#pragma once

#include <core/aggregate_exception.hpp>
#include <core/coro/task.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core::async {
struct task_result_void {};

namespace details {
    template <typename T>
    static inline constexpr bool is_task = false;

    template <typename T>
    static inline constexpr bool is_task<task<T>> = true;

    template <typename T>
    concept task_any = is_task<remove_cvref<T>>;

    template <typename T>
    struct task_result_s {
        using type = T;
    };

    template <>
    struct task_result_s<void> {
        using type = task_result_void;
    };
} // namespace details

template <details::task_any... Ts>
task<tuple<typename details::task_result_s<typename decay<Ts>::result_type>::type...>> wait_all(Ts&&... tasks) {
    using result_type = tuple<typename details::task_result_s<typename decay<Ts>::result_type>::type...>;
    result_type result;

    static constexpr auto _wait_one = []<size_t idx, typename T>(aggregate_exception& e, int_const<idx>, result_type& result, T&& task) -> core::task<void> {
        try {
            if constexpr (is_same<typename decay<T>::result_type, void>) {
                co_await fwd(task);
            } else {
                result[int_c<idx>] = co_await fwd(task);
            }
        } catch (...) {
            e.add_exception(std::current_exception());
        }
    };

    static constexpr auto _wait_all = []<size_t... ids>(aggregate_exception& e, result_type& result, idx_seq<ids...>, auto&&... tasks) -> core::task<void> {
        (co_await _wait_one(e, int_c<ids>, result, fwd(tasks)), ...);
        co_return;
    };

    aggregate_exception e;
    co_await _wait_all(e, result, make_idx_seq<sizeof...(Ts)>(), fwd(tasks)...);
    throw_aggregate_exception(e);

    co_return result;
}
} // namespace core::async

#undef fwd
