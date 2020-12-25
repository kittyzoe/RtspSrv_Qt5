// Copyright (c) 2011 UPTTOP.COM, All rights reserved.
// Streamer Class, Base class for Media File RTSP Multicast
// C++ Header

#ifndef _STREAMER_H_
#define _STREAMER_H_


#include <QObject>
#include <QList>
#include <QVariantList>

#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "pthread.h"
//#include "server.h"


//

#define DBG
#ifdef DBG
#define LOG_DBG printf
#else
#define LOG_DBG
#endif

#ifndef playControlCallBackFunc
typedef void (playControlCallBackFunc)(void *streamer);
#endif


enum MediaType {
    FILE_UNSUPPORTED,
    FILE_AVI,
    FILE_MP4,
    FILE_MP3,
    FILE_MPG,
    FILE_AAC,
    FILE_WAV,
    FILE_TS,
    FILE_VOB,
};

struct MediaFile {
    char * filepath;
    int    isSupport;
    float  duration;
    enum MediaType type;
};


enum StreamStatus {
    ST_NONE,
    ST_INIT,
    ST_PLAY,
    ST_PAUSE,
    ST_STOP,
    ST_SEEK
};



//  Auto lock
class Autolock {
public:
	inline Autolock(pthread_mutex_t *mutex) : mLock(mutex)  { pthread_mutex_lock(mLock); }
	inline ~Autolock() { pthread_mutex_unlock (mLock); }
private:
	pthread_mutex_t* mLock;
};



//  Streamer base class 
class Streamer    {



public:
	virtual ~Streamer();

	//  send start/stop/pause request in control thread
	virtual Boolean ReqStart();
	virtual Boolean ReqStop();
	virtual Boolean ReqPause();

	//  seek to position
	virtual Boolean ReqSeek(float pos);
	
	//  get working thread's current status
	StreamStatus getCurStatus() { return fCurStatus; }

	//  get duration time 
	float getDuration() { return fDuration; }

	//  Report current position of Time(in seconds)
	float getPosition();

	//  Get RTSP URL 
	char * getRTSPUrl() { return fRTSPUrl; }

	char * getMediaFileUrl() { return fFilepath; }

	//  Get A/V source
	FramedSource * getVideoSource() { return fVideoSource; }
	FramedSource * getAudioSource() { return fAudioSource; }

	void setCallBackFunc(playControlCallBackFunc* callback = 0);

protected:
	Streamer(char * filepath, float duration);

     bool putSomdData(QVariantList);
	
protected:
	//  init streamer
	virtual int init() = 0;
	
	//  start playing
	virtual void startPlaying() = 0; 
	
	//  pause 
	virtual void pause() = 0;
	
	//  seek to time 
	virtual int seek() = 0;

	Boolean ReqInit();
	
	//  announce a response to other thread 
	void announceResponse();
protected:
	//  LIVE555 related objects
	UsageEnvironment* fEnv;
	TaskScheduler* fScheduler;
	RTSPServer* rtspServer;
	RTPSink* fVideoSink;
	RTPSink* fAudioSink;
	
	FramedSource* fAudioES;
	FramedSource* fVideoES;
	FramedSource* fVideoSource;
	FramedSource* fAudioSource;

	RTCPInstance* fVideoRTCP;
	RTCPInstance* fAudioRTCP;

	Port *rtpPortAudio;
	Port *rtcpPortAudio;
	Port *rtpPortVideo;
	Port *rtcpPortVideo;

	Groupsock *rtpGroupsockAudio;
	Groupsock *rtcpGroupsockAudio;
	Groupsock *rtpGroupsockVideo;
	Groupsock *rtcpGroupsockVideo;


	char * fFilepath;			//  target media file's local path
	char * fRTSPUrl;            //  RTSP multicast url
	StreamStatus fCurStatus;    //  current status 
	StreamStatus fReqStatus;    //  the status we want change to
	char stop_flag;

	//  thread operation related
	pthread_t fThread;
	pthread_mutex_t fMutex;
	pthread_cond_t fCondition;
	pthread_mutex_t fMResponse;
	pthread_cond_t fCResponse;

	//  for current pos use
	struct timeval fStartTime;
	float fTime;

	//  file duration
	float fDuration;

	//  time to seek to
	float fSeekTime;

	playControlCallBackFunc* funcPlayControlCallBack;
};

#endif

