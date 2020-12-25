/**************************************************************************
**   Copyright @ 2010 TOOTzoe.com
**   Special keywords: zoe 22/9/2016 2016
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



#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#include <QtWidgets>
#include <QtNetwork>

#include "tmainwnd.h"




QUdpSocket TivaUdpSocket;

int TivaSocketId;

struct sockaddr_in TivaSinStruct;
quint16 TivaPort = 9004;

const char* brocastip = "239.1.1.1";

TMainWnd::TMainWnd(QWidget *parent) : QWidget(parent)
{

    QVBoxLayout *mainLO = new QVBoxLayout;

    QPushButton *sendBtn = new QPushButton(tr("Send....."));
    connect(sendBtn, SIGNAL(pressed()) , SLOT(sendBtnClick()));

    QPushButton *sendBtn2 = new QPushButton(tr("Send...22.."));
    connect(sendBtn2, SIGNAL(pressed()) , SLOT(sendBtnClick2()));

    mainLO->addWidget(sendBtn);
    mainLO->addWidget(sendBtn2);

    setLayout(mainLO);

    qsrand(QTime::currentTime().msec());

    resize(600,400);

     QTimer::singleShot( 1000 , this , SLOT(func()));
    //QTimer::singleShot( 1000 , this , SLOT(func2()));
}


void TMainWnd::func()
{


    TivaUdpSocket.bind(TivaPort, QUdpSocket::ShareAddress );
    int TivaSendingFd = TivaUdpSocket.socketDescriptor();


    if(TivaSendingFd != -1)
    {
        struct sockaddr_in TivaSendAddr;
        memset(&TivaSendAddr,0,sizeof(TivaSendAddr));
        TivaSendAddr.sin_family=AF_INET;
        TivaSendAddr.sin_addr.s_addr=inet_addr(brocastip); //Chosen Broadcast address!!
        TivaSendAddr.sin_port=htons(TivaPort);


        ip_mreq mreq;
        memset(&mreq,0,sizeof(ip_mreq));
        mreq.imr_multiaddr.s_addr = inet_addr(brocastip); // group addr
        mreq.imr_interface.s_addr = htons(INADDR_ANY); // use default

        //Make this a member of the Multicast Group

        if(setsockopt(TivaSendingFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq,sizeof(mreq)) < 0)
        {
            QMessageBox::about(this,"Tiva","TivaNet Memship Error!");
        }

        // set TTL (Time To Live)
        unsigned int ttl = 38; // restricted to 38 hops
        if (setsockopt(TivaSendingFd, IPPROTO_IP, IP_MULTICAST_TTL, (const char *)&ttl, sizeof(ttl) ) < 0)
        {
            QMessageBox::about(this,"Tiva","TimeTo Live Error!");

        }
    }
    connect(&TivaUdpSocket, SIGNAL(readyRead()),   SLOT(multicastReadReady()));



#if 1
    //Set up a socket for sending multicast data

    TivaSocketId = socket(AF_INET, SOCK_DGRAM, 0);

    ip_mreq TivaMreq;

    TivaSinStruct.sin_family = AF_INET;
    TivaSinStruct.sin_port = htons(TivaPort);
    TivaSinStruct.sin_addr.s_addr = inet_addr(brocastip);
    unsigned int ttl = 38; // 38 hops allowed
    setsockopt(TivaSocketId, IPPROTO_IP, IP_MULTICAST_TTL, (const char *)&ttl, sizeof(ttl) );

    memset(&TivaMreq,0,sizeof(ip_mreq));
    TivaMreq.imr_multiaddr.s_addr = inet_addr(brocastip); // group addr
    TivaMreq.imr_interface.s_addr = htons(INADDR_ANY); // use default

    setsockopt(TivaSocketId, IPPROTO_IP, IP_ADD_MEMBERSHIP,  (char *)&TivaMreq,sizeof(TivaMreq));
#endif

}



void TMainWnd::func2()
{


//    while(!(udps->isValid()) || udps->state() != QUdpSocket::BoundState ){
//        udps->abort();
//        udps->close();
//        delete udps;
//        udps = new QUdpSocket;
qDebug() << "while!";

        if (!udps.bind(9001, QUdpSocket::ShareAddress  | QUdpSocket::ReuseAddressHint ))
        {
            qDebug() << "#PlayControlThread# udp socket bind error!";
            qDebug() << udps.errorString();
          //  udps->abort();

           // QTimer::singleShot( 1000 , this , SLOT(func2()));
           // return;
        }

        udps.setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);


   // }


sendBtnClick2();


}

void TMainWnd::sendMCMsg(const QString &msg)
{
    sendto(TivaSocketId, msg.toUtf8(), msg.toUtf8().size(), 0,	(struct sockaddr *)&TivaSinStruct, sizeof(TivaSinStruct));

}


struct package_data
{
    int cmd;
    char requestURL[16][128];
};

void TMainWnd::multicastReadReady()
{
    QString OInMessage;
    QHostAddress PeerIp;
    quint16 por=TivaPort;

    while (TivaUdpSocket.hasPendingDatagrams())
    {
    QByteArray datagram;
    datagram.resize(TivaUdpSocket.pendingDatagramSize());
    TivaUdpSocket.readDatagram(datagram.data(), datagram.size(),&PeerIp,&por);
    OInMessage=datagram.data();

    qDebug() << " msg: " << OInMessage << " size: " << datagram.size() << "  ip=" << PeerIp;
    struct package_data msgData;
    memcpy(&msgData, datagram.constData(), datagram.size());
    qDebug() << " cmd: " << msgData.cmd;
    for(int i = 0 ;  i < 16; i ++)
        qDebug() << " i: " << i << "  url=" << msgData.requestURL[i];

    }
}

void TMainWnd::sendBtnClick()
{

    //sendMCMsg(tr(  "this is tootzoe %1").arg( qrand()));

    QByteArray databa;
    int cmd = 9001;
    databa.append((char *)&cmd, sizeof(int));

    for(int i = 0 ; i < 16 ; i ++){
        QString str = "rtsp://192.168.1.88:8554/genericStream0";
         //str = "rtsp://192.168.1.88:8554/testStream";
        databa.append(str);
        databa.append(128 - str.length() , '\0' );
    }

    sendto(TivaSocketId, databa.constData(), databa.size(), 0,	(struct sockaddr *)&TivaSinStruct, sizeof(TivaSinStruct));

    QTimer::singleShot( 1000 , this , SLOT(sendBtnClick()));

}

void TMainWnd::sendBtnClick2()
{

    qDebug() << "sendBtnClick2......";
    QString tmpStr = "tootzoe use qt udp ..........." + QString::number(qrand());
    udps.writeDatagram(tmpStr.toUtf8(), tmpStr.toUtf8().size(),QHostAddress("239.1.1.1") , 9004 );

    //sendMCMsg(tr(  "this is tootzoe %1").arg( qrand()));

}






