#pragma once

#ifndef RESTRICT_DEFINED
#if defined(__cplusplus)
    #if defined(__GNUC__) || defined(__clang__)
        #define restrict __restrict__
    #elif defined(_MSC_VER)
        #define restrict __restrict
    #else
        #define restrict
    #endif
#else
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
        #define restrict restrict
    #else
        #define restrict /* nothing */
    #endif
#endif
#define RESTRICT_DEFINED
#endif
