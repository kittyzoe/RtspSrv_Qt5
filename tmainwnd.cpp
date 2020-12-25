/**************************************************************************
**   Copyright @ 2010 TOOTzoe.com
**   Special keywords: zoe 26/9/2016 2016
**   Environment variables:
**   To protect a percent sign, use '%'.
**
**
**   E-mail : toot@tootzeo.com
**   Tel    :   13802205042  13712943464
**   Website: http://www.tootzoe.com
**
**************************************************************************/



//------------------------------------------------------------------------
#include <QtCore>
#include <QtGui>
//#include <QtCore/QByteArrayList.h>

#ifndef TOOT_ARM
#include <QtWidgets>
#endif
#include <QtNetwork>

#include "tmainwnd.h"

#include "multicast/tudpmulticast.h"

#include "tcommlistdelegate.h"

#if TOOT_ARM
#include "QJson/Parser"
#include "QJson/Serializer"
#endif

extern "C" {
#include "config.h"
#include "libavutil/internal.h"
#ifdef restrict
#undef restrict
#define restrict
#endif
#include "libavformat/avformat.h"
};

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "GenericStreamer.h"
#include "WAVStreamer.h"

#include "tmdidbutil.h"

#define RTSP_MAX_CHANNEL_NUM  (16)

static TMainWnd *gTMainWnd = 0;

static QList <Streamer *> g_cbStreamerLs;

TMainWnd::TMainWnd(QWidget *parent) : QWidget(parent)
{



    gTMainWnd = this;
    initData();
    setupUi();
    initData2();
    setupPopupMenu();

#ifndef TOOT_ARM
   // setFixedSize(1024, 768);
#endif

    QTimer::singleShot(0, this, SLOT(tmrFunc()));


    resize(1000,600);
}

TMainWnd::~TMainWnd()
{
    emit meClosed();
   // emit quitMulticastListen();
    qDebug() << __func__;
    gTMainWnd = 0;
}

void TMainWnd::timerEvent(QTimerEvent * )
{
    updateRunTimeInfo();
}

void TMainWnd::initData()
{

    setWindowTitle(tr("流媒体服务器"));

    m_supportedFileType << "avi"
                        << "mp4"
                        << "mp3"
                        << "mpg"
                        << "mpeg"
                        << "aac"
                        << "m4a"
                        << "wav"
                        << "ts"
                        << "vob"
                           ;

    memset(&m_oldPackageData , 0 , sizeof(m_oldPackageData));
    m_oldPackageData.cmd = 9001;

    m_totalRunTimeSec = 0;
    m_thizSrvType = "A";


    connect(this, SIGNAL(newErrMsg(QString)), SLOT(errMsgArrived(QString)));

    av_register_all();

    startTimer(3000); //update total run time info

}

void TMainWnd::tmrFunc()
{


    connect(&TUdpMulticast::Instance(), SIGNAL(srvTypeChged(QString))
            ,SLOT(thizSrvTypeChged(QString)));
    TUdpMulticast::Instance().startStopTmrSend(true);


    int num = TMdiDBUtil::Instance().maxChannNum();
    m_maxChannSpB->setValue(num < 8 ? 8 : num);

}

void TMainWnd::setupUi()
{
    QVBoxLayout *mainLO = new QVBoxLayout;
    mainLO->setSpacing(0);
    mainLO->setMargin(10);


    QLabel *titleLb = new QLabel(tr("YUENKI RTSP 流媒体服务器"));
    titleLb->setAlignment(Qt::AlignCenter);
    QFont fn = titleLb->font();
    fn.setPixelSize(24);
    titleLb->setFont(fn);
    mainLO->addWidget(titleLb);


    QLabel *verLb = new QLabel(tr("Ver: 4.0.0"));
    verLb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mainLO->addWidget(verLb);


    QGroupBox *msicGB = new QGroupBox(tr("杂项"));
    QHBoxLayout *miscGBLO = new QHBoxLayout;
    miscGBLO->setSpacing(5);
    msicGB->setLayout(miscGBLO);

    m_thizIpLb = new QLabel("0.0.0.0");
    QFormLayout *formLO0 = new QFormLayout;
    formLO0->addRow(tr("本机IP地址： ") , m_thizIpLb);
    miscGBLO->addLayout(formLO0);

#if TOOT_ARM
    m_scanPathLe = new QLineEdit(tr("/mnt/sata/file"));
#else
   // m_scanPathLe = new QLineEdit(tr("/var/www/html/media/ropente_h264_1024kbps/file"));
     m_scanPathLe = new QLineEdit(tr("medias"));
#endif

    formLO0->addRow(tr("自动扫描媒体文件路径: ") , m_scanPathLe);

    QPushButton *rescaneMediaFilesBtn = new QPushButton(tr("重新扫描"));
     connect(rescaneMediaFilesBtn, SIGNAL(released()) ,   SLOT(rescanMediaBtnClick()));
    // miscGBLO->addWidget(rescaneMediaFilesBtn);

    QPushButton *checkUdpMulticastDataBtn = new QPushButton(tr("查看UDP广播内容"));
     connect(checkUdpMulticastDataBtn, SIGNAL(released()) ,   SLOT(chkUdpDataClick()));
   //  miscGBLO->addWidget(checkUdpMulticastDataBtn);


     QPushButton *fileManagerBtn = new QPushButton(tr("文件管理器"));
      connect(fileManagerBtn, SIGNAL(released()) ,   SLOT(lunchFileManager()));
    //  miscGBLO->addWidget(fileManagerBtn);

    mainLO->addWidget(msicGB);


    QGroupBox *channGB = new QGroupBox(tr("通道状态"));
    QVBoxLayout *channGBLO = new QVBoxLayout;
    channGBLO->setSpacing(0);
    channGB->setLayout(channGBLO);

    QHBoxLayout *HLO3 = new QHBoxLayout;
    HLO3->setSpacing(0);
    HLO3->setMargin(0);

    QFormLayout *channFormLO = new QFormLayout;
    channFormLO->setSpacing(0);
    channFormLO->setMargin(0);

    m_maxChannSpB = new QSpinBox;
    m_maxChannSpB->setRange(1, RTSP_MAX_CHANNEL_NUM);
    m_maxChannSpB->setValue(8);
    channFormLO->addRow(tr("最大通道数：") , m_maxChannSpB);
    connect(m_maxChannSpB , SIGNAL(valueChanged(int)) , SLOT(maxChannSpinBoxClick(int)));


    m_currChannNumLE = new QLineEdit("0");


    QFormLayout *channFormLO2 = new QFormLayout;
    channFormLO2->setSpacing(0);
    channFormLO2->setMargin(0);
    channFormLO2->addRow(tr("当前通道数：") ,m_currChannNumLE );

    m_totalRunTimeLb = new QLabel(tr("00 - 00 : 00 : 00"));
    QFormLayout *channFormLO3 = new QFormLayout;
    channFormLO3->setSpacing(0);
    channFormLO3->setMargin(0);
    channFormLO3->addRow(tr("持续运行时间:  ") ,m_totalRunTimeLb );



    HLO3->addLayout(channFormLO);
    HLO3->addLayout(channFormLO2);
    HLO3->addLayout(channFormLO3);

   // channGBLO->addLayout(HLO3);

    QGroupBox *srvTypeGpBox = new QGroupBox(tr("服务器角色"));
    QHBoxLayout *srvTypeBtnGpLO = new QHBoxLayout;
    srvTypeGpBox->setLayout(srvTypeBtnGpLO);

    srvTypeBtnGp = new QButtonGroup;
    QRadioButton *masterRBtn = new QRadioButton(tr("主"));
    QRadioButton *slaveRBtn = new QRadioButton(tr("从"));
    QRadioButton *negerationRBtn = new QRadioButton(tr("自动协商"));
    negerationRBtn->setChecked(true);
    srvTypeBtnGp->addButton(masterRBtn,1);
    srvTypeBtnGp->addButton(slaveRBtn,2);
    srvTypeBtnGp->addButton(negerationRBtn,3);

    srvTypeBtnGpLO->addWidget(masterRBtn);
    srvTypeBtnGpLO->addWidget(slaveRBtn);
    srvTypeBtnGpLO->addWidget(negerationRBtn);


    mainLO->addWidget(srvTypeGpBox);

    m_channListWid = new QListWidget;
    m_channListWid->setItemDelegate(new TCommListDelegate);
    m_channListWid->setContextMenuPolicy(Qt::CustomContextMenu);
    connect( m_channListWid , SIGNAL(customContextMenuRequested(QPoint))  , SLOT(showListCntxMenu(QPoint)));
    connect( m_channListWid , SIGNAL(itemDoubleClicked(QListWidgetItem*))  , SLOT(listItemDBClick( QListWidgetItem*)));


    channGBLO->addWidget(m_channListWid);

    mainLO->addWidget(channGB);

    QGroupBox *logGB = new QGroupBox(tr("运行日志"));
    QVBoxLayout *logGBLO = new QVBoxLayout;
    logGBLO->setSpacing(0);
    logGB->setLayout(logGBLO);

    m_logTe = new QTextEdit;
    m_logTe->setReadOnly(true);
    m_logTe->document()->setMaximumBlockCount(1000);


    logGBLO->addWidget(m_logTe);

    QHBoxLayout *logHLO = new QHBoxLayout;
    logHLO->setSpacing(10);


    QSpinBox *m_maxLineSpb = new QSpinBox();
    m_maxLineSpb->setRange(1000, 50000);
    m_maxLineSpb->setSingleStep(1000);
    m_maxLineSpb->setValue(m_logTe->document()->maximumBlockCount());
    connect(m_maxLineSpb, SIGNAL(valueChanged(int)) , SLOT(logMaxLineSpierClick(int)));


    QFormLayout *logFormLO = new QFormLayout;
    logFormLO->addRow(tr("日志最大行数: ") , m_maxLineSpb);

    logHLO->addLayout(logFormLO);
    m_maxLineSpb->setVisible(false);



    QPushButton *cleanLogBtn = new QPushButton(tr("清除日志"));
    QPushButton *saveLogBtn = new QPushButton(tr("保存日志"));

    connect(cleanLogBtn, SIGNAL(released()) , m_logTe, SLOT(clear()));
    connect(saveLogBtn, SIGNAL(released()) , SLOT(saveLogClick()));


    logHLO->addWidget(cleanLogBtn , 1);
    logHLO->addWidget(saveLogBtn , 1);

   // logGBLO->addLayout(logHLO);



    logGB->setMaximumHeight(150);
    mainLO->addWidget(logGB);
     setLayout(mainLO);
}

void TMainWnd::setupPopupMenu()
{

    m_listPopMenu = new QMenu(this);

     QAction *  m_runAct = m_listPopMenu->addAction(tr("运    行"));
     QAction *  m_pauseAct = m_listPopMenu->addAction(tr("暂    停"));
     QAction *m_stopAct = m_listPopMenu->addAction(tr("停    止"));
     m_listPopMenu->addSeparator();
     QAction *  m_brunAct = m_listPopMenu->addAction(tr("运行所有项"));
     QAction *  m_bpauseAct = m_listPopMenu->addAction(tr("暂停所有项"));
     QAction *m_bstopAct = m_listPopMenu->addAction(tr("停止所有项"));
     m_listPopMenu->addSeparator();
     QAction *m_terminateAct = m_listPopMenu->addAction(tr("终    止"));
     m_listPopMenu->addSeparator();
     QAction *m_sendMsgAct = m_listPopMenu->addAction(tr("发送消息"));
     QAction *m_previewAct = m_listPopMenu->addAction(tr("预览视频"));

    connect(m_runAct, SIGNAL(triggered()) , SLOT(channRunClick()));
    connect(m_pauseAct, SIGNAL(triggered()) , SLOT(channPauseClick()));
    connect(m_stopAct, SIGNAL(triggered()) , SLOT(channStopClick()));

    connect(m_brunAct, SIGNAL(triggered()) , SLOT(batchRunActClick()));
    connect(m_bpauseAct, SIGNAL(triggered()) , SLOT(batchPauseActClick()));
    connect(m_bstopAct, SIGNAL(triggered()) , SLOT(batchStopActClick()));

    connect(m_terminateAct, SIGNAL(triggered()) , SLOT(channTerminateClick()));
    connect(m_sendMsgAct, SIGNAL(triggered()) , SLOT(sendMessages()));
    connect(m_previewAct, SIGNAL(triggered()) , SLOT(channPreviewClick()));

    ///////////////////////////////////////////

    m_mainWndMenu = new QMenu(this);

    QAction *  m_saveConfAct = m_mainWndMenu->addAction(tr("配置文件另存为"));
    connect(m_saveConfAct, SIGNAL(triggered()) , SLOT(saveConfClick()));
    m_mainWndMenu->addSeparator();
    QAction *  m_rebootAct = m_mainWndMenu->addAction(tr("重启系统"));
    connect(m_rebootAct, SIGNAL(triggered()) , SLOT(rebootSys()));



    setContextMenuPolicy(Qt::CustomContextMenu);
    connect( this , SIGNAL(customContextMenuRequested(QPoint))  , SLOT(showMainWndCntxMenu(QPoint)));

}

void TMainWnd::initData2()
{

    QString ipStr ;
    QList<QHostAddress>  thisIpLs;

    foreach (QNetworkInterface interface, QNetworkInterface::allInterfaces()) {
        foreach (QNetworkAddressEntry entry, interface.addressEntries()) {
            QHostAddress broadcastAddress = entry.broadcast();
            if (broadcastAddress != QHostAddress::Null && entry.ip() != QHostAddress::LocalHost) {
               // broadcastAddresses << broadcastAddress;
                   thisIpLs << entry.ip();


                 qDebug() <<  entry.ip();
                 ipStr.append(entry.ip().toString()).append( "  ");
            }
        }
    }

    if(ipStr.trimmed().isEmpty())
        QMessageBox::critical(this,tr("错误!"),tr("未接入网络...."));

    m_thizIpLb->setText(ipStr);
    TUdpMulticast::Instance( ).keepHostIpLs(thisIpLs);


#if toot

    QVariantList varLs;

    QVariantMap vmap1;

    vmap1.insert("toot", "hzf");
    vmap1.insert("zoe", "cmq");

    QVariantMap vmap2;

    vmap2.insert("toot", "hzf2");
    vmap2.insert("zoe", "cmq2");

    varLs << vmap1<< vmap2;


    QJson::Serializer  seri;
   QByteArray serBa =  seri.serialize(varLs);

   qDebug() << serBa;

   m_logTe->append( QString(serBa) );


   QJson::Parser par;
   bool isOK;
   QVariantList resultLs = par.parse(serBa  , &isOK).toList();

   if(isOK){
       foreach (QVariant  m, resultLs) {
           QVariantMap mm = m.toMap();
            qDebug() << mm["toot"].toString();
            qDebug() << mm["zoe"].toString();
       }
   }
#endif

   QTimer::singleShot(1000, this, SLOT(scanMedaiFile()));  //temp disabble tootzoe
}

void TMainWnd::handleMulticastUdpData(const QByteArray &baData)
{
    //qDebug() << baData.toHex();

    struct package_data msgData;
    memcpy(&msgData, baData.constData(), baData.size());
   // qDebug() << " cmd: " << msgData.cmd;

    if(msgData.cmd == 9001){
        //m_networkStreamsLs->clear();

        for(int i = 0 ;  i < 16; i ++)
        {
            QString url = QString(msgData.requestURL[i]);
            if(url.isEmpty()) continue;

            // qDebug() << url  << " lastindexof : " << url.lastIndexOf("/");

            QString iname =  url.right( url.length() -  url.lastIndexOf("/") - 1);
            QListWidgetItem *item = new QListWidgetItem(tr("ch:%1  %2").arg(i).arg(iname));
            item->setData(Qt::UserRole + 1 , url);
            item->setToolTip(url);
           // m_networkStreamsLs->addItem(item);
          //  qDebug() << " i: " << i << "  url=" << ;

        }

    }
}


void TMainWnd::scanMedaiFile()
{


    emit newErrMsg(tr(" 开始扫描分析 %1 下通道文件夹....\n").arg(m_scanPathLe->text()));

    m_rtspChannLs.clear();

    int chanNum =  m_maxChannSpB->value();

    for(int i = 1 ; i <= chanNum ;  i ++){

       QString playlistTxt = tr("%1/%2/playlist.txt")
               .arg( m_scanPathLe->text()).arg(i) ;
       QFileInfo finfo(playlistTxt);
       if(!finfo.exists()){
           emit newErrMsg(tr(" # 通道 %1 文件夹或 playlist.txt 不存在").arg(i));
           continue;
       }

       parsePlayList(finfo  , i);

    }

    // qDebug() << m_rtspChannLs;


    QTimer::singleShot(500, this, SLOT(runAllStreamers()));



}

void TMainWnd::parsePlayList(const QFileInfo &p , int chann)
{

    QString tmpFN = p.absoluteFilePath();
    QString tmpP = p.path();

    QFile txtFile (tmpFN);
    if(!txtFile.open(QIODevice::ReadOnly)){
        emit newErrMsg(tr("打开文件错误: %1 , err = %2").arg(tmpFN).arg(txtFile.errorString()));
        return;
    }

    QByteArray txtData = txtFile.readAll().trimmed();
    txtFile.close();

    txtData.replace('\r',"");
   // QByteArrayList movClipName = txtData.split('\n');

    QString allCnt = QString::fromUtf8(txtData);

    QStringList movClipName = allCnt.split('\n');


   // qDebug() << "movClipName " <<  movClipName;


    QList<QVariantList> tmpVarLsLs;
  //  QMap<int ,QList<QVariantList> > tmpMap;
    for(int i = 0 ; i < movClipName.size() ; i ++){
        QString movName = movClipName.at(i);
        QFileInfo movFInfo (  tmpP + "/" + movName);

        if(!movFInfo.exists()) {
            emit newErrMsg(tr("文件不存在: %1 ").arg( movFInfo.absoluteFilePath()));
            continue;
        }




        if(! m_supportedFileType.contains(movFInfo.suffix() , Qt::CaseInsensitive)){
            emit newErrMsg(tr("文件类型不支持: %1 ").arg( movFInfo.absoluteFilePath()));
            continue;
        }


        QVariantList varLs = parseMediaFile(movFInfo);


       if(!varLs.isEmpty()){
           tmpVarLsLs << varLs;
       }
        //qDebug() << " parseMediaFile " <<  varLs.keys();
    }


     m_rtspChannLs.insert(chann, tmpVarLsLs );

}


QVariantList TMainWnd::parseMediaFile(const QFileInfo &p)
{
    bool canPlay = false;
    QVariantList varLs;

    AVFormatContext *formatCtx = NULL;
  //  qDebug() << isSupp;
 // qDebug() << s;
  //qDebug() << tmpP;

    QString movFP = p.absoluteFilePath();

    emit newErrMsg(tr("分析媒体编码: %1").arg( movFP));

    if(avformat_open_input(&formatCtx,  movFP.toUtf8().constData() , NULL,   NULL) != 0)
    {
        emit newErrMsg(tr("无法打开文件： %1 ").arg(movFP));
        return varLs;
    }

    //  Parse the media file, retrieve the Video/Audio Codec, duration and etc
    if(avformat_find_stream_info(formatCtx, NULL) < 0)
    {
        avformat_close_input(&formatCtx);
        emit newErrMsg(tr("无法找到音频/视频流： %1 ").arg(movFP));
        return varLs;; // Couldn't find stream information
    }

    AVStream *vstream = NULL;
    AVStream *astream = NULL;

    for(unsigned int i = 0; i < formatCtx->nb_streams; i++)
    {
        if(formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            vstream = formatCtx->streams[i];
        }
        else if(formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
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
       avformat_close_input(&formatCtx);
        emit newErrMsg(tr("音频/视频流无效： %1 ").arg(movFP));
        return varLs;
    }

    QVariantMap vLenMap;

    vLenMap["duration"] = formatCtx->duration / 1000000.0;
    vLenMap["pathName"] = movFP;

    MediaType tmpType = FILE_UNSUPPORTED;
    QString tmpSuff = p.suffix().toLower();
    if(tmpSuff == "avi") tmpType = FILE_AVI;
    else if(tmpSuff == "mp4") tmpType = FILE_MP4;
    else if(tmpSuff == "mp3") tmpType = FILE_MP3;
    else if(tmpSuff == "mpg") tmpType = FILE_MPG;
    else if(tmpSuff == "mpeg") tmpType = FILE_MPG;
    else if(tmpSuff == "aac") tmpType = FILE_AAC;
    else if(tmpSuff == "m4a") tmpType = FILE_AAC;
    else if(tmpSuff == "wav") tmpType = FILE_WAV;
    else if(tmpSuff == "ts") tmpType = FILE_TS;
    else if(tmpSuff == "vob") tmpType = FILE_VOB;
    else tmpType = FILE_UNSUPPORTED;

    vLenMap["fileType"] = tmpType;

    switch (tmpType)
    {
        case FILE_AVI:
            //AVI:  support MPEG4+MP3, MPEG4+AC3
            if (vstream != NULL)
            {
               if(
                      (vstream->codecpar->codec_id != AV_CODEC_ID_MPEG4) &&
                       (vstream->codecpar->codec_id != AV_CODEC_ID_H264) &&
                        (vstream->codecpar->codec_id != AV_CODEC_ID_MPEG2VIDEO) &&
                        (vstream->codecpar->codec_id != AV_CODEC_ID_MPEG1VIDEO))
                {
                    break;
                }


            }
            if(astream != NULL)
            {
                if (astream->codecpar->codec_id != AV_CODEC_ID_MP3 &&
                    astream->codecpar->codec_id != AV_CODEC_ID_MP2 &&
                    astream->codecpar->codec_id != AV_CODEC_ID_AC3)
                {
                    break;
                }
            }
            canPlay = true;
            break;


           case FILE_MP4:
            //MP4:  support MPEG4+AAC, H264+Mp3
            if (vstream != NULL)
            {
                if ((vstream->codecpar->codec_id != AV_CODEC_ID_MPEG4) &&
                    (vstream->codecpar->codec_id != AV_CODEC_ID_H264)
                   )
                 {
                    break;
                 }

            }
            if(astream != NULL)
            {
                if (astream->codecpar->codec_id != AV_CODEC_ID_MP3 &&
                    astream->codecpar->codec_id != AV_CODEC_ID_AAC
                    )
                {
                    break;
                }
            }
            canPlay = true;
            break;


        case FILE_MP3:
            //  MP3: support mpeg1-layer3, mpeg2-layer3
            if (vstream != NULL) {
                break;
            }
            if (astream != NULL)
            {
                if (astream->codecpar->codec_id != AV_CODEC_ID_MP3 &&
                    astream->codecpar->codec_id != AV_CODEC_ID_MP2)
                {
                    break;
                }
            }
            canPlay = true;
            break;

        case FILE_MPG:
            //  MPG:  support MPEG2+MP3, MPEG2+AC3
            if(vstream != NULL)
            {
                if (vstream->codecpar->codec_id != AV_CODEC_ID_MPEG2VIDEO &&
                vstream->codecpar->codec_id != AV_CODEC_ID_MPEG1VIDEO)
                {
                    break;
                }
            }
            if(astream != NULL)
            {
                if (astream->codecpar->codec_id != AV_CODEC_ID_MP3 &&
                    astream->codecpar->codec_id != AV_CODEC_ID_MP2 &&
                    astream->codecpar->codec_id != AV_CODEC_ID_AC3) {
                    break;
                }
            }
            canPlay = true;
            break;

    case FILE_WAV:
        //  WAV: support 16bit pcm, a-law, u-law WAV
        if (vstream != NULL) {
            break;
        }
        if (astream != NULL) {
            //  support PCM, a-law and u-law, WAVE_FORMAT_IEEE_FLOAT and WAVE_FORMAT_EXTENSIBLE are not supported
            if (astream->codecpar->codec_tag != 1 &&    // PCM
                astream->codecpar->codec_tag != 6 &&    // a-law
                astream->codecpar->codec_tag != 7 )  {  // u-law
                emit newErrMsg(tr("WAVE_FORMAT_IEEE_FLOAT and WAVE_FORMAT_EXTENSIBLE are not supported...."));
                break;
            }

            //  only support mono and stereo
            if (astream->codecpar->channels > 2) {
                break;
            }

            //  only support 8bit/16bit per sample
            if (av_get_bits_per_sample(astream->codecpar->codec_id) != 8 &&
                av_get_bits_per_sample(astream->codecpar->codec_id) != 16) {
                break;
            }
        }
        canPlay = true;
        break;

    case FILE_AAC:
        //  AAC: support ADTS aac, .m4a(support max 2 channels)
        if (vstream != NULL) {
            break;
        }
        if (astream != NULL) {
            if (astream->codecpar->codec_id != AV_CODEC_ID_AAC) {
                break;
            }

            if (astream->codecpar->channels > 2) {
                break;
            }
        }
        canPlay = true;
        break;

    case FILE_TS:
        //  TS Stream: support MPEG2+AC3, MPEG2+MP3
        if (vstream != NULL) {
            if (vstream->codecpar->codec_id != AV_CODEC_ID_MPEG2VIDEO) {
                break;
            }
        }
        if (astream != NULL) {
            if (astream->codecpar->codec_id != AV_CODEC_ID_MP3 &&
                astream->codecpar->codec_id != AV_CODEC_ID_MP2 &&
                astream->codecpar->codec_id != AV_CODEC_ID_AC3) {
                    break;
            }
        }
        canPlay = true;
        break;

    case FILE_VOB:
        //  VOB file: support MPEG2+AC3, MPEG2+MP3
        if (vstream != NULL) {
            if (vstream->codecpar->codec_id != AV_CODEC_ID_MPEG2VIDEO &&
                vstream->codecpar->codec_id != AV_CODEC_ID_MPEG1VIDEO) {
                break;
            }
        }
        if (astream != NULL) {
            if (astream->codecpar->codec_id != AV_CODEC_ID_MP3 &&
                astream->codecpar->codec_id != AV_CODEC_ID_MP2 &&
                astream->codecpar->codec_id != AV_CODEC_ID_AC3) {
                    break;
            }
        }
        canPlay = true;
        break;
    default:
        break;
    }

   avformat_close_input(&formatCtx);

    if ( !canPlay ) {
       // LOG_ERR("[ERR] Media file %s is not supported\n", file.filepath);
        emit newErrMsg(tr(" 该文件不支持 %1").arg(movFP));
    }else {
        varLs <<  vLenMap ;
    }


    return varLs;
}


void TMainWnd::runAllStreamers()
{

    g_cbStreamerLs.clear();
    QMapIterator <int ,QList<QVariantList> > i (m_rtspChannLs);

    while(i.hasNext())
    {
        i.next();

        int chnnaNum = i.key();

        QList<QVariantList> tmpLs = i.value();

       // qDebug() << chnnaNum << "  =  " << tmpLs;

        if(tmpLs.isEmpty()) continue;

         // qDebug() << tmpLs.size();


        void *stre = createStreamer(tmpLs , chnnaNum );

        Streamer *mystre =(Streamer *) stre;
        mystre->setCallBackFunc(myStreamerCallBack);
        g_cbStreamerLs << mystre;

      //  auto strePtr = reinterpret_cast<std::uintptr_t>(stre);  //C++11
        qulonglong strePtr = (qulonglong)(stre);

        QListWidgetItem *item = new QListWidgetItem(tr("%1").arg(chnnaNum));

        item->setData(Qt::UserRole + 1 ,  mystre->getRTSPUrl());

        item->setData(Qt::UserRole + 2 ,  mystre->getCurStatus() );  //playstatus

        QFileInfo tmpFile(mystre->getMediaFileUrl());
        QTime myTime(0,0,0)  ;
        myTime = myTime.addSecs(mystre->getDuration());
        QString medStr = tr(" %1   时长：%2").arg(tmpFile.fileName()) .arg(myTime.toString("hh:mm:ss"));
        item->setData(Qt::UserRole + 3 ,  medStr);

        item->setData(Qt::UserRole + 4 , QVariant((qulonglong) strePtr) );

        QString allFileName ;
        for(int aa = 0 ; aa < tmpLs.size() ; aa ++){
            QVariantList vl = tmpLs.at(aa);
            if(vl.isEmpty()) continue;
            QVariantMap mm = vl.at(0).toMap();
            QFileInfo fi(mm["pathName"].toString());
            if(fi.exists())
              allFileName += fi.fileName() + ", ";
        }

        allFileName.chop(2);
        QString mediaFilesStr = tr("循环视频： %1").arg(allFileName);

        item->setToolTip(mediaFilesStr);

        m_channListWid->addItem(item);


    }

    m_currChannNumLE->setText(QString::number( m_channListWid->count()) );

    QTimer::singleShot(0,this, SLOT(updateInfoForCallBack()));

    QTimer::singleShot(2000, this, SLOT(batchRunActClick()));  //start streaming.....

}

void TMainWnd::saveLogClick()
{
    QString fn = tr("/mnt/usb/srv_log_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString pathName = QFileDialog::getSaveFileName(this,tr("选择保存路径...."),fn,tr("Text (*.txt)"));

    if(pathName.isEmpty()) return;

    if( m_logTe->toPlainText().trimmed().isEmpty()) return;

    QFile f(pathName);
    if(f.open(QIODevice::WriteOnly)){
        f.write(m_logTe->toPlainText().trimmed().toUtf8());
        f.close();
    }

}

void TMainWnd::chgeLogMaxLine(quint16 l)
{
    m_logTe->document()->setMaximumBlockCount(l);
}

void TMainWnd::logMaxLineSpierClick(int l)
{
    m_logTe->document()->setMaximumBlockCount(l);
}

void TMainWnd::chkUdpDataClick()
{
    QDialog  tmpWid  ;
    tmpWid.setWindowModality(Qt::ApplicationModal);
    tmpWid.setWindowTitle(tr("UDP广播内容"));

    QVBoxLayout  tmpLO  ;
    tmpWid.setLayout(&tmpLO);

    QTextEdit te;

    tmpLO.addWidget(&te);

    te.setText("sadfasfasf sadf asdf as ");

    tmpWid.resize(600,500);
    tmpWid.show();

    QEventLoop evtloop;


    connect(&tmpWid, SIGNAL(accepted()) , &evtloop, SLOT(quit()))  ;
    connect(&tmpWid, SIGNAL(rejected()) , &evtloop, SLOT(quit()))  ;

    evtloop.exec();


}

void TMainWnd::rescanMediaBtnClick()
{
    //remove all channels first.....

    //scanMedaiFile();
}

void TMainWnd::showListCntxMenu(const QPoint &point)
{

   // m_sendMsgAct->setEnabled(m_tableView->currentIndex().isValid());

   m_listPopMenu->popup( mapToGlobal (point) + QPoint(24 , 200)  );
    // m_listPopMenu->popup(   (point) );
}

void TMainWnd::showMainWndCntxMenu(const QPoint &point)
{

   m_mainWndMenu->popup( mapToGlobal (point) /*+ QPoint(24 , 200) */ );
    // m_listPopMenu->popup(   (point) );
}
#define MS_DELAY_UPDATE_ 800
void TMainWnd::streamRunFunc(const QListWidgetItem *it)
{
    if(!it) return;

    if(m_thizSrvType != "M"  ) return;

    Streamer *stre = (Streamer *)(void *)(it->data(Qt::UserRole + 4).toULongLong() );

    stre->ReqStart();


    g_cbStreamerLs << stre;
    QTimer::singleShot(MS_DELAY_UPDATE_,this, SLOT(updateInfoForCallBack()));
}

void TMainWnd::streamStopFunc(const QListWidgetItem *it)
{
    if(!it) return;

    Streamer *stre = (Streamer *)(void *)(it->data(Qt::UserRole + 4).toULongLong() );
    stre->ReqStop();

    g_cbStreamerLs << stre;
    QTimer::singleShot(MS_DELAY_UPDATE_,this, SLOT(updateInfoForCallBack()));
}

void TMainWnd::streamPauseFunc(const QListWidgetItem *it)
{
    if(!it) return;

    Streamer *stre = (Streamer *)(void *)(it->data(Qt::UserRole + 4).toULongLong() );

    stre->ReqPause();

    g_cbStreamerLs << stre;

    QTimer::singleShot(MS_DELAY_UPDATE_,this, SLOT(updateInfoForCallBack()));
}



void TMainWnd::channRunClick()
{
    QListWidgetItem *it = m_channListWid->currentItem();
    streamRunFunc(it);
}
void TMainWnd::channPauseClick()
{
    QListWidgetItem *it = m_channListWid->currentItem();
    streamPauseFunc(it);
}
void TMainWnd::channStopClick()
{
    QListWidgetItem *it = m_channListWid->currentItem();
    streamStopFunc(it);
}

void TMainWnd::batchRunActClick()
{
    for(int j = 0 ; j < m_channListWid->count() ; j ++){
        QListWidgetItem *it = m_channListWid->item(j);        
        streamRunFunc(it);
    }

    raise();
}
void TMainWnd::batchPauseActClick()
{
    for(int j = 0 ; j < m_channListWid->count() ; j ++){
        QListWidgetItem *it = m_channListWid->item(j);
        streamPauseFunc(it);
    }
}
void TMainWnd::batchStopActClick()
{
    for(int j = 0 ; j < m_channListWid->count() ; j ++){
        QListWidgetItem *it = m_channListWid->item(j);
        streamStopFunc(it);
    }
}

void TMainWnd::listItemDBClick(QListWidgetItem *it)
{
    streamRunFunc(it);
}


void TMainWnd::channTerminateClick()
{

}

void TMainWnd::sendMessages()
{

      QMessageBox::information(this,tr("提示！"), tr("未添加该功能....") , QMessageBox::Ok);

    //QMessageBox::information(this, tr("Send message"), tr("send message test...."));

}

void TMainWnd::channPreviewClick()
{
    QMessageBox::information(this,tr("提示！"), tr("未添加该功能....") , QMessageBox::Ok);

}

void TMainWnd::errMsgArrived(const QString &msg)
{
    qDebug() << tr("%1").arg(msg);
    m_logTe->append(msg);
}



void * TMainWnd::createStreamer(QList<QVariantList> valLs , int chann)
{
    Streamer *st = NULL;



    QVariantList firstVal = valLs.at(0);          // just get first movclip name from playlist.txt
    if(firstVal.isEmpty()) return st;             // judge this file type....
    QVariantMap varMap = firstVal.at(0).toMap();

   // qDebug() << varMap;

    MediaType mt = (MediaType)varMap.value("fileType").toInt();


    switch (mt)
    {
    case FILE_TS:
    case FILE_MPG:
    case FILE_VOB:
    case FILE_MP3:
    case FILE_AVI:
    case FILE_MP4:
    case FILE_AAC:
        st = GenericStreamer::createNew(valLs,  chann);
        break;
    case FILE_WAV:
        st = WAVStreamer::createNew(valLs ,   chann);
        break;
    }


    return st;
}




void TMainWnd::myStreamerCallBack (void *streamer)
{
    g_cbStreamerLs << ((Streamer *)streamer);
    if(gTMainWnd){
     QTimer::singleShot(0,gTMainWnd, SLOT(updateInfoForCallBack()));
    }
}

void TMainWnd::updateInfoForCallBack()
{

    while(  g_cbStreamerLs.size() > 0){
        Streamer *stre  = g_cbStreamerLs.at(0);
        g_cbStreamerLs.removeAt(0);

        for(int j = 0 ; j < m_channListWid->count()  ; j ++){
            QListWidgetItem *item = m_channListWid->item(j);
            Streamer *tmpStre =(Streamer *)(void *)(item->data(Qt::UserRole + 4).toULongLong());
            if(stre && tmpStre && stre == tmpStre) {

                QFileInfo tmpFile(stre->getMediaFileUrl());
                QTime myTime(0,0,0)  ;
                myTime = myTime.addSecs(stre->getDuration());
                QString medStr = tr(" %1   时长：%2").arg(tmpFile.fileName()) .arg(myTime.toString("hh:mm:ss"));
                item->setData(Qt::UserRole + 3 ,  medStr);

                item->setData(Qt::UserRole + 2 ,  stre->getCurStatus());

                int channNum = item->data(Qt::DisplayRole).toInt();

                if(channNum > 0 && channNum <= m_maxChannSpB->value()){

                    QString tmpUrl = stre->getRTSPUrl();
                    int size = tmpUrl.toUtf8().size();
                    memcpy(m_oldPackageData.requestURL[channNum /*- 1*/] , tmpUrl.toUtf8().constData(),size > 127 ? 127 : size );  ;
                }

                continue;
            }
        }
    }

    m_channListWid->update();

    updateUdpBroadcastData();

}

void TMainWnd::updateUdpBroadcastData()
{
    QByteArray sentBa;
    sentBa.resize(sizeof(m_oldPackageData));
    memcpy(sentBa.data(), &m_oldPackageData,sizeof(m_oldPackageData));

    //sentBa.append(m_extraJsonData.toUtf8());

    //sentBa.append("\r\n");

    TUdpMulticast::Instance().putDataForSent(sentBa);

}

void TMainWnd::maxChannSpinBoxClick(int c)
{
    TMdiDBUtil::Instance().setMaxChannNum(c);
}

void TMainWnd::saveConfClick()
{


}

void TMainWnd::rebootSys()
{
#if TOOT_ARM
    system("reboot");
#endif
}

void TMainWnd::lunchFileManager()
{
    QMessageBox::information(this,tr("提示！"), tr("该功能未完成....") , QMessageBox::Ok);

}

void TMainWnd::updateRunTimeInfo()
{

    m_totalRunTimeSec += 3;

    QDateTime dt(QDate(1,1,1),QTime(0,0,0));
    dt = dt.addSecs(m_totalRunTimeSec);



    m_totalRunTimeLb->setText(tr("%1 天 %2").arg(dt.date().day() - 1) .arg( dt.toString("hh : mm : ss")));
}

void TMainWnd::thizSrvTypeChged(const QString &t)
{

    qDebug() << "  this srv type chaged to : " << t;
     m_thizSrvType = t;

     if(m_thizSrvType == "S"){
         srvTypeBtnGp->button(2)->setChecked(true);
         batchPauseActClick();
     }
     else {
         srvTypeBtnGp->button(1)->setChecked(true);
         batchRunActClick();
     }

}

static void scanFolder(QStringList &fileLs , const QString &folderPath)
{
    QDir mydir(folderPath);
    if(!mydir.exists()) return;

  //  QStringList nameFilt ;
   // nameFilt << "*.avi";
    QFileInfoList list = mydir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot   );
    for(int i = 0 ; i < list.size(); i ++){
         QFileInfo info = list.at(i);
        if(info.isFile()){
            //  if(info.suffix().toLower() == "avi")
                 fileLs.append(info.absoluteFilePath());

        }else{
             scanFolder( fileLs , info.absoluteFilePath());
        }
    }

}



