#include "Engine.h"
#include "QGraphicsPixelItem.h"
#include "Elements.h"
#include <QPoint>
#include "Hashhelpers.h"
#include <random>
#include <time.h>

Tile Engine::InvalidTile;

Engine::Engine(int width, int height, QObject* parent) :
    QObject(parent)
  , m_width(width)
  , m_height(height)
  , m_currentMaterial(Mat::Material::EMPTY)
  , m_graphicsItem(nullptr)
{
    m_updateTimer.start(33);
    connect(&m_updateTimer, &QTimer::timeout, this, &Engine::UpdateTiles, Qt::DirectConnection);
    ResizeTiles(width, height);
    srand(time(NULL));
    Engine::InvalidTile.element->density = std::numeric_limits<double>::max();
}

void Engine::SetGraphicsItem(QGraphicsPixelItem* graphicsItem){
    m_graphicsItem = graphicsItem;
    m_graphicsItem->update();
}

// Connected to the updateTimer::timeout to control update rates.
void Engine::UpdateTiles(){
    bool left = ( rand() % 100 ) < 50;
    if(left){
        for (int i = 0; i < m_width; ++i) {
            for (int j = m_height; j >= 0; --j) {
                Tile& tile = TileAt(i, j);
                if(!IsEmpty(tile)){
                    tile.Update(this);
                }
            }
        }
    }else{
        for (int i = m_width; i >= 0; --i) {
            for (int j = m_height; j >= 0; --j) {
                Tile& tile = TileAt(i, j);
                if(!IsEmpty(tile)){
                    tile.Update(this);
                }
            }
        }
    }
    m_graphicsItem->update();
}

// Returns whether the tile is a valid coordinate to check against.
bool Engine::InBounds(int xPos, int yPos){
    return xPos >= 0 && xPos < m_width && yPos >= 0 && yPos < m_height;
}

// Returns whether the tile is a valid coordinate to check against.
bool Engine::InBounds(const Tile& tile){
    return InBounds(tile.xPos, tile.yPos);
}

// Returns whether the tile is a valid coordinate to check against.
bool Engine::InBounds(const QPoint& position){
    return InBounds(position.x(), position.y());
}

// Returns whether the tile at location x, y's material is empty
bool Engine::IsEmpty(int xPos, int yPos){
    return InBounds(xPos, yPos) && m_tiles[xPos][yPos].element->material == Mat::Material::EMPTY;
}

// Returns whether the tile at location x, y's material is empty
bool Engine::IsEmpty(const QPoint& position){
    return IsEmpty(position.x(), position.y());
}

// Returns whether the tile at location x, y's material is empty
bool Engine::IsEmpty(const Tile& tile){
    return tile.element->material == Mat::Material::EMPTY;
}

// Controls setting tiles at a particular location.
void Engine::SetTile( const Tile& tile ){
    if(InBounds(tile)){
        m_tiles[tile.xPos][tile.yPos] = tile;
    }
}

// Controls setting tiles at a particular location.
void Engine::SetTile( Tile* tile ){
    if(InBounds(*tile)){
        m_tiles[tile->xPos][tile->yPos] = *tile;
    }
}

// Sets the material that will be inserted on the next mouse-left-click event.
void Engine::SetMaterial(Mat::Material material){
    m_currentMaterial = material;
}

// PhysicsWindow will invoke this on a resize event to make the m_tiles match the size of the window.
void Engine::ResizeTiles(int width, int height){
    m_width  = width;
    m_height = height;
    m_tiles.resize(width);
    for(int i = 0; i < width; ++i ){
        m_tiles[i].resize(height);
        for(int j = 0; j < height; ++j){
            SetTile(Tile(i, j, Mat::Material::EMPTY));
        }
    }
    if(m_graphicsItem != nullptr){
        m_graphicsItem->update();
    }
}

// Convenience for getting a tile at a position.
Tile& Engine::TileAt(int xPos, int yPos){
    if( !InBounds(xPos, yPos) )
        return InvalidTile;
    return m_tiles[xPos][yPos];
}

// Convenience for getting a tile at a position.
Tile& Engine::TileAt(const QPoint& position){
    return TileAt(position.x(), position.y());
}

void Engine::Swap(int xPos1, int yPos1, int xPos2, int yPos2){
    if (!InBounds(xPos1, yPos1)) return;
    if (!InBounds(xPos2, yPos2)) return;

    Tile& originalTile    = TileAt(xPos1, yPos1);
    Tile& destinationTile = TileAt(xPos2, yPos2);

    originalTile.SwapElements(destinationTile);

}

void Engine::Swap(const QPoint& pos1, const QPoint& pos2){
    Swap(pos1.x(), pos1.y(), pos2.x(), pos2.y());
}

void Engine::Swap(const Tile& tile1, const Tile& tile2){
    Swap(tile1.xPos, tile1.yPos, tile2.xPos, tile2.yPos);
}

