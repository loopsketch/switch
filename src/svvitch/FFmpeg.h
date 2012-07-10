#pragma once

#ifndef UINT64_C
#define UINT64_C(c) (c ## ULL)
#endif

/**
 * FFmpeg�֘A�w�b�_�t�@�C��.
 */
extern "C" {
#define inline _inline
#include <libavutil/avstring.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}


#pragma comment(lib, "libgcc.a")
#pragma comment(lib, "libmingwex.a")
#pragma comment(lib, "libavcodec.a")
#pragma comment(lib, "libavfilter.a")
#pragma comment(lib, "libavutil.a")
#pragma comment(lib, "libavformat.a")
#pragma comment(lib, "libavdevice.a")
#pragma comment(lib, "libswscale.a")
#pragma comment(lib, "libswresample.a")

