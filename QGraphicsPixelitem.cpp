#include "QGraphicsPixelItem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>

QGraphicsPixelItem::QGraphicsPixelItem(QVector<QPoint>& pixelsIn, Mat::Material& engineMaterial) :
    QGraphicsItem()
  , pixels(pixelsIn)
  , width(100)
  , height(100)
  , currentMaterial(engineMaterial)
{
    setCacheMode(QGraphicsItem::NoCache);
}

void QGraphicsPixelItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->save();

    QColor alphaMaterialColor(Mat::MaterialToColorMap[currentMaterial]);
    alphaMaterialColor.setAlpha(128);
    painter->setPen(QPen(alphaMaterialColor));

    foreach(const QPoint& point, pixels) {
        painter->drawPoint(point);
    }

    painter->restore();
}
