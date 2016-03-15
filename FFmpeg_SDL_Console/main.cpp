///> Include FFMpeg
// C ����� ��������� ���Խ�ų���� �̷��� ����ϴ°� ����.
extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <SDL.h>
#include <SDL_thread.h>
}

#include <iostream>

///> Library Link On Windows System
// �ڲ� ���� �ȵǼ� ffmpeg�� ��� ���̺귯���� �� ����־��
#pragma comment( lib, "avformat.lib" )	
#pragma comment( lib, "avutil.lib" )
#pragma comment( lib, "avcodec.lib" )
#pragma comment( lib, "avdevice.lib" )	
#pragma comment( lib, "avfilter.lib" )
#pragma comment( lib, "postproc.lib" )
#pragma comment( lib, "swresample.lib" )
#pragma comment( lib, "swscale.lib" )
//SDL ���̺귯��
#pragma comment( lib, "SDLmain.lib")
#pragma comment( lib, "SDL.lib")


// compatibility with newer API
// ������ �ٲ�鼭 �޶��� ���� �̷��� Ŀ���ϴ� �� �ϴ�.
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

// ���� �������� �̹����� �����ϴ� �Լ�
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;
	char szFilename[32];
	int  y;

	// Open file
	sprintf_s(szFilename, "frame%d.ppm", iFrame);
	fopen_s(&pFile, szFilename, "wb");
	if (pFile == NULL)
		return;

	// Write header
	// ȭ�� ������� Į���� ����(?) �� ���� 0~255 ���� ����
	fprintf(pFile, "P6\n%d %d\n255\n", width, height);

	// Write pixel data
	for (y = 0; y<height; y++)
		fwrite(pFrame->data[0] + y*pFrame->linesize[0], 1, width * 3, pFile);

	// Close file
	fclose(pFile);
}

int main(int argc, char *argv[]) {
	// Initalizing these to NULL prevents segfaults!
	AVFormatContext   *pFormatCtx = NULL;
	int               i, videoStream;
	AVCodecContext    *pCodecCtxOrig = NULL;
	AVCodecContext    *pCodecCtx = NULL; // �ڵ� ��Ʈ�ѷ�(?) �̰� ���� ����.
	AVCodec           *pCodec = NULL; // ������ ���ڵ��� �ڵ�
	AVFrame           *pFrame = NULL; // �������� ��� �����.
	AVFrame           *pFrameRGB = NULL; // RGB�� ���� ���������� ������� ���´�.
	AVPacket          packet;
	int               frameFinished;
	int               numBytes;
	uint8_t           *buffer = NULL;
	struct SwsContext *sws_ctx = NULL; // RGB���� ������ ����ϴ��� �ڼ��� �𸣰���

	//SDL ���� ����
	SDL_Overlay     *bmp;
	SDL_Surface     *screen;
	SDL_Rect        rect;
	SDL_Event       event;

	// Register all formats and codecs
	av_register_all();
	//network init �̰� ���� ������ ������ ���� �ϴ� ������. (�� ������ �� �𸣰ڽ�)
	avformat_network_init();

	//SDL �ʱ�ȭ 
	//���� SDL������ ������ ����
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}

	// Open video file
	// ���� �Ǵ� ������ ��Ʈ���� ����.
	if (avformat_open_input(&pFormatCtx, "tcp://192.168.0.15:2222", NULL, NULL) != 0)
	{
		return -1; // Couldn't open file
	}
	
	// Retrieve stream information
	// ������ ��Ʈ���� ������ ���´�.
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		return -1; // Couldn't find stream information
	}
	// Dump information about file onto standard error
	// ������ �߻��� ��� ���� ������ �����ϴ� �ڵ�� ����
	av_dump_format(pFormatCtx, 0, "tcp://192.168.0.15:2222", 0);

	// Find the first video stream
	// ���� ��Ʈ���� ã�°��� - � ������ ������ ��Ʈ������ �Ǻ� ( �츮�� h.264�� �����Ǿ�������...)
	videoStream = -1;
	for (i = 0; (unsigned)i<pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	if (videoStream == -1)
		return -1; // Didn't find a video stream

	// Get a pointer to the codec context for the video stream
	pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
	// Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
	if (pCodec == NULL) {
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}
	// Copy context
	// �� ���� �𸣰����� �׳� ���� �ʰ� �����ؼ� ����Ѵ�.
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
		fprintf(stderr, "Couldn't copy codec context");
		return -1; // Error copying codec context
	}

	// Open codec
	if (avcodec_open2(pCodecCtx, pCodec, NULL)<0)
		return -1; // Could not open codec

	// Allocate video frame
	pFrame = av_frame_alloc();




	// Allocate an AVFrame structure
	// RGB�����͸� �����ϱ����� �غ�
	// �ƹ����� RGB �����͸� �����ϱ� ���ؼ��� ���� ū ���۰� �ʿ��ѵ� �ϴ�.
	// ��Ȯ�� � ������ color�� ������ ������ �ǹ��̴�..
	// �� ������ ���������� ������� ���ͼ� �ָԾ���...

	//... �ⲯ RGB ��ȯ ������ �����ߴµ� ��½ÿ� ���� �ʴ´�.
	/*pFrameRGB = av_frame_alloc();
	if (pFrameRGB == NULL)
		return -1;*/

	// Determine required buffer size and allocate buffer
	numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width,
		pCodecCtx->height);
	buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	// (AVPixelFormat)2 ��� ���ڴ� ���� ������ ���°Ծƴ϶� enum���� ������ �Ǿ��ִ�
	// �ǵ� �̻��ϰ� ã�Ƴ��� ���ϱ淡 �׳� ���ڷ� �����ھҴ�.
	// ������ enum�� ������ ���̴ϱ�...
	// RGB��ȯ�� ���� �ʴ´�.
	/*avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_YUV420P,
		pCodecCtx->width, pCodecCtx->height);*/

	// Make a screen to put our video
#ifndef __DARWIN__
	screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
#else
	screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 24, 0);
#endif
	if (!screen) {
		fprintf(stderr, "SDL: could not set video mode - exiting\n");
		exit(1);
	}

	// Allocate a place to put our YUV image on that screen
	bmp = SDL_CreateYUVOverlay(pCodecCtx->width,
		pCodecCtx->height,
		SDL_YV12_OVERLAY,
		screen);

	// initialize SWS context for software scaling
	sws_ctx = sws_getContext(pCodecCtx->width,
		pCodecCtx->height,
		pCodecCtx->pix_fmt,
		pCodecCtx->width,
		pCodecCtx->height,
		AV_PIX_FMT_YUV420P,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
		);

	// Read frames and save first five frames to disk
	i = 0;
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		// Is this a packet from the video stream?
		if (packet.stream_index == videoStream) {
			// Decode video frame
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

			// Did we get a video frame?
			if (frameFinished) {
				SDL_LockYUVOverlay(bmp);

				AVPicture pict;
				pict.data[0] = bmp->pixels[0];
				pict.data[1] = bmp->pixels[2];
				pict.data[2] = bmp->pixels[1];

				pict.linesize[0] = bmp->pitches[0];
				pict.linesize[1] = bmp->pitches[2];
				pict.linesize[2] = bmp->pitches[1];

				// Convert the image into YUV format that SDL uses
				sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
					pFrame->linesize, 0, pCodecCtx->height,
					pict.data, pict.linesize);

				SDL_UnlockYUVOverlay(bmp);

				rect.x = 0;
				rect.y = 0;
				rect.w = pCodecCtx->width;
				rect.h = pCodecCtx->height;
				SDL_DisplayYUVOverlay(bmp, &rect);

			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
		SDL_PollEvent(&event);
		switch (event.type) {
		case SDL_QUIT:
			SDL_Quit();
			exit(0);
			break;
		default:
			break;
		}

	}

	// Free the RGB image
	av_free(buffer);
	av_frame_free(&pFrameRGB);

	// Free the YUV frame
	av_frame_free(&pFrame);

	// Close the codecs
	avcodec_close(pCodecCtx);
	avcodec_close(pCodecCtxOrig);

	// Close the video file
	avformat_close_input(&pFormatCtx);

	return 0;
}
