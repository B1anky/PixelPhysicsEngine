#ifndef TILESET_H
#define TILESET_H

#include "Tile.h"
#include <QVector>
#include <QMutex>
#include <QReadWriteLock>

class TileSet
{
public:

    typedef QVector<QVector<Tile>> Tiles;
    typedef QVector<int> RandomTileIter;

    TileSet();

    TileSet(const TileSet& tileSet);

    TileSet& operator=(const TileSet& tileSet);

    bool operator==(const TileSet& tileSet) const;

    bool operator!=(const TileSet& tileSet) const;

    // Pumps these Tiles to update.
    void Update();

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
    void SetTile(const Tile& tile);

    // Controls setting tiles at a particular location. This will add it to the dirty set.
    void SetTile(Tile* tile);

    // Controls setting tiles at a particular location, without locking. The callee must lock.
    void SetTileBulkUpdate(Tile tile);

    // Convenience for getting a tile at a position.
    Tile& TileAt(int xPos, int yPos);

    // Convenience for getting a tile at a position.
    Tile& TileAt(const QPoint& position);

    // Resizes m_tiles to fit to width and height. initialization flag forces through a lazy evaluation.
    void ResizeTiles(int width, int height, bool initialization = false);

    // Swaps the Elements owned by the Tiles at xPos1, yPos1 with the element as xPos2, yPos2.
    void Swap(int xPos1, int yPos1, int xPos2, int yPos2);

    // Swaps the Elements owned by the Tile at pos1 with the element as pos2.
    void Swap(const QPoint& pos1, const QPoint& pos2);

    // Swaps the Elements owned by the tile1 with the element at tile2.
    void Swap(const Tile& tile1, const Tile& tile2);

    // Sets all tiles to be the Mat::Empty material.
    void ClearTiles();

    int width() const{
        return m_tileSet.size();
    }

    int height() const{
        return (m_tileSet.size() > 0 ? m_tileSet.at(0).size() : 0);
    }

public:
    QReadWriteLock m_readWriteLock;
    Tiles          m_tileSet;
};

#endif // TILESET_H
