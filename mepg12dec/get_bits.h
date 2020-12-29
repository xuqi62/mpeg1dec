#pragma once
#include <math.h>
#include <assert.h>
#include <stdint.h>
#include "log.h"

#define FFMAX(a, b) ((a) > (b) ? (a) : (b))

#define AV_INPUT_BUFFER_PADDING_SIZE 64
#define INT_MAX       2147483647    // maximum (signed) int value
typedef unsigned char uint8;
typedef char int8;
typedef unsigned int uint32;
typedef int int32;

typedef struct GetBitContext {
    const uint8 *buffer, *buffer_end;

    int index;
    int size_in_bits;        // bit size in buffer
    int size_in_bits_plus8;
} GetBitContext;

static inline int init_get_bits(GetBitContext* s, const uint8* buf, int bit_size)
{
    int ret = 0;
    if (bit_size >= INT_MAX - FFMAX(7, AV_INPUT_BUFFER_PADDING_SIZE * 8) || bit_size < 0 || !buf) {
        bit_size = 0;
        buf = NULL;
        ret = -1;
    }

    int buffer_size = (bit_size + 7) >> 3;

    s->buffer = buf;
    s->size_in_bits = bit_size;
    s->size_in_bits_plus8 = bit_size + 8;
    s->buffer_end = buf + buffer_size;
    s->index = 0;

    return ret;
}

#define NEG_USR32(a, s) (((uint32_t)(a)) >> (32 - (s)))
/**
* Read 1-25 bits.  
* careful: we donot check the end of bitstream
*/
static inline unsigned int get_bits(GetBitContext *s, int n)
{
    unsigned int re_index = s->index;
    uint8* t = (uint8*)(s->buffer) + (re_index >> 3);
    unsigned int re_cache = (t[0]<<24| t[1]<<16 | t[2]<<8| t[3]) << (re_index & 7);
    unsigned int tmp = NEG_USR32(re_cache, n);

    re_index += n;
    s->index = re_index;

    return tmp;
}

static inline void skip_bits(GetBitContext *s, int n)
{
    unsigned int re_index = s->index;
    re_index += n;
    s->index = re_index;
}

/**
* Show 1-25 bits.
*/
#define AV_BSWAP32(x) ((((x) << 8 & 0xff00) | ((x) >> 8 & 0x00ff)) << 16 | ((((x) >> 16) << 8 & 0xff00) | (((x) >> 16) >> 8 & 0x00ff)))
static inline unsigned int show_bits(GetBitContext *s, int n)
{
    unsigned int re_index = (s)->index; unsigned int re_cache;
    re_cache = AV_BSWAP32((*((const uint32_t*)((s)->buffer + (re_index >> 3))))) << (re_index & 7);
    //logd("===> re_cache: %x, n: %d, re_index: %d", re_cache, n, re_index);
    return NEG_USR32(re_cache, n);
}

static inline int get_bits_index(GetBitContext *s)
{
    return s->index;
}