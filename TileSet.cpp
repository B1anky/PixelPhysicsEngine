#include "TileSet.h"
#include "Engine.h"

TileSet::TileSet()
{

}

TileSet::TileSet(const TileSet& tileSet){
    m_tileSet.resize(tileSet.width());
    for(int i = 0; i < tileSet.width(); ++i){
        m_tileSet[i].resize(tileSet.m_tileSet.at(i).size());
        for(int j = 0; j < tileSet.m_tileSet.at(i).size(); ++j){
            m_tileSet[i][j] = Tile(i, j, tileSet.m_tileSet.at(i).at(j).element->material);
        }
    }
}

TileSet& TileSet::operator=(const TileSet& tileSet){
    if(this != &tileSet){
        m_tileSet.resize(tileSet.width());
        for(int i = 0; i < tileSet.width(); ++i){
            m_tileSet[i].resize(tileSet.m_tileSet.at(i).size());
            for(int j = 0; j < tileSet.m_tileSet.at(i).size(); ++j){
                m_tileSet[i][j] = Tile(i, j, tileSet.m_tileSet.at(i).at(j).element->material);
            }
        }
    }
    return *this;
}

bool TileSet::operator==(const TileSet& tileSet) const{
    return m_tileSet == tileSet.m_tileSet;
}

bool TileSet::operator!=(const TileSet& tileSet) const{
    return !(*this == tileSet);
}

void TileSet::Update(){
    RandomTileIter randomOrderIter(QVector<int>(m_tileSet.size()));
    std::iota(randomOrderIter.begin(), randomOrderIter.end(), 0);
    std::random_shuffle(randomOrderIter.begin(), randomOrderIter.end());

    m_readWriteLock.lockForWrite();

    for (int i = 0; i < m_tileSet.size(); ++i) {
        int col = std::min(randomOrderIter.at(i), m_tileSet.size() - 1);
        for (int j = m_tileSet.at(col).size() - 1; j >= 0; --j) {
            Tile& tile = m_tileSet[col][j];
            if(!IsEmpty(tile)){
                tile.Update(*this);
            }
        }
    }

    m_readWriteLock.unlock();

}

// Returns whether the tile is a valid coordinate to check against.
bool TileSet::InBounds(int xPos, int yPos){
    return xPos >= 0 && xPos < m_tileSet.size() && yPos >= 0 && yPos < m_tileSet.at(0).size();
}

// Returns whether the tile is a valid coordinate to check against.
bool TileSet::InBounds(const Tile& tile){
    return InBounds(tile.position);
}

// Returns whether the tile is a valid coordinate to check against.
bool TileSet::InBounds(const QPoint& position){
    return InBounds(position.x(), position.y());
}

// Returns whether the tile at location x, y's material is empty
bool TileSet::IsEmpty(int xPos, int yPos){
    bool empty = false;
    if(InBounds(xPos, yPos)){
        //m_readWriteLock.lockForRead();
        empty = m_tileSet.at(xPos).at(yPos).element->material == Mat::Material::EMPTY;
        //m_readWriteLock.unlock();
    }
    return empty;
}

// Returns whether the tile at location x, y's material is empty
bool TileSet::IsEmpty(const QPoint& position){
    return IsEmpty(position.x(), position.y());
}

// Returns whether the tile at location x, y's material is empty
bool TileSet::IsEmpty(const Tile& tile){
    return tile.element->material == Mat::Material::EMPTY;
}

// Controls setting tiles at a particular location.
void TileSet::SetTile(const Tile& tile){
    m_readWriteLock.lockForWrite();
    if(InBounds(tile)){
        m_tileSet[tile.position.x()][tile.position.y()] = tile;
    }
    m_readWriteLock.unlock();
}

// Controls setting tiles at a particular location.
void TileSet::SetTile(Tile* tile){
    if(InBounds(*tile)){
        m_readWriteLock.lockForWrite();
        m_tileSet[tile->position.x()][tile->position.y()] = *tile;
        m_readWriteLock.unlock();
    }
}

// Controls setting tiles at a particular location, without locking. The callee must lock.
void TileSet::SetTileBulkUpdate(Tile tile){
    if(InBounds(tile)){
        m_tileSet[tile.position.x()][tile.position.y()] = tile;
    }
}

// PhysicsWindow will invoke this on a resize event to make the m_tiles match the size of the window.
void TileSet::ResizeTiles(int width, int height, bool initialization){

    int originalWidth  = 0;
    int originalHeight = 0;
    {
        m_readWriteLock.lockForRead();
        originalWidth  = m_tileSet.size();
        originalHeight = originalWidth > 0 ? m_tileSet.at(0).size() : 0;
        m_readWriteLock.unlock();
    }

    if( ( width != originalWidth || height != originalHeight ) || initialization){

        m_readWriteLock.lockForWrite();

        // Determine how much width and height we may have gained to properly set those tiles.
        m_tileSet.resize(width);
        for(int i = 0; i < width; ++i){
            m_tileSet[i].resize(height);
        }

        for(int i = originalWidth; i < width; ++i){
            for(int j = 0; j < height; ++j){
                SetTileBulkUpdate(Tile(i, j, Mat::Material::EMPTY));
            }
        }

        for(int i = 0; i < width; ++i){
            for(int j = originalHeight; j < height; ++j){
                SetTileBulkUpdate(Tile(i, j, Mat::Material::EMPTY));
            }
        }

        m_readWriteLock.unlock();
    }
}

// Convenience for getting a tile at a position.
Tile& TileSet::TileAt(int xPos, int yPos){
    if( !InBounds(xPos, yPos) )
        return InvalidTile;
    return m_tileSet[xPos][yPos];
}

// Convenience for getting a tile at a position.
Tile& TileSet::TileAt(const QPoint& position){
    return TileAt(position.x(), position.y());
}

void TileSet::Swap(int xPos1, int yPos1, int xPos2, int yPos2){
    if (!InBounds(xPos1, yPos1)) return;
    if (!InBounds(xPos2, yPos2)) return;

    //m_tile_mutex.lockForRead();
    Tile& originalTile    = TileAt(xPos1, yPos1);
    Tile& destinationTile = TileAt(xPos2, yPos2);

    originalTile.SwapElements(destinationTile);
}

void TileSet::Swap(const QPoint& pos1, const QPoint& pos2){
    Swap(pos1.x(), pos1.y(), pos2.x(), pos2.y());
}

void TileSet::Swap(const Tile& tile1, const Tile& tile2){
    Swap(tile1.position, tile2.position);
}

void TileSet::ClearTiles(){
    for(int i = 0; i < m_tileSet.size(); ++i){
        for(int j = 0; j < ( m_tileSet.size() > 0 ? m_tileSet.at(0).size() : 0 ); ++j){
            SetTile(Tile(i, j, Mat::Material::EMPTY));
        }
    }
}