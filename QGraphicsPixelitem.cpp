#include "QGraphicsPixelItem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include "Engine.h"
#include <QDebug>

QGraphicsPixelItem::QGraphicsPixelItem(QVector<QVector<Tile>>& allTilesList) :
    QGraphicsItem()
  , allTiles(allTilesList)
  , width(100)
  , height(100)
{
    setCacheMode(QGraphicsItem::NoCache);
}

void QGraphicsPixelItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->save();

    foreach(const QVector<Tile>& tiles, allTiles) {
        foreach( const Tile& tile, tiles){
            // set your pen color etc.
            QColor color = Mat::MaterialToColorMap[tile.element->material];
            painter->setPen(color);
            painter->drawPoint(QPoint(tile.xPos, tile.yPos));
        }
    }

    painter->restore();
}
