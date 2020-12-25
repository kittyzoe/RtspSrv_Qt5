
QT += core gui network widgets
#


#LIVE555ROOT = E:/workprj/QtPrj/live555_20110314
#FFMPEGROOT = E:/workprj/QtPrj/ffmpeg_20101018
## new version
LIVE555ROOT = E:/workprj/live20170718
FFMPEGROOT = E:/workprj/ffmpeg400

#arm
#LIVE555ROOT = /home/zoe/workprj/yuenki/eight_server/live
#FFMPEGROOT = /home/zoe/workprj/yuenki/eight_server/live/ffmpeg_20101018

INCLUDEPATH += $$LIVE555ROOT/liveMedia/include \
               $$LIVE555ROOT/liveMedia  \
               $$LIVE555ROOT/groupsock/include \
               $$LIVE555ROOT/BasicUsageEnvironment/include \
               $$LIVE555ROOT/UsageEnvironment/include \
               $$FFMPEGROOT




LIBS +=  \
     $$LIVE555ROOT/liveMedia/libliveMedia.a \
     $$LIVE555ROOT/groupsock/libgroupsock.a \
     $$LIVE555ROOT/BasicUsageEnvironment/libBasicUsageEnvironment.a \
     $$LIVE555ROOT/UsageEnvironment/libUsageEnvironment.a \
     $$FFMPEGROOT/libavformat/libavformat.a \
     $$FFMPEGROOT/libavcodec/libavcodec.a \
     $$FFMPEGROOT/libswresample/libswresample.a \
     $$FFMPEGROOT/libswscale/libswscale.a \
     $$FFMPEGROOT/libavutil/libavutil.a \
     -lz -lws2_32 -lwinmm -lwst -lsecur32 -lcrypt32 -liconv -lm -lole32 -loleaut32 -lbcrypt

#arm
#LIBS += -L/home/zoe/workprj/yuenki/zlib-1.2.5

# pc
#QMAKE_CXXFLAGS += -m64 -fPIC -I. -O2 -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -Wall -DBSD=1

# arm
QMAKE_CXXFLAGS += -fPIC -I. -O2 -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -Wall -DBSD=1

QMAKE_LFLAGS += -s


#arm
# DEFINES += TOOT_ARM ARCH=arm



#ifndef UINT64_C
#define UINT64_C(value) __CONCAT(value, ULL)
#endif


SOURCES += \
    WAVStreamer.cpp \
    RecordWAVStreamer.cpp \
    Streamer.cpp \
    GenericStreamer.cpp \
    AACAudioStreamFramer.cpp \
    AVIDemux.cpp \
    AVIDemuxedElementaryStream.cpp \
    AVIVideoDiscreteFramer.cpp \
    multicast/tudpmulticast.cpp \
    main.cpp \
    tmainwnd.cpp \
    tcommlistdelegate.cpp \
    tmdidbutil.cpp \
    nsleep.c



HEADERS += \
    WAVStreamer.h \
    RecordWAVStreamer.h \
    Streamer.h \
    GenericStreamer.h \
    AACAudioStreamFramer.hh \
    AVIDemux.hh \
    AVIDemuxedElementaryStream.hh \
    AVIVideoDiscreteFramer.hh \
    multicast/tudpmulticast.h \
    tmainwnd.h \
    tcommlistdelegate.h \
    tmdidbutil.h
