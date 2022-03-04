#include "QGraphicsEngineItem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include "Engine.h"
#include <QDebug>
#include <QPoint>

QGraphicsEngineItem::QGraphicsEngineItem(QVector<QVector<Tile>>& allTilesList) :
    QGraphicsItem()
  , allTiles(allTilesList)
  , width(100)
  , height(100)
{
    setCacheMode(QGraphicsItem::NoCache);
}

void QGraphicsEngineItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->save();

    //m_tile_mutex.lockForRead();
    foreach(const QVector<Tile>& tiles, allTiles) {
        foreach( const Tile& tile, tiles){
            // set your pen color etc.
            QColor color = Mat::MaterialToColorMap[tile.element->material];
            painter->setPen(color);
            painter->drawPoint(tile.position);
        }
    }
    //m_tile_mutex.unlock();

    painter->restore();
}
