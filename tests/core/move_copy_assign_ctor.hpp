#pragma once

#include <catch2/catch_test_macros.hpp>

#include <core/concepts/assign.hpp>
#include <core/concepts/ctor.hpp>
#include <core/utility/move.hpp>

struct assign_ctor_state {
    bool copy_ctored = false;
    bool move_ctored = false;
    bool copy_assigned = false;
    bool move_assigned = false;
};

struct mca_mcc : assign_ctor_state {
    mca_mcc(assign_ctor_state) {}
    mca_mcc() {}
    mca_mcc(const mca_mcc&) {
        copy_ctored = true;
    }
    mca_mcc(mca_mcc&&) noexcept {
        move_ctored = true;
    }
    mca_mcc& operator=(const mca_mcc&) {
        copy_assigned = true;
        return *this;
    }
    mca_mcc& operator=(mca_mcc&&) noexcept {
        move_assigned = true;
        return *this;
    }
};
struct mca_cc : assign_ctor_state {
    mca_cc(assign_ctor_state) {}
    mca_cc() {}
    mca_cc(const mca_cc&) {
        copy_ctored = true;
    }
    mca_cc& operator=(mca_cc&&) noexcept {
        move_assigned = true;
        return *this;
    }
    mca_cc& operator=(const mca_cc&) {
        copy_assigned = true;
        return *this;
    }
    mca_cc(mca_cc&&) noexcept = delete;
};
struct mca_mc : assign_ctor_state {
    mca_mc(assign_ctor_state) {}
    mca_mc() {}
    mca_mc(mca_mc&&) noexcept {
        move_ctored = true;
    }
    mca_mc& operator=(mca_mc&&) noexcept {
        move_assigned = true;
        return *this;
    }
    mca_mc& operator=(const mca_mc&) {
        copy_assigned = true;
        return *this;
    }
    mca_mc(const mca_mc&) = delete;
};
struct mca_noc : assign_ctor_state {
    mca_noc(assign_ctor_state) {}
    mca_noc() {}
    mca_noc& operator=(mca_noc&&) noexcept {
        move_assigned = true;
        return *this;
    }
    mca_noc& operator=(const mca_noc&) {
        copy_assigned = true;
        return *this;
    }
    mca_noc(mca_noc&&) noexcept = delete;
    mca_noc(const mca_noc&) = delete;
};

struct ma_mcc : assign_ctor_state {
    ma_mcc(assign_ctor_state) {}
    ma_mcc() {}
    ma_mcc(ma_mcc&&) noexcept {
        move_ctored = true;
    }
    ma_mcc(const ma_mcc&) {
        copy_ctored = true;
    }
    ma_mcc& operator=(ma_mcc&&) noexcept {
        move_assigned = true;
        return *this;
    }
    ma_mcc& operator=(const ma_mcc&) = delete;
};
struct ma_cc : assign_ctor_state {
    ma_cc(assign_ctor_state) {}
    ma_cc() {}
    ma_cc(const ma_cc&) {
        copy_ctored = true;
    }
    ma_cc& operator=(ma_cc&&) noexcept {
        move_assigned = true;
        return *this;
    }
    ma_cc(ma_cc&&) noexcept = delete;
    ma_cc& operator=(const ma_cc&) = delete;
};
struct ma_mc : assign_ctor_state {
    ma_mc(assign_ctor_state) {}
    ma_mc() {}
    ma_mc(ma_mc&&) noexcept {
        move_ctored = true;
    }
    ma_mc& operator=(ma_mc&&) noexcept {
        move_assigned = true;
        return *this;
    }
    ma_mc(const ma_mc&) = delete;
    ma_mc& operator=(const ma_mc&) = delete;
};
struct ma_noc : assign_ctor_state {
    ma_noc(assign_ctor_state) {}
    ma_noc() {}
    ma_noc& operator=(ma_noc&&) noexcept {
        move_assigned = true;
        return *this;
    }
    ma_noc(ma_noc&&) noexcept = delete;
    ma_noc(const ma_noc&) = delete;
    ma_noc& operator=(const ma_noc&) = delete;
};

struct ca_mcc : assign_ctor_state {
    ca_mcc(assign_ctor_state) {}
    ca_mcc() {}
    ca_mcc(ca_mcc&&) noexcept {
        move_ctored = true;
    }
    ca_mcc(const ca_mcc&) {
        copy_ctored = true;
    }
    ca_mcc& operator=(const ca_mcc&) {
        copy_assigned = true;
        return *this;
    }
    ca_mcc& operator=(ca_mcc&&) noexcept = delete;
};
struct ca_cc : assign_ctor_state {
    ca_cc(assign_ctor_state) {}
    ca_cc() {}
    ca_cc(const ca_cc&) {
        copy_ctored = true;
    }
    ca_cc& operator=(const ca_cc&) {
        copy_assigned = true;
        return *this;
    }
    ca_cc(ca_cc&&) noexcept = delete;
    ca_cc& operator=(ca_cc&&) noexcept = delete;
};
struct ca_mc : assign_ctor_state {
    ca_mc(assign_ctor_state) {}
    ca_mc() {}
    ca_mc(ca_mc&&) noexcept {
        move_ctored = true;
    }
    ca_mc& operator=(const ca_mc&) {
        copy_assigned = true;
        return *this;
    }
    ca_mc(const ca_mc&) = delete;
    ca_mc& operator=(ca_mc&&) noexcept = delete;
};
struct ca_noc : assign_ctor_state {
    ca_noc(assign_ctor_state) {}
    ca_noc() {}
    ca_noc& operator=(const ca_noc&) {
        copy_assigned = true;
        return *this;
    }
    ca_noc(ca_noc&&) noexcept {
        move_assigned = true;
    }
    ca_noc(const ca_noc&) = delete;
    ca_noc& operator=(ca_noc&&) noexcept = delete;
};

struct noa_mcc : assign_ctor_state {
    noa_mcc(assign_ctor_state) {}
    noa_mcc() {}
    noa_mcc(noa_mcc&&) noexcept {
        move_ctored = true;
    }
    noa_mcc(const noa_mcc&) {
        copy_ctored = true;
    }
    noa_mcc& operator=(noa_mcc&&) noexcept = delete;
    noa_mcc& operator=(const noa_mcc&) = delete;
};
struct noa_cc : assign_ctor_state {
    noa_cc(assign_ctor_state) {}
    noa_cc() {}
    noa_cc(const noa_cc&) {
        copy_ctored = true;
    }
    noa_cc(noa_cc&&) noexcept = delete;
    noa_cc& operator=(noa_cc&&) noexcept = delete;
    noa_cc& operator=(const noa_cc&) = delete;
};
struct noa_mc : assign_ctor_state {
    noa_mc(assign_ctor_state) {}
    noa_mc() {}
    noa_mc(noa_mc&&) noexcept {
        move_ctored = true;
    }
    noa_mc(const noa_mc&) = delete;
    noa_mc& operator=(noa_mc&&) noexcept = delete;
    noa_mc& operator=(const noa_mc&) = delete;
};
struct noa_noc : assign_ctor_state {
    noa_noc(assign_ctor_state) {}
    noa_noc() {}
    noa_noc(noa_noc&&) noexcept = delete;
    noa_noc(const noa_noc&) = delete;
    noa_noc& operator=(noa_noc&&) noexcept = delete;
    noa_noc& operator=(const noa_noc&) = delete;
};

using core::move_ctor;
using core::copy_ctor;
using core::move_assign;
using core::copy_assign;

template <typename T>
concept check_mca_mcc = move_ctor<T> && copy_ctor<T> && move_assign<T> && copy_assign<T>;

template <typename T>
concept check_mca_cc = !move_ctor<T> && copy_ctor<T> && move_assign<T> && copy_assign<T>;

template <typename T>
concept check_mca_mc = move_ctor<T> && !copy_ctor<T> && move_assign<T> && copy_assign<T>;

template <typename T>
concept check_mca_noc = !move_ctor<T> && !copy_ctor<T> && move_assign<T> && copy_assign<T>;

template <typename T>
concept check_ma_mcc = move_ctor<T> && copy_ctor<T> && move_assign<T> && !copy_assign<T>;

template <typename T>
concept check_ma_cc = !move_ctor<T> && copy_ctor<T> && move_assign<T> && !copy_assign<T>;

template <typename T>
concept check_ma_mc = move_ctor<T> && !copy_ctor<T> && move_assign<T> && !copy_assign<T>;

template <typename T>
concept check_ma_noc = !move_ctor<T> && !copy_ctor<T> && move_assign<T> && !copy_assign<T>;

template <typename T>
concept check_ca_mcc = move_ctor<T> && copy_ctor<T> && !move_assign<T> && copy_assign<T>;

template <typename T>
concept check_ca_cc = !move_ctor<T> && copy_ctor<T> && !move_assign<T> && copy_assign<T>;

template <typename T>
concept check_ca_mc = move_ctor<T> && !copy_ctor<T> && !move_assign<T> && copy_assign<T>;

template <typename T>
concept check_ca_noc = !move_ctor<T> && !copy_ctor<T> && !move_assign<T> && copy_assign<T>;

template <typename T>
concept check_noa_mcc = move_ctor<T> && copy_ctor<T> && !move_assign<T> && !copy_assign<T>;

template <typename T>
concept check_noa_cc = !move_ctor<T> && copy_ctor<T> && !move_assign<T> && !copy_assign<T>;

template <typename T>
concept check_noa_mc = move_ctor<T> && !copy_ctor<T> && !move_assign<T> && !copy_assign<T>;

template <typename T>
concept check_noa_noc = !move_ctor<T> && !copy_ctor<T> && !move_assign<T> && !copy_assign<T>;

template <typename T>
inline void test_cctor(auto init, auto getter) {
    T v1{init};
    T v2 = v1;
    REQUIRE(getter(v2).copy_ctored);
}

template <typename T>
inline void test_mctor(auto init, auto getter) {
    T v1{init};
    T v2 = core::mov(v1);
    REQUIRE(getter(v2).move_ctored);
}

template <typename T>
inline void test_mctor_implicitly_deleted(auto init, auto getter) {
    T v1{init};
    T v2 = core::mov(v1);
    REQUIRE(getter(v2).copy_ctored);
    REQUIRE(!getter(v2).move_ctored);
}

template <typename T>
inline void test_cassign(auto init, auto getter) {
    T v1{init};
    T v2{init};
    v2 = v1;
    REQUIRE(getter(v2).copy_assigned);
}

template <typename T>
inline void test_massign(auto init, auto getter) {
    T v1{init};
    T v2{init};
    v2 = core::mov(v1);
    REQUIRE(getter(v2).move_assigned);
}

template <typename T>
inline void test_massign_implicitly_deleted(auto init, auto getter) {
    T v1{init};
    T v2{init};
    v2 = core::mov(v1);
    REQUIRE(getter(v2).copy_assigned);
    REQUIRE(!getter(v2).move_assigned);
}

template <typename T>
inline void test_mca_mcc(auto init, auto getter) {
    test_cctor<T>(init, getter);
    test_mctor<T>(init, getter);
    test_cassign<T>(init, getter);
    test_massign<T>(init, getter);
}

template <typename T>
inline void test_mca_cc(auto init, auto getter) {
    test_cctor<T>(init, getter);
    test_cassign<T>(init, getter);
    test_massign<T>(init, getter);
}

template <typename T>
inline void test_mca_mc(auto init, auto getter) {
    test_mctor<T>(init, getter);
    test_cassign<T>(init, getter);
    test_massign<T>(init, getter);
}

template <typename T>
inline void test_mca_noc(auto init, auto getter) {
    test_cassign<T>(init, getter);
    test_massign<T>(init, getter);
}

template <typename T>
inline void test_ma_mcc(auto init, auto getter) {
    test_cctor<T>(init, getter);
    test_mctor<T>(init, getter);
    test_massign<T>(init, getter);
}

template <typename T>
inline void test_ma_cc(auto init, auto getter) {
    test_cctor<T>(init, getter);
    test_massign<T>(init, getter);
}

template <typename T>
inline void test_ma_mc(auto init, auto getter) {
    test_mctor<T>(init, getter);
    test_massign<T>(init, getter);
}

template <typename T>
inline void test_ma_noc(auto init, auto getter) {
    test_massign<T>(init, getter);
}

template <typename T>
inline void test_ca_mcc(auto init, auto getter) {
    test_cctor<T>(init, getter);
    test_mctor<T>(init, getter);
    test_cassign<T>(init, getter);
}

template <typename T>
inline void test_ca_cc(auto init, auto getter) {
    test_cctor<T>(init, getter);
    test_cassign<T>(init, getter);
}

template <typename T>
inline void test_ca_mc(auto init, auto getter) {
    test_mctor<T>(init, getter);
    test_cassign<T>(init, getter);
}

template <typename T>
inline void test_ca_noc(auto init, auto getter) {
    test_cassign<T>(init, getter);
}

template <typename T>
inline void test_noa_mcc(auto init, auto getter) {
    test_cctor<T>(init, getter);
    test_mctor<T>(init, getter);
}

template <typename T>
inline void test_noa_cc(auto init, auto getter) {
    test_cctor<T>(init, getter);
}

template <typename T>
inline void test_noa_mc(auto init, auto getter) {
    test_mctor<T>(init, getter);
}

template <typename T>
inline void test_noa_noc(auto, auto) {}

#define CTOR_ASSIGN_TEST_V(WHAT, test, ...) static_assert(check_##test<WHAT<test>>); test_##test<WHAT<test>>(__VA_ARGS__)

#define CTOR_ASSIGN_TEST(WHAT, ...)                                                                                    \
    do {                                                                                                               \
        CTOR_ASSIGN_TEST_V(WHAT, mca_mcc, __VA_ARGS__);                                                                \
        CTOR_ASSIGN_TEST_V(WHAT, mca_cc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, mca_mc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, mca_noc, __VA_ARGS__);                                                                \
        CTOR_ASSIGN_TEST_V(WHAT, ma_mcc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, ma_cc, __VA_ARGS__);                                                                  \
        CTOR_ASSIGN_TEST_V(WHAT, ma_mc, __VA_ARGS__);                                                                  \
        CTOR_ASSIGN_TEST_V(WHAT, ma_noc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, ca_mcc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, ca_cc, __VA_ARGS__);                                                                  \
        CTOR_ASSIGN_TEST_V(WHAT, ca_mc, __VA_ARGS__);                                                                  \
        CTOR_ASSIGN_TEST_V(WHAT, ca_noc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, noa_mcc, __VA_ARGS__);                                                                \
        CTOR_ASSIGN_TEST_V(WHAT, noa_cc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, noa_mc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, noa_noc, __VA_ARGS__);                                                                \
    } while (0)

#define CTOR_ASSIGN_IMPLICITLY_DELETED_MCTOR_TEST(WHAT, ...)                                                           \
    do {                                                                                                               \
        CTOR_ASSIGN_TEST_V(WHAT, mca_mcc, __VA_ARGS__);                                                                \
        CTOR_ASSIGN_TEST_V(WHAT, mca_mc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, mca_noc, __VA_ARGS__);                                                                \
        CTOR_ASSIGN_TEST_V(WHAT, ma_mcc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, ma_mc, __VA_ARGS__);                                                                  \
        CTOR_ASSIGN_TEST_V(WHAT, ma_noc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, ca_mcc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, ca_mc, __VA_ARGS__);                                                                  \
        CTOR_ASSIGN_TEST_V(WHAT, ca_noc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, noa_mcc, __VA_ARGS__);                                                                \
        CTOR_ASSIGN_TEST_V(WHAT, noa_mc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, noa_noc, __VA_ARGS__);                                                                \
    } while (0)

#define CTOR_ASSIGN_IMPLICITLY_DELETED_MASSIGN_TEST(WHAT, ...)                                                         \
    do {                                                                                                               \
        CTOR_ASSIGN_TEST_V(WHAT, mca_mcc, __VA_ARGS__);                                                                \
        CTOR_ASSIGN_TEST_V(WHAT, mca_cc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, mca_mc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, mca_noc, __VA_ARGS__);                                                                \
        CTOR_ASSIGN_TEST_V(WHAT, ma_mcc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, ma_cc, __VA_ARGS__);                                                                  \
        CTOR_ASSIGN_TEST_V(WHAT, ma_mc, __VA_ARGS__);                                                                  \
        CTOR_ASSIGN_TEST_V(WHAT, ma_noc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, noa_mcc, __VA_ARGS__);                                                                \
        CTOR_ASSIGN_TEST_V(WHAT, noa_cc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, noa_mc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, noa_noc, __VA_ARGS__);                                                                \
    } while (0)

#define CTOR_ASSIGN_IMPLICITLY_DELETED_MCTOR_MASSIGN_TEST(WHAT, ...)                                                   \
    do {                                                                                                               \
        CTOR_ASSIGN_TEST_V(WHAT, mca_mcc, __VA_ARGS__);                                                                \
        CTOR_ASSIGN_TEST_V(WHAT, mca_mc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, mca_noc, __VA_ARGS__);                                                                \
        CTOR_ASSIGN_TEST_V(WHAT, ma_mcc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, ma_mc, __VA_ARGS__);                                                                  \
        CTOR_ASSIGN_TEST_V(WHAT, ma_noc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, noa_mcc, __VA_ARGS__);                                                                \
        CTOR_ASSIGN_TEST_V(WHAT, noa_mc, __VA_ARGS__);                                                                 \
        CTOR_ASSIGN_TEST_V(WHAT, noa_noc, __VA_ARGS__);                                                                \
    } while (0)

#define MOVE_COPY_TEST(WHAT, ...)                                                                                      \
    do {                                                                                                               \
        CTOR_ASSIGN_TEST_V(WHAT, mca_mcc, __VA_ARGS__);                                                                \
        CTOR_ASSIGN_TEST_V(WHAT, ma_mc, __VA_ARGS__);                                                                  \
        CTOR_ASSIGN_TEST_V(WHAT, ca_cc, __VA_ARGS__);                                                                  \
        CTOR_ASSIGN_TEST_V(WHAT, noa_noc, __VA_ARGS__);                                                                \
    } while (0)

#define MOVE_COPY_IMPLICITLY_DELETED_MOVE_TEST(WHAT, ...)                                                              \
    do {                                                                                                               \
        CTOR_ASSIGN_TEST_V(WHAT, mca_mcc, __VA_ARGS__);                                                                \
        CTOR_ASSIGN_TEST_V(WHAT, ma_mc, __VA_ARGS__);                                                                  \
        test_cctor<WHAT<ca_cc>>(__VA_ARGS__);                                                                          \
        test_cassign<WHAT<ca_cc>>(__VA_ARGS__);                                                                        \
        test_mctor_implicitly_deleted<WHAT<ca_cc>>(__VA_ARGS__);                                                       \
        test_massign_implicitly_deleted<WHAT<ca_cc>>(__VA_ARGS__);                                                     \
        CTOR_ASSIGN_TEST_V(WHAT, noa_noc, __VA_ARGS__);                                                                \
    } while (0)
