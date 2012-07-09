#pragma once

//#include <d3d9.h>
#ifndef UINT64_C
#define UINT64_C(c) (c ## ULL)
#endif

/**
 * FFmpeg関連ヘッダファイル.
 */
extern "C" {
#define inline _inline
#include <libavutil/avstring.h>
#include <libavcodec/avcodec.h>
//#include <libavcodec/dxva2.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}


#pragma comment(lib, "libgcc.a")
#pragma comment(lib, "libmingwex.a")
//#pragma comment(lib, "libz.a")
//#pragma comment(lib, "libbz2.a")
#pragma comment(lib, "libavcodec.a")
#pragma comment(lib, "libavfilter.a")
#pragma comment(lib, "libavutil.a")
#pragma comment(lib, "libavformat.a")
#pragma comment(lib, "libavdevice.a")
#pragma comment(lib, "libswscale.a")
#pragma comment(lib, "libswresample.a")
//#pragma comment(lib, "libmp3lame.a")

