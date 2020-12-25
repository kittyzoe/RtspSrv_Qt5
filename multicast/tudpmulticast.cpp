/**************************************************************************
**   Copyright @ 2010 TOOTzoe.com
**   Special keywords: zoe 12/10/2016 2016
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

#if TOOT_LINUX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include "tudpmulticast.h"




TUdpMulticast *   TUdpMulticast::thizInstance = 0;


TUdpMulticast::TUdpMulticast(bool b, QObject *parent) : QObject(parent)
  ,m_sendTmr(0)
  ,m_needListen(b)
  ,m_mycount(0)
  ,m_recCount(0)
{
    initData();


    m_sendTmr = new QTimer(this);
    m_sendTmr->setInterval(1000);
    connect(m_sendTmr  , SIGNAL(timeout()) , SLOT(tmrSendData()));

}



TUdpMulticast::~TUdpMulticast()
{
    qDebug() << __func__ << " called......";

}

TUdpMulticast &TUdpMulticast::Instance(bool m_needListen)
{
    if( thizInstance == 0){
        thizInstance = new TUdpMulticast(m_needListen);
    }

    return *thizInstance;
}

void TUdpMulticast::initData()
{
    brocastip = "239.1.1.1";
    m_mySrvType = "A";

    #if TOOT_LINUX

    TivaPort = 9004;


    if(m_needListen){
        TivaUdpSocket.bind(TivaPort, QUdpSocket::ShareAddress );
        int TivaSendingFd = TivaUdpSocket.socketDescriptor();

        if(TivaSendingFd != -1)
        {
            struct sockaddr_in TivaSendAddr;
            memset(&TivaSendAddr,0,sizeof(TivaSendAddr));
            TivaSendAddr.sin_family=AF_INET;
            TivaSendAddr.sin_addr.s_addr=inet_addr(brocastip.toUtf8().constData()); //Chosen Broadcast address!!
            TivaSendAddr.sin_port=htons(TivaPort);


            ip_mreq mreq;
            memset(&mreq,0,sizeof(ip_mreq));
            mreq.imr_multiaddr.s_addr = inet_addr(brocastip.toUtf8().constData()); // group addr
            mreq.imr_interface.s_addr = htons(INADDR_ANY); // use default

            //Make this a member of the Multicast Group

            if(setsockopt(TivaSendingFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq,sizeof(mreq)) < 0)
            {
                //QMessageBox::about(this,"Tiva","TivaNet Memship Error!");
                emit errorOccured(QString("TivaNet Memship Error!"));
            }

            // set TTL (Time To Live)
            unsigned int ttl = 38; // restricted to 38 hops
            if (setsockopt(TivaSendingFd, IPPROTO_IP, IP_MULTICAST_TTL, (const char *)&ttl, sizeof(ttl) ) < 0)
            {
                //QMessageBox::about(this,"Tiva","TimeTo Live Error!");
                emit errorOccured(QString("TivaNet Live Error!"));

            }
        }
        connect(&TivaUdpSocket, SIGNAL(readyRead()),   SLOT(multicastReadReady()));


    }

#if 1
    //Set up a socket for sending multicast data

    TivaSocketId = socket(AF_INET, SOCK_DGRAM, 0);

    ip_mreq TivaMreq;

    TivaSinStruct.sin_family = AF_INET;
    TivaSinStruct.sin_port = htons(TivaPort);
    TivaSinStruct.sin_addr.s_addr = inet_addr(brocastip.toUtf8().constData());
    unsigned int ttl = 38; // 38 hops allowed
    setsockopt(TivaSocketId, IPPROTO_IP, IP_MULTICAST_TTL, (const char *)&ttl, sizeof(ttl) );

    memset(&TivaMreq,0,sizeof(ip_mreq));
    TivaMreq.imr_multiaddr.s_addr = inet_addr(brocastip.toUtf8().constData()); // group addr
    TivaMreq.imr_interface.s_addr = htons(INADDR_ANY); // use default

    setsockopt(TivaSocketId, IPPROTO_IP, IP_ADD_MEMBERSHIP,  (char *)&TivaMreq,sizeof(TivaMreq));
#endif

#else
    groupAddress = QHostAddress("239.1.1.1");
    udpSocket = new QUdpSocket(this);


    TivaUdpSocket.bind(QHostAddress::AnyIPv4, 9004, QUdpSocket::ShareAddress);
    TivaUdpSocket.joinMulticastGroup(groupAddress);

    connect(&TivaUdpSocket, SIGNAL(readyRead()),
            this, SLOT(multicastReadReady()));


#endif


}


void TUdpMulticast::multicastReadReady()
{

    QHostAddress PeerIp;
    quint16 por=TivaPort;

    while (TivaUdpSocket.hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(TivaUdpSocket.pendingDatagramSize());
        TivaUdpSocket.readDatagram(datagram.data(), datagram.size(),&PeerIp,&por);

        if(  ( PeerIp.toIPv4Address() >> 8 ) != (m_thisSvrIpLs.at(0).toIPv4Address() >> 8) )
            continue;

        bool ismyself = false;
        foreach (QHostAddress ip, m_thisSvrIpLs) {
            if(PeerIp == ip){
                ismyself = true;
               // qDebug() << " ip : " << ip << "  is myself, ignore...";
                break;
            }
        }

        if(!ismyself){

            int tmpCmd =  9001;   // old server use this number
           // tmpCmd = ((m_thisSvrIpLs.at(0).toIPv4Address()) >> 8) & 0xFF;

          //  qDebug() << " msg: " << QString(datagram) << " size: " << datagram.size() << "  ip=" << PeerIp;
            struct package_data msgData1;
            memcpy(&msgData1, datagram.constData(), datagram.size());

//            qDebug() << " cmd: " << msgData1.cmd;
//                    for(int i = 0 ;  i < 16; i ++)
//                        qDebug() << " i: " << i << "  url=" << msgData1.requestURL[i];


             if( msgData1.cmd != tmpCmd ) return ;

             m_recCount = 0;

            QString data15 = QString(msgData1.requestURL[15]);
            //qDebug() << data15;

            QString tmpType = data15.left(1);
            quint32 tmpVal = data15.mid(1).toInt();

            qDebug() << " T  = " << tmpType << " val = " << tmpVal;

            if( tmpType == "A"){
                if(m_mySrvType == "M"){}
                else if(m_mySrvType == "S"){}


                else if(m_mySrvType == "A"){

                    if(tmpVal == m_mycount) {       // try negenation again
                        m_mycount += qrand() % 10;
                        return;
                    }else if(tmpVal > m_mycount){   // myself will act as slave
                        m_mySrvType = "S";
                        emit srvTypeChged(m_mySrvType);
                    }else if(tmpVal < m_mycount ){   // this srv will act as master
                        m_mySrvType = "M";
                        emit srvTypeChged(m_mySrvType);
                    }
                }
            }else if ( tmpType == "M" ){
                if(m_mySrvType != "S"){
                    m_mySrvType  = "S";
                    emit srvTypeChged(m_mySrvType);
                }
            }else if ( tmpType == "S" ){
                if(m_mySrvType != "M"){
                    m_mySrvType  = "M";
                    emit srvTypeChged(m_mySrvType);
                }
            }



        }


    }

}

void TUdpMulticast::tmrSendData()
{
#if TOOT_LINUX
    if(!m_dataForSend.isEmpty())
      sendto(TivaSocketId, m_dataForSend.constData(), m_dataForSend.size(), 0,	(struct sockaddr *)&TivaSinStruct, sizeof(TivaSinStruct));
#else
   // qDebug() << __func__;

    if( m_mySrvType  != "S") {
        struct package_data *msgData = (struct package_data  *)m_dataForSend.data() ;

        QString tmpStr = tr("%1%2").arg(m_mySrvType).arg(m_mycount);

        memcpy(&msgData->requestURL[15], tmpStr.toUtf8().constData(), tmpStr.toUtf8().size());


        udpSocket->writeDatagram(m_dataForSend.data(), m_dataForSend.size(),
                                 groupAddress, 9004);

    }

    m_mycount++;

   // if(m_mySrvType  != "M" )
    m_recCount ++;

    if(m_recCount > 6){
        m_recCount = 0 ;
        m_mySrvType  = "M";
        emit srvTypeChged(m_mySrvType);
    }

#endif
}

void TUdpMulticast::startStopTmrSend(bool isS)
{
    if(m_sendTmr) {
        if(isS && !m_sendTmr->isActive())
            m_sendTmr->start();
        else
            m_sendTmr->stop();
    }
}

//struct package_data
//{
//    int cmd;
//    char requestURL[16][128];
//};

void TUdpMulticast::putDataForSent(const QByteArray &baData)
{
#if toot
    struct package_data msgData;
    memcpy(&msgData, baData.constData(), baData.size());

    for(int i = 0 ;  i < 16; i ++)
    {
        QString url = QString(msgData.requestURL[i]);
        if(url.isEmpty()) continue;

         qDebug() << url   ;
    }
#endif
    m_dataForSend = baData;

}

void TUdpMulticast::sendData(const QByteArray &databa)
{
    #if TOOT_LINUX
    if(!databa.isEmpty())
    sendto(TivaSocketId, databa.constData(), databa.size(), 0,	(struct sockaddr *)&TivaSinStruct, sizeof(TivaSinStruct));
#endif
}



void TUdpMulticast::sendMCMsg(const QString &msg)
{
    #if TOOT_LINUX
    sendto(TivaSocketId, msg.toUtf8(), msg.toUtf8().size(), 0,	(struct sockaddr *)&TivaSinStruct, sizeof(TivaSinStruct));
#endif
}


