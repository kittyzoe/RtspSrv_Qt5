/**************************************************************************
**   Copyright @ 2010 TOOTzoe.com
**   Special keywords: Administrator 2012-6-23 2012
**   Environment variables: 
**   To protect a percent sign, use '%'.
**
**
**   E-mail : toot@tootzeo.com
**   Tel    : 13712943464
**   Website: http://www.tootzoe.com
**
**************************************************************************/



//------------------------------------------------------------------------




#ifndef TMDIDBUTIL_H
#define TMDIDBUTIL_H

#include <QObject>

#include <QVariantList>


#define _CONFIG_COMMOM_GROUP "COMMOM"
#define _SRV_MAX_CHANNEL_NUM "MaxChannelNum"
#define _SRV_CONFIG_FILENAME "srv.conf"


class TMdiDBUtil :   public QObject
{
public:
    ~TMdiDBUtil();

     static TMdiDBUtil &Instance();

public:


     int maxChannNum();
     void setMaxChannNum(int c);



private: // disable copying
        TMdiDBUtil( QObject *parent = 0);
        TMdiDBUtil(const TMdiDBUtil &);
        const TMdiDBUtil &operator = (const TMdiDBUtil &);



        QString m_configFile;

private:


        static TMdiDBUtil *thizInstance;


};

#endif // TMDIDBUTIL_H
