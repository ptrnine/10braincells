#pragma once

#define TBC_STRINGIFY(...) _tBC_STRINGIFY2(__VA_ARGS__)
#define _tBC_STRINGIFY2(...) #__VA_ARGS__

#if defined(TBC_DSA_DEBUG)
    #define TBC_DSA_LOG(...) __builtin_printf(__VA_ARGS__)
#else
    #define TBC_DSA_LOG(...) ((void)0)
#endif
