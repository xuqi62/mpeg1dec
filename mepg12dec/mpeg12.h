#pragma once
#include "get_bits.h"

typedef unsigned char uint8;

#define MP2_I_PICTURE 0x1
#define MP2_P_PICTURE 0x2
#define MP2_B_PICTURE 0x3
#define MP2_D_PICTURE 0x4

enum PIC_STRUCTURE
{
    TOP_FIELD = 1,
    BOTTOM_FIELD,
    FRAME,
};

typedef struct sequence_header
{
    int horizontal_size; //12bit
    int vertical_size;  // 12bit

    int aspect_ratio;
    int frame_rate;

    int bitrate;
    int vbv_buffer_size;

    int constrained_param;
    bool load_intra_quantiser_matrix;
    bool load_non_intra_quantiser_matrix;

    uint8 intra_quantiser_matrix[64];
    uint8 non_intra_quantiser_matrix[64];
}sequence_header;

typedef struct gop
{
    int   time_code;        //25bit
    bool  close_gop;        // 1bit
    bool  broken_link;      // 1bit
}gop;

typedef struct picture
{
    int   temporal_reference;        // picture display order, 10bit
    int   picture_coding_type;        //001: I, 010: P, 011: B, 3bit
    int   vbv_delay;                    // 16bit
    bool  full_pel_forward_vector;      // 1bit
    int   forward_f_code;               // 3bit
    bool  full_pel_backward_vector;
    int   backward_f_code;
}picture;

typedef struct
{
    unsigned long slice_start_code;
    unsigned long vertical_position;
    unsigned long vertical_position_extension;
    unsigned long priority_breakpoint;
    unsigned long quantiser_scale;		//MPEG1
    unsigned long quantiser_scale_code;	//MPEG2
    unsigned long intra_slice;
}slice_header;

typedef struct
{
    unsigned long mb_type;
    unsigned long mb_quant;
    unsigned long motion_forward;
    unsigned long motion_backward;
    unsigned long mb_pattern;
    unsigned long mb_intra;
    unsigned long coded_block_pattern;
    unsigned long cbp;
    unsigned long macroblock_address_increment;
    unsigned int quantiser_scale;

    int predict_mode;
#define FRAME_BASE 1
#define FILED_BASE 2
#define DUAL_PRIM  4

    // mpeg1
    // forward
    unsigned int motion_horizontal_forward_r;
    int motion_horizontal_forward_code;
    unsigned int motion_vertical_forward_r;
    int motion_vertical_forward_code;
    //backward
    int motion_horizontal_backward_code;
    unsigned int motion_horizontal_backward_r;
    int motion_vertical_backward_code;
    unsigned int motion_vertical_backward_r;

    int recon_right_for;
    int recon_down_for;
    int recon_right_back;
    int recon_down_back;
    //用于P/B帧帧间宏块mv预测，上一宏块的mv值
    int recon_right_for_prev;
    int recon_down_for_prev;
    int recon_right_back_prev;
    int recon_down_back_prev;

    long right_for_y; // 单位为像素大小
    long down_for_y;
    long right_half_for_y; //取值为0或1，表示该方向是否为半精度
    long down_half_for_y;
    long right_for_c;
    long down_for_c;
    long right_half_for_c;
    long down_half_for_c;
    long right_back_y;
    long down_back_y;
    long right_half_back_y;
    long down_half_back_y;
    long right_back_c;
    long down_back_c;
    long right_half_back_c;
    long down_half_back_c;

    //MPEG2
    unsigned long spatial_temporal_weight_code_flag;
    unsigned long spatial_temporal_weight_code_table_index;
    unsigned long spatial_temporal_weight_code;
    unsigned long frame_motion_type;
    unsigned long field_motion_type;
    unsigned long decode_dct_type;
    unsigned long dct_type;
    unsigned long quantiser_scale_code;
    unsigned long dmv;
    unsigned long mv_format;
    unsigned long motion_vector_count;
    unsigned long motion_vertical_field_select[2][2];
    // motion_code[r][s][t]
    // r: First/Second motion vector in Macroblock;
    // s: Forward/Backwards motion Vector 
    // t: Horizontal/Vertical Component 
    //    NOTE - r also takes the values 2 and 3 for derived motion vectors used with dualprime
    //    prediction.Since these motion vectors are derived they do not
    //    themselves have motion vector predictors.
    int motion_code[2][2][2];
    unsigned long motion_residual[2][2][2];
    long dmvector[2];
    int vector[4][2][2];

    //* reconstruct pixel for 6 blocks
    //* 4 blocks for luma, 2 blocks for chrom
    short recon[6][64]; 
}macroblock_info;

typedef struct
{
    unsigned long extension_start_code_identifier;
    unsigned long profile_and_level_indication;
    unsigned long progressive_sequence;
    unsigned long chroma_format;
    unsigned long horizontal_size_extension;
    unsigned long vertical_size_extension;
    unsigned long bit_rate_extension;
    unsigned long vbv_buffer_size_extension;
    unsigned long low_delay;
    unsigned long frame_rate_extension_n;
    unsigned long frame_rate_extension_d;
}sequence_extension_info;

typedef struct
{
    unsigned long extension_start_code_identifier;
    unsigned long f_code[2][2];
    unsigned long intra_dc_precision;
    unsigned long picture_structure;
    unsigned long top_field_first;
    unsigned long frame_pred_frame_dct;
    unsigned long concealment_motion_vectors;
    unsigned long q_scale_type;
    unsigned long intra_vlc_format;
    unsigned long alternate_scan;
    unsigned long repeat_first_field;
    unsigned long chroma_420_type;
    unsigned long progressive_frame;
    unsigned long composite_display_flag;
    unsigned long v_axis;
    unsigned long field_sequence;
    unsigned long sub_carrier;
    unsigned long burst_amplitude;
    unsigned long sub_carrier_phase;
}pic_coding_extension_info;


typedef struct
{
    uint8 buffer[4000 * 3000 * 3 / 2];
    int display_count; //显示顺序
    int decoded_count; //解码顺序
    int is_ref; // 是否为参考帧
}frame_buffer_info;

#define FRAME_BUFFER_NUM (5)
typedef struct mpeg12ctx
{
    sequence_header seq_header;
    sequence_extension_info seq_extension;
    pic_coding_extension_info pic_code_extension;
    gop group_of_pic;
    picture pic;
    slice_header sh;
    GetBitContext gb;
    macroblock_info mb_info;

    int mb_width;
    int mb_height;

    int is_mpeg2;

    uint8* scan_table;

    int cur_mblk_addr;
    int prev_mblk_addr;

    // 用于intra_mb DC预测
    int last_dc[3];

    int mb_x;
    int mb_y;

    int gop_pic_num;  // 一个gop内帧数
    int decoded_pic_num;

    int display_num_base;
    int display_pic_num; // display_pic_num = display_num_base + pic.temporal_ref

    // simple buffer manager, two ref picture
    frame_buffer_info frame_buf[FRAME_BUFFER_NUM];
    int cur_pic_idx;   // 范围0-2
    int last_pic_idx;  // 范围0-2
    int next_pic_idx;  // 范围0-2

    FILE* fp_dct_coeff;   // dct 参数文件
    FILE* fp_idct_out;    // IDCT结果文件
    FILE* fp_mv;  // 每个mb的mv信息
    FILE* fp_pred;  // 宏块的预测像素值
    FILE* fp_recon; //宏块重建值
    FILE* fp_pic_info;  // 每一帧的信息
    FILE* fp_yuv;
    int debug;  // dump data
}mpeg12ctx;

// api
int mpeg1_calc_mv(mpeg12ctx* mpeg12);
void mpeg2_update_qscale(mpeg12ctx* mpeg12);
void mb_do_mc(mpeg12ctx* mpeg12, int mb_x, int mb_y);
void mpeg1_decode_skipped_mb(mpeg12ctx* mpeg12, int mb_idx);