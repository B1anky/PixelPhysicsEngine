#ifndef QGRAPHICSENGINEITEM_H
#define QGRAPHICSENGINEITEM_H

#include <QObject>
#include <QGraphicsItem>
#include <QVector>
#include <QRectF>
#include <QImage>
#include "TileSet.h"

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
class Tile;

class QGraphicsEngineItem : public QGraphicsItem{

public:

    explicit QGraphicsEngineItem(QImage& imageToPaint);

    QRectF boundingRect() const override;

    void SetTileSet(TileSet& tileSetToPaint);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

public:
    TileSet  prevFrameTiles;
    QImage& paintImage;
};


#endif // QGRAPHICSENGINEITEM_H
