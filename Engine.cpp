#include "Engine.h"
#include "QGraphicsEngineItem.h"
#include "QGraphicsPixelItem.h"
#include "Elements.h"
#include <QPoint>
#include "Hashhelpers.h"
#include <random>
#include <time.h>

Tile Engine::InvalidTile;

void Worker::UpdateTiles(){
    m_tile_mutex.lockForWrite();
    QVector<int> localWidths = randomWidths_;
    std::iota(localWidths.begin(), localWidths.end(), 0);
    std::random_shuffle(localWidths.begin(), localWidths.end());

    for (int i = 0; i < dirtyTiles_.size(); ++i) {
        int col = std::min(localWidths.at(i), dirtyTiles_.size() - 1);
        for (int j = dirtyTiles_.at(col).size() - 1; j >= 0; --j) {
            Tile& tile = dirtyTiles_[col][j];
            if(!IsEmpty(tile)){
                tile.Update(engine_, dirtyTiles_);
            }
        }
    }

    allTiles_ = dirtyTiles_;
    m_tile_mutex.unlock();
}

Engine::Engine(int width, int height, QObject* parent) :
    QObject(parent)
  , m_width(width)
  , m_height(height)
  , m_currentMaterial(Mat::Material::EMPTY)
  , m_engineGraphicsItem(nullptr)
  , m_workerThread(m_tiles, randomWidths, this)
{    
    UpdateLoop();
    ResizeTiles(width, height, true);
    srand(time(NULL));
    Engine::InvalidTile.element->density = std::numeric_limits<double>::max();
    ClearTiles(m_tiles);
    m_initialized = false;
}

void Engine::UpdateLoop(){
    m_updateTimer.start(33);
    connect(&m_updateTimer, &QTimer::timeout, this, &Engine::UpdateTiles, Qt::DirectConnection);
}

Engine::~Engine(){
    // Determine if we should clean worker up properly or if it doesn't matter.
}

void Engine::SetEngineGraphicsItem(QGraphicsEngineItem* engineGraphicsItem){
    m_engineGraphicsItem = engineGraphicsItem;
    m_engineGraphicsItem->update();
    m_initialized = true;
    m_workerThread.dirtyTiles_ = m_tiles;
    QThread* thread = new QThread(this);
    m_workerThread.setup(thread, "1");
    m_workerThread.moveToThread(thread);
    thread->start();
}

// Connected to the updateTimer::timeout to control update rates.
void Engine::UpdateTiles(){
    m_engineGraphicsItem->update();
}

// Returns whether the tile is a valid coordinate to check against.
bool Engine::InBounds(int xPos, int yPos, QVector<QVector<Tile> >& tileToCheckAgainst){
    return xPos >= 0 && xPos < tileToCheckAgainst.size() && yPos >= 0 && yPos < tileToCheckAgainst.at(0).size();
}

// Returns whether the tile is a valid coordinate to check against.
bool Engine::InBounds(const Tile& tile, QVector<QVector<Tile> >& tileToCheckAgainst){
    return InBounds(tile.position, tileToCheckAgainst);
}

// Returns whether the tile is a valid coordinate to check against.
bool Engine::InBounds(const QPoint& position, QVector<QVector<Tile> >& tileToCheckAgainst){
    return InBounds(position.x(), position.y(), tileToCheckAgainst);
}

// Returns whether the tile at location x, y's material is empty
bool Engine::IsEmpty(int xPos, int yPos, QVector<QVector<Tile>>& tileToCheckAgainst){
    bool empty = false;
    if(InBounds(xPos, yPos, tileToCheckAgainst)){
        //m_tile_mutex.lockForRead();
        empty = tileToCheckAgainst.at(xPos).at(yPos).element->material == Mat::Material::EMPTY;
        //m_tile_mutex.unlock();
    }
    return empty;
}

// Returns whether the tile at location x, y's material is empty
bool Engine::IsEmpty(const QPoint& position, QVector<QVector<Tile> >& tileToCheckAgainst){
    return IsEmpty(position.x(), position.y(), tileToCheckAgainst);
}

// Returns whether the tile at location x, y's material is empty
bool Engine::IsEmpty(const Tile& tile){
    return tile.element->material == Mat::Material::EMPTY;
}

// Controls setting tiles at a particular location.
void Engine::SetTile( const Tile& tile, QVector<QVector<Tile> >& tileToCheckAgainst){
    m_tile_mutex.lockForWrite();
    if(InBounds(tile, tileToCheckAgainst)){
        tileToCheckAgainst[tile.position.x()][tile.position.y()] = tile;
    }
    m_tile_mutex.unlock();
}

// Controls setting tiles at a particular location.
void Engine::SetTile( Tile* tile, QVector<QVector<Tile> >& tileToCheckAgainst){
    if(InBounds(*tile, tileToCheckAgainst)){
        //m_tile_mutex.lockForWrite();
        tileToCheckAgainst[tile->position.x()][tile->position.y()] = *tile;
        //m_tile_mutex.unlock();
    }
}

// Sets the material that will be inserted on the next mouse-left-click event.
void Engine::SetMaterial(Mat::Material material){
    m_currentMaterial = material;
}

// PhysicsWindow will invoke this on a resize event to make the m_tiles match the size of the window.
void Engine::ResizeTiles(int width, int height, bool initialization){

    int originalWidth  = 0;
    int originalHeight = 0;
    {
        m_tile_mutex.lockForRead();
        originalWidth  = m_width;
        originalHeight = m_height;
        m_tile_mutex.unlock();
    }

    if( ( width != originalWidth || height != originalHeight ) || initialization){

        // Determine how much width and height we may have gained to properly set those tiles.
        {
            m_tile_mutex.lockForWrite();

            m_width  = width;
            m_height = height;

            m_tiles.resize(width);
            for(int i = 0; i < width; ++i){
                m_tiles[i].resize(height);
            }

            m_tile_mutex.unlock();
        }

        for(int i = originalWidth; i < width; ++i){
            for(int j = 0; j < height; ++j){
                SetTile(Tile(i, j, Mat::Material::EMPTY), m_tiles);
            }
        }

        for(int i = 0; i < width; ++i){
            for(int j = originalHeight; j < height; ++j){
                SetTile(Tile(i, j, Mat::Material::EMPTY), m_tiles);
            }
        }

        m_tile_mutex.lockForWrite();

        m_workerThread.dirtyTiles_ = m_tiles;
        randomWidths.resize(width);
        m_tile_mutex.unlock();

        if(m_engineGraphicsItem != nullptr){
            m_engineGraphicsItem->update();
        }
    }
}

// Convenience for getting a tile at a position.
Tile& Engine::TileAt(int xPos, int yPos, QVector<QVector<Tile> >& tileToCheckAgainst){
    if( !InBounds(xPos, yPos, tileToCheckAgainst) )
        return InvalidTile;
    return tileToCheckAgainst[xPos][yPos];
}

// Convenience for getting a tile at a position.
Tile& Engine::TileAt(const QPoint& position, QVector<QVector<Tile> >& tileToCheckAgainst){
    return TileAt(position.x(), position.y(), tileToCheckAgainst);
}

void Engine::Swap(int xPos1, int yPos1, int xPos2, int yPos2, QVector<QVector<Tile> >& tileToCheckAgainst){
    if (!InBounds(xPos1, yPos1, tileToCheckAgainst)) return;
    if (!InBounds(xPos2, yPos2, tileToCheckAgainst)) return;

    //m_tile_mutex.lockForRead();
    Tile& originalTile    = TileAt(xPos1, yPos1, tileToCheckAgainst);
    Tile& destinationTile = TileAt(xPos2, yPos2, tileToCheckAgainst);
    //m_tile_mutex.unlock();

    //m_tile_mutex.lockForWrite();
    originalTile.SwapElements(destinationTile);
    //m_tile_mutex.unlock();
}

void Engine::Swap(const QPoint& pos1, const QPoint& pos2, QVector<QVector<Tile> >& tileToCheckAgainst){
    Swap(pos1.x(), pos1.y(), pos2.x(), pos2.y(), tileToCheckAgainst);
}

void Engine::Swap(const Tile& tile1, const Tile& tile2, QVector<QVector<Tile> >& tileToCheckAgainst){
    Swap(tile1.position, tile2.position, tileToCheckAgainst);
}

void Engine::ClearTiles( QVector<QVector<Tile> >& tileToCheckAgainst ){
    for(int i = 0; i < m_width; ++i ){
        for(int j = 0; j < m_height; ++j){
            SetTile(Tile(i, j, Mat::Material::EMPTY), tileToCheckAgainst);
        }
    }
}
