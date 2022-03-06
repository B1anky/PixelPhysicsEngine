#ifndef QGRAPHICSPIXELITEM_H
#define QGRAPHICSPIXELITEM_H

#include <QObject>
#include <QGraphicsItem>
#include <QSet>
#include <QRectF>
#include "Elements.h"

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
class Tile;

class QGraphicsPixelItem : public QGraphicsItem{

public:

    explicit QGraphicsPixelItem(QSet<QPoint>& pixelsIn, Mat::Material& engineMaterial);

    QRectF boundingRect() const override{
        return QRectF(0, 0, width_, height_);
    }

    void UpdateExtents(int width, int height);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

public:

    QSet<QPoint>& pixels;
    QImage pixelImage_;
    int x_;
    int y_;
    int width_;
    int height_;
    Mat::Material& currentMaterial;
};

#endif // QGRAPHICSPIXELITEM_H
