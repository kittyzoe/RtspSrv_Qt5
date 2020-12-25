/**************************************************************************
**   Copyright @ 2012 QOOCC.com
**   Special keywords: hzf 2012/12/12 2012
**   Environment variables: 
**
**
**
**   E-mail : toot@tootzeo.com
**   Tel    : 13712943464
**   Website: http://www.qoocc.com
**
**************************************************************************/



//------------------------------------------------------------------------

#include "QtGui/QtGui"




#include "tcommlistdelegate.h"


//extern QPixmap cached(const QString &img);

enum StreamStatus {
    ST_NONE,
    ST_INIT,
    ST_PLAY,
    ST_PAUSE,
    ST_STOP,
    ST_SEEK
};

TCommListDelegate::TCommListDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}




void TCommListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect rc = option.rect;
    bool isSelect = option.state & QStyle::State_Selected;
    bool isFocus = option.state & QStyle::State_HasFocus;


     StreamStatus playStatus = (StreamStatus)(index.data(Qt::UserRole + 2).toInt());

     QString currStatus = tr("初始状态");
     QColor baseColor =  QColor(255,255, 0 , 150);
    if(playStatus == ST_PLAY){
        currStatus = tr("活动");
        baseColor = QColor(0,255, 0 ,  60);
    }
    else if(playStatus == ST_PAUSE) {
          currStatus = tr("暂停");
          baseColor = QColor(255,255, 0 , 100);
    }
    else if(playStatus == ST_STOP){
        currStatus = tr("停止");
        baseColor = QColor(255,0, 0 , 100);
    }
    else baseColor = QColor(255,255, 255 , 150);


    painter->fillRect(rc, baseColor);



    if(isFocus ){
        //        painter->drawPixmap(rc,cached(":/ListItemFocusBg"));
        baseColor = QColor(62,118, 255 , 150);
        painter->fillRect(rc, QColor(0,255,255,120));
    }


    //gradient
//    QLinearGradient gradient(0, 0, 0, rc.height());
//    gradient.setSpread(QGradient::ReflectSpread);
//    gradient.setColorAt(0.0, baseColor);
//    gradient.setColorAt(0.4, QColor(255,255,255,0));
//    gradient.setColorAt(0.6, QColor(255,255,255,0));
//    gradient.setColorAt(1.0, baseColor);



//     QBrush brush(gradient);
//     painter->setBrush(brush);
     painter->setPen(Qt::NoPen);
     if(isFocus)
      painter->drawRoundRect(1, 1, rc.width() - 2, rc.height() - 2, 4, 4);


     painter->setPen( QPen(QBrush(QColor(94,42,0, 100)) , 4));

     QFont fn = painter->font();
     fn.setBold(false);
     fn.setPixelSize(48);
     painter->setFont(fn);
     painter->drawText(rc.adjusted(2,-1,0,0), index.data(Qt::DisplayRole).toString() );

     painter->setPen( QPen(QBrush(QColor(94,42,0)) , 2));
     fn.setBold(true);
     fn.setPixelSize(20);
     painter->setFont(fn);

    painter->drawText(rc.adjusted(58,2,0,0), index.data(Qt::UserRole + 1).toString() );

    painter->setPen( QPen(QBrush(QColor(0,94,42)) , 1));
    fn.setBold(false);
    fn.setPixelSize(14);
    painter->setFont(fn);
    painter->drawText(rc.adjusted(62,26,0,0), tr("媒体文件：%1").arg( index.data(Qt::UserRole + 3).toString()) );


    if( isSelect){
//        painter->drawPixmap(rc,cached(":/ListItemFocusBg"));

     painter->setPen( (QColor(255,58,112)));



   }

//    else{
//       painter->setPen( Qt::NoPen);
//    }

     //painter->setBrush(Qt::NoBrush);

   // painter->drawRoundRect(2, 2, rc.width() - 4, rc.height() - 4, 6, 6);

    painter->setPen( QPen(QBrush(QColor(200,94,0)) , 1));
    fn.setBold(false);
    fn.setPixelSize(14);
    painter->setFont(fn);

    painter->drawText(rc.adjusted(rc.width() /4 * 3 ,20,0,0),   tr("当前状态： %1").arg(currStatus) );

    painter->drawLine( rc.bottomLeft() - QPoint(0, 1), rc.bottomRight()- QPoint(0, 1));




}


QSize TCommListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int myIndex = index.data(Qt::UserRole).toInt();

    QRect rc = option.rect;


    //if(myIndex == 1) return QSize(rc.width(),54)  ;

    return  QSize(rc.width(),48)  ;
}


//#ifdef TOOT_ANDROID
//#include "gen/moc_tcommlistdelegate.cpp"
//#endif










