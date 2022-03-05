#ifndef QGRAPHICSENGINEITEM_H
#define QGRAPHICSENGINEITEM_H

#include <QObject>
#include <QGraphicsItem>
#include <QVector>
#include <QRectF>
#include <QImage>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
class Tile;
class TileSet;

class QGraphicsEngineItem : public QGraphicsItem{

public:

    explicit QGraphicsEngineItem(TileSet& tileSetToPaint);

    QRectF boundingRect() const override;

    void SetTileSet(TileSet& tileSetToPaint);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

public:

    TileSet& paintTiles;
};


#endif // QGRAPHICSENGINEITEM_H
