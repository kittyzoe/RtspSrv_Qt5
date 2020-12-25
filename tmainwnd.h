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



#ifndef TMAINWND_H
#define TMAINWND_H

#include <QWidget>
#include <QFileInfo>
#include <QMap>
#include <QVariant>
#include <QHostAddress>

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QListWidget)
QT_FORWARD_DECLARE_CLASS(QListWidgetItem)
QT_FORWARD_DECLARE_CLASS(QGroupBox)
QT_FORWARD_DECLARE_CLASS(QButtonGroup)
QT_FORWARD_DECLARE_CLASS(QTextEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QSpinBox)



class Streamer;
class TMainWnd : public QWidget
{
    Q_OBJECT



    struct package_data
    {
        int cmd;
        char requestURL[16][128];
    };



public:
    explicit TMainWnd(QWidget *parent = 0);
    ~TMainWnd();


    static void myStreamerCallBack (void *streamer);

signals:
    void meClosed();
    void newErrMsg(const QString & );



public slots:

    void chgeLogMaxLine(quint16 l);

    void errMsgArrived(const QString &msg) ;


    void scanMedaiFile();

protected:
    void timerEvent(QTimerEvent *evt);

private slots:

    void tmrFunc();


    void handleMulticastUdpData(const QByteArray &baData);

    void showListCntxMenu(const QPoint &);
    void showMainWndCntxMenu(const QPoint &);

    void saveLogClick();
    void chkUdpDataClick();
    void logMaxLineSpierClick(int);
    void sendMessages();


    void channRunClick();
    void channPauseClick();
    void channStopClick();
    void channTerminateClick();

    void channPreviewClick();

    void batchRunActClick();
    void batchStopActClick();
    void batchPauseActClick();

    void streamRunFunc(const QListWidgetItem *it);
    void streamStopFunc(const QListWidgetItem *it);
    void streamPauseFunc(const QListWidgetItem *it);

    void listItemDBClick(  QListWidgetItem *it);

    void saveConfClick();
    void rebootSys();

    void runAllStreamers();
    void rescanMediaBtnClick();

    void maxChannSpinBoxClick(int);

    void updateInfoForCallBack();
    void updateUdpBroadcastData();

    void lunchFileManager();

    void updateRunTimeInfo();

    void thizSrvTypeChged(const QString &);

private:
    void initData();
    void setupUi();
    void initData2();

    void setupPopupMenu();

    void parsePlayList(const QFileInfo &p, int chann);
    QVariantList parseMediaFile(const QFileInfo &p);

    void * createStreamer( QList<QVariantList> ,   int chann);


    QTextEdit *m_logTe;
    QLabel *m_thizIpLb;

    QListWidget *m_channListWid;
    QLineEdit *m_scanPathLe;
    QSpinBox *m_maxChannSpB;
    QLineEdit *m_currChannNumLE;
    QLabel *m_totalRunTimeLb;


    QTimer *m_oneSecTmr;

    QMenu *m_listPopMenu;
    QMenu *m_mainWndMenu;

    QButtonGroup *srvTypeBtnGp;




    QStringList m_supportedFileType;
    //QList< QMap<int ,QList<QVariantList> > > m_rtspChannLs;
    QMap<int ,QList<QVariantList> >  m_rtspChannLs;


    struct package_data m_oldPackageData;
    QString m_extraJsonData;

    quint32 m_totalRunTimeSec;


    QString m_thizSrvType;


};

#endif // TMAINWND_H
