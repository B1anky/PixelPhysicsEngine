#include "QGraphicsPixelItem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>

QGraphicsPixelItem::QGraphicsPixelItem(QSet<QPoint>& pixelsIn, Mat::Material& engineMaterial) :
    QGraphicsItem()
  , pixels(pixelsIn)
  , x_(0)
  , y_(0)
  , width_(0)
  , height_(0)
  , currentMaterial(engineMaterial)
{
    setCacheMode(QGraphicsItem::NoCache);
    setOpacity(0.5);
}

void QGraphicsPixelItem::UpdateExtents(int width, int height){
    if(width_ != width || height_ != height){
        prepareGeometryChange();
        width_ = width;
        height_ = height;
        pixelImage_ = QImage(width_, height_, QImage::Format_ARGB32);
        pixelImage_.fill(QColor(0,0,0,0));
    }
}

void QGraphicsPixelItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->save();

    QColor currentColor(Mat::MaterialToColorMap[currentMaterial]);
    pixelImage_.fill(QColor(0,0,0,0));
    foreach(const QPoint& point, pixels) {
        if(pixelImage_.rect().contains(point)){
            pixelImage_.setPixelColor(point.x(), point.y(), currentColor);
        }
    }
    painter->drawImage(0, 0, pixelImage_);

    painter->restore();
}
