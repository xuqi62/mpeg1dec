#ifndef __LOG_H__
#define __LOG_H__

#include "stdafx.h"
#include <io.h>
#include <fcntl.h>

#ifndef LOG_TAG
#define LOG_TAG "mpeg12dec"
#endif

#define DEBUG

#define logv(...) {}
//#define logv(...) { printf("ver  : %s <%s:%u>: ", LOG_TAG, __FUNCTION__, __LINE__); \
//                        printf(__VA_ARGS__); printf("\n"); }

#ifdef DEBUG
#define logd(...) { printf("debug  : %s <%s:%u>: ", LOG_TAG, __FUNCTION__, __LINE__); \
                        printf(__VA_ARGS__); printf("\n"); }
#else
#define logd(...) {}
#endif

#define logw(...) { printf("warning  : %s <%s:%u>: ", LOG_TAG, __FUNCTION__, __LINE__); \
                        printf(__VA_ARGS__); printf("\n"); }
#define loge(...) { printf("error  : %s <%s:%u>: ", LOG_TAG, __FUNCTION__, __LINE__); \
                        printf(__VA_ARGS__); printf("\n"); }
#endif