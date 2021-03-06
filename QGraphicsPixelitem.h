#ifndef QGRAPHICSPIXELITEM_H
#define QGRAPHICSPIXELITEM_H

#include <QObject>
#include <QGraphicsItem>
#include <QVector>
#include <QRectF>
#include "Elements.h"

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
class Tile;

class QGraphicsPixelItem : public QGraphicsItem{

public:

    explicit QGraphicsPixelItem(QVector<QPoint>& pixelsIn, Mat::Material& engineMaterial);

    QRectF boundingRect() const override{
        return QRectF(0, 0, width, height);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

public:

    QVector<QPoint>& pixels;
    int width;
    int height;
    Mat::Material& currentMaterial;
};

#endif // QGRAPHICSPIXELITEM_H
