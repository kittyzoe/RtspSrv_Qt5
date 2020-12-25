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




#ifndef TCOMMLISTDELEGATE_H
#define TCOMMLISTDELEGATE_H

//#include <QStyledItemDelegate>

#if TOOT_ARM
#include <qstyleditemdelegate.h>
#else
//#include "QtWidgets/qstyleditemdelegate.h"
#include <qstyleditemdelegate.h>
#endif


class TCommListDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit TCommListDelegate(QObject *parent = 0);
    

    void paint(QPainter *painter, const QStyleOptionViewItem &option
               , const QModelIndex &index) const;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;




signals:
    
public slots:
    
};

#endif // TCOMMLISTDELEGATE_H
