#pragma once

void* create_mpeg12_decoder();
int destroy_mpeg12_decoder(void* p);
int mpeg12_decode(void* p, unsigned char* buf, int len);