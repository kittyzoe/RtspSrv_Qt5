// Copyright (c) 2011 UPTTOP.COM, All rights reserved.
// RTSP MultiCast Server, support AVI, MP3, MPG, AAC, WAV, VOB, TS media files
// header file

#ifndef _SERVER_H_
#define _SERVER_H_

#ifdef   _cplusplus
extern   "C"
{
#endif   /* _cplusplus */

enum MediaType {
	FILE_UNSUPPORTED,
	FILE_AVI,
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


#ifndef StreamStatus
enum StreamStatus {
	ST_NONE,
	ST_INIT,
	ST_PLAY,
	ST_PAUSE,
	ST_STOP,
	ST_SEEK
};
#endif

#ifndef playControlCallBackFunc
typedef void (playControlCallBackFunc)(void *streamer);
#endif

#define MEDIA_CHANNEL_NUM			16

//  init server with a list file
int initServer(void);
int init_media_file(char* listfile,int list_index);
//  clean-up
void  clearServer(int list_index);

const struct MediaFile * getFile(unsigned int,int);

//  create a server instance with index or media file path, if url is set, index is ignored
//    return value -- NULL if failed, or the handle of the server instance
void* createInstance(unsigned int index, char *url,int, playControlCallBackFunc *callback);

//  clear the server
void  clearInstance(void *handle);

//  play/pause/stop/seek control
void  requestPlay(void *handle);
void  requestStop(void *handle);
void  requestPause(void *handle);
void  requestSeek(void *handle, float pos);

//  rtsp mulitcast url
const char *getRTSPUrl(void *handle);

//  media file's duration
float getDuration(void *handle);

//  current playing position
float getCurrentPos(void *handle);

//  current status of the streamer
enum StreamStatus getCurrentStatus(void *handle);

const char *getMediaFileUrl(void *handle);
	
#ifdef   _cplusplus
}
#endif /* _cplusplus */ 
#endif

