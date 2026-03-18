#pragma once

#include <cstdlib>

#include <core/basic_types.hpp>
#include <core/concepts/convertible_to.hpp>
#include <core/null.hpp>
#include <core/traits/is_array.hpp>
#include <core/traits/is_same.hpp>
#include <core/traits/remove_extent.hpp>

namespace core {
template <typename T, typename Deleter = void>
class box {
public:
    using type = remove_extent<T>;

    constexpr box(): ptr(nullptr) {}

    explicit constexpr box(type* pointer): ptr(pointer) {}
    explicit constexpr box(std::nullptr_t): ptr(nullptr) {}
    explicit constexpr box(null_t): ptr(nullptr) {}

    constexpr box(const box&)            = delete;
    constexpr box& operator=(const box&) = delete;

    constexpr box(box&& another_box) noexcept: ptr(another_box.release()) {}

    template <typename U> requires convertible_to<U*,T*> && (!is_array<U>)
    constexpr box(box<U>&& another_box) noexcept: ptr(another_box.release()) {}

    constexpr ~box() {
        if (ptr)
            destroy();
    }

    constexpr auto& operator=(box&& another_box) noexcept {
        if (ptr == another_box.ptr)
            return *this;

        destroy();
        ptr = another_box.release();

        return *this;
    }

    explicit constexpr operator type*() const {
        return ptr;
    }

    constexpr operator bool() const {
        return ptr;
    }

    template <typename D2>
    constexpr auto operator<=>(const box<T, D2>& rhs) const {
        return ptr <=> rhs.ptr;
    }

    constexpr type* get() const {
        return ptr;
    }

    constexpr type& operator*() const {
        return *get();
    }

    constexpr type* operator->() const {
        return get();
    }

    void replace(type* new_ptr) {
        ptr = new_ptr;
    }

    T* release() {
        T* result = ptr;
        ptr = nullptr;
        return result;
    }

protected:
    void destroy() const {
        if constexpr (is_same<Deleter, void>) {
            if constexpr (is_array<T>)
                delete [] ptr;
            else
                delete ptr;
        }
        else {
            Deleter{}(ptr);
        }
    }

    type* ptr;
};

template <typename T>
box(T) -> box<remove_extent<T>>;

template <typename T> requires (!is_array<T>)
constexpr box<T> boxed(auto&&... args) {
    return box<T>(new T(static_cast<decltype(args)>(args)...));
}

template <typename T> requires is_array<T>
constexpr box<T> boxed(size_t size) {
    return box<T>(new remove_extent<T>[size]);
}

struct malloc_deleter {
    void operator()(void* ptr) {
        __builtin_free(ptr);
    }
};

template <typename T>
using malloc_box = box<T, malloc_deleter>;

template <typename T> requires is_array<T>
constexpr box<T, malloc_deleter> malloc_boxed(size_t size) {
    return {(remove_extent<T>*)__builtin_malloc(size * sizeof(remove_extent<T>))};
}
} // namespace core
