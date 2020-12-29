#include "stdafx.h"
#include <stdlib.h>
#include "mpeg12.h"
#include "vlc.h"

void mpeg12_set_skipped_mb_info(mpeg12ctx* mpeg12, int mb_addr)
{
    mpeg12->mb_x = mb_addr % mpeg12->mb_width;
    mpeg12->mb_y = mb_addr / mpeg12->mb_width;

    if (mpeg12->pic.picture_coding_type == MP2_P_PICTURE)
    {

    }
    else  // B picture
    {

    }
    
}

void mpeg2_update_qscale(mpeg12ctx* mpeg12)
{
    if (mpeg12->pic_code_extension.q_scale_type == 1)
    {
        mpeg12->mb_info.quantiser_scale = non_linear_mquant_table[mpeg12->mb_info.quantiser_scale_code];
    }
    else
    {
        mpeg12->mb_info.quantiser_scale = mpeg12->mb_info.quantiser_scale_code;// << 1;
    }
}

static void mpeg1_motion_vector_back(mpeg12ctx* mpeg12)
{
    int complement_horizontal_backward_r;
    int complement_vertical_backward_r;
    int right_big, right_little;
    int down_big, down_little;
    int new_vector;
    int tmp, tmp1;

    int backward_f = 1 << (mpeg12->pic.backward_f_code - 1);
    int back_max = (backward_f << 4) - 1;
    int back_min = -(backward_f << 4);

    if (backward_f == 1 || mpeg12->mb_info.motion_horizontal_backward_code == 0)
        complement_horizontal_backward_r = 0;
    else
        complement_horizontal_backward_r = backward_f - 1 - mpeg12->mb_info.motion_horizontal_backward_r;
    if (backward_f == 1 || mpeg12->mb_info.motion_vertical_backward_code == 0)
        complement_vertical_backward_r = 0;
    else
        complement_vertical_backward_r = backward_f - 1 - mpeg12->mb_info.motion_vertical_backward_r;
    right_little = mpeg12->mb_info.motion_horizontal_backward_code << (mpeg12->pic.backward_f_code - 1); //*gPicHeaderInfo.backward_f;
    if (right_little == 0)
        right_big = 0;
    else
    {
        if (right_little > 0)
        {
            right_little = right_little - complement_horizontal_backward_r;
            right_big = right_little - (backward_f << 5);
        }
        else
        {
            right_little = right_little + complement_horizontal_backward_r;
            right_big = right_little + (backward_f << 5);
        }
    }
    down_little = mpeg12->mb_info.motion_vertical_backward_code << (mpeg12->pic.backward_f_code - 1); //*gPicHeaderInfo.backward_f;
    if (down_little == 0)
        down_big = 0;
    else
    {
        if (down_little > 0)
        {
            down_little = down_little - complement_vertical_backward_r;
            down_big = down_little - (backward_f << 5);
        }
        else
        {
            down_little = down_little + complement_vertical_backward_r;
            down_big = down_little + (backward_f << 5);
        }
    }

    new_vector = mpeg12->mb_info.recon_right_back_prev + right_little;
    if (new_vector <= back_max && new_vector >= back_min)
        mpeg12->mb_info.recon_right_back = mpeg12->mb_info.recon_right_back_prev + right_little;
    else
        mpeg12->mb_info.recon_right_back = mpeg12->mb_info.recon_right_back_prev + right_big;
    mpeg12->mb_info.recon_right_back_prev = mpeg12->mb_info.recon_right_back;
    if (mpeg12->pic.full_pel_backward_vector)
        mpeg12->mb_info.recon_right_back <<= 1;

    new_vector = mpeg12->mb_info.recon_down_back_prev + down_little;
    if (new_vector <= back_max && new_vector >= back_min)
        mpeg12->mb_info.recon_down_back = mpeg12->mb_info.recon_down_back_prev + down_little;
    else
        mpeg12->mb_info.recon_down_back = mpeg12->mb_info.recon_down_back_prev + down_big;
    mpeg12->mb_info.recon_down_back_prev = mpeg12->mb_info.recon_down_back;
    if (mpeg12->pic.full_pel_backward_vector)
        mpeg12->mb_info.recon_down_back <<= 1;

    mpeg12->mb_info.right_back_y = mpeg12->mb_info.recon_right_back >> 1;
    mpeg12->mb_info.down_back_y = mpeg12->mb_info.recon_down_back >> 1;
    mpeg12->mb_info.right_back_c = (mpeg12->mb_info.recon_right_back / 2) >> 1;
    mpeg12->mb_info.down_back_c = (mpeg12->mb_info.recon_down_back / 2) >> 1;

    mpeg12->mb_info.right_half_back_y = mpeg12->mb_info.recon_right_back & 0x01;
    mpeg12->mb_info.down_half_back_y = mpeg12->mb_info.recon_down_back & 0x01;
    tmp = mpeg12->mb_info.recon_right_back / 2;
    mpeg12->mb_info.right_half_back_c = tmp & 0x01;
    tmp = mpeg12->mb_info.recon_down_back / 2;
    mpeg12->mb_info.down_half_back_c = tmp & 0x01;
}

static void mpeg1_motion_vector_for(mpeg12ctx* mpeg12)
{
    int complement_horizontal_forward_r;
    int complement_vertical_forward_r;
    int right_big, right_little;
    int down_big, down_little;
    int new_vector;
    int tmp, tmp1;

    int forward_f = 1 << (mpeg12->pic.forward_f_code - 1);
    int for_max = (forward_f << 4) - 1;
    int for_min = -(forward_f << 4);

    if (forward_f == 1 || mpeg12->mb_info.motion_horizontal_forward_code == 0)
        complement_horizontal_forward_r = 0;
    else
        complement_horizontal_forward_r = forward_f - 1 - mpeg12->mb_info.motion_horizontal_forward_r;
    if (forward_f == 1 || mpeg12->mb_info.motion_vertical_forward_code == 0)
        complement_vertical_forward_r = 0;
    else
        complement_vertical_forward_r = forward_f - 1 - mpeg12->mb_info.motion_vertical_forward_r;
    right_little = mpeg12->mb_info.motion_horizontal_forward_code << (mpeg12->pic.forward_f_code - 1); //*gPicHeaderInfo.forward_f;
    if (right_little == 0)
        right_big = 0;
    else
    {
        if (right_little > 0)
        {
            right_little = right_little - complement_horizontal_forward_r;
            right_big = right_little - (forward_f << 5);
        }
        else
        {
            right_little = right_little + complement_horizontal_forward_r;
            right_big = right_little + (forward_f << 5);
        }
    }
    down_little = mpeg12->mb_info.motion_vertical_forward_code << (mpeg12->pic.forward_f_code - 1); //*gPicHeaderInfo.forward_f;
    if (down_little == 0)
        down_big = 0;
    else
    {
        if (down_little > 0)
        {
            down_little = down_little - complement_vertical_forward_r;
            down_big = down_little - (forward_f << 5);
        }
        else
        {
            down_little = down_little + complement_vertical_forward_r;
            down_big = down_little + (forward_f << 5);
        }
    }

    new_vector = mpeg12->mb_info.recon_right_for_prev + right_little;
    if (new_vector <= for_max && new_vector >= for_min)
        mpeg12->mb_info.recon_right_for = mpeg12->mb_info.recon_right_for_prev + right_little;
    else
        mpeg12->mb_info.recon_right_for = mpeg12->mb_info.recon_right_for_prev + right_big;
    mpeg12->mb_info.recon_right_for_prev = mpeg12->mb_info.recon_right_for;
    if (mpeg12->pic.full_pel_forward_vector)
        mpeg12->mb_info.recon_right_for <<= 1;

    new_vector = mpeg12->mb_info.recon_down_for_prev + down_little;
    if (new_vector <= for_max && new_vector >= for_min)
        mpeg12->mb_info.recon_down_for = mpeg12->mb_info.recon_down_for_prev + down_little;
    else
        mpeg12->mb_info.recon_down_for = mpeg12->mb_info.recon_down_for_prev + down_big;
    mpeg12->mb_info.recon_down_for_prev = mpeg12->mb_info.recon_down_for;
    if (mpeg12->pic.full_pel_forward_vector)
        mpeg12->mb_info.recon_down_for <<= 1;

    mpeg12->mb_info.right_for_y = mpeg12->mb_info.recon_right_for >> 1;
    mpeg12->mb_info.down_for_y = mpeg12->mb_info.recon_down_for >> 1;
    mpeg12->mb_info.right_for_c = (mpeg12->mb_info.recon_right_for / 2) >> 1;
    mpeg12->mb_info.down_for_c = (mpeg12->mb_info.recon_down_for/2) >> 1;

    mpeg12->mb_info.right_half_for_y = mpeg12->mb_info.recon_right_for & 0x01;
    mpeg12->mb_info.down_half_for_y = mpeg12->mb_info.recon_down_for & 0x01;
    tmp = mpeg12->mb_info.recon_right_for / 2;
    mpeg12->mb_info.right_half_for_c = tmp & 0x01;
    tmp = mpeg12->mb_info.recon_down_for / 2;
    mpeg12->mb_info.down_half_for_c = tmp & 0x01;
}

int mpeg1_calc_mv(mpeg12ctx* mpeg12)
{
    struct vlc_tab1 *tab;

    //* 1. forward mv
    if (mpeg12->mb_info.motion_forward)
    {
        tab = &motion_code_table[show_bits(&mpeg12->gb, 11)];
        if (tab->len)
        {
            skip_bits(&mpeg12->gb, tab->len);
            mpeg12->mb_info.motion_horizontal_forward_code = tab->value;
            mpeg12->mb_info.motion_code[0][0][0] = mpeg12->mb_info.motion_horizontal_forward_code;
        }
        else
        {
            loge("motion_code horizointal error");
            return -1;
        }

        if (mpeg12->pic.forward_f_code != 1 && mpeg12->mb_info.motion_horizontal_forward_code)
        {
            mpeg12->mb_info.motion_horizontal_forward_r = get_bits(&mpeg12->gb, mpeg12->pic.forward_f_code - 1);
            mpeg12->mb_info.motion_residual[0][0][0] = mpeg12->mb_info.motion_horizontal_forward_r;
        }

        tab = &motion_code_table[show_bits(&mpeg12->gb, 11)];
        if (tab->len)
        {
            skip_bits(&mpeg12->gb, tab->len);
            mpeg12->mb_info.motion_vertical_forward_code = tab->value;
            mpeg12->mb_info.motion_code[0][0][1] = mpeg12->mb_info.motion_vertical_forward_code;
        }
        else
        {
            loge("motion_code vertical error");
            return -1;
        }

        if (mpeg12->pic.forward_f_code != 1 && mpeg12->mb_info.motion_vertical_forward_code != 0) {
            mpeg12->mb_info.motion_vertical_forward_r = get_bits(&mpeg12->gb, mpeg12->pic.forward_f_code - 1);
            mpeg12->mb_info.motion_residual[0][0][1] = mpeg12->mb_info.motion_vertical_forward_r;
        }

        // calc forward motion vector 
        mpeg1_motion_vector_for(mpeg12);
    }
    else
    {
        //* 如果当前P宏块没有编码mv，则当前宏块mv信息置0;
        //* 如果是B帧，则使用上一宏块的mv信息
        if (mpeg12->pic.picture_coding_type == MP2_P_PICTURE)
        {
            mpeg12->mb_info.recon_right_for = 0;
            mpeg12->mb_info.recon_down_for = 0;
            mpeg12->mb_info.recon_right_for_prev = 0;
            mpeg12->mb_info.recon_down_for_prev = 0;
            mpeg12->mb_info.right_half_for_y = 0;
            mpeg12->mb_info.down_half_for_y = 0;
            mpeg12->mb_info.right_half_for_c = 0;
            mpeg12->mb_info.down_half_for_c = 0;
        }
        else // B picture
        {
            // 本来应该在这给mv变量赋值，但直接复用上一个mb信息，不需要再做赋值操作
            mpeg12->mb_info.recon_right_for = mpeg12->mb_info.recon_right_for_prev;
            mpeg12->mb_info.recon_down_for = mpeg12->mb_info.recon_down_for_prev;
        }
    
    }

    //* 2. backward mv
    if (mpeg12->mb_info.motion_backward)
    {
        tab = &motion_code_table[show_bits(&mpeg12->gb, 11)];
        if (tab->len)
        {
            skip_bits(&mpeg12->gb, tab->len);
            mpeg12->mb_info.motion_horizontal_backward_code = tab->value;
            mpeg12->mb_info.motion_code[0][1][0] = mpeg12->mb_info.motion_horizontal_backward_code;
        }
        else
        {
            loge("motion_code horizointal error");
            return -1;
        }

        if (mpeg12->pic.backward_f_code != 1 && mpeg12->mb_info.motion_horizontal_backward_code)
        {
            mpeg12->mb_info.motion_horizontal_backward_r = get_bits(&mpeg12->gb, mpeg12->pic.backward_f_code - 1);
            mpeg12->mb_info.motion_residual[0][1][0] = mpeg12->mb_info.motion_horizontal_backward_r;
        }

        tab = &motion_code_table[show_bits(&mpeg12->gb, 11)];
        if (tab->len)
        {
            skip_bits(&mpeg12->gb, tab->len);
            mpeg12->mb_info.motion_vertical_backward_code = tab->value;
            mpeg12->mb_info.motion_code[0][1][1] = mpeg12->mb_info.motion_vertical_backward_code;
        }
        else
        {
            loge("motion_code vertical error");
            return -1;
        }

        if (mpeg12->pic.backward_f_code != 1 && mpeg12->mb_info.motion_vertical_backward_code != 0) {
            mpeg12->mb_info.motion_vertical_backward_r = get_bits(&mpeg12->gb, mpeg12->pic.backward_f_code - 1);
            mpeg12->mb_info.motion_residual[0][1][1] = mpeg12->mb_info.motion_vertical_backward_r;
        }

        // calc backward motion vector 
        mpeg1_motion_vector_back(mpeg12);
    }
    else
    {
        mpeg12->mb_info.recon_right_back = mpeg12->mb_info.recon_right_back_prev;
        mpeg12->mb_info.recon_down_back = mpeg12->mb_info.recon_down_back_prev;
    }

    if (mpeg12->debug)
    {
        fprintf(mpeg12->fp_mv, "forward mv: (%d  %d), backward(%d %d)\n",
            mpeg12->mb_info.recon_right_for, mpeg12->mb_info.recon_down_for,
            mpeg12->mb_info.recon_right_back, mpeg12->mb_info.recon_down_back);
    }
    logd("forward mv: (%d  %d), backward(%d %d)\n",
        mpeg12->mb_info.recon_right_for, mpeg12->mb_info.recon_down_for,
        mpeg12->mb_info.recon_right_back, mpeg12->mb_info.recon_down_back);
}

//* 从source buffer中（x，y）位置取一块 hor_len * ver_len 大小的数据
static void mpeg12_read_ref_picture(uint8 *target,
    int target_pitch,
    uint8 *source,
    int x,
    int y,
    int source_pitch,
    int hor_len,
    int ver_len,
    int picwidth,
    int picheight,
    int fieldflag,
    int ycflag	//y:0,c:1
)
{
    int i, j, m, n, k;
    uint8 *srcptr;
    k = source_pitch / picwidth;

    for (i = 0; i<ver_len; i++)
    {
        m = y + i*k;
        if (m >= picheight)
            m = picheight - 1;
        if (m<0)
            m = 0;
        srcptr = source + m*picwidth;
        for (j = 0; j<hor_len; j++)
        {
            n = x + j;
            if (n >= picwidth)
                n = picwidth - 1;
            if (n<0)
                n = 0;
            target[j] = srcptr[n];
        }
        target += target_pitch;
    }
}

static void save_half_pel(uint8 *target,
    int target_pitch,
    uint8 *source,
    int source_pitch,
    int hor_len,
    int ver_len,
    int hor_half_pel,
    int ver_half_pel,
    int y_c_flag)	//y:1,c:0
{
    int i, j;
    int no_roundingflag;

    no_roundingflag = 0;

    if (hor_half_pel && ver_half_pel)
    {
        for (i = 0; i<ver_len; i++)
        {
            for (j = 0; j<hor_len; j++)
            {
                target[j] = ((uint16_t)source[j] + (uint16_t)source[j + 1] + (uint16_t)source[j + source_pitch] + 
                            (uint16_t)source[j + source_pitch + 1] + 2 - no_roundingflag) >> 2;
            }
            source += source_pitch;
            target += target_pitch;
        }
    }
    else if (hor_half_pel && !ver_half_pel)
    {
        for (i = 0; i<ver_len; i++)
        {
            for (j = 0; j<hor_len; j++)
            {
                target[j] = ((uint16_t)source[j] + (uint16_t)source[j + 1] + 1 - no_roundingflag) >> 1;
            }
            source += source_pitch;
            target += target_pitch;
        }
    }
    else if (!hor_half_pel && ver_half_pel)
    {
        for (i = 0; i<ver_len; i++)
        {
            for (j = 0; j<hor_len; j++)
            {
                target[j] = ((uint16_t)source[j] + (uint16_t)source[j + source_pitch] + 1 - no_roundingflag) >> 1;
            }
            source += source_pitch;
            target += target_pitch;
        }
    }
    else if (!hor_half_pel && !ver_half_pel)
    {
        for (i = 0; i<ver_len; i++)
        {
            for (j = 0; j<hor_len; j++)
            {
                target[j] = source[j];
            }
            source += source_pitch;
            target += target_pitch;
        }
    }
}

#define getdata(data) (((data)<0)?0:((data)>255)?255:(data))
static void add_recon(uint8 *target,
    int target_pitch,
    short *recon,
    int hor_len,
    int ver_len)
{
    int i, j;

    for (i = 0; i<ver_len; i++)
    {
        for (j = 0; j<hor_len; j++)
        {
            target[j] = getdata(target[j] + *recon);
            recon++;
        }
        target += target_pitch;
    }
}

static void save_pixel(uint8 *target,
    int target_pitch,
    uint8 *source,
    int source_pitch,
    int hor_len,
    int ver_len)
{
    int i, j;

    for (i = 0; i<ver_len; i++)
    {
        for (j = 0; j<hor_len; j++)
        {
            target[j] = source[j];//i*target_pitch+
        }
        target += target_pitch;
        source += source_pitch;
    }
}

// (target + source) /2
static void add_and_mean_pixel(uint8 *target,
    int target_pitch,
    uint8 *source,
    int source_pitch,
    int hor_len,
    int ver_len)
{
    int i, j;

    for (i = 0; i<ver_len; i++)
    {
        for (j = 0; j<hor_len; j++)
        {
            target[j] = (target[j]+source[j])/2;//i*target_pitch+
        }
        target += target_pitch;
        source += source_pitch;
    }
}

//* 计算mv为0时重建宏块，包括skipped宏块；
//*  skipped_flag=1为skipped宏块，不需要再加残差
//*  skipped_flag=0时，需要加上残差数据
static void do_mc_with_zero_mv(mpeg12ctx* mpeg12, int mb_x, int mb_y, int skipped_flag)
{
    int start_x = mb_x << 4;
    int start_y = mb_y << 4;
    int pic_width = mpeg12->mb_width << 4;
    int pic_height = mpeg12->mb_height << 4;
    int pic_width_c = pic_width >> 1;
    int pic_height_c = pic_height >> 1;

    uint8* target_y = mpeg12->frame_buf[mpeg12->cur_pic_idx].buffer + (start_y*pic_width + start_x);
    uint8* target_cb = mpeg12->frame_buf[mpeg12->cur_pic_idx].buffer + pic_width*pic_height
        + (start_y / 2 * pic_width_c + start_x / 2);
    uint8* target_cr = mpeg12->frame_buf[mpeg12->cur_pic_idx].buffer + pic_width*pic_height * 5 / 4
        + (start_y / 2 * pic_width_c + start_x / 2);

    uint8* src_y = mpeg12->frame_buf[mpeg12->last_pic_idx].buffer + (start_y*pic_width + start_x);
    uint8* src_cb = mpeg12->frame_buf[mpeg12->last_pic_idx].buffer + pic_width*pic_height
        + (start_y / 2 * pic_width_c + start_x / 2);
    uint8* src_cr = mpeg12->frame_buf[mpeg12->last_pic_idx].buffer + pic_width*pic_height * 5 / 4
        + (start_y / 2 * pic_width_c + start_x / 2);

    if (mpeg12->pic.picture_coding_type == MP2_P_PICTURE)
    {
        save_pixel(target_y, pic_width, src_y, pic_width, 16, 16);
        save_pixel(target_cb, pic_width_c, src_cb, pic_width_c, 8, 8);
        save_pixel(target_cr, pic_width_c, src_cr, pic_width_c, 8, 8);
    }
    else if (mpeg12->pic.picture_coding_type == MP2_B_PICTURE)
    {
        uint8* src_next_y = mpeg12->frame_buf[mpeg12->next_pic_idx].buffer + (start_y*pic_width + start_x);
        uint8* src_next_cb = mpeg12->frame_buf[mpeg12->next_pic_idx].buffer + pic_width*pic_height
            + (start_y / 2 * pic_width_c + start_x / 2);
        uint8* src_next_cr = mpeg12->frame_buf[mpeg12->next_pic_idx].buffer + pic_width*pic_height * 5 / 4
            + (start_y / 2 * pic_width_c + start_x / 2);
        // B帧内的skipped mb的mb_type与上一宏块一样，并且B宏块的前一个宏块不能是I宏块
        // 在这用的是上一宏块的motion_forward值
        if (mpeg12->mb_info.motion_forward && !mpeg12->mb_info.motion_backward)
        { 
            // 前向参考
            save_pixel(target_y, pic_width, src_y, pic_width, 16, 16);
            save_pixel(target_cb, pic_width_c, src_cb, pic_width_c, 8, 8);
            save_pixel(target_cr, pic_width_c, src_cr, pic_width_c, 8, 8);
        }
        else if (!mpeg12->mb_info.motion_forward && mpeg12->mb_info.motion_backward)
        {
            // 后向参考
            save_pixel(target_y, pic_width, src_next_y, pic_width, 16, 16);
            save_pixel(target_cb, pic_width_c, src_next_cb, pic_width_c, 8, 8);
            save_pixel(target_cr, pic_width_c, src_next_cr, pic_width_c, 8, 8);
        }
        else
        {
            // 双向参考
            save_pixel(target_y, pic_width, src_y, pic_width, 16, 16);
            save_pixel(target_cb, pic_width_c, src_cb, pic_width_c, 8, 8);
            save_pixel(target_cr, pic_width_c, src_cr, pic_width_c, 8, 8);

            add_and_mean_pixel(target_y, pic_width, src_next_y, pic_width, 16, 16);
            add_and_mean_pixel(target_cb, pic_width_c, src_next_cb, pic_width_c, 8, 8);
            add_and_mean_pixel(target_cr, pic_width_c, src_next_cr, pic_width_c, 8, 8);
        }
    }

    if (!skipped_flag)
    {
        if (mpeg12->mb_info.cbp & 0x20)
            add_recon(target_y, pic_width, &mpeg12->mb_info.recon[0][0], 8, 8);
        if (mpeg12->mb_info.cbp & 0x10)
            add_recon(target_y + 8, pic_width, &mpeg12->mb_info.recon[1][0], 8, 8);
        if (mpeg12->mb_info.cbp & 0x8)
            add_recon(target_y + 8* pic_width, pic_width, &mpeg12->mb_info.recon[2][0], 8, 8);

        if (mpeg12->mb_info.cbp & 0x4)
            add_recon(target_y + 8 * pic_width + 8, pic_width, &mpeg12->mb_info.recon[3][0], 8, 8);
        if (mpeg12->mb_info.cbp & 0x2)
            add_recon(target_cb , pic_width_c, &mpeg12->mb_info.recon[4][0], 8, 8);
        if (mpeg12->mb_info.cbp & 0x1)
            add_recon(target_cr, pic_width_c, &mpeg12->mb_info.recon[5][0], 8, 8);
    }

    if (mpeg12->debug)
    {
        uint8* y_data = target_y;
        for (int i = 0; i < 16; i++)
        {
            for (int j = 0; j < 16; j++)
            {
                fprintf(mpeg12->fp_pred, "%d ", y_data[j]);
            }
            fprintf(mpeg12->fp_pred, "\n");
            y_data += pic_width;
        }

        uint8* cb_data = target_cb;
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                fprintf(mpeg12->fp_pred, "%d ", cb_data[j]);
            }
            fprintf(mpeg12->fp_pred, "\n");
            cb_data += pic_width_c;
        }
        uint8* cr_data = target_cr;
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                fprintf(mpeg12->fp_pred, "%d ", cr_data[j]);
            }
            fprintf(mpeg12->fp_pred, "\n");
            cr_data += pic_width_c;
        }
    }
}
// 根据预测方式（fram-base/field-base/dual-prime）、预测方向（前向/后向/双向）、帧结构（帧/顶场/底场）选择不同的运动补偿方式
void mb_do_mc(mpeg12ctx* mpeg12, int mb_x, int mb_y)
{
    if (!mpeg12->is_mpeg2)
    {
        mpeg12->pic_code_extension.picture_structure = FRAME;
        mpeg12->mb_info.predict_mode = FRAME_BASE;
        //mpeg12->mb_info.frame_motion_type = 
        // frame-base predict
    }

    int pic_width = mpeg12->mb_width << 4;
    int pic_height = mpeg12->mb_height << 4;
    if (mpeg12->pic_code_extension.picture_structure != 3)
        pic_height <<= 1;

    int pic_width_c = pic_width >> 1;
    int pic_height_c = pic_height >> 1;
    uint8* target_y = (uint8*)(mpeg12->frame_buf[mpeg12->cur_pic_idx].buffer) + (mb_y*16*pic_width) + (mb_x<<4);
    uint8* target_cb = (uint8*)(mpeg12->frame_buf[mpeg12->cur_pic_idx].buffer) + pic_width*pic_height
                        + (mb_y * 8 * pic_width_c) + (mb_x << 3);
    uint8* target_cr = (uint8*)(mpeg12->frame_buf[mpeg12->cur_pic_idx].buffer) + pic_width*pic_height + pic_width*pic_height/4
                        + (mb_y * 8 * pic_width_c) + (mb_x << 3);;
   
    uint8 tmp_mb_buf_for[384]; //64*6
    uint8 tmp_mb_buf_back[384];
    uint8 tmp_mb_ref_buf[400];//18*18];//+2*9*10];
    uint8* tmp_mb_buf_ptr;

    memset(tmp_mb_buf_for, 0, 384);
    memset(tmp_mb_buf_back, 0, 384);
    tmp_mb_buf_ptr = tmp_mb_buf_for;

    uint8* source_cb = mpeg12->frame_buf[mpeg12->last_pic_idx].buffer + pic_width* pic_height;

    // mv is 0
    if (!mpeg12->mb_info.motion_forward && !mpeg12->mb_info.motion_backward)
    {
        do_mc_with_zero_mv(mpeg12, mb_x, mb_y, 0);
        return;
    }

    //logd("=====> last_pic: %d,cur: %d", mpeg12->last_pic_idx, mpeg12->cur_pic_idx);
    //* frame based
    if (mpeg12->pic_code_extension.picture_structure == FRAME && mpeg12->mb_info.predict_mode == FRAME_BASE)
    {
        if (mpeg12->mb_info.motion_forward)
        {
            // Y component
            uint8* source_y = mpeg12->frame_buf[mpeg12->last_pic_idx].buffer;
            int source_y_x = (mb_x << 4) + mpeg12->mb_info.right_for_y;
            int source_y_y = (mb_y << 4) + mpeg12->mb_info.down_for_y;
            int source_y_pitch = pic_width;

            mpeg12_read_ref_picture(tmp_mb_ref_buf, 17, source_y, 
                            source_y_x, source_y_y, source_y_pitch, 
                            17, 17, pic_width, pic_height, 0, 0);
            
            save_half_pel(tmp_mb_buf_for, 16,
                tmp_mb_ref_buf, 17, 16, 16,
                mpeg12->mb_info.right_half_for_y, mpeg12->mb_info.down_half_for_y, 1);

            // Cb
            uint8* source_cb = mpeg12->frame_buf[mpeg12->last_pic_idx].buffer + pic_width* pic_height;
            int source_c_x = (mb_x << 3) + mpeg12->mb_info.right_for_c;
            int source_c_y = (mb_y << 3) + mpeg12->mb_info.down_for_c;
            int source_c_pitch = pic_width >> 1;

            mpeg12_read_ref_picture(tmp_mb_ref_buf, 9, source_cb,
                source_c_x, source_c_y, source_c_pitch, 9, 9,
                pic_width_c, pic_height_c, 0, 1);

            save_half_pel(tmp_mb_buf_for + 256, 8,
                tmp_mb_ref_buf, 9, 8, 8,
                mpeg12->mb_info.right_half_for_c, mpeg12->mb_info.down_half_for_c, 0);
            
            // Cr
            uint8* source_cr = mpeg12->frame_buf[mpeg12->last_pic_idx].buffer + pic_width* pic_height + pic_width* pic_height / 4;
            mpeg12_read_ref_picture(tmp_mb_ref_buf, 9, source_cr,
                source_c_x, source_c_y, source_c_pitch, 9, 9,
                pic_width_c, pic_height_c, 0, 1);

            save_half_pel(tmp_mb_buf_for + 256 + 64, 8,
                tmp_mb_ref_buf, 9, 8, 8,
                mpeg12->mb_info.right_half_for_c, mpeg12->mb_info.down_half_for_c, 0);

            tmp_mb_buf_ptr = tmp_mb_buf_for;
        }
        

        if (mpeg12->mb_info.motion_backward)
        {
            //logd("==== backward next_pic_idx: %d  decode_num: %d, display_num: %d", 
            //    mpeg12->next_pic_idx, mpeg12->frame_buf[mpeg12->next_pic_idx].decoded_count,
            //    mpeg12->frame_buf[mpeg12->next_pic_idx].display_count);

            // Y component
            uint8* source_y = mpeg12->frame_buf[mpeg12->next_pic_idx].buffer;
            int source_y_x = (mb_x << 4) + mpeg12->mb_info.right_back_y;
            int source_y_y = (mb_y << 4) + mpeg12->mb_info.down_back_y;
            int source_y_pitch = pic_width;

            mpeg12_read_ref_picture(tmp_mb_ref_buf, 17, source_y,
                source_y_x, source_y_y, source_y_pitch,
                17, 17, pic_width, pic_height, 0, 0);


            save_half_pel(tmp_mb_buf_back, 16,
                tmp_mb_ref_buf, 17, 16, 16,
                mpeg12->mb_info.right_half_back_y, mpeg12->mb_info.down_half_back_y, 1);

            // Cb
            uint8* source_cb = mpeg12->frame_buf[mpeg12->next_pic_idx].buffer + pic_width* pic_height;
            int source_c_x = (mb_x << 3) + mpeg12->mb_info.right_back_c;
            int source_c_y = (mb_y << 3) + mpeg12->mb_info.down_back_c;
            int source_c_pitch = pic_width >> 1;

            mpeg12_read_ref_picture(tmp_mb_ref_buf, 9, source_cb,
                source_c_x, source_c_y, source_c_pitch, 9, 9,
                pic_width_c, pic_height_c, 0, 1);


            save_half_pel(tmp_mb_buf_back + 256, 8,
                tmp_mb_ref_buf, 9, 8, 8,
                mpeg12->mb_info.right_half_back_c, mpeg12->mb_info.down_half_back_c, 0);

            // Cr
            uint8* source_cr = mpeg12->frame_buf[mpeg12->next_pic_idx].buffer + pic_width* pic_height + pic_width* pic_height / 4;
            mpeg12_read_ref_picture(tmp_mb_ref_buf, 9, source_cr,
                source_c_x, source_c_y, source_c_pitch, 9, 9,
                pic_width_c, pic_height_c, 0, 1);
            {
            //    for (int i = 0; i<64; i++)
            //        printf("%x ", tmp_mb_ref_buf[i]);
            //    printf("\n");
            }
            save_half_pel(tmp_mb_buf_back + 256 + 64, 8,
                tmp_mb_ref_buf, 9, 8, 8,
                mpeg12->mb_info.right_half_back_c, mpeg12->mb_info.down_half_back_c, 0);
            {
            //    for (int i = 0; i<8; i++)
            //        printf("%x ", tmp_mb_buf_back[i]);
            //    printf("\n");
            }

            tmp_mb_buf_ptr = tmp_mb_buf_back;
        }

        if (mpeg12->mb_info.motion_forward && mpeg12->mb_info.motion_backward)
        {
            int i;
            for (i = 0; i < 384; i++)
                tmp_mb_buf_for[i] = ((uint16_t)tmp_mb_buf_for[i] + (uint16_t)tmp_mb_buf_back[i] + 1) >> 1;
            tmp_mb_buf_ptr = tmp_mb_buf_for;
        }

        if (mpeg12->fp_pred)
        {
            int i, j;
            for (i = 0; i < 16; i++)
            {
                for (j = 0; j < 16; j++)
                {
                    fprintf(mpeg12->fp_pred, "%d ", tmp_mb_buf_ptr[i * 16 + j]);
                }
                fprintf(mpeg12->fp_pred, "\n");
            }

            for (i = 0; i < 8; i++)
            {
                for (j = 0; j < 8; j++)
                {
                    fprintf(mpeg12->fp_pred, "%d ", tmp_mb_buf_ptr[256+ i * 8 + j]);
                }
                fprintf(mpeg12->fp_pred, "\n");
            }

            for (i = 0; i < 8; i++)
            {
                for (j = 0; j < 8; j++)
                {
                    fprintf(mpeg12->fp_pred, "%d ", tmp_mb_buf_ptr[256+64+i * 8 + j]);
                }
                fprintf(mpeg12->fp_pred, "\n");
            }
        }

        if (mpeg12->mb_info.cbp & 0x20)
            add_recon(tmp_mb_buf_ptr, 16, &mpeg12->mb_info.recon[0][0], 8, 8);
        if(mpeg12->mb_info.cbp & 0x10)
            add_recon(tmp_mb_buf_ptr+8, 16, &mpeg12->mb_info.recon[1][0], 8, 8);
        if (mpeg12->mb_info.cbp & 0x8)
            add_recon(tmp_mb_buf_ptr + 128, 16, &mpeg12->mb_info.recon[2][0], 8, 8);
                 
        if (mpeg12->mb_info.cbp & 0x4)
            add_recon(tmp_mb_buf_ptr+128+8, 16, &mpeg12->mb_info.recon[3][0], 8, 8);
        if (mpeg12->mb_info.cbp & 0x2)
            add_recon(tmp_mb_buf_ptr+256, 8, &mpeg12->mb_info.recon[4][0], 8, 8);
        if (mpeg12->mb_info.cbp & 0x1)
            add_recon(tmp_mb_buf_ptr+256+64, 8, &mpeg12->mb_info.recon[5][0], 8, 8);
    }

    save_pixel(target_y, pic_width, tmp_mb_buf_ptr, 16, 16, 16);
    save_pixel(target_cb, pic_width_c, tmp_mb_buf_ptr + 256, 8, 8, 8);
    save_pixel(target_cr, pic_width_c, tmp_mb_buf_ptr + 256 + 64, 8, 8, 8);

    {
    //    for (int i = 0; i<6; i++)
     //       printf("%d ", mpeg12->frame_buf[mpeg12->cur_pic_idx].buffer[i+2560]);
    //    printf("\n");
    }

    //if (mpeg12->pic.picture_coding_type == MP2_B_PICTURE)
    //    abort();
}


void mpeg1_decode_skipped_mb(mpeg12ctx* mpeg12, int mb_idx)
{
    int mb_x = mb_idx % mpeg12->mb_width;
    int mb_y = mb_idx / mpeg12->mb_width;

    if (mpeg12->fp_mv)
    {
        fprintf(mpeg12->fp_mv, "mb (%d %d)\n", mb_x, mb_y);
        fprintf(mpeg12->fp_mv, "skipped mb\n");
    }

    //* P帧内skipped宏块mv为0，重建宏块直接从参考帧获取数据
    //* B帧内skipped宏块mb_type和mv继承于上一个宏块，需要做运动补偿
    if(mpeg12->pic.picture_coding_type == MP2_P_PICTURE)
        do_mc_with_zero_mv(mpeg12, mb_x, mb_y, 1);
    else
    {
        memset(mpeg12->mb_info.recon[0], 0, sizeof(short) * 384);
        mb_do_mc(mpeg12, mb_x, mb_y);
    }
}

