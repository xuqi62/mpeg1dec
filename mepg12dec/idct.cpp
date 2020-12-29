#include "stdafx.h"
#include "idct.h"

static int idct_tab1[16] =
    { 1,0,0,0,0,0,-1,0,-1,0,0,0,1,0,1,0 };
static int idct_tab2[16] =
{ 1,0,0,0,-1,0,-1,0,0,1,0,0,0,0,1,0 };
static int idct_tab3[16] =
{ 1,0,-1,0,1,0,1,0,1,0,0,-1,0,0,-1,0 };
static int idct_tab4[16] =
{ 1,0,-1,0,-1,0,1,0,1,0,0,0,0,0,1,0 };
static int idct_tab5[16] =
{ 0,1,0,0,1,0,0,-1,0,0,1,0,0,-1,0,1 };
static int idct_tab6[16] =
    { 1,0,-1,0,0,0,1,0,0,0,0,0,-1,0,0,0 }; //>>1
static int idct_tab7[16] =
    { 1,0,-1,0,0,1,0,0,0,0,-1,0,0,1,0,-1 };  //>>2

// inverse DCT for 16x16 mb 
int mpeg_idct(short *data)
{
    long idct_data1[64], idct_data2[64];
    long *fdata1, *fdata2;
    static unsigned char index = 0;
    // Even Coefficients
    int u0_cos2, u0_cos4, u0_cos6;
    int u2_cos2, u2_cos4, u2_cos6;
    int u4_cos2, u4_cos4, u4_cos6;
    int u6_cos2, u6_cos4, u6_cos6;
    // Odd Coefficients
    int u1_cos1, u1_cos3, u1_cos5, u1_cos7;
    int u3_cos1, u3_cos3, u3_cos5, u3_cos7;
    int u5_cos1, u5_cos3, u5_cos5, u5_cos7;
    int u7_cos1, u7_cos3, u7_cos5, u7_cos7;
    int int_dout0, int_dout2, int_dout4, int_dout6;
    int int_dout1, int_dout3, int_dout5, int_dout7;
    int i, j, k, hor;

    memset(idct_data1, 0, 4 * 64);
    memset(idct_data2, 0, 4 * 64);
    fdata1 = idct_data1;
    fdata2 = idct_data2;
    //21-bit
    for (i = 0; i<64; i++)
        fdata1[i] = ((long)(*(data + i))) << 10;

    for (k = 0; k<2; k++)
    {
        for (hor = 0; hor<8; hor++)
        {
            u0_cos2 = u0_cos6 = u2_cos2 = u2_cos6 = u4_cos2 = u4_cos6 = u6_cos2 = u6_cos6 = 0;
            u0_cos4 = u2_cos4 = u4_cos4 = u6_cos4 = 0;
            u1_cos1 = u1_cos3 = u1_cos5 = u1_cos7 = u3_cos1 = u3_cos3 = u3_cos5 = u3_cos7 = 0;
            u5_cos1 = u5_cos3 = u5_cos5 = u5_cos7 = u7_cos1 = u7_cos3 = u7_cos5 = u7_cos7 = 0;
            i = hor << 3;

            for (j = 0; j<16; j++)
            {
                u0_cos2 += (fdata1[i] >> (j)) * idct_tab2[j];
                u0_cos4 += (fdata1[i] >> (j)) * idct_tab4[j];
                u0_cos6 += (fdata1[i] >> (j)) * idct_tab6[j];
                u2_cos2 += (fdata1[i + 2] >> (j)) * idct_tab2[j];
                u2_cos4 += (fdata1[i + 2] >> (j)) * idct_tab4[j];
                u2_cos6 += (fdata1[i + 2] >> (j)) * idct_tab6[j];
                u4_cos2 += (fdata1[i + 4] >> (j)) * idct_tab2[j];
                u4_cos4 += (fdata1[i + 4] >> (j)) * idct_tab4[j];
                u4_cos6 += (fdata1[i + 4] >> (j)) * idct_tab6[j];
                u6_cos2 += (fdata1[i + 6] >> (j)) * idct_tab2[j];
                u6_cos4 += (fdata1[i + 6] >> (j)) * idct_tab4[j];
                u6_cos6 += (fdata1[i + 6] >> (j)) * idct_tab6[j];

                u1_cos1 += (fdata1[i + 1] >> (j)) * idct_tab1[j];
                u1_cos3 += (fdata1[i + 1] >> (j)) * idct_tab3[j];
                u1_cos5 += (fdata1[i + 1] >> (j)) * idct_tab5[j];
                u1_cos7 += (fdata1[i + 1] >> (j)) * idct_tab7[j];
                u3_cos1 += (fdata1[i + 3] >> (j)) * idct_tab1[j];
                u3_cos3 += (fdata1[i + 3] >> (j)) * idct_tab3[j];
                u3_cos5 += (fdata1[i + 3] >> (j)) * idct_tab5[j];
                u3_cos7 += (fdata1[i + 3] >> (j)) * idct_tab7[j];
                u5_cos1 += (fdata1[i + 5] >> (j)) * idct_tab1[j];
                u5_cos3 += (fdata1[i + 5] >> (j)) * idct_tab3[j];
                u5_cos5 += (fdata1[i + 5] >> (j)) * idct_tab5[j];
                u5_cos7 += (fdata1[i + 5] >> (j)) * idct_tab7[j];
                u7_cos1 += (fdata1[i + 7] >> (j)) * idct_tab1[j];
                u7_cos3 += (fdata1[i + 7] >> (j)) * idct_tab3[j];
                u7_cos5 += (fdata1[i + 7] >> (j)) * idct_tab5[j];
                u7_cos7 += (fdata1[i + 7] >> (j)) * idct_tab7[j];
            }

            int_dout0 = u0_cos4 + u2_cos2 + u4_cos4 + ((u6_cos6 + 1) >> 1);
            int_dout2 = u0_cos4 + ((u2_cos6 + 1) >> 1) - u4_cos4 - u6_cos2;
            int_dout4 = u0_cos4 - ((u2_cos6 + 1) >> 1) - u4_cos4 + u6_cos2;
            int_dout6 = u0_cos4 - u2_cos2 + u4_cos4 - ((u6_cos6 + 1) >> 1);

            int_dout1 = u1_cos1 + u3_cos3 + u5_cos5 + ((u7_cos7 + 2) >> 2);
            int_dout3 = u1_cos3 - ((u3_cos7 + 2) >> 2) - u5_cos1 - u7_cos5;
            int_dout5 = u1_cos5 - u3_cos1 + ((u5_cos7 + 2) >> 2) + u7_cos3;
            int_dout7 = ((u1_cos7 + 2) >> 2) - u3_cos5 + u5_cos3 - u7_cos1;

            fdata2[hor] = (int_dout0 + int_dout1);
            fdata2[(7 << 3) + hor] = (int_dout0 - int_dout1);
            fdata2[(1 << 3) + hor] = (int_dout2 + int_dout3);
            fdata2[(6 << 3) + hor] = (int_dout2 - int_dout3);
            fdata2[(2 << 3) + hor] = (int_dout4 + int_dout5);
            fdata2[(5 << 3) + hor] = (int_dout4 - int_dout5);
            fdata2[(3 << 3) + hor] = (int_dout6 + int_dout7);
            fdata2[(4 << 3) + hor] = (int_dout6 - int_dout7);
        }
        if (k == 0)
        {
            for (j = 0; j<64; j++)
            {
                fdata2[j] = (fdata2[j] + 32) >> 6;
                fdata2[j] <<= 4;
            }
            fdata1 = idct_data2;
            fdata2 = idct_data1;
        }
    }
    for (i = 0; i<64; i++)
    {
        fdata2[i] = (fdata2[i] + 512) >> 10;
        *(data + i) = (short)fdata2[i];

        // clip to [-256, 255]

        if (*(data + i)>255)
            *(data + i) = 255;
        else if (*(data + i)<-256)
            *(data + i) = -256;
    }
    return 0;
}