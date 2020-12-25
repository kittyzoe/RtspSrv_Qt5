// Copyright (c) 2011 UPTTOP.COM, All rights reserved.
// Class Streamer, for Media File Multicast
// C++ Header

#include <QObject>
#include <QList>
#include <QVariantList>

#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "AVIVideoDiscreteFramer.hh"

#include "pthread.h"

#include "AVIDemux.hh"
#include "Streamer.h"





//  Generic Streamer
class GenericStreamer : public Streamer   {

public:
	static GenericStreamer* createNew(char *file, float duration, int index);
    static GenericStreamer* createNew (QList<QVariantList> valLs, int index);
	~GenericStreamer();
	
	static void afterPlaying(void* clientData);
	
	virtual void startPlaying();

	virtual void pause();

	virtual int seek();

	static void *streamingThread(void*);
    void myAfterPlaying();
protected:
	GenericStreamer(char *file, float duration, int index);
    GenericStreamer(QList<QVariantList> valLs, int index);
	virtual int init();
	
private:
	void *streaming();
	static void runOnce(void* clientData);

    int continuePlay();

    unsigned hzfindex ;

	
private:
    friend class TOOTDumyClass;
	AVIDemux* fDemux;
	int fIndex;

    QList<QVariantList> m_playLs;
};

