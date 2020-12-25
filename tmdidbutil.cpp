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

#include <QtCore>



#include "tmdidbutil.h"





TMdiDBUtil *TMdiDBUtil::thizInstance = 0;



TMdiDBUtil::TMdiDBUtil(QObject *parent ) :  QObject(parent)
{
    m_configFile = _SRV_CONFIG_FILENAME;
}

TMdiDBUtil::~TMdiDBUtil()
{

  //  qDebug() << "TMdiDBUtil destructed called......";
}


TMdiDBUtil &TMdiDBUtil::Instance()
{
    if( thizInstance == 0){
        thizInstance = new TMdiDBUtil;
    }

    return *thizInstance;
}


int TMdiDBUtil::maxChannNum()
{
    QSettings setting( m_configFile,  QSettings::IniFormat );
    setting.beginGroup(_CONFIG_COMMOM_GROUP);

    bool isOK;
    int channN = setting.value(_SRV_MAX_CHANNEL_NUM).toInt(&isOK);
    if(!isOK) channN = 0;

    setting.endGroup();

    return channN;
}

void TMdiDBUtil::setMaxChannNum(int c)
{
    QSettings setting( m_configFile,  QSettings::IniFormat );
    setting.beginGroup(_CONFIG_COMMOM_GROUP);
    setting.setValue(_SRV_MAX_CHANNEL_NUM , c);
    setting.endGroup();
    setting.sync();

}

