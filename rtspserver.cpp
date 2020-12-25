/**********
  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the
  Free Software Foundation; either version 2.1 of the License, or (at your
  option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
  more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 **********/
// Copyright (c) 1996-2010, Live Networks, Inc.  All rights reserved
// and streams it using RTP
// main program

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include "AVIVideoDiscreteFramer.hh"
#include "AVIDemux.hh"

#define DBG 1

UsageEnvironment* env;
MPEG4VideoStreamFramer* videoSource;
MPEG1or2AudioStreamFramer *audioSource;
RTPSink* videoSink;
RTPSink* audioSink;

AVIDemux* aviDemux;

#define FILE_SUFFIX  ".avi"
static int rtsp_port = 8554;
static int broadcast_port = 12345;

int play_index = 0;
char stop_flag_toot = 0;   //  set 1 to stop loop
int mode_toot = 0;
int filecount = 0;
char** filelist = NULL;
const char *video_finish_toot = "DONE";

void play(char * filename); // forward
void freelist();


//  boardcast thread
void *boardcast_thread_toot(void *p)
{
	int sockfd;
	struct sockaddr_in their_addr;
	
	int broadcast = 1;
	int num = 0;
	char *rtsp_url = (char *)p;
	
	if( (sockfd = socket(AF_INET,SOCK_DGRAM,0)) == -1 )
	{
		perror("socket function!\n");
		exit(1);
	}

	if( setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,&broadcast,sizeof(broadcast)) == -1)
	{
		perror("setsockopt function!\n");
		exit(1);
	}

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(broadcast_port);
	their_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	if( (num = sendto(sockfd, rtsp_url, strlen(rtsp_url), 0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr) )) == -1)
	{
		perror("sendto function!\n");
		exit(1);
	}

    mode_toot = 0;
	
    while (!stop_flag_toot) {
        if (mode_toot == 0) {
			if( (num = sendto(sockfd, rtsp_url, strlen(rtsp_url), 0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr) )) == -1) {
				perror("sendto function!\n");
			}
			sleep(1);
		}
        else if (mode_toot == 1) {  // switch video
            sendto(sockfd, video_finish_toot, strlen(video_finish_toot), 0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr));
            sendto(sockfd, video_finish_toot, strlen(video_finish_toot), 0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr));
            sendto(sockfd, video_finish_toot, strlen(video_finish_toot), 0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr));
            mode_toot = 0;
            stop_flag_toot = 1;
		}
#if 0
        printf("Broadcast a message....\n");
        fflush(stdout);
#endif
    }
	
	close(sockfd);
	
	//  free URL
	delete[] rtsp_url;
	return 0;
}


void * rtsp_server(void *p) {
	// Begin by setting up our usage environment:
	
#if DBG
    printf("fileCount = %d \n", filecount);
	for (int i = 0; i < filecount; i++) {
		printf("%s \n", filelist[i]);
        fflush(stdout);
    }
#endif

	/*
	if(filecount <= 0) {
		*env << "No media file found, quit RTSP server\n";
		return 0;
	}
	play_index = filecount - 1;
	
	*/
	
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	env = BasicUsageEnvironment::createNew(*scheduler);

	// Create 'groupsocks' for RTP and RTCP:
	struct in_addr destinationAddress;
	destinationAddress.s_addr = chooseRandomIPv4SSMAddress(*env);
	// Note: This is a multicast address.  If you wish instead to stream
	// using unicast, then you should use the "testOnDemandRTSPServer"
	// test program - not this test program - as a model.

    const unsigned short rtpPortNumAudio = 18882;
	const unsigned short rtcpPortNumAudio = rtpPortNumAudio+1;
    const unsigned short rtpPortNumVideo = 18892;
	const unsigned short rtcpPortNumVideo = rtpPortNumVideo+1;
    const unsigned char ttl =   7; // low, in case routers don't admin scope

	const Port rtpPortAudio(rtpPortNumAudio);
	const Port rtcpPortAudio(rtcpPortNumAudio);
	const Port rtpPortVideo(rtpPortNumVideo);
	const Port rtcpPortVideo(rtcpPortNumVideo);

	Groupsock rtpGroupsockAudio(*env, destinationAddress, rtpPortAudio, ttl);
	Groupsock rtcpGroupsockAudio(*env, destinationAddress, rtcpPortAudio, ttl);
	Groupsock rtpGroupsockVideo(*env, destinationAddress, rtpPortVideo, ttl);
	Groupsock rtcpGroupsockVideo(*env, destinationAddress, rtcpPortVideo, ttl);

	rtpGroupsockAudio.multicastSendOnly();
	rtcpGroupsockAudio.multicastSendOnly();
	rtpGroupsockVideo.multicastSendOnly();
	rtcpGroupsockVideo.multicastSendOnly();

    OutPacketBuffer::maxSize =  1000000;
	
	// Create a 'MPEG Audio RTP' sink from the RTP 'groupsock':
	audioSink = MPEG1or2AudioRTPSink::createNew(*env, &rtpGroupsockAudio);

	// Create (and start) a 'RTCP instance' for this RTP sink:
	const unsigned estimatedSessionBandwidthAudio = 160; // in kbps; for RTCP b/w share
	const unsigned maxCNAMElen = 100;
	unsigned char CNAME[maxCNAMElen+1];
	gethostname((char*)CNAME, maxCNAMElen);
	CNAME[maxCNAMElen] = '\0'; // just in case

	RTCPInstance* audioRTCP = RTCPInstance::createNew(*env, &rtcpGroupsockAudio,
				estimatedSessionBandwidthAudio, CNAME,
				audioSink, NULL /* we're a server */, True);
				
	// Create a 'MPEG-4 Video RTP' sink from the RTP 'groupsock':
	videoSink = MPEG4ESVideoRTPSink::createNew(*env, &rtpGroupsockVideo, 96);

	// Create (and start) a 'RTCP instance' for this RTP sink:
    const unsigned estimatedSessionBandwidth =  500; // in kbps; for RTCP b/w share

	RTCPInstance* videoRTCP
		= RTCPInstance::createNew(*env, &rtcpGroupsockVideo,
				estimatedSessionBandwidth, CNAME,
				videoSink, NULL /* we're a server */,
				True /* we're a SSM source */);
	// Note: This starts RTCP running automatically
	
    int port =  rtsp_port;
	// start rtsp server with port 8554
    RTSPServer* rtspServer = NULL;

    //while (1) {
        rtspServer = RTSPServer::createNew(*env , port );
		if (rtspServer == NULL) {
			*env << "Failed to create RTSP server: " << env->getResultMsg() << " Try another port\n";
			port ++;
            return 0;
        }
		else {            
    //		break;
		}
    //}
	
	ServerMediaSession* sms
		= ServerMediaSession::createNew(*env, "testStream", NULL,
				"Session streamed by \"rtspserver\"",
				True /*SSM*/);
    sms->addSubsession(PassiveServerMediaSubsession::createNew(*audioSink, audioRTCP));
    sms->addSubsession(PassiveServerMediaSubsession::createNew(*videoSink, videoRTCP));
  
	rtspServer->addServerMediaSession(sms);

	char* url = rtspServer->rtspURL(sms);
	*env << "Play this stream using the URL \"" << url << "\"\n";

	// Start the boardcast thread
    pthread_t boardcast;
    pthread_create(&boardcast, NULL, boardcast_thread_toot, (void *)url);
	
    //  free url in thread boardcast_thread_toot
	//	delete[] url;

	
	// Start the streaming:
	*env << "Beginning streaming...\n";
	play((char *)p);
    stop_flag_toot = 0;
	
	
    env->taskScheduler().doEventLoop(&stop_flag_toot); // does not return

	//  do clean job
	//  freelist();

    pthread_join(boardcast, NULL);

    stop_flag_toot = 0;
	return 0; // only to prevent compiler warning
}

void afterPlaying(void* clientData) {
    // *env << "...done reading from file\n";

	// One of the sinks has ended playing.
  // Check whether any of the sources have a pending read.  If so,
  // wait until its sink ends playing also:
	if (audioSource->isCurrentlyAwaitingData()
	  || videoSource->isCurrentlyAwaitingData()) return;

	// Now that both sinks have ended, close both input sources,
	// and start playing again:
    // *env << "...done reading from file\n";
	
	// anounce "DONE" message
//    mode_toot = 1;	//tootzoe
//    while(mode_toot) {
//        usleep(1000);
//    }
	
	audioSink->stopPlaying();
	videoSink->stopPlaying();
	  // ensures that both are shut down
	Medium::close(audioSource);
	Medium::close(videoSource);
	Medium::close(aviDemux);
	
	*env << "all clean-up \n";
  // Note: This also closes the input file that this source read from.

  // Start playing once again:
 
	// Note that this also closes the input file that this source read from
	
	//play_index --;
	
	// Start playing once again:
	
	//  wait until the UDP messages are sent out
	/*
	
	sleep(3);
	if (play_index >= 0) {
		play(filelist[play_index]);
	}
	else {
    //	stop_flag_toot = 1;
		play_index = filecount - 1; 
		play(filelist[play_index]);
	}
*/	
#if 1
    play_index++;
    if(play_index >= filecount) play_index = 0;
     play(filelist[play_index]);
#endif

}

void play(char * filename) {
#if 0
	// Open the input file as a 'byte-stream file source':
	ByteStreamFileSource* fileSource
		= ByteStreamFileSource::createNew(*env, filename);
	if (fileSource == NULL) {
		*env << "Unable to open file \"" << filename
			<< "\" as a byte-stream file source\n";
		exit(1);
	}
#endif
	/*
	FramedSource* videoES = fileSource;

	// Create a framer for the Video Elementary Stream:
	videoSource = MPEG4VideoStreamFramer::createNew(*env, videoES);

	// Finally, start playing:
	*env << "Beginning to read from file...\n";
	videoSink->startPlaying(*videoSource, afterPlaying, videoSink);
	*/
	

	// We must demultiplex Audio and Video Elementary Streams
  // from the input source:
	aviDemux = AVIDemux::createNew(*env, filename);
	aviDemux->prepare();

    FramedSource* audioES =  (FramedSource*) (aviDemux->newAudioStream());
    FramedSource* videoES = (FramedSource*) (aviDemux->newVideoStream());

	// Create a framer for each Elementary Stream:
	audioSource
		= MPEG1or2AudioStreamFramer::createNew(*env, audioES);
	videoSource
		= AVIVideoDiscreteFramer::createNew(*env, videoES);

	// Finally, start playing each sink.
	*env << "Beginning to read from file...\n";
    videoSink->startPlaying(*videoSource, afterPlaying, videoSink);
	audioSink->startPlaying(*audioSource, afterPlaying, audioSink);

}

void add2list(char *filename)
{
	filecount++;
	filelist = (char **) realloc(filelist, sizeof(char *) * filecount);
	filelist[filecount - 1] = strdup(filename);
}

void freelist()
{
	for (int i = 0; i < filecount; i++) {
		free(filelist[i]);
	}
	free(filelist);
}

//  directory scan
int dir_list(const char *dir)
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    if((dp = opendir(dir)) == NULL) {
        fprintf(stderr,"cannot open directory: %s\n", dir);
        return -1;
    }
    chdir(dir);
    while((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name,&statbuf);
        if(S_ISDIR(statbuf.st_mode )) {
            // Found a directory, ignore it
			continue;
        }
        else {
			// Found a normal file, check the suffix
			if (strstr(entry->d_name, FILE_SUFFIX) != 0) {
				add2list(entry->d_name);
			}
		}		
    }
//    chdir("..");
    closedir(dp);
	return 0;
}


#if 0
int main(int argc, char **argv) 
{
	//  Get media file list
	//  initialize filecount & filelist
	if (argc == 1) {
		dir_list(".");
	}
	else {
		dir_list(argv[1]);
	}
	
	//  start RTSP thread 
//	pthread_t p;
//	pthread_create(&p, NULL, rtsp_server, NULL);	
	
//	pthread_join(p, NULL);
	
	if (filecount <= 0) {
		printf("No media file found \n");
		return -1;
	}

	//for (int i = 0; i < filecount; i++) {
	//	printf("%s \n", filelist[i]);
	
	int i = 0;

    //while(1)
    {
		if (i >= filecount)
			i = 0;
        play_index = 0;
		rtsp_server(filelist[i]);
		i++;
	}

	return 0;
	
}
#else
#if 0
extern "C" {
#include "server.h"
}

void myCallB (void *streamer)
{
    printf(" tootzoe...0x%x....%s\n\n" ,streamer, __func__);
}

int main2(int argc, char **argv)
{
    int hzf = 0;
    void *srvHandle = 0;

    initServer();


    srvHandle = createInstance(0, "dearest.avi" ,1  , myCallB);


    requestPlay(srvHandle);


    while(hzf ++ < 20)
    {
         printf(" hzf = %d\n", hzf);
        fflush(stdout);
        sleep(1);
    }

    clearInstance(srvHandle);
    //clearServer(srvHandle);

        return 0;
}
#endif
#endif
