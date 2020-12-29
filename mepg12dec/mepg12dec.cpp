// mepg12dec.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include<stdlib.h>
#include "log.h"
#include "mpeg12.h"
#include "vlc.h"
#include "idct.h"

#define EXTENSION_START_CODE	0x000001B5
#define SEQUENCE_START_CODE		0x000001B3
#define GROUP_START_CODE		0x000001B8
#define PICTURE_START_CODE		0x00000100
#define SEQUENCE_END_CODE		0x000001B7
#define SEQUENCE_ERROR_CODE		0x000001B4
#define USER_DATA_START_CODE	0x000001B2

#define SEQUENCE_EXTENSION_ID					1
#define SEQUENCE_DISPLAY_EXTENSION_ID			2
#define QUANT_MATRIX_EXTENSION_ID				3
#define COPYRIGHT_EXTENSION_ID					4
#define SEQUENCE_SCALABLE_EXTENSION_ID			5
#define PICTURE_DISPLAY_EXTENSION_ID			7
#define PICTURE_CODING_EXTENSION_ID				8
#define PICTURE_SPATIAL_SCALABLE_EXTENSION_ID	9
#define PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID	10

// return: start code offset
int search_start_code(GetBitContext* gb)
{
    int i = 0;
    while (1)
    {
        if (show_bits(gb, 24) == 0x000001)
            return 0;
        skip_bits(gb, 1);
    }

    return -1;
}

static int update_frame_buffer_idx(mpeg12ctx* mpeg12)
{
    if (mpeg12->pic.picture_coding_type != MP2_B_PICTURE)
    {
        mpeg12->frame_buf[mpeg12->last_pic_idx].is_ref = 0;
        mpeg12->last_pic_idx = mpeg12->next_pic_idx;
        mpeg12->frame_buf[mpeg12->last_pic_idx].is_ref = 1;
        mpeg12->next_pic_idx = mpeg12->cur_pic_idx;
        mpeg12->frame_buf[mpeg12->next_pic_idx].is_ref = 1;
    }
    return 0;
}

static int sequence_extension(mpeg12ctx* mpeg12)
{
    logd("=======> mpeg2");
    mpeg12->is_mpeg2 = 1;
    mpeg12->seq_extension.extension_start_code_identifier = get_bits(&mpeg12->gb, 4);
    mpeg12->seq_extension.profile_and_level_indication = get_bits(&mpeg12->gb, 8);
    mpeg12->seq_extension.progressive_sequence = get_bits(&mpeg12->gb, 1);
    mpeg12->seq_extension.chroma_format = get_bits(&mpeg12->gb, 2);
    mpeg12->seq_extension.horizontal_size_extension = get_bits(&mpeg12->gb, 2);
    mpeg12->seq_extension.vertical_size_extension = get_bits(&mpeg12->gb, 2);
    mpeg12->seq_extension.bit_rate_extension  = get_bits(&mpeg12->gb, 2);
    skip_bits(&mpeg12->gb, 1);  // skip_bits(1) marker_bit
    mpeg12->seq_extension.vbv_buffer_size_extension = get_bits(&mpeg12->gb, 8);
    mpeg12->seq_extension.low_delay = get_bits(&mpeg12->gb, 1);
    mpeg12->seq_extension.frame_rate_extension_n = get_bits(&mpeg12->gb, 2);
    mpeg12->seq_extension.frame_rate_extension_d = get_bits(&mpeg12->gb, 5);

    if (mpeg12->seq_extension.chroma_format == 2 || mpeg12->seq_extension.chroma_format == 3)
    {
        logw("donot support this format(%d)", mpeg12->seq_extension.chroma_format);
    }

    return 0;
}

static int picture_coding_extension(mpeg12ctx* mpeg12)
{
    pic_coding_extension_info* ext = &mpeg12->pic_code_extension;
    ext->extension_start_code_identifier = get_bits(&mpeg12->gb, 4);
    ext->f_code[0][0] = get_bits(&mpeg12->gb, 4);
    ext->f_code[0][1] = get_bits(&mpeg12->gb, 4);
    ext->f_code[1][0] = get_bits(&mpeg12->gb, 4);
    ext->f_code[1][1] = get_bits(&mpeg12->gb, 4);
    ext->intra_dc_precision = get_bits(&mpeg12->gb, 2);
    ext->picture_structure = get_bits(&mpeg12->gb, 2);
    ext->top_field_first = get_bits(&mpeg12->gb, 1);
    ext->frame_pred_frame_dct = get_bits(&mpeg12->gb, 1);
    ext->concealment_motion_vectors = get_bits(&mpeg12->gb, 1);
    ext->q_scale_type = get_bits(&mpeg12->gb, 1);
    ext->intra_vlc_format = get_bits(&mpeg12->gb, 1);

    ext->alternate_scan = get_bits(&mpeg12->gb, 1);
    ext->repeat_first_field = get_bits(&mpeg12->gb, 1);
    ext->chroma_420_type = get_bits(&mpeg12->gb, 1);;
    ext->progressive_frame = get_bits(&mpeg12->gb, 1);;
    ext->composite_display_flag = get_bits(&mpeg12->gb, 1);
    if (ext->composite_display_flag)
    {
        ext->v_axis = get_bits(&mpeg12->gb, 1);
        ext->field_sequence = get_bits(&mpeg12->gb, 3);
        ext->sub_carrier = get_bits(&mpeg12->gb, 1);
        ext->burst_amplitude = get_bits(&mpeg12->gb, 7);
        ext->sub_carrier_phase = get_bits(&mpeg12->gb, 8);
    }

    if (ext->alternate_scan)
        mpeg12->scan_table = ff_alternate_vertical_scan;
    else
        mpeg12->scan_table = ff_zigzag_direct;

    logd("================= picture coding extension =======================");
    logd("f_code: %d %d %d %d", ext->f_code[0][0], ext->f_code[0][1], ext->f_code[1][0], ext->f_code[1][1]);
    logd("intra_dc_precision: %d", ext->intra_dc_precision);
    logd("picture_structure: %d", ext->picture_structure);
    logd("top_field_first: %d", ext->top_field_first);
    logd("frame_pred_frame_dct: %d", ext->frame_pred_frame_dct);
    logd("q_scale_type: %d", ext->q_scale_type);
    logd("intra_vlc_format: %d", ext->intra_vlc_format);
    logd("progressive_frame: %d", ext->progressive_frame);

    return 0;
}

static int extension_and_user_data(mpeg12ctx* mpeg12)
{
    int offset = 0;
    uint32 ext_id;
    uint32 nextbit32;

    nextbit32 = show_bits(&mpeg12->gb, 32);
    logd("====> bits: %08x", nextbit32);
    // extension start code or user data start code
    while ((nextbit32= show_bits(&mpeg12->gb, 32)) == EXTENSION_START_CODE || nextbit32 == USER_DATA_START_CODE)
    {
        if (nextbit32 == EXTENSION_START_CODE)
        {
            skip_bits(&mpeg12->gb, 32);
            ext_id = show_bits(&mpeg12->gb, 4); // get_bits(4)
            switch (ext_id)
            {
            case SEQUENCE_EXTENSION_ID:
                sequence_extension(mpeg12);
                break;
            case SEQUENCE_DISPLAY_EXTENSION_ID:
                break;
            case QUANT_MATRIX_EXTENSION_ID:
                
                break;
            case SEQUENCE_SCALABLE_EXTENSION_ID:
                
                break;
            case PICTURE_DISPLAY_EXTENSION_ID:
                
                break;
            case PICTURE_CODING_EXTENSION_ID:
                if (mpeg12->is_mpeg2)
                {
                    picture_coding_extension(mpeg12);
                }
                break;
            case PICTURE_SPATIAL_SCALABLE_EXTENSION_ID:
                
                break;
            case PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID:
                
                break;
            case COPYRIGHT_EXTENSION_ID:
                
                break;
            default:
                logd("reserved extension start code ID %d\n", ext_id);
                break;
            }
            search_start_code(&mpeg12->gb);
        }
        else
        {
            // skip user data
            skip_bits(&mpeg12->gb, 32);
            search_start_code(&mpeg12->gb);
        }
    }

    return 0;
}

static int decode_picture_header(mpeg12ctx* mpeg12, picture* pic, unsigned char* buf, int len)
{
    pic->temporal_reference = get_bits(&mpeg12->gb, 10); // 10bit
    pic->picture_coding_type = get_bits(&mpeg12->gb, 3); //3bit
    pic->vbv_delay = get_bits(&mpeg12->gb, 16); // 10bit

    update_frame_buffer_idx(mpeg12);

    mpeg12->frame_buf[mpeg12->cur_pic_idx].display_count = mpeg12->display_num_base + pic->temporal_reference;

    if (pic->picture_coding_type == 2 || pic->picture_coding_type == 3)
    {
        //abort();
        pic->full_pel_forward_vector = get_bits(&mpeg12->gb, 1);
        pic->forward_f_code = get_bits(&mpeg12->gb, 3);
    }
    if (pic->picture_coding_type == 3)
    {
        pic->full_pel_backward_vector = get_bits(&mpeg12->gb, 1);//1bit
        pic->backward_f_code = get_bits(&mpeg12->gb, 3);      // 3bit
    }

    logd("=========== picture header ================= \n");
    logd("temporal_reference: %d \n", pic->temporal_reference);
    logd("picture_coding_type: %d \n", pic->picture_coding_type);
    logd("============================ \n");

    search_start_code(&mpeg12->gb);
    // picture coding extension
    extension_and_user_data(mpeg12);
    return 0;
}

static int mpeg1_iq(mpeg12ctx* mpeg12, short* block, uint8* quant_matrix)
{
    int i, level = 0;
    if (mpeg12->mb_info.mb_intra)
    {
        block[0] = block[0] << 3;
        if (block[0]>2047)
            block[0] = 2047;
        
        for (i = 1; i < 64; i++)
        {
            level = block[i];
            if (level)
            {
                if (level < 0)
                {
                    level = -level;
                    level = (int)(level * mpeg12->mb_info.quantiser_scale * quant_matrix[i]) >> 3;
                    if (!(level & 0x1))
                        level -= 1;
                    level = -level;
                }
                else
                {
                    level = (int)(level * mpeg12->mb_info.quantiser_scale * quant_matrix[i]) >> 3;
                    if (!(level & 0x1))
                        level -= 1;
                }
                //saturation
                if (level>2047)
                    level = 2047;
                else if (level<-2048)
                    level = -2048;
                block[i] = level;
                logv("block: i: %d, level: %d, qmatrix[i]: %d, qscale: %d", i, level, quant_matrix[i], mpeg12->mb_info.quantiser_scale);
            }
        }
    }
    else
    {
        for (i = 0; i < 64; i++)
        {
            level = block[i];
            if (level)
            {
                if (level < 0)
                {
                    level = -level;
                    level = (((level << 1) + 1) * mpeg12->mb_info.quantiser_scale * mpeg12->seq_header.non_intra_quantiser_matrix[i]) >> 4;
                    if (!(level & 0x1))
                        level -= 1;
                    level = -level;
                }
                else
                {
                    level = (((level << 1) + 1) * mpeg12->mb_info.quantiser_scale * mpeg12->seq_header.non_intra_quantiser_matrix[i]) >> 4;
                    if (!(level & 0x1))
                        level -= 1;
                }
                //saturation
                if (level>2047)
                    level = 2047;
                else if (level<-2048)
                    level = -2048;
                block[i] = level;
            }
        }
    }
    return 0;
}

static int decode_dc(mpeg12ctx* mpeg12, int block_num, short* dc)
{
    int run, level, sign_level;
    struct vlc_tab1 *dds_ptr;
    int size, dct_dc_differential = 0;

    int component = (block_num <4 ) ? 0 : block_num-4+1;

    // Y componengt DC
    if (component == 0)
    {
        // show 7 bits
        dds_ptr = &dct_dc_size_luminance_table[show_bits(&mpeg12->gb, 7) << 2];
        size = dds_ptr->value;
        // skip the real bit len of dct_dc_size_luma
        skip_bits(&mpeg12->gb, dds_ptr->len);      
    }
    else // i>4
    {
        // show 8 bits
        dds_ptr = &dct_dc_size_chrominance_table[show_bits(&mpeg12->gb, 8) << 2];
        size = dds_ptr->value;
        skip_bits(&mpeg12->gb, dds_ptr->len);
    }

    if (size)
    {
        dct_dc_differential = get_bits(&mpeg12->gb, size);
        if (dct_dc_differential&(1 << (size - 1)))
            mpeg12->last_dc[component] += dct_dc_differential;
        else
            mpeg12->last_dc[component] += (-1 << size) | (dct_dc_differential + 1);
    }

    level = mpeg12->last_dc[component];
    if (level < 0)
        level = 0;
    else if (level > 255)
        level = 255;

    mpeg12->last_dc[component] = level;

    *dc = level;
    return 0;
}

static int decode_ac(mpeg12ctx* mpeg12, int block_num)
{
    int n, run, level, sign_level;
    unsigned long code;
    unsigned long sign;
    unsigned long tab;
    
    int pos = (mpeg12->mb_info.mb_intra) ? 0 : -1;
    short* reconp = mpeg12->mb_info.recon[block_num];
    for (; ;)
    {
        //logd(" nextbit 32: %x", show_bits(&mpeg12->gb, 32));
        code = show_bits(&mpeg12->gb, 17);
        if (code >= 32768)
        {
            //* intra mb需要单独解析DC，non-intra mb通过 DCTtabfirst解析
            if(pos == -1)
                tab = DCTtabfirst[code >> 12];
            else
                tab = DCTtabnext[code >> 12];
        }
        else if (code >= 2048)
            tab = DCTtab0[code >> 8];
        else
        {
            if (code >= 1024)
                tab = DCTtab1[(code >> 6)];
            else if (code >= 512)
                tab = DCTtab2[(code >> 4)];
            else if (code >= 256)
                tab = DCTtab3[(code >> 3)];
            else if (code >= 128)
                tab = DCTtab4[(code >> 2)];
            else if (code >= 64)
                tab = DCTtab5[(code >> 1)];
            else if (code >= 32)
                tab = DCTtab6[code];
            else
            {
                loge("intra_block ac coefficient decoding error code(%d).\n", code);
                return -1;
            }
        }

        skip_bits(&mpeg12->gb, tab & 0xff);
        run = tab >> 24;
        if (run < 64) // nonescape //
        {
            level = (tab >> 16) & 0xff;
            sign = tab & 0xff00;
        }
        else if (run == 64) //EOB//
        {
            break;
        }
        else //if(run == 65) //ESCAPE//
        {
            //logd("==== escape");
            run = get_bits(&mpeg12->gb, 6);//GetBits(6);
            level = get_bits(&mpeg12->gb, 8);//GetBits(8);
            if (level & 0x7f)
            {
                if (sign = (level>128))
                    level = 256 - level;
            }
            else
            {

                level = (level << 8) | get_bits(&mpeg12->gb, 8);;//GetBits(8);
                if (sign = (level>32768))
                    level = (32768 + 256) - level;
            }
        }

        sign_level = sign ? -level : level;
        pos += run + 1;
        if (pos > 63) break;

        reconp[mpeg12->scan_table[pos]] = sign_level;

        logv("pos: %d, scan_pos: %d, run: %d, level: %d, get_index: %d",
            pos, mpeg12->scan_table[pos], run, sign_level, get_bits_index(&mpeg12->gb));
    }
    return 0;
}

int mpeg1_intra_6block(mpeg12ctx* mpeg12)
{
    int size;
    int n, run, level, i, sign_level;
    unsigned long code;
    short *reconp;
    
    unsigned long sign;
    unsigned long tab;

    mpeg12->mb_info.mb_intra = 1;
    unsigned int tmp;

    for (i = 0; i < 6; i++)
    {   
        //logd("+++++++ get_index: %d, showbits: %x", get_bits_index(&mpeg12->gb), show_bits(&mpeg12->gb, 32));
   
        reconp = mpeg12->mb_info.recon[i];
        memset(reconp, 0, 64*sizeof(short));
        
        // 1. parse DC coeff
        decode_dc(mpeg12, i, &reconp[0]);

        logv("DC coeff: %d, i: %d, mb_intra: %d, showbits: %x", reconp[0], i, mpeg12->mb_info.mb_intra, show_bits(&mpeg12->gb, 32));

        //* 2. AC coeff
        if ((int)show_bits(&mpeg12->gb, 32) <= (int)0xBFFFFFFF)
        {
            skip_bits(&mpeg12->gb, 2);
        }
        else if (mpeg12->pic.picture_coding_type != MP2_D_PICTURE)
        {
            decode_ac(mpeg12, i);
        } // if (mpeg12->pic.picture_coding_type != MP2_D_PICTURE) 

        //* 3. inverse quantiser
        mpeg1_iq(mpeg12, reconp, mpeg12->seq_header.intra_quantiser_matrix);
        if (mpeg12->debug)
        {
            fprintf(mpeg12->fp_dct_coeff, "block num(%d)\n", i);
            int ii = 0, jj = 0;
            for (ii = 0; ii < 8; ii++)
            {
                for (jj = 0; jj < 8; jj++)
                {
                    fprintf(mpeg12->fp_dct_coeff, "%d ", reconp[8 * ii + jj]);
                }
                fprintf(mpeg12->fp_dct_coeff, "\n");
            }
        }

        //* 4. IDCT
        mpeg_idct(reconp);  

        if (mpeg12->debug)
        {
            fprintf(mpeg12->fp_idct_out, "block num(%d)\n", i);
            int ii = 0, jj = 0;
            for (ii = 0; ii < 8; ii++)
            {
                for (jj = 0; jj < 8; jj++)
                {
                    fprintf(mpeg12->fp_idct_out, "%d ", reconp[8 * ii + jj]);
                }
                fprintf(mpeg12->fp_idct_out, "\n");
            }
        }
    }

    //if (mpeg12->mb_x == 0 && mpeg12->mb_y == 5) abort();

    return 0;
}

static int mpeg1_inter_6block(mpeg12ctx* mpeg12)
{
    int i;
    short *reconp;
    for (i = 0; i < 6; i++)
    {
        reconp = mpeg12->mb_info.recon[i];
        memset(reconp, 0, 64 * sizeof(short));
        if (mpeg12->mb_info.cbp & (0x20 >> i))
        {
            reconp = mpeg12->mb_info.recon[i];
            //* 5.1. parse non-intra block DCT coeff
            // mpeg1_nonotra_block
            decode_ac(mpeg12, i);

            //* 5.2. inverse quantiser
            mpeg1_iq(mpeg12, reconp, mpeg12->seq_header.non_intra_quantiser_matrix);

            if (mpeg12->debug)
            {
                fprintf(mpeg12->fp_dct_coeff, "block num(%d)\n", i);
                int ii = 0, jj = 0;
                for (ii = 0; ii < 8; ii++)
                {
                    for (jj = 0; jj < 8; jj++)
                    {
                        fprintf(mpeg12->fp_dct_coeff, "%d ", reconp[8 * ii + jj]);
                    }
                    fprintf(mpeg12->fp_dct_coeff, "\n");
                }
            }

            //* 5.3  IDCT
            mpeg_idct(reconp);

            if (mpeg12->debug)
            {
                fprintf(mpeg12->fp_idct_out, "block num(%d)\n", i);
                int ii = 0, jj = 0;
                for (ii = 0; ii < 8; ii++)
                {
                    for (jj = 0; jj < 8; jj++)
                    {
                        fprintf(mpeg12->fp_idct_out, "%d ", reconp[8 * ii + jj]);
                    }
                    fprintf(mpeg12->fp_idct_out, "\n");
                }
            }
        }
    }
    return 0;
}

#define getdata(data) (((data)<0)?0:((data)>255)?255:(data))
void clip_blocks(uint8 *target,
    short *source)
{
    int i, j;
    //for (i = 0; i<6; i++)
    {
        for (j = 0; j<64; j++)
        {
            *target = getdata(*source);
            target++;
            source++;
        }
    }
}

// mpeg12->recon[6][64] ----> mpeg12->frame_buffer
static int copydata_to_frame(mpeg12ctx* mpeg12)
{
    int i, j;
    int y_stride = mpeg12->mb_width * 16;

    uint8 tmp_mb_ref_buf[400];

    // Y data
    uint8* dst;
    for (i = 0; i < 4; i++)
    {
        int start_x = mpeg12->mb_x * 16 + (i & 1) * 8;
        int start_y = mpeg12->mb_y * 16 + (i / 2) * 8;
        
        for (j = 0; j < 8; j++)
        {
            dst = mpeg12->frame_buf[mpeg12->cur_pic_idx].buffer + (j + start_y)*y_stride + start_x;
            for (int k = 0; k < 8; k++)
            {
                dst[k] = getdata(mpeg12->mb_info.recon[i][8 * j + k]);
            }
        }
    }

    //logd("===== %d %d %d %d", dst[0], dst[1], dst[2], dst[3]); //abort();

    // UV data
    for (i = 4; i < 6; i++)
    {
        int start_x = mpeg12->mb_x * 8;
        int start_y = mpeg12->mb_y * 8;

        for (j = 0; j < 8; j++)
        {
            uint8* dst = mpeg12->frame_buf[mpeg12->cur_pic_idx].buffer + (j + start_y)*y_stride / 2 + start_x +
                (mpeg12->mb_width * 16)*(mpeg12->mb_height * 16) + (mpeg12->mb_width * 16)*(mpeg12->mb_height * 16)/4*(i-4);

            for (int k = 0; k < 8; k++)
            {
                dst[k] = mpeg12->mb_info.recon[i][8 * j + k];
            }
        }
    }
    return 0;
}

static int mpeg1_macroblock(mpeg12ctx* mpeg12)
{
    static int num = 0;
    //if (num > 2310) abort();
    num++;

    int n = 0;
    int i = 0;

    int mb_addr_inc;
    uint32 nextbit11;
    while ((nextbit11 = show_bits(&mpeg12->gb, 11)) == 0x0f)//macroblock_stuffing
        skip_bits(&mpeg12->gb, 11);
    while ((nextbit11 = show_bits(&mpeg12->gb, 11)) == 0x08)////macroblock_escape
    {
        skip_bits(&mpeg12->gb, 11);
        n += 33;
    }

    struct vlc_tab1 *tab;
    
    tab = &macroblock_address_increment_table[show_bits(&mpeg12->gb, 11)];
    if (tab->len)
    {
        skip_bits(&mpeg12->gb, tab->len);
        mb_addr_inc = tab->value;
    }
    else
    {
        loge("macroblock_address_increment error!");
        
        fclose(mpeg12->fp_dct_coeff);
        abort();
        return -1;
    }
    logv("inc len: %d %d", tab->len, mb_addr_inc);
    logv("======> index: %d", get_bits_index(&mpeg12->gb));

    n += mb_addr_inc;
    mpeg12->cur_mblk_addr = mpeg12->prev_mblk_addr+n;
    mpeg12->mb_info.macroblock_address_increment = n;
    mpeg12->mb_x = mpeg12->cur_mblk_addr % mpeg12->mb_width;
    mpeg12->mb_y = mpeg12->cur_mblk_addr / mpeg12->mb_width;

    if (mpeg12->cur_mblk_addr > mpeg12->mb_height*mpeg12->mb_width)
    {
        loge("cur_mb_addr(%d) > total mb num", mpeg12->cur_mblk_addr);
        return -1;
    }

    //* 1. decode skipped macroblock in P picture
    if (n > 1)
    {
        for (i = mpeg12->prev_mblk_addr + 1; i < mpeg12->cur_mblk_addr; i++)
        {
            mpeg1_decode_skipped_mb(mpeg12, i);
        }
        mpeg12->last_dc[0] = mpeg12->last_dc[1] = mpeg12->last_dc[2] = 128;
    }

    logv("mb (%d %d) \n", mpeg12->mb_x, mpeg12->mb_y);

    if (mpeg12->debug)
    {
        fprintf(mpeg12->fp_dct_coeff, "mb (%d %d) \n", mpeg12->mb_x, mpeg12->mb_y);
        fprintf(mpeg12->fp_idct_out, "mb (%d %d) \n", mpeg12->mb_x, mpeg12->mb_y);
        fprintf(mpeg12->fp_mv, "mb (%d %d) pic_num(%d) pic_type(%s)\n", 
            mpeg12->mb_x, mpeg12->mb_y, mpeg12->decoded_pic_num,
            mpeg12->pic.picture_coding_type == MP2_I_PICTURE? "I" : (mpeg12->pic.picture_coding_type == MP2_P_PICTURE? "P": "B"));
        fprintf(mpeg12->fp_pred, "mb (%d %d) \n", mpeg12->mb_x, mpeg12->mb_y);
    }

    if (mpeg12->pic.picture_coding_type == MP2_I_PICTURE 
        /*|| mpeg12->pic.picture_coding_type == MP2_D_PICTURE*/)
    {
        // 如果mb_type为1，macroblock_quant为0；
        // 否则，mb_type占2bit
        if (get_bits(&mpeg12->gb, 1))
        {
            mpeg12->mb_info.mb_quant = 0;
        }
        else
        {
            // mb_type vlc code is 01
            skip_bits(&mpeg12->gb, 1);
            mpeg12->mb_info.mb_quant = 1;
            mpeg12->mb_info.quantiser_scale = get_bits(&mpeg12->gb, 5);
        }

        // cbp is 0x3f for intra macroblock
        mpeg12->mb_info.cbp = 0x3f;

        mpeg1_intra_6block(mpeg12);
        copydata_to_frame(mpeg12);
    }
    else if (mpeg12->pic.picture_coding_type == MP2_P_PICTURE 
        || mpeg12->pic.picture_coding_type == MP2_B_PICTURE)
    {
        logd("mb (%d %d) \n", mpeg12->mb_x, mpeg12->mb_y);
        //* 1. parse macroblock type
        if (mpeg12->pic.picture_coding_type == MP2_P_PICTURE)
            tab = &macroblock_type_p_table[show_bits(&mpeg12->gb, 6)];
        else
            tab = &macroblock_type_b_table[show_bits(&mpeg12->gb, 6)];
        if (tab->len)
        {
            skip_bits(&mpeg12->gb, tab->len);
            mpeg12->mb_info.mb_type = tab->value;
        }
        else
        {
            loge("macroblock type error");
            return -1;
        }

        /*  
          mb_type = mb_intra | mb_pattern | motion_backward | motion_forward | mb_quant
        */
        logv("mpeg12->mb_info.mb_type: %d", mpeg12->mb_info.mb_type);
        mpeg12->mb_info.mb_intra = mpeg12->mb_info.mb_type & 0x10;
        mpeg12->mb_info.mb_pattern = mpeg12->mb_info.mb_type & 0x08;
        if(mpeg12->pic.picture_coding_type == MP2_P_PICTURE)
            mpeg12->mb_info.motion_backward = 0;
        else
            mpeg12->mb_info.motion_backward = mpeg12->mb_info.mb_type & 0x04;
        mpeg12->mb_info.motion_forward = mpeg12->mb_info.mb_type & 0x02;
        mpeg12->mb_info.mb_quant = mpeg12->mb_info.mb_type & 0x01;

        if (mpeg12->debug)
        {
            fprintf(mpeg12->fp_mv, "mb_type: %d, mb_intra: %d forward: %d, backward: %d, pattern: %d\n", 
                mpeg12->mb_info.mb_type, mpeg12->mb_info.mb_intra, mpeg12->mb_info.motion_forward,
                mpeg12->mb_info.motion_backward, mpeg12->mb_info.mb_pattern);
        }
        printf("mb(%d %d) mb_type: %d, mb_intra: %d forward: %d, backward: %d, pattern: %d\n",
            mpeg12->mb_x, mpeg12->mb_y,
            mpeg12->mb_info.mb_type, mpeg12->mb_info.mb_intra, mpeg12->mb_info.motion_forward,
            mpeg12->mb_info.motion_backward, mpeg12->mb_info.mb_pattern);

        if (n > 1)
        {
            //p帧 skipped mb需要把mv的预测值置0
            if (mpeg12->pic.picture_coding_type == MP2_P_PICTURE)
            {
                mpeg12->mb_info.recon_down_for_prev = 0;
                mpeg12->mb_info.recon_right_for_prev = 0;
            }
            mpeg12->last_dc[0] = 128;
            mpeg12->last_dc[1] = 128;
            mpeg12->last_dc[2] = 128;
        }

        if (mpeg12->mb_info.mb_intra)
        {
            mpeg12->mb_info.recon_right_for_prev = 0;
            mpeg12->mb_info.recon_down_for_prev = 0;
            mpeg12->mb_info.recon_right_back_prev = 0;
            mpeg12->mb_info.recon_down_back_prev = 0;
        }
        else
        {
            // DC预测值在以下场景需要置成128：
            // 1.skipped mb
            // 2. non-intra mb
            mpeg12->last_dc[0] = mpeg12->last_dc[1] = mpeg12->last_dc[2] = 128;
        }


        //* 2. update q_scale
        if (mpeg12->mb_info.mb_quant)
        {
            mpeg12->mb_info.quantiser_scale_code = get_bits(&mpeg12->gb, 5);
        }

        mpeg2_update_qscale(mpeg12);

        //* 3. calc forward motion vector 
        mpeg1_calc_mv(mpeg12);

        logv("++++++ offset: %d", get_bits_index(&mpeg12->gb));
        //* 4. get cbp
        if (mpeg12->mb_info.mb_pattern)
        {
            tab = &coded_block_pattern_table[show_bits(&mpeg12->gb, 9)];
            if (tab->len)
            {
                skip_bits(&mpeg12->gb, tab->len);
                mpeg12->mb_info.cbp = tab->value;
            }
            else
            {
                loge("coded_block_pattern code error!\n");
                return -1;
            }
        }
        else if (mpeg12->mb_info.mb_intra)
            mpeg12->mb_info.cbp = 0x3f;
        else
            mpeg12->mb_info.cbp = 0;

        logv("cbp: %x, intra: %d, quant: %d, forward: %d, backward: %d, mb_pattern: %d",
            mpeg12->mb_info.cbp, mpeg12->mb_info.mb_intra, mpeg12->mb_info.mb_quant,
            mpeg12->mb_info.motion_forward, mpeg12->mb_info.motion_backward, mpeg12->mb_info.mb_pattern);

        //* 5. decode the macroblock
        memset(mpeg12->mb_info.recon[0], 0, 768); // 64*sizeof(short)*6
        if (!mpeg12->mb_info.mb_intra)
        {
            if(mpeg12->mb_info.mb_pattern)
                mpeg1_inter_6block(mpeg12);

            int pic_width = mpeg12->mb_width << 4;
            int pic_height = mpeg12->mb_height << 4;
            uint8* source_cb = mpeg12->frame_buf[mpeg12->last_pic_idx].buffer + pic_width* pic_height;
            //logd("===> decode mb ref  %x %x %x %x", source_cb[0], source_cb[1], source_cb[2], source_cb[3]);
            
            //* 6. do mc
            mb_do_mc(mpeg12, mpeg12->mb_x, mpeg12->mb_y);

        }
        else
        {
            mpeg1_intra_6block(mpeg12);
            copydata_to_frame(mpeg12);
        }  

        if (mpeg12->mb_x == 9 && mpeg12->mb_y == 4 && (mpeg12->pic.picture_coding_type == MP2_B_PICTURE))
        {
            //abort();
        }
    }

    mpeg12->prev_mblk_addr = mpeg12->cur_mblk_addr;
    
    return 0;
}



static int mpeg1_slice_header(mpeg12ctx* mpeg12, unsigned char* buf, int len)
{
    int offset = 0;
    slice_header sh = mpeg12->sh;
    sh.quantiser_scale = buf[0] >> 3;
    mpeg12->last_dc[0] = 128;
    mpeg12->last_dc[1] = 128;
    mpeg12->last_dc[2] = 128;

#if 0
    if (!mpeg12->is_mpeg1 /*&& mb_height > 2800/16 */)
    {
        skip_bits(&mpeg12->gb, 3);
    }
#endif

    mpeg12->sh.quantiser_scale = get_bits(&mpeg12->gb, 5);
    mpeg12->mb_info.quantiser_scale = mpeg12->sh.quantiser_scale;
    logv("===== qscale: %d, offset: %d", mpeg12->sh.quantiser_scale, get_bits_index(&mpeg12->gb));

    // init the mb_info q_scale
    mpeg12->mb_info.quantiser_scale = mpeg12->sh.quantiser_scale;
    mpeg12->mb_info.quantiser_scale_code = mpeg12->sh.quantiser_scale;

    // reset the prev mv when first mb in the slice
    mpeg12->mb_info.recon_right_back_prev = 0;
    mpeg12->mb_info.recon_down_back_prev = 0;
    mpeg12->mb_info.recon_down_for_prev = 0;
    mpeg12->mb_info.recon_right_for_prev = 0;

    /* extra slice info */
    while (get_bits(&mpeg12->gb, 1))
    {
        skip_bits(&mpeg12->gb, 8);
    }

    do {
        // decode mb
        if (mpeg12->is_mpeg2)
        {
            loge("donot supprt mpeg2 now");
        }
        else
        {
            mpeg1_macroblock(mpeg12);
        }  
    } while (show_bits(&mpeg12->gb, 23) != 0);

    offset = get_bits_index(&mpeg12->gb) / 8;
    //logd("get_bits_index(&mpeg12->gb): %d, offset: %d", get_bits_index(&mpeg12->gb), offset);

    return offset;
}

static int decode_picture(mpeg12ctx* mpeg12, unsigned char* buf, int len)
{
    decode_picture_header(mpeg12, &mpeg12->pic, buf, len);
    int start_code = -1;
    mpeg12->cur_mblk_addr = -1;
    mpeg12->prev_mblk_addr = -1;
    
    do
    {
        // search start code
        search_start_code(&mpeg12->gb);
        skip_bits(&mpeg12->gb, 24);
        start_code = get_bits(&mpeg12->gb, 8);

        mpeg12->sh.vertical_position = start_code*mpeg12->mb_width -1;
        logv("===== ver_pos: %d", mpeg12->sh.vertical_position);
        // decode slice
        mpeg1_slice_header(mpeg12, buf, len);

        search_start_code(&mpeg12->gb);
        start_code = show_bits(&mpeg12->gb, 32) & 0xff;
    } while (start_code >= 0x01 && start_code <= 0xaf);

    return 0;
}

static int decode_group_of_picture(mpeg12ctx* mpeg12)
{
    int offset = 0;
    gop* g = &mpeg12->group_of_pic;
    g->time_code = get_bits(&mpeg12->gb, 25);
    g->close_gop = get_bits(&mpeg12->gb, 1);
    g->broken_link = get_bits(&mpeg12->gb, 1);

    logd("=========== gop ================= \n");
    logd("time_code: %d \n", g->time_code);
    logd("close_gop: %d \n", g->close_gop);
    logd("============================ \n");

    return 0;
}

static int decode_sequence_header(mpeg12ctx* mpeg12, unsigned char* buf, int len)
{
    sequence_header* seq = &mpeg12->seq_header;

    int i = 0;
    seq->horizontal_size = get_bits(&mpeg12->gb, 12); //12bit
    seq->vertical_size = get_bits(&mpeg12->gb, 12);  // 12bit

    seq->aspect_ratio = get_bits(&mpeg12->gb, 4);
    seq->frame_rate = get_bits(&mpeg12->gb, 4);

    seq->bitrate = get_bits(&mpeg12->gb, 18);
    skip_bits(&mpeg12->gb, 1);
    seq->vbv_buffer_size = get_bits(&mpeg12->gb, 10);

    seq->constrained_param = get_bits(&mpeg12->gb, 1);
    seq->load_intra_quantiser_matrix = get_bits(&mpeg12->gb, 1);

    if (seq->load_intra_quantiser_matrix)
    {
        for (i = 0; i < 64; i++)
        {
            seq->intra_quantiser_matrix[i] = get_bits(&mpeg12->gb, 8);
        }
    }
    else
    {
        for (i = 0; i < 64; i++)
        {
            seq->intra_quantiser_matrix[i] = mpeg2_intra_q[i];
        }
    }

    seq->load_non_intra_quantiser_matrix = get_bits(&mpeg12->gb, 1);
    if (seq->load_non_intra_quantiser_matrix)
    {
        for (i = 0; i < 64; i++)
        {
            seq->non_intra_quantiser_matrix[i] = get_bits(&mpeg12->gb, 8);
        }
    }
    else 
    {
        for (i = 0; i < 64; i++)
        {
            seq->non_intra_quantiser_matrix[i] = 16;
        }
    }

    logd("=========== sequence header ================= \n");
    logd("horizontal_size: %d \n", seq->horizontal_size);
    logd("vertical_size: %d \n", seq->vertical_size);
    logd("aspect_ratio: %d \n", seq->aspect_ratio);
    logd("frame_rate: %d \n", seq->frame_rate);
    logd("load_intra_quantiser_matrix: %d \n", seq->load_intra_quantiser_matrix);
    logd("load_non_intra_quantiser_matrix: %d \n", seq->load_non_intra_quantiser_matrix);
    logd("============================ \n");

    search_start_code(&mpeg12->gb);
    extension_and_user_data(mpeg12);
    return 0;
}

static void print_framebuffer_info(mpeg12ctx* mpeg12)
{
    logd("===================================");
    for (int idx = 0; idx < FRAME_BUFFER_NUM; idx++)
    {
        logd("frame idx(%d) display_num(%d) decode_num(%d)", 
            idx, mpeg12->frame_buf[idx].display_count, mpeg12->frame_buf[idx].decoded_count);
    }
}
static int save_out_yuv(mpeg12ctx* mpeg12)
{
    int i, j;
    print_framebuffer_info(mpeg12);
    if (mpeg12->fp_yuv)
    {
        int idx = 0;
        for (idx = 0; idx < FRAME_BUFFER_NUM; idx++)
        {
            if (mpeg12->frame_buf[idx].display_count == mpeg12->display_pic_num)
            {
                logd("find this display buffer, idx(%d), display_num(%d)", idx, mpeg12->display_pic_num);
                break;
            }
        }
        logd("idx: %d", idx);

        if (idx == FRAME_BUFFER_NUM)
        {
            logw("cannot find this display picture(%d)", mpeg12->display_pic_num);
            return -1;
        }
        int real_w = mpeg12->seq_header.horizontal_size;
        int real_h = mpeg12->seq_header.vertical_size;
        int w = mpeg12->mb_width * 16;
        int h = mpeg12->mb_height * 16;

        // Y
        for (i = 0; i < real_h; i++)
        {
            fwrite(mpeg12->frame_buf[idx].buffer+i*w, 1, real_w, mpeg12->fp_yuv);
        }
        //Cb
        for (i = 0; i < real_h / 2; i++)
        {
            fwrite(mpeg12->frame_buf[idx].buffer+w*h+w/2*i, 1, real_w/2, mpeg12->fp_yuv);
        }
        // Cr
        for (i = 0; i < real_h / 2; i++)
        {
            fwrite(mpeg12->frame_buf[idx].buffer + w*h*5/4 + w / 2 * i, 1, real_w / 2, mpeg12->fp_yuv);
        }   
        mpeg12->display_pic_num++;
    }
    return 0;
}

static void save_pixel_to_file(FILE* fp, uint8*src, int stride, int w, int h)
{
    uint8* tmp = src;
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            fprintf(fp, "%d ", tmp[j]);
        }
        fprintf(fp, "\n");
        tmp += stride;
    }
}
static void save_recon_data(mpeg12ctx* mpeg12)
{
    int pic_w = mpeg12->mb_width * 16;
    int pic_h = mpeg12->mb_height * 16;
    int pic_w_c = mpeg12->mb_width * 8;
    int pic_h_c = mpeg12->mb_height * 8;

    for (int j = 0; j < mpeg12->mb_height; j++)
    {
        for (int i = 0; i < mpeg12->mb_width; i++)
        {
            fprintf(mpeg12->fp_recon, "mb(%d %d) \n", i, j);
            int stride_y = mpeg12->mb_width * 16;
            int stride_c = stride_y / 2;
            
            uint8* y_data = mpeg12->frame_buf[mpeg12->cur_pic_idx].buffer + j * 16 * pic_w + i * 16;
            save_pixel_to_file(mpeg12->fp_recon, y_data, stride_y, 16, 16);
            if (i == 0 && j == 0)
            {
                uint8* tmp = y_data;
                for (int i = 0; i < 16; i++)
                {
                    for (int j = 0; j < 16; j++)
                    {
                        printf("%d ", tmp[j]);
                    }
                    printf("\n");
                    tmp += stride_y;
                }
            }
            uint8* cb_data = mpeg12->frame_buf[mpeg12->cur_pic_idx].buffer + pic_w*pic_h + j * 8 * pic_w_c + i * 8;
            save_pixel_to_file(mpeg12->fp_recon, cb_data, stride_c, 8, 8);
            uint8* cr_data = mpeg12->frame_buf[mpeg12->cur_pic_idx].buffer + pic_w*pic_h * 5 / 4 + j * 8 * pic_w_c + i * 8;
            save_pixel_to_file(mpeg12->fp_recon, cr_data, stride_c, 8, 8);
        }
    }
}

// remove the system start code in buf;
// return the real buf data len after process
static int parse_pes(unsigned char* buf, int len, unsigned char** pro_buf, int* d_len)
{
    FILE* fp = fopen("./pes.bin", "wb");
    int offset = 0;
    int dst_pos = 0;
    int start_code;
    GetBitContext gb;
    int pes_packet_len = 0;
    init_get_bits(&gb, buf, len);
    int data_len;

    int tmp_len = 0;
    while (get_bits_index(&gb) / 8 + 4 < len)
    {
        search_start_code(&gb);
        start_code = get_bits(&gb, 32);
        pes_packet_len = get_bits(&gb, 16);
        data_len = pes_packet_len;
        if (start_code != 0x01BC &&
            start_code != 0x01BE &&
            start_code != 0x01BF &&
            start_code != 0x01F0 &&
            start_code != 0x01F1 &&
            start_code != 0x01FF &&
            start_code != 0x01F2 &&
            start_code != 0x01F8)
        {
            if (!((start_code >= 0x1c0 && start_code <= 0x1df) ||
                (start_code >= 0x1e0 && start_code <= 0x1ef) ||
                (start_code == 0x1bd) ||
                (start_code == 0x1bf) ||
                (start_code == 0x1fd)))
            {
                continue;
            }
            logd("start_code: %x", start_code);

            int c;
            for (;;)
            {
                c = get_bits(&gb, 8);
                data_len--;
                // for mpeg1
                if (c != 0xff)
                    break;
            }

            if ((c & 0xc0) == 0x40) {
                /* buffer scale & size */
                get_bits(&gb, 8);
                c = get_bits(&gb, 8);
                data_len -= 2;
            }

            if ((c & 0xe0) == 0x20) {
                // pts
                get_bits(&gb, 32);
                data_len -= 4;
                if (c & 0x10) {
                    // dts
                    get_bits(&gb, 8);
                    get_bits(&gb, 32);
                    data_len -= 5;
                }
            }
            else if ((c & 0xc0) == 0x80)
            {
                // mpeg2 pes
                int PTS_DTS_flag = get_bits(&gb, 2);
                int ESCR_flag = get_bits(&gb, 1);
                int ES_rate_flag = get_bits(&gb, 1);
                int DSM_trick_mode_flag = get_bits(&gb, 1);
                int additional_copy_info_flag = get_bits(&gb, 1);;
                int PES_CRC_flag = get_bits(&gb, 1);
                int PES_extension_flag = get_bits(&gb, 1);
                int PES_header_data_len = get_bits(&gb, 8);
                logd("===> PES_header_data_len: %d, pes_packet_len: %d", PES_header_data_len, pes_packet_len);

                skip_bits(&gb, PES_header_data_len * 8);
                data_len = pes_packet_len - 3 - PES_header_data_len;
            }
            *d_len = data_len;

            logd("===> data offset: %d, len； %d", get_bits_index(&gb) / 8, data_len);
            for (int i = 0; i < data_len; i++)
            {
                unsigned char c = get_bits(&gb, 8);
                (*pro_buf)[tmp_len+i] = c;
                fwrite(&c, 1, 1, fp);
            }
            tmp_len += data_len;
            //abort();
        }
    }
    fclose(fp);

    if ((*d_len) == 0)
    {
        *d_len = len;
        memcpy(*pro_buf, buf, len);
    }
    
exit:
    return 0;
}

// buf: the bit stream buffer
int mpeg12_decode(void* p, unsigned char* buf, int len)
{
    mpeg12ctx* mpeg12 = (mpeg12ctx*)p;
    //logd("=== decode \n");
    unsigned char start_code = 0xdd;
    int offset = 0;
    int ret = 0;
    unsigned char* pro_buf = (unsigned char*)malloc(8000000);
    memset(pro_buf, 0, 8000000);
    int data_len = 0;


    parse_pes(buf, len, &pro_buf, &data_len);

    init_get_bits(&mpeg12->gb, pro_buf, data_len *8);
    logd("%x %x %x %x", pro_buf[0], pro_buf[1], pro_buf[2], pro_buf[3]);
    
    while (offset < len-2)
    {
        ret = search_start_code(&mpeg12->gb);
        if (ret < 0)
        {
            logd("cannot find start code");
            return -1;
        }
       // logd(" offset: %d %x %x %x \n", offset, buf[0], buf[1], buf[2]);
        logd("start_code: 0x%8x \n", show_bits(&mpeg12->gb, 32));
        skip_bits(&mpeg12->gb, 24);
        start_code = get_bits(&mpeg12->gb, 8);

        // sequence header
        switch (start_code)
        {
        case 0xb3:
            logd("=====> sequence header \n");
            mpeg12->scan_table = ff_zigzag_direct;
            decode_sequence_header(mpeg12, buf + offset, len-offset);
            mpeg12->mb_width = (mpeg12->seq_header.horizontal_size + 15) / 16;
            mpeg12->mb_height = (mpeg12->seq_header.vertical_size + 15) / 16;
            // sequence header
            break;

        case 0xb8:
            // gop
            logd("=====> gop \n");
            mpeg12->display_num_base += mpeg12->gop_pic_num;
            mpeg12->gop_pic_num = 0;
            decode_group_of_picture(mpeg12);
            break;

        case 0x00: // picture
            logd("=====> picture \n");

            //* get an unused buffer
            for (int i = 0; i < FRAME_BUFFER_NUM; i++)
            {
                if (mpeg12->frame_buf[i].is_ref == 0)
                {
                    mpeg12->cur_pic_idx = i;
                    break;
                }
            }

            uint8* source_y = mpeg12->frame_buf[mpeg12->last_pic_idx].buffer;
            logd("===============> picture %x %x %x %x", source_y[0], source_y[1], source_y[2], source_y[3]);
            offset += decode_picture(mpeg12, buf+offset, len-offset);   

            mpeg12->frame_buf[mpeg12->cur_pic_idx].decoded_count = mpeg12->decoded_pic_num;
            save_out_yuv(mpeg12);
            save_recon_data(mpeg12);

            if (mpeg12->fp_pic_info)
            {
                int type = mpeg12->pic.picture_coding_type;
                fprintf(mpeg12->fp_pic_info, "pic decoded num(%d), display num(%d), type(%s), last_pic(%d), next_pic(%d)\n",
                    mpeg12->frame_buf[mpeg12->cur_pic_idx].decoded_count, 
                    mpeg12->frame_buf[mpeg12->cur_pic_idx].display_count,
                    (type == MP2_I_PICTURE)? "I" :((type == MP2_P_PICTURE)? "P": "B"),
                    mpeg12->frame_buf[mpeg12->last_pic_idx].display_count,
                    mpeg12->frame_buf[mpeg12->next_pic_idx].display_count);
                fprintf(mpeg12->fp_pic_info, "cur_idx: %d, last_idx: %d, next_idx: %d\n",
                    mpeg12->cur_pic_idx, mpeg12->last_pic_idx, mpeg12->next_pic_idx);
            }

            mpeg12->decoded_pic_num++;
            mpeg12->gop_pic_num++;

            printf("======= cur ====\n");
            {
                for (int i = 0; i<6; i++)
                    printf("%d ", mpeg12->frame_buf[mpeg12->cur_pic_idx].buffer[i + 2560]);
                printf("\n");
            }
            
            if (mpeg12->decoded_pic_num > 60)
                return -1;
            break;
        }
        offset = get_bits_index(&mpeg12->gb) / 8;    
    }
    
    free(pro_buf);
    return 0;
}

void* create_mpeg12_decoder()
{
    init_vlcs();
    mpeg12ctx* mpeg12 = (mpeg12ctx*)malloc(sizeof(mpeg12ctx));
    memset(mpeg12, 0, sizeof(mpeg12ctx));
    for (int i = 0; i < FRAME_BUFFER_NUM; i++)
    {
        mpeg12->frame_buf[i].decoded_count = -1;
        mpeg12->frame_buf[i].display_count = -1;
    }
    mpeg12->debug = 1;
    if (mpeg12->debug)
    {
        mpeg12->fp_dct_coeff = fopen("./trace/dct_coeff.txt", "w");
        mpeg12->fp_idct_out = fopen("./trace/idct_out.txt", "w");
        mpeg12->fp_mv = fopen("./trace/mv.txt", "w");
        
        mpeg12->fp_pred = fopen("./trace/pred.txt", "w");
        mpeg12->fp_recon = fopen("./trace/recon.txt", "w");
        mpeg12->fp_pic_info = fopen("./trace/pic_info.txt", "w");
    }
    mpeg12->fp_yuv = fopen("./trace/out.yuv", "wb");

    mpeg12->cur_pic_idx = 0;
    mpeg12->last_pic_idx = 0;
    mpeg12->next_pic_idx = 0;

    return mpeg12;
}

int destroy_mpeg12_decoder(void* p)
{
    mpeg12ctx* mpeg12 = (mpeg12ctx*)p;
    if(mpeg12->fp_dct_coeff)
        fclose(mpeg12->fp_dct_coeff);
    if(mpeg12->fp_idct_out)
        fclose(mpeg12->fp_idct_out);
    if(mpeg12->fp_yuv)
        fclose(mpeg12->fp_yuv);
    if(mpeg12->fp_pred)
        fclose(mpeg12->fp_pred);
    if (mpeg12->fp_recon)
        fclose(mpeg12->fp_recon);
    if(mpeg12->fp_mv)
        fclose(mpeg12->fp_mv);
    if (mpeg12->fp_pic_info)
        fclose(mpeg12->fp_pic_info);
    free(mpeg12);
    return 0;
}


