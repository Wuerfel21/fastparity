#pragma once
#include <cstdint>

#ifdef __clang__
// Clang can do most of these optimizations itself
// .. and is also too dumb to properly understand the asm statements
#define FASTPARITY_NOASM
#endif

#ifndef FASTPARITY_NOASM
    #ifdef __i386__
    #define FASTPARITY_i386
    #define FASTPARITY_x86_any
    #define FASTPARITY_SHIFT16_OVER32
    #endif
    #ifdef __x86_64__
    #define FASTPARITY_x86_64
    #define FASTPARITY_x86_any
    #define FASTPARITY_SHIFT16_OVER32
    #endif
#endif

#if (__GNUC__ >= 9) || defined FASTPARITY_NOASM
    #define FASTPARITY_MAYBE_CONSTEXPR constexpr
#else
    #define FASTPARITY_MAYBE_CONSTEXPR 
#endif


inline int fastparity32_impl(uint32_t x) {
    #if defined FASTPARITY_x86_any && !defined __POPCNT__
    int asmout;
    uint32_t clobber;
    uint32_t x2 = x;
    asm(R"(
        {movl %[x],%[clobber] |mov %[clobber], %[x]}
        {shrl $16,%[x]        |shr %[x], 16}
        {xorl %[x],%[clobber] |xor %[clobber], %[x]}
        {xorb %h[clobber], %b[clobber]|
                xor %b[clobber], %h[clobber]}
    )"
    : [x] "+r" (x2),"=@ccnp" (asmout), [clobber] "=Q" (clobber)
    : 
    : "cc" );

    return asmout;
    #else
    return __builtin_parity(x);
    #endif
}

inline int fastparity16_impl(uint16_t x) {
    #if defined FASTPARITY_x86_any
    int asmout;
    uint32_t x2 = x;
    asm(R"(
        {xorb %h[x], %b[x]|xor %b[x],%h[x]}
    )"
    : [x] "+Q" (x2),"=@ccnp" (asmout)
    : 
    : "cc" );
    return asmout;
    #else
    return fastparity32_impl(x);
    #endif
}

inline int fastparity8_impl(uint8_t x) {
    #if defined FASTPARITY_x86_any
    int asmout;
    uint32_t x2 = x;
    asm(R"(
        {orb $0, %b[x]|or %b[x],0}
    )"
    : [x] "+q" (x2),"=@ccnp" (asmout)
    : 
    : "cc" );
    return asmout;
    #else
    return fastparity16_impl(x);
    #endif
}


FASTPARITY_MAYBE_CONSTEXPR inline int fastparity8(uint8_t x) {
    if(__builtin_constant_p(__builtin_parity(x))) return __builtin_parity(x);
    if(__builtin_constant_p(__builtin_popcount(x)<=1)) return x ? 1 : 0;
    #ifndef FASTPARITY_NOASM
    return fastparity8_impl(x);
    #else
    return __builtin_parity(x);
    #endif
}


FASTPARITY_MAYBE_CONSTEXPR inline int fastparity16(uint16_t x) {
    if(__builtin_constant_p(__builtin_parity(x))) return __builtin_parity(x);
    if(__builtin_constant_p(__builtin_popcount(x)<=1)) return x ? 1 : 0;
    if(__builtin_constant_p(x <= 0xFF) && x <= 0xFF) return fastparity8((uint8_t)x);

    #ifndef FASTPARITY_NOASM
    return fastparity16_impl(x);
    #else
    return __builtin_parity(x);
    #endif
}

FASTPARITY_MAYBE_CONSTEXPR inline int fastparity32(uint32_t x) {
    if(__builtin_constant_p(__builtin_parity(x))) return __builtin_parity(x);
    if(__builtin_constant_p(__builtin_popcount(x)<=1)) return x ? 1 : 0;
    if(__builtin_constant_p(x <= 0xFFFF) && x <= 0xFFFF) return fastparity16((uint16_t)x);
    #ifdef FASTPARITY_SHIFT16_OVER32
    if(__builtin_constant_p((x & 0xFFFF0000) == x)) return fastparity16((uint16_t)(x>>16));
    #endif

    #ifndef FASTPARITY_NOASM
    return fastparity32_impl(x);
    #else
    return __builtin_parity(x);
    #endif
}
