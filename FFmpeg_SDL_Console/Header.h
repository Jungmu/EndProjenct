#pragma once

///> Include FFMpeg and SDL
extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <windows.h>
}

///> Library Link On Windows System
// 자꾸 뭔가 안되서 ffmpeg의 모든 라이브러리를 다 집어넣어둠
#pragma comment( lib, "avformat.lib" )	
#pragma comment( lib, "avutil.lib" )
#pragma comment( lib, "avcodec.lib" )
#pragma comment( lib, "avdevice.lib" )	
#pragma comment( lib, "avfilter.lib" )
#pragma comment( lib, "postproc.lib" )
#pragma comment( lib, "swresample.lib" )
#pragma comment( lib, "swscale.lib" )
//SDL 라이브러리
#pragma comment( lib, "SDLmain.lib")
#pragma comment( lib, "SDL.lib")

//윈 소켓
#pragma comment (lib,"ws2_32.lib")