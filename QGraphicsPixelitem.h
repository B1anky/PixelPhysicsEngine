#ifndef QGRAPHICSPIXELITEM_H
#define QGRAPHICSPIXELITEM_H

#include <QObject>
#include <QGraphicsItem>
#include <QVector>
#include <QRectF>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
class Tile;

class QGraphicsPixelItem : public QGraphicsItem
{
public:
    explicit QGraphicsPixelItem(QVector<QVector<Tile>>& allTilesList );
    QRectF boundingRect() const override{
        return QRectF(0, 0, width, height);
    }
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
    QVector<QVector<Tile>>& allTiles;
    int width;
    int height;
};

#endif // QGRAPHICSPIXELITEM_H
