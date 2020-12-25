// Copyright (c) 2011 UPTTOP.COM, All rights reserved.
// Generic Streamer, for AVI/MPG/MP3/VOB/TS/AAC File's RTSP Multicast
// Implementation


#include <QtCore>

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "GenericStreamer.h"
#include "AACAudioStreamFramer.hh"

//  Generic Streamer Implementation

GenericStreamer::GenericStreamer(char *file, float duration, int index)
:Streamer(file, duration),fIndex(index)
{
	fDemux = NULL;
	fAudioES = fVideoES = NULL;
	fVideoSource = fAudioSource = NULL;

	// create streaming thread
	{
		Autolock lock(&fMutex);
		pthread_create(&fThread, NULL, streamingThread, this);
		pthread_mutex_lock(&fMResponse);
	}

//	waitResponse
	pthread_cond_wait(&fCResponse, &fMResponse);
	pthread_mutex_unlock(&fMResponse);

	ReqInit();
}

GenericStreamer::GenericStreamer(QList<QVariantList> valLs, int index)
:Streamer("", 1),fIndex(index)
,m_playLs(valLs)
{
    fDemux = NULL;
    fAudioES = fVideoES = NULL;
    fVideoSource = fAudioSource = NULL;
    hzfindex = 0;


    QVariantList tmpVarLs = m_playLs.at(0);
    if(! putSomdData(tmpVarLs) ) return;


    // create streaming thread
    {
        Autolock lock(&fMutex);
        pthread_create(&fThread, NULL, streamingThread, this);
        pthread_mutex_lock(&fMResponse);
    }

//	waitResponse
    pthread_cond_wait(&fCResponse, &fMResponse);
    pthread_mutex_unlock(&fMResponse);

    ReqInit();
}

GenericStreamer::~GenericStreamer()
{
	if (fCurStatus != ST_STOP) {
		ReqStop();
	}
	pthread_join(fThread, NULL);

	if (fAudioSink)
		fAudioSink->stopPlaying();
	
	if (fVideoSink)
		fVideoSink->stopPlaying();
	
	Medium::close(fVideoRTCP);
	Medium::close(fAudioRTCP);
	Medium::close(fAudioSink);
	Medium::close(fVideoSink);
	
	Medium::close(fVideoSource);
	Medium::close(fAudioSource);
	
	delete rtpGroupsockAudio;
	delete rtcpGroupsockAudio;
	delete rtpGroupsockVideo;
	delete rtcpGroupsockVideo;
	
	delete rtpPortAudio;
	delete rtcpPortAudio;
	delete rtpPortVideo;
	delete rtcpPortVideo;
	
	Medium::close(rtspServer);
	Medium::close(fDemux);
}



void * GenericStreamer::streamingThread(void* p)
{
	return ((GenericStreamer *)p)->streaming();
}


void * GenericStreamer::streaming()
{
	pthread_mutex_lock(&fMutex);
	if (init() == 0) {
		fCurStatus = ST_INIT;
		announceResponse();
	} else {
		//  init failed
		announceResponse();
		pthread_mutex_unlock(&fMutex);
		return NULL;
	}

	
	//  Start/Pause/Stop loop
	LOG_DBG("[DEBUG] streaming Start/Pause/Stop/Seek loop \n");

	while (1)
	{	
		//  wait for main thread's control message 
		pthread_cond_wait(&fCondition, &fMutex);

		LOG_DBG("[DEBUG] fReqStatus = %d, fCurStatus = %d \n", fReqStatus, fCurStatus);
		if (fReqStatus == ST_INIT && fCurStatus == ST_INIT) {
			stop_flag = 0;	
			announceResponse();
			fEnv->taskScheduler().doEventLoop(&stop_flag); // wait a request
		}
		if (fReqStatus == ST_PLAY && fCurStatus == ST_INIT) {
			//  Start Playing
			stop_flag = 0;
			startPlaying();
		}
		if (fReqStatus == ST_PLAY && fCurStatus == ST_PAUSE) {
			//  Continue Playing
			stop_flag = 0;
			startPlaying();
		}	
		else if (fReqStatus == ST_PAUSE && fCurStatus == ST_PLAY) {
			//  Pause Playing
			fCurStatus = ST_PAUSE;
			pause();
			//  Start msg loop:
			stop_flag = 0;
			announceResponse();
			fEnv->taskScheduler().doEventLoop(&stop_flag); // does not return
		} 
		else if (fReqStatus == ST_STOP) {
			//  Stop Playing
			fCurStatus = ST_STOP;
		}
		else if (fReqStatus == ST_SEEK) {
			if (fCurStatus == ST_PLAY) {
				pause();
				seek();
				stop_flag = 0;
				startPlaying();
			} 
			else if (fCurStatus == ST_PAUSE){
				seek();
				//  Start msg loop:
				stop_flag = 0;
				announceResponse();
				fEnv->taskScheduler().doEventLoop(&stop_flag); // does not return
			}
			else if (fCurStatus == ST_INIT) {
				seek();
				announceResponse();
			}
			else {
				announceResponse();
			}
		}
		else {
			//  invalid request
			announceResponse();
		}

		//  if it's stopped, jumps out the loop
		if (fCurStatus == ST_STOP) {
			pthread_mutex_unlock(&fMutex);
			announceResponse();

			(*funcPlayControlCallBack)(this);
			break;
		}
	}
	
	//  Handle Stop
	return NULL;
}


GenericStreamer* GenericStreamer::createNew(char * file, float duration, int index)
{
	return new GenericStreamer(file, duration, index);
}	

GenericStreamer* GenericStreamer::createNew ( QList<QVariantList> valLs, int index)
{
    return new GenericStreamer(valLs, index);
}

void GenericStreamer::afterPlaying(void* clientData)
{
	//  reach the end of current media file 
	GenericStreamer *pStream = (GenericStreamer *)clientData;
	FramedSource * audioSource = pStream->getAudioSource();
	FramedSource * videoSource = pStream->getVideoSource();
	
	// wait until its sinks end playing
	if ((audioSource != NULL && audioSource->isCurrentlyAwaitingData()) || 
		(videoSource != NULL && videoSource->isCurrentlyAwaitingData())) {
    	return;
  	}
	
#if toot
	pStream->stop_flag = 1;
	pStream->fCurStatus = ST_STOP;
#endif


	(*(pStream->funcPlayControlCallBack))(pStream);

  //  qDebug() << __func__;
    pStream->myAfterPlaying();

}


void GenericStreamer::myAfterPlaying ()
{
   //
        fAudioSink->stopPlaying();
        fVideoSink->stopPlaying();

        Medium::close(fVideoSource);
        Medium::close(fAudioSource);


        Medium::close(fDemux);

        fVideoSource = fAudioSource = NULL;



        hzfindex ++;
        if(hzfindex >= m_playLs.size() ) hzfindex = 0;

        qDebug() << "  list size " << m_playLs.size()
                 << "  index " << hzfindex
                 << "  current playing " << fFilepath;


        QVariantList tmpVarLs = m_playLs.at(hzfindex);


        if(! putSomdData(tmpVarLs) ) {

            qDebug() <<   " update fFilepath failed.... " << fFilepath;

            return;
        }


        continuePlay();

}

int GenericStreamer::continuePlay()
{

     //qDebug() << __func__ << " currMediaFile " << fFilepath;



    fDemux = AVIDemux::createNew(*fEnv, fFilepath);
    if (fDemux->prepare() !=0 ) {
         return -1;
    }


    if (fDemux->getAudioIndex() >= 0)
        fAudioES =(FramedSource*) (fDemux->newAudioStream());

    if (fDemux->getVideoIndex() >= 0)
        fVideoES = (FramedSource*)  (fDemux->newVideoStream());

    // Create a framer for each Elementary Stream:
    if (fAudioES) {
        uint32_t codec_id = fDemux->getCodecID(fDemux->getAudioIndex());
        if (codec_id ==AV_CODEC_ID_MP3 || codec_id ==AV_CODEC_ID_MP2) {
            fAudioSource = MPEG1or2AudioStreamFramer::createNew(*fEnv, fAudioES);
        }
        else if (codec_id ==AV_CODEC_ID_AC3) {
            fAudioSource = AC3AudioStreamFramer::createNew(*fEnv, fAudioES);
        }
        else if (codec_id ==AV_CODEC_ID_AAC) {
            fAudioSource = AACAudioStreamFramer::createNew(*fEnv, fAudioES);
        }
    }

    if (fVideoES) {
        uint32_t codec_id = fDemux->getCodecID(fDemux->getVideoIndex());

        if (codec_id ==AV_CODEC_ID_MPEG1VIDEO || codec_id ==AV_CODEC_ID_MPEG2VIDEO) {
            fVideoSource = MPEG1or2VideoStreamFramer::createNew(*fEnv, fVideoES);
        }
        else if (codec_id ==AV_CODEC_ID_MPEG4) {
            fVideoSource = AVIVideoDiscreteFramer::createNew(*fEnv, fVideoES);
        }
        else if (codec_id ==AV_CODEC_ID_H264)
        {
            fVideoSource = H264VideoStreamFramer::createNew(*fEnv, fVideoES);
        }
    }

    if (fVideoSource) {
        fVideoSink->startPlaying(*fVideoSource, afterPlaying, this);
    }

    if (fAudioSource) {
        fAudioSink->startPlaying(*fAudioSource, afterPlaying, this);
    }


}


void GenericStreamer::pause()
{
	if (fVideoSource) {
		fVideoSink->stopPlaying();
	}

	if (fAudioSource) {
		fAudioSink->stopPlaying();
	}

	(*funcPlayControlCallBack)(this);
}

int GenericStreamer::seek()
{
	if(fDemux->seekStream(fSeekTime) != -1)
	{
		fTime = fSeekTime;
	}
	else
	{
		return -1;
	}

	return 0;
}

void GenericStreamer::startPlaying()
{
	if (fTime != 0.0) {
		fSeekTime = fTime;
		seek();
	}
	
	fCurStatus = ST_PLAY;

	(*funcPlayControlCallBack)(this);

	// Finally, start playing each sink.
	if (fAudioES) {
		uint32_t codec_id = fDemux->getCodecID(fDemux->getAudioIndex());
        if (codec_id ==AV_CODEC_ID_MP3 || codec_id ==AV_CODEC_ID_MP2) {
			MPEG1or2AudioStreamFramer * source = (MPEG1or2AudioStreamFramer *)fAudioSource;
			source->flushInput();
		}
        else if (codec_id ==AV_CODEC_ID_AC3) {
			AC3AudioStreamFramer * source = (AC3AudioStreamFramer *)fAudioSource;
			source->flushInput();
		}
        else if (codec_id ==AV_CODEC_ID_AAC) {
			AACAudioStreamFramer * source = (AACAudioStreamFramer *)fAudioSource;
			source->flushInput();
		}
	}

	if (fVideoES) {
		uint32_t codec_id = fDemux->getCodecID(fDemux->getVideoIndex());

        if (codec_id ==AV_CODEC_ID_MPEG1VIDEO || codec_id ==AV_CODEC_ID_MPEG2VIDEO)
		{
            MPEG1or2VideoStreamFramer * source = (MPEG1or2VideoStreamFramer *)fVideoSource;
            source->flushInput();
            double time = source->fPresentationTimeBase.tv_sec + (double)source->fPresentationTimeBase.tv_usec / 1000000.0;
            time += fDemux->fVideo_diff;

            //printf("A/V Diff is %f \n", fDemux->fVideo_diff);
            source->fPresentationTimeBase.tv_sec = (int)time;
            source->fPresentationTimeBase.tv_usec = (time - (double)source->fPresentationTimeBase.tv_sec) * 1000000L;
        }
        else if (codec_id ==AV_CODEC_ID_MPEG4)
		{
			AVIVideoDiscreteFramer * source = (AVIVideoDiscreteFramer *)fVideoSource;
			source->flushInput();
		}
        else if(codec_id ==AV_CODEC_ID_H264)
        {
            H264VideoStreamFramer * source = (H264VideoStreamFramer *)fVideoSource;
			source->flushInput();
		}
	}
	
	if (fVideoSource) {
       fVideoSink->startPlaying(*fVideoSource, afterPlaying, this);
	}

	if (fAudioSource) {
        // fAudioSink->startPlaying(*fAudioSource, afterPlaying, this);
	}
	
	gettimeofday(&fStartTime, NULL);
	
	//  RTCP 
	double t = fStartTime.tv_sec + fStartTime.tv_usec / 1000000.0;
	if (fVideoRTCP) {
		fVideoRTCP->reschedule(t);
	}
	if (fAudioRTCP) {
		fAudioRTCP->reschedule(t);
	}

	announceResponse();
	
	// Start the streaming:
	fEnv->taskScheduler().doEventLoop(&stop_flag); // does not return

	struct timeval now, duration;
	gettimeofday(&now, NULL);
	duration.tv_sec = now.tv_sec - fStartTime.tv_sec;
	duration.tv_usec = now.tv_usec - fStartTime.tv_usec;
	if (duration.tv_usec < 0) {
		duration.tv_usec += 1000000L;
		duration.tv_sec --;
	}

	fTime += duration.tv_sec + duration.tv_usec / 1000000.0;
};

//static int genericStream_num = 1;


static netAddressBits tootChooseFixedIPv4SSMAddress(UsageEnvironment& env , int inde) {
  // First, a hack to ensure that our random number generator is seeded:
  (void) ourIPAddress(env);

  // Choose a random address in the range [232.0.1.0, 232.255.255.255)
  // i.e., [0xE8000100, 0xE8FFFFFF)
  netAddressBits const first = 0xE8000100, lastPlus1 = 0xE8FFFFFF;
  netAddressBits const range = lastPlus1 - first;

  static netAddressBits hzf = 0x2000100 + inde * 4;
  return htonl(first +  (( hzf++)  % range) );
}

int GenericStreamer::init()
{

	//  choose a multicasst address
	struct in_addr destinationAddress;    

#if 1
    destinationAddress.s_addr = chooseRandomIPv4SSMAddress(*fEnv);
#else
    destinationAddress.s_addr = tootChooseFixedIPv4SSMAddress(*fEnv, fIndex);
#endif

	const unsigned short rtpPortNumAudio = 6666 + fIndex * 2;
	const unsigned short rtcpPortNumAudio = rtpPortNumAudio+1;
	const unsigned short rtpPortNumVideo = 8888 + fIndex * 2 ;
	const unsigned short rtcpPortNumVideo = rtpPortNumVideo+1;
	const unsigned char ttl = 7; // low, in case routers don't admin scope

	rtpPortAudio  = new Port(rtpPortNumAudio);
	rtcpPortAudio = new Port(rtcpPortNumAudio);
	rtpPortVideo  = new Port(rtpPortNumVideo);
	rtcpPortVideo = new Port(rtcpPortNumVideo);



    rtpGroupsockAudio  = new Groupsock(*fEnv, destinationAddress, *rtpPortAudio, ttl);
    rtcpGroupsockAudio = new Groupsock(*fEnv, destinationAddress, *rtcpPortAudio, ttl);
    rtpGroupsockVideo  = new Groupsock(*fEnv, destinationAddress, *rtpPortVideo, ttl);
    rtcpGroupsockVideo = new Groupsock(*fEnv, destinationAddress, *rtcpPortVideo, ttl);



	rtpGroupsockAudio->multicastSendOnly();
	rtcpGroupsockAudio->multicastSendOnly();
	rtpGroupsockVideo->multicastSendOnly();
	rtcpGroupsockVideo->multicastSendOnly();
	


	
	fDemux = AVIDemux::createNew(*fEnv, fFilepath);
	if (fDemux->prepare() !=0 ) {
		return -1;
	}
	
	const unsigned maxCNAMElen = 100;
	unsigned char CNAME[maxCNAMElen+1];
	gethostname((char*)CNAME, maxCNAMElen);
	CNAME[maxCNAMElen] = '\0'; // just in case
		
	// Create a 'MPEG Audio RTP' sink from the RTP 'groupsock':
	char index = fDemux->getAudioIndex();
	if (index >= 0) {
		uint32_t codec_id = fDemux->getCodecID(index);
        if (codec_id ==AV_CODEC_ID_MP3 || codec_id ==AV_CODEC_ID_MP2) {
			fAudioSink = MPEG1or2AudioRTPSink::createNew(*fEnv, rtpGroupsockAudio);
		}
        else if (codec_id ==AV_CODEC_ID_AC3) {
			fAudioSink = AC3AudioRTPSink::createNew(*fEnv, rtpGroupsockAudio, 97, fDemux->getSamplingRate());
		}
        else if (codec_id ==AV_CODEC_ID_AAC){
			uint8_t *data;
			int size;
			fDemux->getConfigData(index, &data, &size);
			char* fmtp = new char[size * 2 + 1];
			char* ptr = fmtp;
			for (int i = 0; i < size; ++i) {
				sprintf(ptr, "%02X", data[i]);
				ptr += 2;
			}

			fAudioSink = MPEG4GenericRTPSink::createNew(*fEnv, 
												  rtpGroupsockAudio, 97, 
												  fDemux->getSamplingRate(),
												  "audio", "AAC-hbr",
												  fmtp,
												  fDemux->getNumChannels());
			delete[] fmtp;
		}
		else {
			*fEnv << "Unsupported Audio Type, codec_id " <<codec_id<< ", Ref to ffmpeg \n"; 
			return -1;
		}

		fAudioRTCP = RTCPInstance::createNew(*fEnv, rtcpGroupsockAudio,
						fDemux->getBitrate(index), CNAME,
						fAudioSink, NULL /* we're a server */, True);
	}
		
	index = fDemux->getVideoIndex();
	if (index >= 0)
	{
		uint32_t codec_id = fDemux->getCodecID(index);
        if (codec_id ==AV_CODEC_ID_MPEG4)
		{
			// Create a 'MPEG-4 Video RTP' sink from the RTP 'groupsock':
			fVideoSink = MPEG4ESVideoRTPSink::createNew(*fEnv, rtpGroupsockVideo, 96);
		}
        else if (codec_id ==AV_CODEC_ID_MPEG1VIDEO || codec_id ==AV_CODEC_ID_MPEG2VIDEO)
		{
			// Create a 'MPEG-2 Video RTP' sink from the RTP 'groupsock':
			fVideoSink = MPEG1or2VideoRTPSink::createNew(*fEnv, rtpGroupsockVideo);
		}
        else if(codec_id ==AV_CODEC_ID_H264)
		{
            OutPacketBuffer::maxSize = 1000000;
			fVideoSink = H264VideoRTPSink::createNew(*fEnv, rtpGroupsockVideo, 96);
		}


//        qDebug() << " 文件名 " << fFilepath  << "  原比特率 = " << fDemux->getBitrate(index)
//                     << "  新比特率 = " << qMax(fDemux->getBitrate(index) , (unsigned)1800)
//                    ;

		// Create (and start) a 'RTCP instance' for this RTP sink:
		fVideoRTCP = RTCPInstance::createNew(*fEnv, rtcpGroupsockVideo,
                            fDemux->getBitrate(index) , CNAME,
							fVideoSink, NULL /* we're a server */,
							True /* we're a SSM source */);
	}
	
    int port = 8554;   // for pc
    //int port =  554;
	// start rtsp server with port 8554

	while (1) {        
		rtspServer = RTSPServer::createNew(*fEnv, port);
		if (rtspServer == NULL) {
			*fEnv << "Failed to create RTSP server: " << fEnv->getResultMsg() << " Try another port\n";
			port ++;
		}
		else {
			break;
		}
	}
#if toot
	char buf[40];
//	int rand_num = rand();
	int rand_num = genericStream_num;

	genericStream_num = genericStream_num + 1;

    sprintf(buf, "genericStream%d", rand_num);
#endif
	ServerMediaSession* sms
        = ServerMediaSession::createNew(*fEnv, /*buf  */ "vga", NULL,
				"Session streamed by \"rtspserver\"",
				True /*SSM*/);

	if (fAudioSink) {
		sms->addSubsession(PassiveServerMediaSubsession::createNew(*fAudioSink, fAudioRTCP));
	}

	if (fVideoSink) {
		sms->addSubsession(PassiveServerMediaSubsession::createNew(*fVideoSink, fVideoRTCP));
	}
  
	rtspServer->addServerMediaSession(sms);

	{
        if (fDemux->getAudioIndex() >= 0)
            fAudioES =(FramedSource*) (fDemux->newAudioStream());
		
        if (fDemux->getVideoIndex() >= 0)
            fVideoES = (FramedSource*)  (fDemux->newVideoStream());

		// Create a framer for each Elementary Stream:
		if (fAudioES) {
			uint32_t codec_id = fDemux->getCodecID(fDemux->getAudioIndex());
            if (codec_id ==AV_CODEC_ID_MP3 || codec_id ==AV_CODEC_ID_MP2) {
				fAudioSource = MPEG1or2AudioStreamFramer::createNew(*fEnv, fAudioES);
			}
            else if (codec_id ==AV_CODEC_ID_AC3) {
				fAudioSource = AC3AudioStreamFramer::createNew(*fEnv, fAudioES);
			}
            else if (codec_id ==AV_CODEC_ID_AAC) {
				fAudioSource = AACAudioStreamFramer::createNew(*fEnv, fAudioES);
			}
		}

		if (fVideoES) {
			uint32_t codec_id = fDemux->getCodecID(fDemux->getVideoIndex());

            if (codec_id ==AV_CODEC_ID_MPEG1VIDEO || codec_id ==AV_CODEC_ID_MPEG2VIDEO) {
				fVideoSource = MPEG1or2VideoStreamFramer::createNew(*fEnv, fVideoES);
			}
            else if (codec_id ==AV_CODEC_ID_MPEG4) {
				fVideoSource = AVIVideoDiscreteFramer::createNew(*fEnv, fVideoES);
			}
            else if (codec_id ==AV_CODEC_ID_H264)
			{
                fVideoSource = H264VideoStreamFramer::createNew(*fEnv, fVideoES);
			}
		}
		
		if (fVideoSource) {
			fVideoSink->startPlaying(*fVideoSource, afterPlaying, this);
		}

		if (fAudioSource) {
            fAudioSink->startPlaying(*fAudioSource, afterPlaying, this);
		}


		fEnv->taskScheduler().scheduleDelayedTask(0, (TaskFunc*)runOnce, (void*)this);
		stop_flag = 0;
		fEnv->taskScheduler().doEventLoop(&stop_flag);
			
		char * sdp = sms->generateSDPDescription();
		delete[] sdp;

		if (fVideoSource) {
			fVideoSink->stopPlaying();
		}

		if (fAudioSource) {
			fAudioSink->stopPlaying();
		}

		fSeekTime = 0.0;
		seek();
	}
	 

	fRTSPUrl = rtspServer->rtspURL(sms);
    qDebug() << "Play this stream using the URL \"" << fRTSPUrl << "\"\n";

	return 0;
}

void GenericStreamer::runOnce(void* clientData)
{

	GenericStreamer *p = (GenericStreamer *)clientData;
	p->stop_flag = 1;
}
