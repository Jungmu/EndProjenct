#include "DataCollector.h"
#include "VideoSocket.h"

// compatibility with newer API
// ������ �ٲ�鼭 �޶��� ���� �̷��� Ŀ���ϴ� �� �ϴ�.
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

bool rest = true;

// ���� �Լ�
// ȭ�� �������� ������ 2:3 �� ��쿡�� ���������� ���۵ȴ�.
void ZoomIn(int &width,int &height,int limit)
{
	if (width < limit)
	{
		height += 20;
		width += 30;
	}	
}

// �� �ƿ� �Լ�
// ȭ�� �������� ������ 2:3 �� ��쿡�� ���������� ���۵ȴ�.
void ZoomOut(int &width, int &height, int limit)
{
	if (limit < width)
	{
		height -= 20;
		width -= 30;
	}
}

// ������ ����
// ������ ī�޶� ������� �����͸� ������ ���̱� ������ �� ���� 1�� ���ڿ��� ������.
void SendData(SOCKET ClientSocket, char* Message, SOCKADDR_IN &ToServer)
{
	sendto(ClientSocket, Message, 1 , 0, (struct sockaddr*) &ToServer, sizeof(ToServer));
	
}

void InitSDL(){
	// Register all formats and codecs
	av_register_all();
	//network init �̰� ���� ������ ������ ���� �ϴ� ������. (�� ������ �� �𸣰ڽ�)
	avformat_network_init();

	//SDL �ʱ�ȭ 
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}
}


int main(int argc, char *argv[]) 
{
	
	// Initalizing these to NULL prevents segfaults!
	AVFormatContext   *pFormatCtx = NULL;
	int               i, videoStream;
	AVCodecContext    *pCodecCtxOrig = NULL;
	AVCodecContext    *pCodecCtx = NULL; // �ڵ� ��Ʈ�ѷ�(?) �̰� ���� ����.
	AVCodec           *pCodec = NULL; // ������ ���ڵ��� �ڵ�
	AVFrame           *pFrame = NULL; // �������� ��� �����.
	AVPacket          packet;
	int               frameFinished;
	struct SwsContext *sws_ctx = NULL; // Convert the image into YUV format that SDL uses

	//SDL ���� ����
	SDL_Overlay     *bmp;
	SDL_Surface     *screen;
	SDL_Rect        rect;
	SDL_Event       event;

	CVideoSocket videoSocket;
	
	//���� �� �ƿ��� ���� ����
	int rect_w = 0;
	int rect_h = 0;


	
	// We catch any exceptions that might occur below -- see the catch statement for more details.
	try 
	{
	// ������� ���̿� �ʱ�ȭ
	// First, we create a Hub with our application identifier. Be sure not to use the com.example namespace when
	// publishing your application. The Hub provides access to one or more Myos.
	// ���̿����� �����ϴ� ���ø����̼ǰ� �����ϴ� ��� ����
	myo::Hub hub("com.example.hello-myo");

	// ���̿� ã���� ...
	std::cout << "Attempting to find a Myo..." << std::endl;

	// Next, we attempt to find a Myo to use. If a Myo is already paired in Myo Connect, this will return that Myo
	// immediately.
	// waitForMyo() takes a timeout value in milliseconds. In this case we will try to find a Myo for 10 seconds, and
	// if that fails, the function will return a null pointer.
	// ���̿��� ã�� ���� ����ϴ� �ҽ��ڵ�
	myo::Myo* myo = hub.waitForMyo(10000);

	// If waitForMyo() returned a null pointer, we failed to find a Myo, so exit with an error message.
	// ���̿��� �������� ������� ����ó��
	if (!myo) 
	{
		throw std::runtime_error("Unable to find a Myo!");
	}

	// We've found a Myo.
	//std::cout << "Connected to a Myo armband!" << std::endl << std::endl;

	// Next we construct an instance of our DeviceListener, so that we can register it with the Hub.
	// ���̿����� ���� �����͸� �������ִ� Ŭ����
	DataCollector collector;

	// Hub::addListener() takes the address of any object whose class inherits from DeviceListener, and will cause
	// Hub::run() to send events to all registered device listeners.
	// �����͸� ���������� �޾ƿ´�.
	hub.addListener(&collector);

	//---������� ���̿� �ʱ�ȭ
	
	// SDL �ʱ�ȭ
	InitSDL();

	// Open video file
	// ���� �Ǵ� ������ ��Ʈ���� ����.
	if (avformat_open_input(&pFormatCtx, videoSocket.videoStreamUrl, NULL, NULL) != 0)
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
	av_dump_format(pFormatCtx, 0, videoSocket.videoStreamUrl, 0);

	// Find the first video stream
	// ���� ��Ʈ���� ã�°��� - � ������ ������ ��Ʈ������ �Ǻ� ( �츮�� h.264�� �����Ǿ�������...)
	videoStream = -1;
	for (i = 0; (unsigned)i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) 
		{
			videoStream = i;
			break;
		}
	}	
	if (videoStream == -1)
	{
		return -1; // Didn't find a video stream
	}
	// Get a pointer to the codec context for the video stream
	pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
	// Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
	if (pCodec == NULL) 
	{
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}
	// Copy context
	// �� ���� �𸣰����� �׳� ���� �ʰ� �����ؼ� ����Ѵ�.
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) 
	{
		fprintf(stderr, "Couldn't copy codec context");
		return -1; // Error copying codec context
	}

	// Open codec
	if (avcodec_open2(pCodecCtx, pCodec, NULL)<0)
	{
		return -1; // Could not open codec
	}
	
	// Allocate video frame
	pFrame = av_frame_alloc();

	// Make a screen to put our video
	// ��ũ���� ����
#ifndef __DARWIN__
	screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
#else
	screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 24, 0);
#endif
	if (!screen) 
	{
		fprintf(stderr, "SDL: could not set video mode - exiting\n");
		exit(1);
	}

	// Allocate a place to put our YUV image on that screen
	// �̹����� ��ũ���� �׸�
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

	while (av_read_frame(pFormatCtx, &packet) >= 0) 
	{
		// ���� ����

		// In each iteration of our main loop, we run the Myo event loop for a set number of milliseconds.
		// �����͸� ������� �ֱ�� �޾ƿ��� ���ϴ� �ҽ�
		// �� ���� �������� ������ �޾ƿ��µ��� �����̰� �ɸ��⶧���� ���ϴ� fps�� ����ؾ��Ѵ�.
		hub.run(1000 / 500);
		// After processing events, we call the print() member function we defined above to print out the values we've
		// obtained from any events that have occurred.
		// ���̿� ���� ����͸� �ڵ�
		collector.print();

		// ���̿� ���� �������


		// Is this a packet from the video stream?
		if (packet.stream_index == videoStream) 
		{
			// Decode video frame
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

			// Did we get a video frame?
			// ���� �������� ��Ʈ�� �̹����� ��ȯ
			if (frameFinished) 
			{
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

				// ����Ʈ��������� ���� �ܾƿ��� �ϱ����� ������������ ����� ����
				rect.x = -rect_w/2;
				rect.y = -rect_h/2;
				rect.w = pCodecCtx->width + rect_w;
				rect.h = pCodecCtx->height + rect_h;
				SDL_DisplayYUVOverlay(bmp, &rect);

			}
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
		SDL_PollEvent(&event);

		//// ���̿��� ������ üũ�ؼ� �޽��� �۽�
		//// �¿� ī�޶� ��Ʈ��
		if (collector.currentPose == myo::Pose::waveOut)
		{
			SendData(videoSocket.ClientSocket, "right", videoSocket.ToServer);rest = true;
				//sendto(ClientSocket, right_m, sizeof(right_m), 0, (struct sockaddr*) &ToServer, sizeof(ToServer));
		}	
		if (collector.currentPose == myo::Pose::waveIn)
		{
			SendData(videoSocket.ClientSocket, "left", videoSocket.ToServer);rest = true;
				//sendto(ClientSocket, left_m, sizeof(left_m), 0, (struct sockaddr*) &ToServer, sizeof(ToServer));
		}
		// ���� ī�޶� ��Ʈ��
		if (collector.currentPose == myo::Pose::fingersSpread && collector.pitch_w > 10)
		{
			SendData(videoSocket.ClientSocket, "up", videoSocket.ToServer);rest = true;
		}
		if (collector.currentPose == myo::Pose::fingersSpread && collector.pitch_w < 6)
		{
			SendData(videoSocket.ClientSocket, "down", videoSocket.ToServer);
			rest = true;
		}
		if (collector.currentPose == myo::Pose::rest &&rest == true)
		{
			SendData(videoSocket.ClientSocket, "stop", videoSocket.ToServer);
			rest = false;
		}
		if (collector.currentPose == myo::Pose::doubleTap && collector.roll_w <= 5)
		{
			collector.currentPose = myo::Pose::rest;
			myo->lock();
						
		}
		if (collector.currentPose == myo::Pose::doubleTap && collector.roll_w > 5)
		{

			myo->unlock(myo::Myo::unlockHold);

		}
		// ���̿��� ������ üũ�ؼ� ���� �� �ƿ�
		if (collector.currentPose == myo::Pose::fist && collector.roll_w < 6)
		{
			ZoomOut(rect_w, rect_h, 0);
		}
		if (collector.currentPose == myo::Pose::fist && collector.roll_w > 8)
		{
			ZoomIn(rect_w, rect_h, 300);
		}
		// Ű �̺�Ʈ�� �޴� �Լ�
		switch (event.type) 
		{
		case SDL_QUIT:
			SDL_Quit();
			exit(0);
			break;
		case SDL_KEYDOWN:
			/* Check the SDLKey values and move change the coords */
			switch (event.key.keysym.sym){
			case SDLK_LEFT:
				// ���ڿ� �۽�
				SendData(videoSocket.ClientSocket, "left", videoSocket.ToServer);
				break;
			case SDLK_RIGHT:
				// ���ڿ� �۽�
				SendData(videoSocket.ClientSocket, "right", videoSocket.ToServer);
				break;
			case SDLK_UP:
				SendData(videoSocket.ClientSocket, "up", videoSocket.ToServer);
				break;
			case SDLK_DOWN:
				SendData(videoSocket.ClientSocket, "down", videoSocket.ToServer);
				break;
			case SDLK_q: // �� ��
				ZoomIn(rect_w,rect_h,300);			
				break;
			case SDLK_w: // �� �ƿ�
				ZoomOut(rect_w, rect_h, 0);								
				break;
			case SDLK_s: // test
				SendData(videoSocket.ClientSocket, "stop", videoSocket.ToServer);
				break;
			case SDLK_x: // �ñ׷� ����
				SDL_Quit();
				exit(0);
				break;
			default:
				break;
			}
		default:
			break;
		}

	}
	
	// Free the YUV frame
	av_frame_free(&pFrame);

	// Close the codecs
	avcodec_close(pCodecCtx);
	avcodec_close(pCodecCtxOrig);

	// Close the video file
	avformat_close_input(&pFormatCtx);

	// ���� �ݱ�
	closesocket(videoSocket.ClientSocket);
	WSACleanup();

	return 0;

	}
	// ���������� exception handling�� �̷����ϴ°� �������� ������...
	// �������� �̷��� ����Ͽ��⿡ �ϴ� �̷��� �ξ���.
	catch (const std::exception& e) 
	{
		std::cerr << "Error: " << e.what() << std::endl;
		std::cerr << "Press enter to continue.";
		std::cin.ignore();
		return 1;
	}
	
}

