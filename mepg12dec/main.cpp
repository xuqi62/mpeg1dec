// stdafx.cpp : source file that includes just the standard includes
// mepg12dec.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include "mpeg12dec_api.h"


int main()
{
    int tmp = _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //tmp |= _CRTDBG_CHECK_ALWAYS_DF;
    tmp = _CrtSetDbgFlag(tmp);

    //FILE* fp = fopen("./video/DELTA.MPG", "rb");
    FILE* fp = fopen("./video/a_movie2.mpeg", "rb");
    if (fp == NULL)
    {
        printf("open file failed \n");
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);

    printf("len : %d \n", len);
    fseek(fp, 0, SEEK_SET);
    unsigned char* buf = (unsigned char*)malloc(len);
    fread(buf, 1, len, fp);
    fclose(fp);
    

    void* dec = create_mpeg12_decoder();
    mpeg12_decode(dec, buf, len);
    destroy_mpeg12_decoder(dec);

    return 0;
}
