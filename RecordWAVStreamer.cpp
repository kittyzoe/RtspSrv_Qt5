// Copyright (c) 2011 UPTTOP.COM, All rights reserved.
// WAV Streamer, for WAV File Multicast
// Implementation


#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "RecordWAVStreamer.h"

//  WAV Streamer Implementation

RecordWAVStreamer::RecordWAVStreamer(char *file, float duration, int index)
:Streamer(file, duration), fIndex(index)
{
	fPCMSource = NULL;
	fAudioES = NULL;
	fAudioSource = NULL;

	fBitsPerSample = 0;
	audioFormat = 0;
	fSamplingFrequency = 0;
	fNumChannels = 0;
	fBitsPerSecond = 0;

	// create streaming thread
	{
		Autolock lock(&fMutex);
		pthread_create(&fThread, NULL, streamingThread, this);
		pthread_mutex_lock(&fMResponse);
	}

//	waitResponse();
	pthread_cond_wait(&fCResponse, &fMResponse);
	pthread_mutex_unlock(&fMResponse);

	ReqInit();
}

RecordWAVStreamer::~RecordWAVStreamer()
{
	if (fCurStatus != ST_STOP) {
		ReqStop();
	}
	pthread_join(fThread, NULL);

	if (fAudioSink)
		fAudioSink->stopPlaying();
	
	Medium::close(fAudioRTCP);
	Medium::close(fAudioSink);
	
	Medium::close(fAudioSource);
	
	delete rtpGroupsockAudio;
	delete rtcpGroupsockAudio;
	
	delete rtpPortAudio;
	delete rtcpPortAudio;
	
	Medium::close(rtspServer);
}


void *RecordWAVStreamer::streamingThread(void* p)
{
	return ((RecordWAVStreamer *)p)->streaming();
}


void *RecordWAVStreamer::streaming()
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


RecordWAVStreamer* RecordWAVStreamer::createNew(char * file, float duration, int index)
{
	return new RecordWAVStreamer(file, duration, index);
}	

void RecordWAVStreamer::afterPlaying(void* clientData)
{
	//  reach the end of current media file 
	RecordWAVStreamer *pStream = (RecordWAVStreamer *)clientData;
	pStream->stop_flag = 1;
	pStream->fCurStatus = ST_STOP;

	(*(pStream->funcPlayControlCallBack))(pStream);
}

void RecordWAVStreamer::pause()
{
	if (fAudioSource) {
		fAudioSink->stopPlaying();
		fAudioSource->stopGettingFrames();
	}

	(*funcPlayControlCallBack)(this);
}

int RecordWAVStreamer::seek()
{
	if (fSeekTime < 0 || fSeekTime > fDuration) {
		return -1;
	}
    else {
        fTime = fSeekTime;
    }

	WAVAudioFileSource* wavSource = fPCMSource;

	unsigned seekSampleNumber = (unsigned)(fSeekTime * fSamplingFrequency);
	unsigned seekByteNumber = (seekSampleNumber*fNumChannels*fBitsPerSample)/8;

	unsigned numDurationSamples = (unsigned)(fSeekTime * fSamplingFrequency);
	unsigned numDurationBytes = (numDurationSamples*fNumChannels*fBitsPerSample)/8;

   wavSource->seekToPCMByte(seekByteNumber /* , numDurationBytes*/);  //tootzoe
   //  wavSource->seekToPCMByte(seekByteNumber   , numDurationBytes );  //tootzoe

	return 0;
}

void RecordWAVStreamer::startPlaying()
{
	gettimeofday(&fStartTime, NULL);

	// start playing audio sink
	if (fAudioSource) {
		fAudioSink->startPlaying(*fAudioSource, afterPlaying, this);
	}
	
	//  RTCP 
	double t = fStartTime.tv_sec + fStartTime.tv_usec/1000000.0;

	if (fAudioRTCP) {
		fAudioRTCP->reschedule(t);
	}
		
	fCurStatus = ST_PLAY;

	(*funcPlayControlCallBack)(this);

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

int RecordWAVStreamer::init()
{
	fPCMSource = WAVAudioFileSource::createNew(*fEnv, fFilepath);
	if (fPCMSource == NULL) {
		*fEnv << "Unable to open file \"" << fFilepath
			<< "\" as a WAV audio file source: "
			<< fEnv->getResultMsg() << "\n";
		return -1;
	}

	// Get attributes of the audio source:
	fBitsPerSample = fPCMSource->bitsPerSample();
	audioFormat = fPCMSource->getAudioFormat();
	if (fBitsPerSample != 8 && fBitsPerSample !=  16) {
		*fEnv << "The input file contains " << fBitsPerSample
			  << " bit-per-sample audio, which we don't handle\n";
		return -1;
	}
	fAudioSource = fPCMSource;
	fSamplingFrequency = fPCMSource->samplingFrequency();
	fNumChannels = fPCMSource->numChannels();
	fBitsPerSecond = fSamplingFrequency*fBitsPerSample*fNumChannels;
//	*env << "Audio source parameters:\n\t" << samplingFrequency << " Hz, ";
//	*env << bitsPerSample << " bits-per-sample, ";
//	*env << numChannels << " channels => ";
//	*env << bitsPerSecond << " bits-per-second\n";

	// Add in any filter necessary to transform the data prior to streaming.
	// (This is where any audio compression would get added.)
	char const* mimeType;
	unsigned char payloadFormatCode;
	int rtpPayloadTypeIfDynamic = 96;
	
	if (audioFormat == WA_PCM) {
		if (fBitsPerSample == 16) {
		    // The 16-bit samples in WAV files are in little-endian order.
		    // Add a filter that converts them to network (i.e., big-endian) order:
			fAudioSource = EndianSwap16::createNew(*fEnv, fPCMSource);
			if (fAudioSource == NULL) {
				*fEnv << "Unable to create a little->bit-endian order filter from the PCM audio source: "
					<< fEnv->getResultMsg() << "\n";
				return -1;
			}
			mimeType = "L16";
			if (fSamplingFrequency == 44100 && fNumChannels == 2) {
				payloadFormatCode = 10; // a static RTP payload type
			} else if (fSamplingFrequency == 44100 && fNumChannels == 1) {
				payloadFormatCode = 11; // a static RTP payload type
			} else {
				payloadFormatCode = rtpPayloadTypeIfDynamic;
			}
		} else { // bitsPerSample == 8
			mimeType = "L8";
			payloadFormatCode = rtpPayloadTypeIfDynamic;
		}
	} else if (audioFormat == WA_PCMU) {
		mimeType = "PCMU";
		if (fSamplingFrequency == 8000 && fNumChannels == 1) {
			payloadFormatCode = 0; // a static RTP payload type
		} else {
			payloadFormatCode = rtpPayloadTypeIfDynamic;
		}
    } else if (audioFormat == WA_PCMA) {
		mimeType = "PCMA";
		if (fSamplingFrequency == 8000 && fNumChannels == 1) {
			payloadFormatCode = 8; // a static RTP payload type
		} else {
			payloadFormatCode = rtpPayloadTypeIfDynamic;
		}
    } else if (audioFormat == WA_IMA_ADPCM) {
		mimeType = "DVI4";
		payloadFormatCode = rtpPayloadTypeIfDynamic; // by default; could be changed below:
		// Use a static payload type, if one is defined:
		if (fNumChannels == 1) {
			if (fSamplingFrequency == 8000) {
				payloadFormatCode = 5; // a static RTP payload type
			} else if (fSamplingFrequency == 16000) {
				payloadFormatCode = 6; // a static RTP payload type
			} else if (fSamplingFrequency == 11025) {
				payloadFormatCode = 16; // a static RTP payload type
			} else if (fSamplingFrequency == 22050) {
				payloadFormatCode = 17; // a static RTP payload type
			}
		}
    } else { //unknown format
		return -1;
	}

	//  choose a multicasst address
	struct in_addr destinationAddress;
	destinationAddress.s_addr = chooseRandomIPv4SSMAddress(*fEnv);

	const unsigned short rtpPortNumAudio = 6666 + fIndex * 2;
	const unsigned short rtcpPortNumAudio = rtpPortNumAudio+1;
	const unsigned char ttl = 7; // low, in case routers don't admin scope

	rtpPortAudio  = new Port(rtpPortNumAudio);
	rtcpPortAudio = new Port(rtcpPortNumAudio);

	rtpGroupsockAudio  = new Groupsock(*fEnv, destinationAddress, *rtpPortAudio, ttl);
	rtcpGroupsockAudio = new Groupsock(*fEnv, destinationAddress, *rtcpPortAudio, ttl);

	rtpGroupsockAudio->multicastSendOnly();
	rtcpGroupsockAudio->multicastSendOnly();
	
	// Create an appropriate audio RTP sink (using "SimpleRTPSink")
	// from the RTP 'groupsock':
	fAudioSink = SimpleRTPSink::createNew(*fEnv, rtpGroupsockAudio,
		payloadFormatCode, fSamplingFrequency,
		"audio", mimeType, fNumChannels);

	// Create (and start) a 'RTCP instance' for this RTP sink:
	const unsigned estimatedSessionBandwidth = fBitsPerSecond/1000;
	// in kbps; for RTCP b/w share
	const unsigned maxCNAMElen = 100;
	unsigned char CNAME[maxCNAMElen+1];
	gethostname((char*)CNAME, maxCNAMElen);
	CNAME[maxCNAMElen] = '\0'; // just in case
	fAudioRTCP = RTCPInstance::createNew(*fEnv, rtcpGroupsockAudio,
		estimatedSessionBandwidth, CNAME,
		fAudioSink, NULL /* we're a server */,
		True /* we're a SSM source*/);
	// Note: This starts RTCP running automatically


	// start rtsp server with port 8554	
	int port = 8554;

	// Create and start a RTSP server to serve this stream:
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

	char buf[40];
	int rand_num = rand();

	sprintf(buf, "wavStream%d", rand_num);

	ServerMediaSession* sms
		= ServerMediaSession::createNew(*fEnv, buf, fFilepath,
		"Session streamed by \"RecordWAVStreamer\"", True/*SSM*/);
	sms->addSubsession(PassiveServerMediaSubsession::createNew(*fAudioSink, fAudioRTCP));
	rtspServer->addServerMediaSession(sms);

	fRTSPUrl = rtspServer->rtspURL(sms);
	*fEnv << "Play this stream using the URL \"" << fRTSPUrl << "\"\n";
	
	return 0;
}

	
	
