#pragma once

#include <stuff/core.hpp>

#include <limits>

namespace rv::arith {

template<typename T>
constexpr auto assert_false() -> bool {
    static_assert(!std::is_same_v<T, T>);
    return true;
}

template<typename T, T v>
constexpr auto assert_false() -> bool {
    return assert_false<std::integral_constant<T, v>>();
}

template<typename T, int SrcBits, int TypeBits = std::numeric_limits<T>::digits>
    requires(SrcBits > 0 && std::unsigned_integral<T>)
constexpr auto sext(T v) -> T {
    if constexpr (SrcBits >= TypeBits) {
        return v;
    } else {
        constexpr T mask = (T)-1 >> (T)SrcBits << (T)SrcBits;
        const auto sgn = ((v >> (SrcBits - 1)) & 1) == (T)1;

        return T(sgn * mask) | v;
    }
}

static_assert(sext<u16, 7>(0b0000'0000'0101'0101) == 0b1111'1111'1101'0101);
static_assert(sext<stf::nuint<16>, 7, 16>(0b0000'0000'0101'0101) == 0b1111'1111'1101'0101);
//static_assert(sext<stf::nuint<65>, 1, 65>(1) == (unsigned _BitInt(65))(-1));

template<std::unsigned_integral T>
constexpr auto sign_bit(T v) -> std::common_type_t<T, int> {
    constexpr auto bits = std::numeric_limits<T>::digits;
    return v >> (bits - 1);
}

static_assert(sign_bit<u32>(0) == 0);
static_assert(sign_bit<u32>(-1) == 1);
static_assert(sign_bit<u32>(-2) == 1);
static_assert(sign_bit<u32>(0x8000'0000) == 1);

template<std::unsigned_integral T, bool UseNative = false>
constexpr auto signed_compare(T lhs, T rhs) -> std::strong_ordering {
    if constexpr (UseNative) {
        using U = std::make_signed_t<T>;
        return (U)lhs <=> (U)rhs;
    }

    /* sign: ff tf ft tt
     *  >  : >  <  >  <
     *  <  : <  <  >  >
     *  == : == <  >  ==
     */

    const auto lhs_sgn = sign_bit(lhs);
    const auto rhs_sgn = sign_bit(rhs);

    if (lhs_sgn && !rhs_sgn) {
        return std::strong_ordering::less;
    }

    if (!lhs_sgn && rhs_sgn) {
        return std::strong_ordering::greater;
    }

    const auto base = (lhs << 1) <=> (rhs << 1);

    if (base == std::strong_ordering::equivalent) {
        return base;
    }

    const auto invert = lhs_sgn && rhs_sgn;

    if (!invert) {
        return base;
    }

    if (base == std::strong_ordering::less) {
        return std::strong_ordering::greater;
    }

    if (base == std::strong_ordering::greater) {
        return std::strong_ordering::less;
    }

    std::unreachable();
}

template<std::unsigned_integral T, bool UseNative = false>
constexpr auto arithmetic_shr(T v, T amt) -> T {
    if constexpr (UseNative) {
        using U = std::make_signed_t<T>;
        return (T)((U)v >> (U)amt);
    }

    const T mask = (T)-1 >> amt << amt;
    const auto sgn = sign_bit(v);
    return (v >> amt) | (mask * sgn);
}

static_assert(arithmetic_shr<u32>(-8, 2) == (u32)-2);
static_assert(arithmetic_shr<u32>(-4, 2) == (u32)-1);
static_assert(arithmetic_shr<u32>(-2, 2) == (u32)-1);
static_assert(arithmetic_shr<u32>(-1, 2) == (u32)-1);

static_assert(arithmetic_shr<u32, true>(-8, 2) == (u32)-2);
static_assert(arithmetic_shr<u32, true>(-4, 2) == (u32)-1);
static_assert(arithmetic_shr<u32, true>(-2, 2) == (u32)-1);
static_assert(arithmetic_shr<u32, true>(-1, 2) == (u32)-1);

template<std::unsigned_integral T, bool ExtendLhs = false, bool ExtendRhs = false, bool UseNative = true>
constexpr auto multiply(T lhs, T rhs) -> std::pair<T, T> {
    constexpr int base_bits = sizeof(T) * 8;
    using U = stf::nuint<base_bits * 2uz>;

    const auto x = ExtendLhs ? sext<U, base_bits>(static_cast<U>(lhs)) : static_cast<U>(lhs);
    const auto y = ExtendRhs ? sext<U, base_bits>(static_cast<U>(rhs)) : static_cast<U>(rhs);
    const auto res = static_cast<U>(x * y);

    return {static_cast<T>(res >> (sizeof(T) * 8)), static_cast<T>(res)};
}

static_assert(multiply<u64, false, false>(1, 2).first == 0);
static_assert(multiply<u64, false, false>(1, 2).second == 2);
static_assert(multiply<u64, false, false>(3, 0x7FFF'FFFF'FFFF'FFFFull).first == 1);

//static_assert(multiply<u8, true, true>(-2, -128).first == 1);
static_assert(multiply<u8, true, true>(2, -128).first == (u8)-1);
static_assert(multiply<u8, true, false>(2, -128).first != (u8)-1);

static_assert(multiply<u64, true, true>(1, 2).second == 2);
static_assert(multiply<u64, true, true>(-1, 2).second == (u64)-2);
static_assert(multiply<u64, true, true>(1, -2).second == (u64)-2);
static_assert(multiply<u64, true, true>(-1, -2).second == 2);
static_assert(multiply<u64, true, true>(3, 0x7FFF'FFFF'FFFF'FFFFull).first == 1);

}  // namespace rv::arith
