// Copyright (c) 2011 UPTTOP.COM, All rights reserved.
// Class Streamer, for Media File Multicast
// C++ Header


#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "WAVAudioFileSource.hh"
#include "Streamer.h"
#include "pthread.h"



//  AVI Streamer
class RecordWAVStreamer : public Streamer {
public:
	static RecordWAVStreamer* createNew(char *file, float duration, int index = 0);
	~RecordWAVStreamer();
	
	static void afterPlaying(void* clientData);
	
	virtual void startPlaying();

	virtual void pause();

	virtual int seek();

	static void *streamingThread(void*);
	
protected:
	RecordWAVStreamer(char *file, float duration, int index);
	virtual int init();
	
private:
	void *streaming();

private:
	WAVAudioFileSource* fPCMSource;
	unsigned char fBitsPerSample;
	unsigned char audioFormat ;
	unsigned fSamplingFrequency;
	unsigned char fNumChannels;
	unsigned fBitsPerSecond;

	int fIndex;
};

