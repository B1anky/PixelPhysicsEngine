#include "QGraphicsEngineItem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include "TileSet.h"
#include <QDebug>
#include <QPoint>

QGraphicsEngineItem::QGraphicsEngineItem(TileSet& tileSetToPaint) :
    QGraphicsItem()
  , paintTiles(tileSetToPaint)
{
    setCacheMode(QGraphicsItem::NoCache);
}

QRectF QGraphicsEngineItem::boundingRect() const{
    return QRectF(0, 0, paintTiles.width(), paintTiles.height());
}

void QGraphicsEngineItem::SetTileSet(TileSet& tileSetToPaint){
    paintTiles = tileSetToPaint;
}

void QGraphicsEngineItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->save();

    paintTiles.m_readWriteLock.lockForRead();

    foreach(const QVector<Tile>& tiles, paintTiles.m_tileSet) {
        foreach( const Tile& tile, tiles){
            // set your pen color etc.
            QColor color = Mat::MaterialToColorMap[tile.element->material];
            painter->setPen(color);
            painter->drawPoint(tile.position);
        }
    }

    paintTiles.m_readWriteLock.unlock();

    painter->restore();
}
