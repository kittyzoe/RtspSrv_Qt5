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



#ifndef TUDPMULTICAST_H
#define TUDPMULTICAST_H

#if TOOT_LINUX
#include <netinet/in.h>
#else
#include <QHostAddress>

QT_BEGIN_NAMESPACE
class QUdpSocket;
QT_END_NAMESPACE
#endif

#include <QObject>


#include <QtNetwork/QUdpSocket>

#include <QtCore/QTimer>

class TUdpMulticast : public QObject
{

    Q_OBJECT


    struct package_data
    {
        int cmd;
        char requestURL[16][128];
    };


public:

    static TUdpMulticast &Instance(bool m_needListen = /*false*/true);
    //explicit TUdpMulticast(QObject *parent = 0);
    virtual ~TUdpMulticast();


    void keepHostIpLs(  QList<QHostAddress> ipLs){m_thisSvrIpLs = ipLs;}

signals:
    void errorOccured(const QString &err);

    void srvTypeChged(const QString &);

public slots:
    void sendData(const QByteArray &ba);

    void startStopTmrSend(bool isStart = true);

    void putDataForSent(const QByteArray &ba);

protected:

    TUdpMulticast(bool b, QObject *parent = 0);


private slots:
    void multicastReadReady();

    void sendMCMsg(const QString &msg);

    void tmrSendData();


private:

    void initData();




    QUdpSocket TivaUdpSocket;
    int TivaSocketId;

    #if TOOT_LINUX
    struct sockaddr_in TivaSinStruct;

#else
    QUdpSocket *udpSocket;
    //QUdpSocket *recUdpSocket;
    QHostAddress groupAddress;
    #endif
    quint16 TivaPort  ;
    QString brocastip;




    QTimer *m_sendTmr;

    bool m_needListen;


    QByteArray m_dataForSend;
    quint32 m_mycount;
    quint32 m_recCount;
    QList< QHostAddress > m_thisSvrIpLs;
    QString m_mySrvType;

private:

   // TUdpMulticastPrivate * const d_ptr;

    TUdpMulticast(const TUdpMulticast &);

    static TUdpMulticast *   thizInstance;

    // Q_DECLARE_PRIVATE(TUdpMulticast)
    //Q_DISABLE_COPY(TUdpMulticast)


};

#endif // TUDPMULTICAST_H
