// Copyright (c) 2010-2011 UPTTOP.COM, All rights reserved.
// Class Streamer, for Media File Multicast
// Implementation

#include <QDebug>

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "Streamer.h"
#include "AVIVideoDiscreteFramer.hh"

Streamer::Streamer(char * filepath, float duration)
{
	fFilepath = strDup(filepath);
	fDuration = duration;
		
	//  create Scheduler and Environment
	fScheduler = BasicTaskScheduler::createNew();
	fEnv = BasicUsageEnvironment::createNew(*fScheduler);

	/* Initialize mutex and condition variable objects */
	pthread_mutex_init(&fMutex, NULL);
	pthread_cond_init (&fCondition, NULL);
	pthread_mutex_init(&fMResponse, NULL);
	pthread_cond_init (&fCResponse, NULL);
	

	//  set internal Buffer size
	OutPacketBuffer::maxSize = 1000000;
	fCurStatus = ST_NONE;

	rtspServer = NULL;

	rtpPortAudio = rtcpPortAudio = rtpPortVideo = rtcpPortVideo = NULL;

	rtpGroupsockAudio  = NULL;
	rtcpGroupsockAudio = NULL;
	rtpGroupsockVideo  = NULL;
	rtcpGroupsockVideo = NULL;
	stop_flag = 0;

	fAudioSink = fVideoSink = NULL;
	fAudioRTCP = fVideoRTCP = NULL;

	fStartTime.tv_sec = fStartTime.tv_usec = -1;

	fTime = 0.0;
	fSeekTime = -1.0;
	fRTSPUrl = NULL;
}


Streamer::~Streamer()
{
	if (fCurStatus != ST_NONE) {
		ReqStop();
	}
	
	delete[] fFilepath;
	delete[] fRTSPUrl;

	delete fScheduler;
	fEnv->reclaim();
	
	//  Clean up
	pthread_mutex_destroy(&fMutex);
	pthread_cond_destroy(&fCondition);
	pthread_mutex_destroy(&fMResponse);
	pthread_cond_destroy (&fCResponse);
    printf("tootzoe %s\n" , __func__);
}

//  Report current position of Time(in seconds)
float Streamer::getPosition()
{
	struct timeval now, duration;

	if (fStartTime.tv_sec == -1 && fStartTime.tv_usec == -1) {
		return 0.0;
	}

	gettimeofday(&now, NULL);
	duration.tv_sec = now.tv_sec - fStartTime.tv_sec;
	duration.tv_usec = now.tv_usec - fStartTime.tv_usec;
	if (duration.tv_usec < 0) {
		duration.tv_usec += 1000000L;
		duration.tv_sec --;
	}
	
	if (fCurStatus != ST_PLAY) {
		return fTime;
	}
	else {
		return fTime + duration.tv_sec + duration.tv_usec / 1000000.0;
	}
}

Boolean Streamer::ReqStart()
{


	if (fCurStatus == ST_NONE || fCurStatus == ST_PLAY || fCurStatus == ST_STOP) {
		//  invalid request
		return False;
	}

	{
		stop_flag = 1;
		Autolock lock(&fMutex);
		fReqStatus = ST_PLAY;
		pthread_cond_signal(&fCondition);
		pthread_mutex_lock(&fMResponse);
	}
	
	//waitResponse();
	pthread_cond_wait(&fCResponse, &fMResponse);
	pthread_mutex_unlock(&fMResponse);
	return fReqStatus == fCurStatus;
}

Boolean Streamer::ReqStop()
{
	if (fCurStatus == ST_STOP || fCurStatus == ST_NONE) {
		return True;
	}

	{
		stop_flag = 1;
		Autolock lock(&fMutex);
		fReqStatus = ST_STOP;
		pthread_cond_signal(&fCondition);
		pthread_mutex_lock(&fMResponse);
	}
	
	pthread_cond_wait(&fCResponse, &fMResponse);
	pthread_mutex_unlock(&fMResponse);
	return fReqStatus == fCurStatus;
}

Boolean Streamer::ReqInit()
{

	if (fCurStatus != ST_INIT) {
		return False;
	}

	{
		stop_flag = 1;
		Autolock lock(&fMutex);
		fReqStatus = ST_INIT;
		pthread_cond_signal(&fCondition);
		pthread_mutex_lock(&fMResponse);
	}
	
	pthread_cond_wait(&fCResponse, &fMResponse);
	pthread_mutex_unlock(&fMResponse);
	return fReqStatus == fCurStatus;
}

Boolean Streamer::ReqPause()
{
	if (fCurStatus == ST_NONE || fCurStatus == ST_PAUSE || fCurStatus == ST_STOP) {
		return False;
	}

	{
		stop_flag = 1;
		Autolock lock(&fMutex);
		fReqStatus = ST_PAUSE;
		pthread_cond_signal(&fCondition);
		pthread_mutex_lock(&fMResponse);
	}

	//waitResponse();
	pthread_cond_wait(&fCResponse, &fMResponse);
	pthread_mutex_unlock(&fMResponse);
	return fReqStatus == fCurStatus;
}


Boolean Streamer::ReqSeek(float time)
{
	if (fCurStatus == ST_STOP) {
		return False;
	}

	if (time < 0 || time > fDuration) {
		return False;
	}

	{
		fSeekTime = time;
		stop_flag = 1;
		Autolock lock(&fMutex);
		fReqStatus = ST_SEEK;
		pthread_cond_signal(&fCondition);
		pthread_mutex_lock(&fMResponse);
	}

	pthread_cond_wait(&fCResponse, &fMResponse);
	pthread_mutex_unlock(&fMResponse);

	return True;
}

void Streamer::announceResponse()
{
    //printf("anounceResponse \n");
	Autolock lock(&fMResponse);
	pthread_cond_signal(&fCResponse);
}

void Streamer::setCallBackFunc(playControlCallBackFunc* callback)
{
	funcPlayControlCallBack	= callback;
}


bool Streamer::putSomdData(QVariantList tmpVarLs)
{
    if(tmpVarLs.isEmpty()){

        return false;
    }

    QVariantMap tmpMap = tmpVarLs.at(0).toMap();

    delete[] fFilepath;

    fFilepath = strDup( tmpMap.value("pathName").toString().toUtf8().constData());
    fDuration =  tmpMap.value("duration").toDouble();

   // qDebug() << "current fFilepath for playing = " << fFilepath;

    return true;
}






