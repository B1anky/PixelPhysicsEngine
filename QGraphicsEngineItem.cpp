#include "QGraphicsEngineItem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include "TileSet.h"
#include <QDebug>
#include <QPoint>

QGraphicsEngineItem::QGraphicsEngineItem(TileSet& tileSetToPaint, QImage& imageToPaint) :
    QGraphicsItem()
  , paintTiles(tileSetToPaint)
  , paintImage(imageToPaint)
{
    setCacheMode(QGraphicsItem::NoCache);
}

QRectF QGraphicsEngineItem::boundingRect() const{
    return QRectF(0, 0, paintImage.width(), paintImage.height());
}

void QGraphicsEngineItem::SetTileSet(TileSet& tileSetToPaint){
    paintTiles = tileSetToPaint;
}

void QGraphicsEngineItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->save();

    painter->drawImage(0, 0, paintImage);

    painter->restore();
}
