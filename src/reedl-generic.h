#pragma once
#include <stdarg.h>
#include <syslog.h>
#include <inttypes.h>

#ifndef CTASSERT /* Allow lint to override */
#define CTASSERT(x) _CTASSERT((x), __LINE__)
#define _CTASSERT(x, y) __CTASSERT(x, y)
#define __CTASSERT(x, y) typedef char __assert##y[(x) ? 1 : -1]
#endif

#define TBD 0

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/*
 * BIT Field operations
 */
#define FIELD_MASK(_field) _FIELD_MASK(_field)
#define FIELD_VAL(_field, _val) _FIELD_VAL(_field, _val)
#define FIELD_SET(_x, _field, _val) _SET_BITS(_x, _field, _val)
#define FIELD_CLR(_x, _field) _SET_BITS(_x, _field, 0)
#define FIELD_GET(_x, _field) _GET_BITS(_x, _field)
#define FIELD_TST(_x, _field) _TST_BITS(_x, _field)

#define _FIELD_MASK(_pos, _bits) (((1 << _bits) - 1) << _pos)

#define _SET_BITS(_x, _pos, _bits, _val)                 \
    do                                                   \
    {                                                    \
        _x = (_x & ~_FIELD_MASK(_pos, _bits)) |          \
             (((_val) & ((1 << _bits) - 1)) << _pos);    \
    } while (0)

#define _FIELD_VAL(_pos, _bits, _val) \
    (((_val) & ((1 << (_bits)) - 1)) << (_pos))

#define _GET_BITS(_x, _pos, _bits) (((_x) >> (_pos)) & ((1 << (_bits)) - 1))

#define _TST_BITS(_x, _pos, _bits) ((_x)&_FIELD_MASK(_pos, _bits))

static void __attribute__((unused))
unreferenced_vaargs(int __attribute__((unused)) x, ...)
{
}
#define UNREFERENCED_PARAMETER(P) unreferenced_vaargs(0, P)

#define MAC_IS_EMPTY(_mac_addr) ((_mac_addr)[0] == 0xFF)

#if 0
//        (mac_addr[0] == 0xFF && mac_addr[1] == 0xFF && mac_addr[2] == 0xFF
//         mac_addr[3] == 0xFF && mac_addr[4] == 0xFF && mac_addr[5] == 0xFF)
#endif

#define COUNT_OF(arr) (sizeof(arr) / sizeof(0 [arr]))

#define PRINTF_MAC_FORMAT "%02X:%02X:%02X:%02X:%02X:%02X"
#define PRINTF_MAC_VALUE(_addr)                                                \
    (_addr)[0], (_addr)[1], (_addr)[2], (_addr)[3], (_addr)[4], (_addr)[5]

#ifndef RELEASE_BUILD
#define RELEASE_BUILD 0
#endif

#if (0 && !RELEASE_BUILD)
#define DBG_IDX_MAX 256
int g_dbg[DBG_IDX_MAX] = { 0 };
int g_dbg_idx = 0;
#define push_dbg(x)                                                            \
    do                                                                         \
    {                                                                          \
        g_dbg[g_dbg_idx] = x;                                                  \
        g_dbg_idx++;                                                           \
        if (g_dbg_idx == DBG_IDX_MAX)                                          \
            g_dbg_idx = 0;                                                     \
    } while (0)

#endif


#ifndef DBG_LOG_EN
#define DBG_LOG_EN 1
#endif

#ifndef DBG_TAG
#define DBG_TAG "REEDL"
#endif

#ifndef DBG_LVL
#define DBG_LVL LOG_ERR
#endif

#if RELEASE_BUILD || !DBG_LOG_EN
#define DBG_LOG(...)                                                           \
    do                                                                         \
    {                                                                          \
        while (0)                                                              \
        {                                                                      \
            unreferenced_vaargs(0, __VA_ARGS__);                               \
        }                                                                      \
    } while (0)
#define DBG_INFO(...)                                                          \
    do                                                                         \
    {                                                                          \
        while (0)                                                              \
        {                                                                      \
            unreferenced_vaargs(0, __VA_ARGS__);                               \
        }                                                                      \
    } while (0)
#define DBG_ERR(...)                                                           \
    do                                                                         \
    {                                                                          \
        while (0)                                                              \
        {                                                                      \
            unreferenced_vaargs(0, __VA_ARGS__);                               \
        }                                                                      \
    } while (0)

#else
#define _DBG_PRINT(dbg_lvl, ...)                                               \
    do                                                                         \
    {                                                                          \
        if (DBG_LVL >= dbg_lvl)                                                \
            syslog(dbg_lvl, __VA_ARGS__);                                      \
    } while (0)

#define DBG_LOG(...) _DBG_PRINT(LOG_DEBUG, __VA_ARGS__)
#define DBG_INFO(...) _DBG_PRINT(LOG_INFO, __VA_ARGS__)
#define DBG_ERR(...) _DBG_PRINT(LOG_ERR, __VA_ARGS__)

#define SOCKADDR_IS_VALID(sock_addr)                                           \
    ((sock_addr).sin_addr.s_addr != INADDR_NONE)
#define SOCKADDR_IS_BC(sock_addr)                                              \
    (SOCKADDR_IS_VALID(sock_addr) &&                                           \
     ((sock_addr).sin_addr.s_addr == peer_addrs.bc.sin_addr.s_addr))

#define SOCKADDR_IS_NET(sock_addr)                                             \
    (SOCKADDR_IS_VALID(sock_addr) &&                                           \
     (((sock_addr).sin_addr.s_addr ^ peer_addrs.bc.sin_addr.s_addr) &          \
      peer_addrs.mask.sin_addr.s_addr) == 0)

#endif
