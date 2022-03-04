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

class QGraphicsEngineItem : public QGraphicsItem{

public:

    explicit QGraphicsEngineItem(QVector<QVector<Tile>>& allTilesList );

    QRectF boundingRect() const override{
        return QRectF(0, 0, width, height);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

public:

    QVector<QVector<Tile>>& allTiles;
    QImage tileImage;
    int width;
    int height;
};


#endif // QGRAPHICSENGINEITEM_H
