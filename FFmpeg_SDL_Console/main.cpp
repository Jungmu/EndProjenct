#include "DataCollector.h"
#include "VideoSocket.h"

// compatibility with newer API
// 버전이 바뀌면서 달라진 점을 이렇게 커버하는 듯 하다.
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

// 줌인 함수
// 화면 사이즈의 비율이 2:3 일 경우에만 정상적으로 동작된다.
void ZoomIn(int &width,int &height,int limit)
{
	if (width < limit)
	{
		height += 20;
		width += 30;
	}	
}

// 줌 아웃 함수
// 화면 사이즈의 비율이 2:3 일 경우에만 정상적으로 동작된다.
void ZoomOut(int &width, int &height, int limit)
{
	if (limit < width)
	{
		height -= 20;
		width -= 30;
	}
}

// 데이터 샌드
// 어차피 카메라 제어를위해 데이터를 보내는 것이기 때문에 맨 앞의 1개 문자열만 보낸다.
void SendData(SOCKET ClientSocket, char* Message, SOCKADDR_IN &ToServer)
{
	sendto(ClientSocket, Message, 1 , 0, (struct sockaddr*) &ToServer, sizeof(ToServer));
}

void InitSDL(){
	// Register all formats and codecs
	av_register_all();
	//network init 이걸 적지 않으면 워링이 나서 일단 적었다. (왜 적는지 잘 모르겠슴)
	avformat_network_init();

	//SDL 초기화 
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
	AVCodecContext    *pCodecCtx = NULL; // 코덱 컨트롤러(?) 이걸 자주 쓴다.
	AVCodec           *pCodec = NULL; // 영상을 디코딩할 코덱
	AVFrame           *pFrame = NULL; // 영상데이터 라고 보면됨.
	AVPacket          packet;
	int               frameFinished;
	struct SwsContext *sws_ctx = NULL; // Convert the image into YUV format that SDL uses

	//SDL 관련 변수
	SDL_Overlay     *bmp;
	SDL_Surface     *screen;
	SDL_Rect        rect;
	SDL_Event       event;

	CVideoSocket videoSocket;
	
	//줌인 줌 아웃을 위한 변수
	int rect_w = 0;
	int rect_h = 0;


	
	// We catch any exceptions that might occur below -- see the catch statement for more details.
	try 
	{
	// 여기부터 마이오 초기화
	// First, we create a Hub with our application identifier. Be sure not to use the com.example namespace when
	// publishing your application. The Hub provides access to one or more Myos.
	// 마이오에서 제공하는 어플리케이션과 연결하는 허브 생성
	myo::Hub hub("com.example.hello-myo");

	// 마이오 찾는중 ...
	std::cout << "Attempting to find a Myo..." << std::endl;

	// Next, we attempt to find a Myo to use. If a Myo is already paired in Myo Connect, this will return that Myo
	// immediately.
	// waitForMyo() takes a timeout value in milliseconds. In this case we will try to find a Myo for 10 seconds, and
	// if that fails, the function will return a null pointer.
	// 마이오를 찾는 동안 대기하는 소스코드
	myo::Myo* myo = hub.waitForMyo(10000);

	// If waitForMyo() returned a null pointer, we failed to find a Myo, so exit with an error message.
	// 마이오가 존재하지 않을경우 예외처리
	if (!myo) 
	{
		throw std::runtime_error("Unable to find a Myo!");
	}

	// We've found a Myo.
	std::cout << "Connected to a Myo armband!" << std::endl << std::endl;

	// Next we construct an instance of our DeviceListener, so that we can register it with the Hub.
	// 마이오에서 얻은 데이터를 가공해주는 클래스
	DataCollector collector;

	// Hub::addListener() takes the address of any object whose class inherits from DeviceListener, and will cause
	// Hub::run() to send events to all registered device listeners.
	// 데이터를 지속적으로 받아온다.
	hub.addListener(&collector);

	//---여기까지 마이오 초기화
	
	// SDL 초기화
	InitSDL();

	// Open video file
	// 파일 또는 데이터 스트림을 연다.
	if (avformat_open_input(&pFormatCtx, videoSocket.videoStreamUrl, NULL, NULL) != 0)
	{
		return -1; // Couldn't open file
	}
	// Retrieve stream information
	// 데이터 스트림의 정보를 얻어온다.
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		return -1; // Couldn't find stream information
	}
	// Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, videoSocket.videoStreamUrl, 0);

	// Find the first video stream
	// 비디로 스트림을 찾는과정 - 어떤 형식의 데이터 스트림인지 판별 ( 우리는 h.264로 고정되어있지만...)
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
	// 왜 인지 모르겠지만 그냥 쓰지 않고 복사해서 사용한다.
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
	// 스크린을 생성
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
	// 이미지를 스크린에 그림
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
		// 메인 루프

		// In each iteration of our main loop, we run the Myo event loop for a set number of milliseconds.
		// 데이터를 어느정도 주기로 받아올지 정하는 소스
		// 이 값이 낮아지면 영상을 받아오는데도 딜레이가 걸리기때문에 원하는 fps를 고려해야한다.
		hub.run(1000 / 500);
		// After processing events, we call the print() member function we defined above to print out the values we've
		// obtained from any events that have occurred.
		// 마이오 상태 모니터링 코드
		collector.print();

		// 마이오 루프 여기까지


		// Is this a packet from the video stream?
		if (packet.stream_index == videoStream) 
		{
			// Decode video frame
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

			// Did we get a video frame?
			// 비디오 프레임을 비트맵 이미지로 변환
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

				// 소프트웨어상으로 줌인 줌아웃을 하기위해 영상프레임의 사이즈를 조절
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

		// 마이오의 동작을 체크해서 메시지 송신
		// 좌우 카메라 컨트롤
		if (collector.currentPose == myo::Pose::waveOut)
		{
			SendData(videoSocket.ClientSocket, "right", videoSocket.ToServer);
				//sendto(ClientSocket, right_m, sizeof(right_m), 0, (struct sockaddr*) &ToServer, sizeof(ToServer));
		}	
		if (collector.currentPose == myo::Pose::waveIn)
		{
			SendData(videoSocket.ClientSocket, "left", videoSocket.ToServer);
				//sendto(ClientSocket, left_m, sizeof(left_m), 0, (struct sockaddr*) &ToServer, sizeof(ToServer));
		}
		// 상하 카메라 컨트롤
		if (collector.currentPose == myo::Pose::fingersSpread && collector.pitch_w > 10)
		{
			SendData(videoSocket.ClientSocket, "up", videoSocket.ToServer);
		}
		if (collector.currentPose == myo::Pose::fingersSpread && collector.pitch_w < 6)
		{
			SendData(videoSocket.ClientSocket, "down", videoSocket.ToServer);
		}
		// 마이오의 동작을 체크해서 줌인 줌 아웃
		if (collector.currentPose == myo::Pose::fist && collector.roll_w < 6)
		{
			ZoomOut(rect_w, rect_h, 0);
		}
		if (collector.currentPose == myo::Pose::fist && collector.roll_w > 8)
		{
			ZoomIn(rect_w, rect_h, 300);
		}
		// 키 이벤트를 받는 함수
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
				// 문자열 송신
				SendData(videoSocket.ClientSocket, "left", videoSocket.ToServer);
				break;
			case SDLK_RIGHT:
				// 문자열 송신
				SendData(videoSocket.ClientSocket, "right", videoSocket.ToServer);
				break;
			case SDLK_UP:
				SendData(videoSocket.ClientSocket, "up", videoSocket.ToServer);
				break;
			case SDLK_DOWN:
				SendData(videoSocket.ClientSocket, "down", videoSocket.ToServer);
				break;
			case SDLK_q: // 줌 인
				ZoomIn(rect_w,rect_h,300);			
				break;
			case SDLK_w: // 줌 아웃
				ZoomOut(rect_w, rect_h, 0);								
				break;
			case SDLK_x: // 플그램 종료
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

	// 소켓 닫기
	closesocket(videoSocket.ClientSocket);
	WSACleanup();

	return 0;

	}
	// 개인적으로 exception handling을 이렇게하는걸 좋아하지 않지만...
	// 예제에서 이렇게 사용하였기에 일단 이렇게 두었다.
	catch (const std::exception& e) 
	{
		std::cerr << "Error: " << e.what() << std::endl;
		std::cerr << "Press enter to continue.";
		std::cin.ignore();
		return 1;
	}
	
}

