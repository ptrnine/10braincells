#pragma once

#define TBC_VEC_GEN_GET_2(A, B) \
    inline auto A##B() const { \
        return DerivedT<T, 2>{this->A(), this->B()}; \
    }

#define TBC_VEC_GEN_SET_2(A, B) \
    template <typename TT> \
    inline const auto& A##B(const DerivedT<TT, 2>& vec) { \
        this->A(vec.v[0]); this->B(vec.v[1]); \
        return static_cast<DerivedT<T, S>&>(*this);\
    } \
    template <typename T1, typename T2> \
    inline const auto& A##B(T1 a, T2 b) { \
        this->A(a); this->B(b); \
        return static_cast<DerivedT<T, S>&>(*this); \
    }

#define TBC_VEC_GEN_GET_3(A, B, C) \
    inline auto A##B##C() const { \
        return DerivedT<T, 3>{this->A(), this->B(), this->C()}; \
    }

#define TBC_VEC_GEN_SET_3(A, B, C) \
    template <typename TT> \
    inline const auto& A##B##C(const DerivedT<TT, 3>& vec) { \
        this->A(vec.v[0]); this->B(vec.v[1]); this->C(vec.v[2]); \
        return static_cast<DerivedT<T, S>&>(*this);\
    } \
    template <typename T1, typename T2, typename T3> \
    inline const auto& A##B##C(T1 a, T2 b, T3 c) { \
        this->A(a); this->B(b); this->C(c); \
        return static_cast<DerivedT<T, S>&>(*this); \
    }

#define TBC_VEC_GEN_GET_4(A, B, C, D) \
    inline auto A##B##C##D() const { \
        return DerivedT<T, 4>{this->A(), this->B(), this->C(), this->D()}; \
    }

#define TBC_VEC_GEN_SET_4(A, B, C, D) \
    template <typename TT> \
    inline const auto& A##B##C##D(const DerivedT<TT, 4>& vec) { \
        this->A(vec.v[0]); this->B(vec.v[1]); this->C(vec.v[2]); this->D(vec.v[3]); \
        return static_cast<DerivedT<T, S>&>(*this);\
    } \
    template <typename T1, typename T2, typename T3, typename T4> \
    inline const auto& A##B##C##D(T1 a, T2 b, T3 c, T4 d) { \
        this->A(a); this->B(b); this->C(c); this->D(d); \
        return static_cast<DerivedT<T, S>&>(*this); \
    }

#define TBC_VEC_GEN_GET_2_PERM(A, B) \
    TBC_VEC_GEN_GET_2(A, B) \
    TBC_VEC_GEN_GET_2(B, A)

#define TBC_VEC_GEN_GET_2_FROM_VEC2(A, B) \
    TBC_VEC_GEN_GET_2_PERM(A, B) \
    TBC_VEC_GEN_GET_2(A, A) \
    TBC_VEC_GEN_GET_2(B, B)

#define TBC_VEC_GEN_GET_2_FROM_VEC3(A, B, C) \
    TBC_VEC_GEN_GET_2_PERM(A, C) \
    TBC_VEC_GEN_GET_2_PERM(B, C) \
    TBC_VEC_GEN_GET_2(C, C)

#define TBC_VEC_GEN_GET_2_FROM_VEC4(A, B, C, D) \
    TBC_VEC_GEN_GET_2_PERM(A, D) \
    TBC_VEC_GEN_GET_2_PERM(B, D) \
    TBC_VEC_GEN_GET_2_PERM(C, D) \
    TBC_VEC_GEN_GET_2(D, D)

#define TBC_VEC_GEN_GET_3_PERM(A, B, C) \
    TBC_VEC_GEN_GET_3(A, B, C) \
    TBC_VEC_GEN_GET_3(B, C, A) \
    TBC_VEC_GEN_GET_3(C, A, B)

#define TBC_VEC_GEN_GET_3_FROM_VEC3(A, B, C) \
    TBC_VEC_GEN_GET_3_PERM(A, B, C) \
    TBC_VEC_GEN_GET_3_PERM(A, C, B) \
    TBC_VEC_GEN_GET_3_PERM(A, A, B) \
    TBC_VEC_GEN_GET_3_PERM(A, A, C) \
    TBC_VEC_GEN_GET_3_PERM(B, B, A) \
    TBC_VEC_GEN_GET_3_PERM(B, B, C) \
    TBC_VEC_GEN_GET_3_PERM(C, C, A) \
    TBC_VEC_GEN_GET_3_PERM(C, C, B) \
    TBC_VEC_GEN_GET_3(A, A, A) \
    TBC_VEC_GEN_GET_3(B, B, B) \
    TBC_VEC_GEN_GET_3(C, C, C)

#define TBC_VEC_GEN_GET_3_FROM_VEC4(A, B, C, D) \
    TBC_VEC_GEN_GET_3_PERM(A, B, D) \
    TBC_VEC_GEN_GET_3_PERM(A, D, B) \
    TBC_VEC_GEN_GET_3_PERM(A, C, D) \
    TBC_VEC_GEN_GET_3_PERM(A, D, C) \
    TBC_VEC_GEN_GET_3_PERM(B, C, D) \
    TBC_VEC_GEN_GET_3_PERM(B, D, C) \
    TBC_VEC_GEN_GET_3_PERM(A, A, D) \
    TBC_VEC_GEN_GET_3_PERM(B, B, D) \
    TBC_VEC_GEN_GET_3_PERM(C, C, D) \
    TBC_VEC_GEN_GET_3_PERM(D, D, A) \
    TBC_VEC_GEN_GET_3_PERM(D, D, B) \
    TBC_VEC_GEN_GET_3_PERM(D, D, C) \
    TBC_VEC_GEN_GET_3(D, D, D)

#define TBC_VEC_GEN_GET_4_FROM_VEC4(A, B, C, D) \
    TBC_VEC_GEN_GET_4(A, A, A, A) \
    TBC_VEC_GEN_GET_4(A, A, A, B) \
    TBC_VEC_GEN_GET_4(A, A, A, C) \
    TBC_VEC_GEN_GET_4(A, A, A, D) \
    TBC_VEC_GEN_GET_4(A, A, B, A) \
    TBC_VEC_GEN_GET_4(A, A, B, B) \
    TBC_VEC_GEN_GET_4(A, A, B, C) \
    TBC_VEC_GEN_GET_4(A, A, B, D) \
    TBC_VEC_GEN_GET_4(A, A, C, A) \
    TBC_VEC_GEN_GET_4(A, A, C, B) \
    TBC_VEC_GEN_GET_4(A, A, C, C) \
    TBC_VEC_GEN_GET_4(A, A, C, D) \
    TBC_VEC_GEN_GET_4(A, A, D, A) \
    TBC_VEC_GEN_GET_4(A, A, D, B) \
    TBC_VEC_GEN_GET_4(A, A, D, C) \
    TBC_VEC_GEN_GET_4(A, A, D, D) \
    TBC_VEC_GEN_GET_4(A, B, A, A) \
    TBC_VEC_GEN_GET_4(A, B, A, B) \
    TBC_VEC_GEN_GET_4(A, B, A, C) \
    TBC_VEC_GEN_GET_4(A, B, A, D) \
    TBC_VEC_GEN_GET_4(A, B, B, A) \
    TBC_VEC_GEN_GET_4(A, B, B, B) \
    TBC_VEC_GEN_GET_4(A, B, B, C) \
    TBC_VEC_GEN_GET_4(A, B, B, D) \
    TBC_VEC_GEN_GET_4(A, B, C, A) \
    TBC_VEC_GEN_GET_4(A, B, C, B) \
    TBC_VEC_GEN_GET_4(A, B, C, C) \
    TBC_VEC_GEN_GET_4(A, B, C, D) \
    TBC_VEC_GEN_GET_4(A, B, D, A) \
    TBC_VEC_GEN_GET_4(A, B, D, B) \
    TBC_VEC_GEN_GET_4(A, B, D, C) \
    TBC_VEC_GEN_GET_4(A, B, D, D) \
    TBC_VEC_GEN_GET_4(A, C, A, A) \
    TBC_VEC_GEN_GET_4(A, C, A, B) \
    TBC_VEC_GEN_GET_4(A, C, A, C) \
    TBC_VEC_GEN_GET_4(A, C, A, D) \
    TBC_VEC_GEN_GET_4(A, C, B, A) \
    TBC_VEC_GEN_GET_4(A, C, B, B) \
    TBC_VEC_GEN_GET_4(A, C, B, C) \
    TBC_VEC_GEN_GET_4(A, C, B, D) \
    TBC_VEC_GEN_GET_4(A, C, C, A) \
    TBC_VEC_GEN_GET_4(A, C, C, B) \
    TBC_VEC_GEN_GET_4(A, C, C, C) \
    TBC_VEC_GEN_GET_4(A, C, C, D) \
    TBC_VEC_GEN_GET_4(A, C, D, A) \
    TBC_VEC_GEN_GET_4(A, C, D, B) \
    TBC_VEC_GEN_GET_4(A, C, D, C) \
    TBC_VEC_GEN_GET_4(A, C, D, D) \
    TBC_VEC_GEN_GET_4(A, D, A, A) \
    TBC_VEC_GEN_GET_4(A, D, A, B) \
    TBC_VEC_GEN_GET_4(A, D, A, C) \
    TBC_VEC_GEN_GET_4(A, D, A, D) \
    TBC_VEC_GEN_GET_4(A, D, B, A) \
    TBC_VEC_GEN_GET_4(A, D, B, B) \
    TBC_VEC_GEN_GET_4(A, D, B, C) \
    TBC_VEC_GEN_GET_4(A, D, B, D) \
    TBC_VEC_GEN_GET_4(A, D, C, A) \
    TBC_VEC_GEN_GET_4(A, D, C, B) \
    TBC_VEC_GEN_GET_4(A, D, C, C) \
    TBC_VEC_GEN_GET_4(A, D, C, D) \
    TBC_VEC_GEN_GET_4(A, D, D, A) \
    TBC_VEC_GEN_GET_4(A, D, D, B) \
    TBC_VEC_GEN_GET_4(A, D, D, C) \
    TBC_VEC_GEN_GET_4(A, D, D, D) \
    TBC_VEC_GEN_GET_4(B, A, A, A) \
    TBC_VEC_GEN_GET_4(B, A, A, B) \
    TBC_VEC_GEN_GET_4(B, A, A, C) \
    TBC_VEC_GEN_GET_4(B, A, A, D) \
    TBC_VEC_GEN_GET_4(B, A, B, A) \
    TBC_VEC_GEN_GET_4(B, A, B, B) \
    TBC_VEC_GEN_GET_4(B, A, B, C) \
    TBC_VEC_GEN_GET_4(B, A, B, D) \
    TBC_VEC_GEN_GET_4(B, A, C, A) \
    TBC_VEC_GEN_GET_4(B, A, C, B) \
    TBC_VEC_GEN_GET_4(B, A, C, C) \
    TBC_VEC_GEN_GET_4(B, A, C, D) \
    TBC_VEC_GEN_GET_4(B, A, D, A) \
    TBC_VEC_GEN_GET_4(B, A, D, B) \
    TBC_VEC_GEN_GET_4(B, A, D, C) \
    TBC_VEC_GEN_GET_4(B, A, D, D) \
    TBC_VEC_GEN_GET_4(B, B, A, A) \
    TBC_VEC_GEN_GET_4(B, B, A, B) \
    TBC_VEC_GEN_GET_4(B, B, A, C) \
    TBC_VEC_GEN_GET_4(B, B, A, D) \
    TBC_VEC_GEN_GET_4(B, B, B, A) \
    TBC_VEC_GEN_GET_4(B, B, B, B) \
    TBC_VEC_GEN_GET_4(B, B, B, C) \
    TBC_VEC_GEN_GET_4(B, B, B, D) \
    TBC_VEC_GEN_GET_4(B, B, C, A) \
    TBC_VEC_GEN_GET_4(B, B, C, B) \
    TBC_VEC_GEN_GET_4(B, B, C, C) \
    TBC_VEC_GEN_GET_4(B, B, C, D) \
    TBC_VEC_GEN_GET_4(B, B, D, A) \
    TBC_VEC_GEN_GET_4(B, B, D, B) \
    TBC_VEC_GEN_GET_4(B, B, D, C) \
    TBC_VEC_GEN_GET_4(B, B, D, D) \
    TBC_VEC_GEN_GET_4(B, C, A, A) \
    TBC_VEC_GEN_GET_4(B, C, A, B) \
    TBC_VEC_GEN_GET_4(B, C, A, C) \
    TBC_VEC_GEN_GET_4(B, C, A, D) \
    TBC_VEC_GEN_GET_4(B, C, B, A) \
    TBC_VEC_GEN_GET_4(B, C, B, B) \
    TBC_VEC_GEN_GET_4(B, C, B, C) \
    TBC_VEC_GEN_GET_4(B, C, B, D) \
    TBC_VEC_GEN_GET_4(B, C, C, A) \
    TBC_VEC_GEN_GET_4(B, C, C, B) \
    TBC_VEC_GEN_GET_4(B, C, C, C) \
    TBC_VEC_GEN_GET_4(B, C, C, D) \
    TBC_VEC_GEN_GET_4(B, C, D, A) \
    TBC_VEC_GEN_GET_4(B, C, D, B) \
    TBC_VEC_GEN_GET_4(B, C, D, C) \
    TBC_VEC_GEN_GET_4(B, C, D, D) \
    TBC_VEC_GEN_GET_4(B, D, A, A) \
    TBC_VEC_GEN_GET_4(B, D, A, B) \
    TBC_VEC_GEN_GET_4(B, D, A, C) \
    TBC_VEC_GEN_GET_4(B, D, A, D) \
    TBC_VEC_GEN_GET_4(B, D, B, A) \
    TBC_VEC_GEN_GET_4(B, D, B, B) \
    TBC_VEC_GEN_GET_4(B, D, B, C) \
    TBC_VEC_GEN_GET_4(B, D, B, D) \
    TBC_VEC_GEN_GET_4(B, D, C, A) \
    TBC_VEC_GEN_GET_4(B, D, C, B) \
    TBC_VEC_GEN_GET_4(B, D, C, C) \
    TBC_VEC_GEN_GET_4(B, D, C, D) \
    TBC_VEC_GEN_GET_4(B, D, D, A) \
    TBC_VEC_GEN_GET_4(B, D, D, B) \
    TBC_VEC_GEN_GET_4(B, D, D, C) \
    TBC_VEC_GEN_GET_4(B, D, D, D) \
    TBC_VEC_GEN_GET_4(C, A, A, A) \
    TBC_VEC_GEN_GET_4(C, A, A, B) \
    TBC_VEC_GEN_GET_4(C, A, A, C) \
    TBC_VEC_GEN_GET_4(C, A, A, D) \
    TBC_VEC_GEN_GET_4(C, A, B, A) \
    TBC_VEC_GEN_GET_4(C, A, B, B) \
    TBC_VEC_GEN_GET_4(C, A, B, C) \
    TBC_VEC_GEN_GET_4(C, A, B, D) \
    TBC_VEC_GEN_GET_4(C, A, C, A) \
    TBC_VEC_GEN_GET_4(C, A, C, B) \
    TBC_VEC_GEN_GET_4(C, A, C, C) \
    TBC_VEC_GEN_GET_4(C, A, C, D) \
    TBC_VEC_GEN_GET_4(C, A, D, A) \
    TBC_VEC_GEN_GET_4(C, A, D, B) \
    TBC_VEC_GEN_GET_4(C, A, D, C) \
    TBC_VEC_GEN_GET_4(C, A, D, D) \
    TBC_VEC_GEN_GET_4(C, B, A, A) \
    TBC_VEC_GEN_GET_4(C, B, A, B) \
    TBC_VEC_GEN_GET_4(C, B, A, C) \
    TBC_VEC_GEN_GET_4(C, B, A, D) \
    TBC_VEC_GEN_GET_4(C, B, B, A) \
    TBC_VEC_GEN_GET_4(C, B, B, B) \
    TBC_VEC_GEN_GET_4(C, B, B, C) \
    TBC_VEC_GEN_GET_4(C, B, B, D) \
    TBC_VEC_GEN_GET_4(C, B, C, A) \
    TBC_VEC_GEN_GET_4(C, B, C, B) \
    TBC_VEC_GEN_GET_4(C, B, C, C) \
    TBC_VEC_GEN_GET_4(C, B, C, D) \
    TBC_VEC_GEN_GET_4(C, B, D, A) \
    TBC_VEC_GEN_GET_4(C, B, D, B) \
    TBC_VEC_GEN_GET_4(C, B, D, C) \
    TBC_VEC_GEN_GET_4(C, B, D, D) \
    TBC_VEC_GEN_GET_4(C, C, A, A) \
    TBC_VEC_GEN_GET_4(C, C, A, B) \
    TBC_VEC_GEN_GET_4(C, C, A, C) \
    TBC_VEC_GEN_GET_4(C, C, A, D) \
    TBC_VEC_GEN_GET_4(C, C, B, A) \
    TBC_VEC_GEN_GET_4(C, C, B, B) \
    TBC_VEC_GEN_GET_4(C, C, B, C) \
    TBC_VEC_GEN_GET_4(C, C, B, D) \
    TBC_VEC_GEN_GET_4(C, C, C, A) \
    TBC_VEC_GEN_GET_4(C, C, C, B) \
    TBC_VEC_GEN_GET_4(C, C, C, C) \
    TBC_VEC_GEN_GET_4(C, C, C, D) \
    TBC_VEC_GEN_GET_4(C, C, D, A) \
    TBC_VEC_GEN_GET_4(C, C, D, B) \
    TBC_VEC_GEN_GET_4(C, C, D, C) \
    TBC_VEC_GEN_GET_4(C, C, D, D) \
    TBC_VEC_GEN_GET_4(C, D, A, A) \
    TBC_VEC_GEN_GET_4(C, D, A, B) \
    TBC_VEC_GEN_GET_4(C, D, A, C) \
    TBC_VEC_GEN_GET_4(C, D, A, D) \
    TBC_VEC_GEN_GET_4(C, D, B, A) \
    TBC_VEC_GEN_GET_4(C, D, B, B) \
    TBC_VEC_GEN_GET_4(C, D, B, C) \
    TBC_VEC_GEN_GET_4(C, D, B, D) \
    TBC_VEC_GEN_GET_4(C, D, C, A) \
    TBC_VEC_GEN_GET_4(C, D, C, B) \
    TBC_VEC_GEN_GET_4(C, D, C, C) \
    TBC_VEC_GEN_GET_4(C, D, C, D) \
    TBC_VEC_GEN_GET_4(C, D, D, A) \
    TBC_VEC_GEN_GET_4(C, D, D, B) \
    TBC_VEC_GEN_GET_4(C, D, D, C) \
    TBC_VEC_GEN_GET_4(C, D, D, D) \
    TBC_VEC_GEN_GET_4(D, A, A, A) \
    TBC_VEC_GEN_GET_4(D, A, A, B) \
    TBC_VEC_GEN_GET_4(D, A, A, C) \
    TBC_VEC_GEN_GET_4(D, A, A, D) \
    TBC_VEC_GEN_GET_4(D, A, B, A) \
    TBC_VEC_GEN_GET_4(D, A, B, B) \
    TBC_VEC_GEN_GET_4(D, A, B, C) \
    TBC_VEC_GEN_GET_4(D, A, B, D) \
    TBC_VEC_GEN_GET_4(D, A, C, A) \
    TBC_VEC_GEN_GET_4(D, A, C, B) \
    TBC_VEC_GEN_GET_4(D, A, C, C) \
    TBC_VEC_GEN_GET_4(D, A, C, D) \
    TBC_VEC_GEN_GET_4(D, A, D, A) \
    TBC_VEC_GEN_GET_4(D, A, D, B) \
    TBC_VEC_GEN_GET_4(D, A, D, C) \
    TBC_VEC_GEN_GET_4(D, A, D, D) \
    TBC_VEC_GEN_GET_4(D, B, A, A) \
    TBC_VEC_GEN_GET_4(D, B, A, B) \
    TBC_VEC_GEN_GET_4(D, B, A, C) \
    TBC_VEC_GEN_GET_4(D, B, A, D) \
    TBC_VEC_GEN_GET_4(D, B, B, A) \
    TBC_VEC_GEN_GET_4(D, B, B, B) \
    TBC_VEC_GEN_GET_4(D, B, B, C) \
    TBC_VEC_GEN_GET_4(D, B, B, D) \
    TBC_VEC_GEN_GET_4(D, B, C, A) \
    TBC_VEC_GEN_GET_4(D, B, C, B) \
    TBC_VEC_GEN_GET_4(D, B, C, C) \
    TBC_VEC_GEN_GET_4(D, B, C, D) \
    TBC_VEC_GEN_GET_4(D, B, D, A) \
    TBC_VEC_GEN_GET_4(D, B, D, B) \
    TBC_VEC_GEN_GET_4(D, B, D, C) \
    TBC_VEC_GEN_GET_4(D, B, D, D) \
    TBC_VEC_GEN_GET_4(D, C, A, A) \
    TBC_VEC_GEN_GET_4(D, C, A, B) \
    TBC_VEC_GEN_GET_4(D, C, A, C) \
    TBC_VEC_GEN_GET_4(D, C, A, D) \
    TBC_VEC_GEN_GET_4(D, C, B, A) \
    TBC_VEC_GEN_GET_4(D, C, B, B) \
    TBC_VEC_GEN_GET_4(D, C, B, C) \
    TBC_VEC_GEN_GET_4(D, C, B, D) \
    TBC_VEC_GEN_GET_4(D, C, C, A) \
    TBC_VEC_GEN_GET_4(D, C, C, B) \
    TBC_VEC_GEN_GET_4(D, C, C, C) \
    TBC_VEC_GEN_GET_4(D, C, C, D) \
    TBC_VEC_GEN_GET_4(D, C, D, A) \
    TBC_VEC_GEN_GET_4(D, C, D, B) \
    TBC_VEC_GEN_GET_4(D, C, D, C) \
    TBC_VEC_GEN_GET_4(D, C, D, D) \
    TBC_VEC_GEN_GET_4(D, D, A, A) \
    TBC_VEC_GEN_GET_4(D, D, A, B) \
    TBC_VEC_GEN_GET_4(D, D, A, C) \
    TBC_VEC_GEN_GET_4(D, D, A, D) \
    TBC_VEC_GEN_GET_4(D, D, B, A) \
    TBC_VEC_GEN_GET_4(D, D, B, B) \
    TBC_VEC_GEN_GET_4(D, D, B, C) \
    TBC_VEC_GEN_GET_4(D, D, B, D) \
    TBC_VEC_GEN_GET_4(D, D, C, A) \
    TBC_VEC_GEN_GET_4(D, D, C, B) \
    TBC_VEC_GEN_GET_4(D, D, C, C) \
    TBC_VEC_GEN_GET_4(D, D, C, D) \
    TBC_VEC_GEN_GET_4(D, D, D, A) \
    TBC_VEC_GEN_GET_4(D, D, D, B) \
    TBC_VEC_GEN_GET_4(D, D, D, C) \
    TBC_VEC_GEN_GET_4(D, D, D, D)


#define TBC_VEC_GEN_SET_2_PERM(A, B) \
    TBC_VEC_GEN_SET_2(A, B) \
    TBC_VEC_GEN_SET_2(B, A)

#define TBC_VEC_GEN_SET_2_FROM_VEC2(A, B) \
    TBC_VEC_GEN_SET_2_PERM(A, B)

#define TBC_VEC_GEN_SET_2_FROM_VEC3(A, B, C) \
    TBC_VEC_GEN_SET_2_PERM(A, C) \
    TBC_VEC_GEN_SET_2_PERM(B, C)

#define TBC_VEC_GEN_SET_2_FROM_VEC4(A, B, C, D) \
    TBC_VEC_GEN_SET_2_PERM(A, D) \
    TBC_VEC_GEN_SET_2_PERM(B, D) \
    TBC_VEC_GEN_SET_2_PERM(C, D)

#define TBC_VEC_GEN_SET_3_PERM(A, B, C) \
    TBC_VEC_GEN_SET_3(A, B, C) \
    TBC_VEC_GEN_SET_3(B, C, A) \
    TBC_VEC_GEN_SET_3(C, A, B)

#define TBC_VEC_GEN_SET_3_FROM_VEC3(A, B, C) \
    TBC_VEC_GEN_SET_3_PERM(A, B, C) \
    TBC_VEC_GEN_SET_3_PERM(A, C, B) \

#define TBC_VEC_GEN_SET_3_FROM_VEC4(A, B, C, D) \
    TBC_VEC_GEN_SET_3_PERM(A, B, D) \
    TBC_VEC_GEN_SET_3_PERM(A, D, B) \
    TBC_VEC_GEN_SET_3_PERM(A, C, D) \
    TBC_VEC_GEN_SET_3_PERM(A, D, C) \
    TBC_VEC_GEN_SET_3_PERM(B, C, D) \
    TBC_VEC_GEN_SET_3_PERM(B, D, C) \

#define TBC_VEC_GEN_SET_4_FROM_VEC4(A, B, C, D) \
    TBC_VEC_GEN_SET_4(A, B, C, D) \
    TBC_VEC_GEN_SET_4(A, B, D, C) \
    TBC_VEC_GEN_SET_4(A, C, B, D) \
    TBC_VEC_GEN_SET_4(A, C, D, B) \
    TBC_VEC_GEN_SET_4(A, D, B, C) \
    TBC_VEC_GEN_SET_4(A, D, C, B) \
    TBC_VEC_GEN_SET_4(B, A, C, D) \
    TBC_VEC_GEN_SET_4(B, A, D, C) \
    TBC_VEC_GEN_SET_4(B, C, A, D) \
    TBC_VEC_GEN_SET_4(B, C, D, A) \
    TBC_VEC_GEN_SET_4(B, D, A, C) \
    TBC_VEC_GEN_SET_4(B, D, C, A) \
    TBC_VEC_GEN_SET_4(C, A, B, D) \
    TBC_VEC_GEN_SET_4(C, A, D, B) \
    TBC_VEC_GEN_SET_4(C, B, A, D) \
    TBC_VEC_GEN_SET_4(C, B, D, A) \
    TBC_VEC_GEN_SET_4(C, D, A, B) \
    TBC_VEC_GEN_SET_4(C, D, B, A) \
    TBC_VEC_GEN_SET_4(D, A, B, C) \
    TBC_VEC_GEN_SET_4(D, A, C, B) \
    TBC_VEC_GEN_SET_4(D, B, A, C) \
    TBC_VEC_GEN_SET_4(D, B, C, A) \
    TBC_VEC_GEN_SET_4(D, C, A, B) \
    TBC_VEC_GEN_SET_4(D, C, B, A)
