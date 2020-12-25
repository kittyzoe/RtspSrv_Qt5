// Copyright (c) 2011 UPTTOP.COM, All rights reserved.
// RTSP MultiCast Server, support AVI, MP3, MPG, AAC, WAV, VOB, TS media files
// Implementation

#ifdef WIN32
#ifdef _DEBUG
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#define StrICmp _stricmp
#else
#define StrICmp strcasecmp
#endif

#ifndef _cplusplus  
#define _cplusplus  
#endif   
#include "server.h"
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "GenericStreamer.h"
#include "WAVStreamer.h"
#include <pthread.h>


extern "C" {
#include "config.h"
#include "libavutil/internal.h"
#ifdef restrict
#undef restrict
#define restrict
#endif
#include "libavcodec/mpegvideo.h"
#include "libavcodec/parser.h"
};


#define LOG_DBG printf
#define LOG_INFO printf
#define LOG_ERR printf


static int broadcast_port = 12345;
const char *video_finish = "DONE";

//   check file's suffix and return the type value
MediaType getType(char * file)
{
	char *suffix = NULL;
	int len = strlen(file);
	while (len)
	{
		len--;
		if(file[len] == '.') {
			suffix = file + len + 1;
			break;
		}
	}
	if (suffix == NULL) {
		//  no suffix found
		return FILE_UNSUPPORTED;
	}
	
	if( StrICmp(suffix, "avi") == 0 ) {
		return FILE_AVI;
	}
	else if( StrICmp(suffix, "mp3") == 0 ) {
		return FILE_MP3;
	}
	else if( StrICmp(suffix, "mpg") == 0 ) {
		return FILE_MPG;
	}
	else if( StrICmp(suffix, "mpeg") == 0 ) {
		return FILE_MPG;
	}
	else if( StrICmp(suffix, "aac") == 0 ) {
		return FILE_AAC;
	}
	else if( StrICmp(suffix, "m4a") == 0 ) {
		return FILE_AAC;
	}
	else if( StrICmp(suffix, "wav") == 0 ) {
		return FILE_WAV;
	}
	else if( StrICmp(suffix, "ts") == 0 ) {
		return FILE_TS;
	}
	else if( StrICmp(suffix, "vob") == 0 ) {
		return FILE_VOB;
	}
	
	return FILE_UNSUPPORTED;
}

#ifndef WIN32
#include <iconv.h> 
//  GB2312 -> UTF-8 convert
char * GB2312toUTF8(char * src)
{
	iconv_t conv_desc;
	size_t iconv_value;
	
	char * utf8;
    size_t len;
    size_t utf8len;
	
	char * utf8start;
    const char * src_start;
    int len_start;
    int utf8len_start;
	
	len = strlen (src);
    if (!len) {
        return NULL;
    }
	
	conv_desc = iconv_open("utf-8", "gb2312");
	if (conv_desc == (iconv_t)-1) {
		return NULL;
	}
	
    /* Assign enough space to put the UTF-8. */
    utf8len = 2*len;
    utf8 = new char[utf8len];
	memset(utf8, 0, utf8len);
	 /* Keep track of the variables. */
    len_start = len;
    utf8len_start = utf8len;
    utf8start = utf8;
    src_start = src;
    
    iconv_value = iconv (conv_desc, (char **)& src, & len, & utf8, & utf8len);
    /* Handle failures. */
    if (iconv_value == (size_t)-1) {
		iconv_close (conv_desc);
		return NULL;
	}
	
	iconv_close (conv_desc);
	return utf8start;
}
#endif

int parseFile(MediaFile& file)
{
	AVFormatContext *formatCtx;
	
	file.type = getType(file.filepath);

	//  if is unsupported file type, no need to parse it
	if (file.type == FILE_UNSUPPORTED)
	{
		return -1;
	}

	if(av_open_input_file(&formatCtx, file.filepath, NULL, 0, NULL) != 0)
	{
		// Couldn't open file, try covert the path from GB2312 to UTF-8
		LOG_INFO("[Info] cannot open file %s \n", file.filepath);
#ifndef WIN32
		char * utf8 = GB2312toUTF8(file.filepath);

		if(utf8 != NULL)
		{
			delete [] file.filepath;
			file.filepath = strDup(utf8);
			delete [] utf8;
			LOG_INFO("[Info] try to open %s\n", file.filepath);
			if(av_open_input_file(&formatCtx, file.filepath, NULL, 0, NULL) != 0)
			{
				return -1;
			}
		}
		else {
			return -1;
		}
#else
		return -1;	
#endif
	}

	//  Parse the media file, retrieve the Video/Audio Codec, duration and etc
	if(av_find_stream_info(formatCtx) < 0)
	{
		av_close_input_file(formatCtx);
		return -1; // Couldn't find stream information
	}

	AVStream *vstream = NULL;
	AVStream *astream = NULL;
	
	for(unsigned int i = 0; i < formatCtx->nb_streams; i++)
	{
		if(formatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
		{
			vstream = formatCtx->streams[i];
		}
		else if(formatCtx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO)
		{
			astream = formatCtx->streams[i];
		}
		if (vstream && astream)
		{
			break;
		}
	}

	if(vstream == NULL && astream == NULL)
	{
		av_close_input_file(formatCtx);
		return -1;
	}

	//  set duration value
	file.duration = formatCtx->duration / 1000000.0;

	switch (file.type)
	{
		case FILE_AVI:
			//AVI:  support MPEG4+MP3, MPEG4+AC3
			if (vstream != NULL)
			{
				if (vstream->codec->codec_id == CODEC_ID_MPEG4)
				{
					//some MPEG4 streams are not supported by the VPU of IMX.51, so check it
					ParseContext1 *pc = (ParseContext1 *)vstream->parser->priv_data;
					if (pc)
					{
						MpegEncContext *s = pc->enc;
						if (s) {
							if (s->vol_sprite_usage != 0)
							{  
                                //  There is GMC and iMX51's VPU does not support
                                LOG_DBG("There is GMC and iMX51's VPU does not support....\n");
								break;
							}	

							if (s->avctx->time_base.den == 1)
							{
								//  vop_time_increment_resolution is 1 and iMX51's VPU does not support
                                LOG_DBG("vop_time_increment_resolution is 1 and iMX51's VPU does not support....\n");
								break;
							}
						}
					}
				}
				else if((vstream->codec->codec_id != CODEC_ID_H264) &&
						(vstream->codec->codec_id != CODEC_ID_MPEG2VIDEO) &&
						(vstream->codec->codec_id != CODEC_ID_MPEG1VIDEO))
				{
					break;
				}
				
				
			}
			if(astream != NULL)
			{
				if (astream->codec->codec_id != CODEC_ID_MP3 &&
					astream->codec->codec_id != CODEC_ID_MP2 &&
					astream->codec->codec_id != CODEC_ID_AC3)
				{
					break;
				}
			}
			file.isSupport = 1;
			break;

		case FILE_MP3:
			//  MP3: support mpeg1-layer3, mpeg2-layer3
			if (vstream != NULL) {
				break;
			}
			if (astream != NULL)
			{
				if (astream->codec->codec_id != CODEC_ID_MP3 &&
					astream->codec->codec_id != CODEC_ID_MP2)
				{
					break;
				}
			}
			file.isSupport = 1;
			break;

		case FILE_MPG:
			//  MPG:  support MPEG2+MP3, MPEG2+AC3
			if(vstream != NULL)
			{
				if (vstream->codec->codec_id != CODEC_ID_MPEG2VIDEO &&
				vstream->codec->codec_id != CODEC_ID_MPEG1VIDEO)
				{
					break;
				}
			}
			if(astream != NULL)
			{
				if (astream->codec->codec_id != CODEC_ID_MP3 &&
					astream->codec->codec_id != CODEC_ID_MP2 &&
					astream->codec->codec_id != CODEC_ID_AC3) {
					break;
				}
			}
			file.isSupport = 1;
			break;

	case FILE_WAV:
		//  WAV: support 16bit pcm, a-law, u-law WAV
		if (vstream != NULL) {
			break;
		}
		if (astream != NULL) {
			//  support PCM, a-law and u-law, WAVE_FORMAT_IEEE_FLOAT and WAVE_FORMAT_EXTENSIBLE are not supported
			if (astream->codec->codec_tag != 1 &&    // PCM
				astream->codec->codec_tag != 6 &&    // a-law
				astream->codec->codec_tag != 7 )  {  // u-law
				break;
			}
			
			//  only support mono and stereo 
			if (astream->codec->channels > 2) {
				break;
			}	

			//  only support 8bit/16bit per sample
			if (av_get_bits_per_sample(astream->codec->codec_id) != 8 && 
				av_get_bits_per_sample(astream->codec->codec_id) != 16) {
				break;
			}
		}
		file.isSupport = 1;
		break;

	case FILE_AAC:
		//  AAC: support ADTS aac, .m4a(support max 2 channels)
		if (vstream != NULL) {
			break;
		}
		if (astream != NULL) {
			if (astream->codec->codec_id != CODEC_ID_AAC) {
				break;
			}

			if (astream->codec->channels > 2) {
				break;
			}
		}
		file.isSupport = 1;
		break;

	case FILE_TS:
		//  TS Stream: support MPEG2+AC3, MPEG2+MP3
		if (vstream != NULL) {
			if (vstream->codec->codec_id != CODEC_ID_MPEG2VIDEO) {
				break;
			}
		}
		if (astream != NULL) {	
			if (astream->codec->codec_id != CODEC_ID_MP3 &&
				astream->codec->codec_id != CODEC_ID_MP2 &&
				astream->codec->codec_id != CODEC_ID_AC3) {
					break;
			}
		}
		file.isSupport = 1;
		break;

	case FILE_VOB:
		//  VOB file: support MPEG2+AC3, MPEG2+MP3
		if (vstream != NULL) {
			if (vstream->codec->codec_id != CODEC_ID_MPEG2VIDEO &&
				vstream->codec->codec_id != CODEC_ID_MPEG1VIDEO) {
				break;
			}
		}
		if (astream != NULL) {	
			if (astream->codec->codec_id != CODEC_ID_MP3 &&
				astream->codec->codec_id != CODEC_ID_MP2 &&
				astream->codec->codec_id != CODEC_ID_AC3) {
					break;
			}
		}
		file.isSupport = 1;
		break;
	default:
		break;
	}
	
	if (file.isSupport != 1) {
		LOG_ERR("[ERR] Media file %s is not supported\n", file.filepath);
	}
	av_close_input_file(formatCtx);
	return 0;
}

MediaFile** initMediaFile(/*std::vector<char *>&*/const char ** filelist, int count)
{
	MediaFile **list = new MediaFile*[count];

	for (int i = 0; i < count; i++) {
		MediaFile *file = new MediaFile();
		file->isSupport = 0; 
		file->duration = 0.0;
		file->type = FILE_UNSUPPORTED;
		
		file->filepath = strDup(filelist[i]);
		parseFile(*file);
		list[i] = file;
	}

	return list;
}

Streamer *StartPlaying(MediaFile* file, int index)
{
	int type = file->type;
	Streamer *streamer = NULL;
	switch (type)
	{
	case FILE_TS:
	case FILE_MPG:
	case FILE_VOB:
	case FILE_MP3:
	case FILE_AVI:
	case FILE_AAC:
		streamer = GenericStreamer::createNew(file->filepath, file->duration, index);
		break;
	case FILE_WAV:
		streamer = WAVStreamer::createNew(file->filepath, file->duration, index);
		break;
	}
	
	return streamer;
}


int stop_flag = 0;
int mode = 0;
//  boardcast thread
void *boardcast_thread(void *p)
{
	int sockfd;
	struct sockaddr_in their_addr;
	
	int broadcast = 1;
	int num = 0;
	char *rtsp_url = strDup((char *)p);
	
	if( (sockfd = socket(AF_INET,SOCK_DGRAM,0)) == -1 )
	{
		perror("socket function!\n");
		return 0;
	}

	if( setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,&broadcast,sizeof(broadcast)) == -1)
	{
		perror("setsockopt function!\n");
		return 0;
	}

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(broadcast_port);
	their_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	if( (num = sendto(sockfd, rtsp_url, strlen(rtsp_url), 0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr) )) == -1)
	{
		perror("sendto function!\n");
		return 0;
	}

	mode = 0;
	
	while (!stop_flag) {
		if (mode == 0) {
			if( (num = sendto(sockfd, rtsp_url, strlen(rtsp_url), 0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr) )) == -1) {
				perror("sendto function!\n");
			}
			sleep(1);
		}
		else if (mode == 1) {  // switch video 
			sendto(sockfd, video_finish, strlen(video_finish), 0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr));
			sendto(sockfd, video_finish, strlen(video_finish), 0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr));
			sendto(sockfd, video_finish, strlen(video_finish), 0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr));
			mode = 0;
			stop_flag = 1;
		}
		
	//	printf("Broadcast a message\n");
    }
	
	close(sockfd);
	
	//  free URL
	delete [] rtsp_url;
	return 0;
}


MediaFile** g_list[MEDIA_CHANNEL_NUM];
unsigned int g_count[MEDIA_CHANNEL_NUM];

//  initialize server 
int initServer(void)
{
	int i;

	av_register_all();

	for(i=0;i<MEDIA_CHANNEL_NUM;i++)
	{
		g_list[i] = NULL;
		g_count[i] = 0;
	}

	return 0;
}

int init_media_file(char* listfile,int list_index)
{
	FILE *fp = fopen(listfile, "rb");
	if(fp == NULL)
	{
		LOG_ERR("Open %s failed \n", listfile);
		return -1;
	}

	//  splitter listfile and get the directory 
	char *tmp_str = strDup(listfile);
	int i = strlen(tmp_str);
	while(i)
	{
		if (tmp_str[i - 1] == '/')
		{
			tmp_str[i - 1] = '\0';
			break;
		}
        i--;
	}

	if (i == 0)
	{
		//  directory not found
		tmp_str[0] = '\0';
	}
#if 0
	std::vector<char *> filelist;
	{
		char buf[1024];
		while (fgets(buf, 1024, fp) != NULL) {
			int size = strlen(buf);
			while(size > 0) {
				if (buf[size - 1] == 0x0d || buf[size - 1] == 0x0a) {
					buf[size - 1] = 0;
					size --;
				} else {
					break;
				}
			}
			if (size > 0) {
				if (strlen(tmp_str) > 0) {
					char *file = new char[size + 2 + strlen(tmp_str)];
					snprintf(file, size + 2 + strlen(tmp_str), "%s/%s", tmp_str, buf);
					filelist.push_back(file);
				}
				else {
					char *file = strDup(buf);
					filelist.push_back(file);
				}
			}
		}
	}

	delete[] tmp_str;
	fclose(fp);
	
	g_count[list_index] = filelist.size();

	g_list[list_index] = initMediaFile(filelist, g_count[list_index]);
#endif
	for(unsigned int i = 0; i < g_count[list_index]; i++)
	{
        //delete[] filelist[i];
	}

	return g_count[list_index];
}

void clearServer(int list_index)
{
	for(unsigned int i = 0; i < g_count[list_index]; i++)
	{
		delete[] g_list[list_index][i]->filepath;
		delete g_list[list_index][i];
	}
	delete[] g_list[list_index];
}

const struct MediaFile * getFile(unsigned int index,int list_index)
{
	if(index > g_count[list_index] || index < 0)
	{
		//LOG_ERR("Wrong Index input \n");
		return NULL;
	}
	else
	{ 
		return g_list[list_index][index];
	}
}

//  create server instance from index or file url
void* createInstance(unsigned int index, char *url,int list_index, playControlCallBackFunc *callback)
{
	if (url != NULL)
	{
		struct MediaFile file;
		file.isSupport = 0; 
		file.duration = 0.0;
		file.type = FILE_UNSUPPORTED;

		file.filepath = strDup(url);
		parseFile(file);
		if(file.isSupport == 1)
		{
			Streamer *streamer = StartPlaying(&file, -1);
			streamer->setCallBackFunc(callback);
			delete[] file.filepath;
			return streamer;
		}
		else
		{
			delete[] file.filepath;
			return NULL;
		}
	}
	else
	{
		if(index >= g_count[list_index] || index < 0)
		{
			//LOG_ERR("Wrong Index input %d %d\n",index,g_count[list_index]);
			return NULL;
		}
		
		if(g_list[list_index][index]->isSupport == 1)
		{
			Streamer *streamer = StartPlaying(g_list[list_index][index], index);
			streamer->setCallBackFunc(callback);
			if (streamer->getCurStatus() != ST_INIT)
			{
				//  open failed 
				delete streamer;
				return NULL;
			}
			return streamer;
		}
		return NULL;
	}
}

void clearInstance(void * handle)
{
	Streamer *streamer = (Streamer *) handle;
	delete streamer;
}


void requestPlay(void *handle)
{
	Streamer *streamer = (Streamer *) handle;
	streamer->ReqStart();
}


void requestPause(void *handle)
{
	Streamer *streamer = (Streamer *) handle;
	streamer->ReqPause();
}

void requestStop(void *handle)
{
	Streamer *streamer = (Streamer *) handle;
	streamer->ReqStop();
}

void requestSeek(void *handle, float pos)
{
	Streamer *streamer = (Streamer *) handle;
	streamer->ReqSeek(pos);
}

float getDuration(void *handle)
{
	Streamer *streamer = (Streamer *) handle;
	return streamer->getDuration();
}

float getCurrentPos(void *handle)
{
	Streamer *streamer = (Streamer *) handle;
	return streamer->getPosition();
}

StreamStatus getCurrentStatus(void *handle)
{
	Streamer *streamer = (Streamer *) handle;
	return streamer->getCurStatus();
}

const char *getRTSPUrl(void *handle)
{
	Streamer *streamer = (Streamer *) handle;
	return streamer->getRTSPUrl();
}

const char *getMediaFileUrl(void *handle)
{
	Streamer *streamer = (Streamer *) handle;
	return streamer->getMediaFileUrl();
}
