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



#ifndef TMAINWND_H
#define TMAINWND_H

#include <QWidget>

#include <QtNetwork/QUdpSocket>

class TMainWnd : public QWidget
{
    Q_OBJECT
public:
    explicit TMainWnd(QWidget *parent = 0);






signals:

public slots:

    void func();
    void func2();

    void multicastReadReady();

    void sendMCMsg(const QString &);


private slots:
    void sendBtnClick();
    void sendBtnClick2();

private:
    QUdpSocket  udps ;

};

#endif // TMAINWND_H
