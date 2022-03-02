#ifndef ENGINE_H
#define ENGINE_H

#include "Tile.h"
#include <QObject>
#include <QTimer>
#include <QVector>
#include <QMetaEnum>
#include <QColor>
#include <QPoint>
#include <QSet>

class QGraphicsEngineItem;
class Element;

template<typename QEnum>
QString QtEnumToQString (const QEnum value)
{
  return QString(QMetaEnum::fromType<QEnum>().valueToKey(value));
}

class PhysicsWindow;

class Engine : public QObject
{

    Q_OBJECT

    friend class PhysicsWindow;
    friend class Element;
    friend class GravityAbidingProperty;
    friend class SpreadAbidingProperty;
    friend class Liquid;

public:

    static Tile InvalidTile;

    explicit Engine(int width, int height, QObject* parent = nullptr);

    void SetEngineGraphicsItem(QGraphicsEngineItem* engineGraphicsItem);

    // Returns whether the tile is a valid coordinate to check against.
    bool InBounds(int xPos, int yPos);

    // Returns whether the tile is a valid coordinate to check against.
    bool InBounds(const QPoint& position);

    // Returns whether the tile is a valid coordinate to check against.
    bool InBounds(const Tile& tile);

    // Returns whether the tile at location x, y's material is empty.
    bool IsEmpty(int xPos, int yPos);

    // Returns whether the tile at location x, y's material is empty.
    bool IsEmpty(const QPoint& position);

    // Returns whether the tile at location x, y's material is empty.
    bool IsEmpty(const Tile& tile);

    // Controls setting tiles at a particular location. This will add it to the dirty set.
    void SetTile( const Tile& tile );

    // Controls setting tiles at a particular location. This will add it to the dirty set.
    void SetTile( Tile* tile );

    // Sets the material that will be inserted on the next mouse-left-click event.
    void SetMaterial(Mat::Material material);

    // Convenience for getting a tile at a position.
    Tile& TileAt(int xPos, int yPos);

    // Convenience for getting a tile at a position.
    Tile& TileAt(const QPoint& position);

    void Swap(int xPos1, int yPos1, int xPos2, int yPos2);
    void Swap(const QPoint& pos1, const QPoint& pos2);
    void Swap(const Tile& tile1, const Tile& tile2);

protected:

    // Connected to the updateTimer::timeout to control update rates.
    void UpdateTiles();

    // PhysicsWindow will invoke this on a resize event to make the m_tiles match the size of the window.
    void ResizeTiles(int width, int height);

protected:

    int m_width;
    int m_height;
    Mat::Material m_currentMaterial;
    QVector<QVector<Tile>> m_tiles;
    QVector<int> randomWidths;
    QTimer m_updateTimer;
    QGraphicsEngineItem* m_engineGraphicsItem;

};

#endif // ENGINE_H
