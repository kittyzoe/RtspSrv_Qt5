// Copyright (c) 2011 UPTTOP.COM, All rights reserved.
// Class Streamer, for Media File Multicast
// C++ Header
#include <QObject>
#include <QList>
#include <QVariantList>

#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "WAVAudioFileSource.hh"
#include "Streamer.h"
#include "pthread.h"



//  AVI Streamer
class WAVStreamer : public Streamer {
public:
	static WAVStreamer* createNew(char *file, float duration, int index = 0);
    static WAVStreamer* createNew(QList<QVariantList> valLs, int index = 0);

	~WAVStreamer();

	static void afterPlaying(void* clientData);
	
	virtual void startPlaying();

	virtual void pause();

	virtual int seek();

	static void *streamingThread(void*);

        void myAfterPlaying();
	
protected:
	WAVStreamer(char *file, float duration, int index);
    WAVStreamer(QList<QVariantList> valLs, int index);
	virtual int init();
	
private:
	void *streaming();

    int continuePlay();

    int hzfindex  ;

private:
	WAVAudioFileSource* fPCMSource;
	unsigned char fBitsPerSample;
	unsigned char audioFormat ;
	unsigned fSamplingFrequency;
	unsigned char fNumChannels;
	unsigned fBitsPerSecond;

	int fIndex;

    QList<QVariantList> m_playLs;
};

