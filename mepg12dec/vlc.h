#pragma once

#include <stdint.h>
typedef struct
{
    int run, level, len, sign;
} DCTtab;

struct VLC1
{
    int len;
    unsigned int code;
    int value;
};

struct VLC2
{
    int len;
    unsigned int code;
    int run;
    int level;
};

struct vlc_tab1
{
    int len;
    int value;
};

struct vlc_tab2
{
    char len;
    char run;
    char level;
    char padding_byte;
};

extern unsigned char non_linear_mquant_table[32];
extern unsigned char mpeg2_intra_q[64];
extern unsigned char mpeg2_inter_q[64];
extern unsigned char ff_zigzag_direct[64];
extern unsigned char ff_alternate_horizontal_scan[64];
extern unsigned char ff_alternate_vertical_scan[64];

extern char zigzag_direct[2][64];
extern unsigned char q_scale[2][32];
extern unsigned long DCTtabfirst[];
extern unsigned long DCTtabnext[];
extern unsigned long DCTtab0[];
extern unsigned long DCTtab0a[];
extern unsigned long DCTtab1[];
extern unsigned long DCTtab1a[];
extern unsigned long DCTtab2[];
extern unsigned long DCTtab3[];
extern unsigned long DCTtab4[];
extern unsigned long DCTtab5[];
extern unsigned long DCTtab6[];
extern struct vlc_tab1 dct_dc_size_luminance_table[512];
extern struct vlc_tab1 dct_dc_size_chrominance_table[1024];
extern struct vlc_tab1 motion_code_table[2048];
extern struct vlc_tab1 macroblock_address_increment_table[2048];
extern struct vlc_tab1 coded_block_pattern_table[512];
extern struct vlc_tab1 macroblock_type_p_table[64];
extern struct vlc_tab1 macroblock_type_b_table[64];

void init_vlcs();