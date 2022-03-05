#include "Engine.h"
#include "QGraphicsEngineItem.h"
#include "QGraphicsPixelItem.h"
#include "Elements.h"
#include "Hashhelpers.h"
#include "TileSet.h"

#include <QPoint>
#include <random>
#include <time.h>

void Worker::UpdateTiles(){

    if(needToResize){
        m_tileSet.ResizeTiles(m_resizeRequest.width(), m_resizeRequest.height());
        xOffset_ = assignedWorkerColumn_ * m_resizeRequest.width();
        yOffset_ = assignedWorkerRow_    * m_resizeRequest.height();
        needToResize = false;
    }

    m_tile_mutex.lockForWrite();
    QVector<int> localWidths(QVector<int>(m_tileSet.width()));
    std::iota(localWidths.begin(), localWidths.end(), 0);
    std::random_shuffle(localWidths.begin(), localWidths.end());

    for (int i = 0; i < m_tileSet.width(); ++i) {
        for (int j = m_tileSet.height() - 1; j >= 0; --j) {
            Tile& tile = m_tileSet.TileAt(i, j);
            if(!IsEmpty(tile)){
                tile.Update(m_tileSet);
            }
        }
    }
    m_tile_mutex.unlock();

    m_tile_mutex.lockForRead();

    // Based on current width and heights, determine where
    // we index into the mainThreadTiles.
    for (int i = 0; i < m_tileSet.width(); ++i) {
        for (int j = m_tileSet.height() - 1; j >= 0; --j) {
            mainThreadTiles.SetTile(Tile(i + xOffset_, j + yOffset_, m_tileSet.TileAt(i, j).element->material));
        }
    }
    m_tile_mutex.unlock();

}

Engine::Engine(int width, int height, QObject* parent) :
    QObject(parent)
  , m_width(width)
  , m_height(height)
  , m_currentMaterial(Mat::Material::EMPTY)
  , m_engineGraphicsItem(nullptr)
{
    ::InvalidTile.element->density = std::numeric_limits<double>::max();

    ResizeTiles(width, height, true);
    srand(time(NULL));
    ClearTiles();
    m_workersInitialized = false;
    m_updateTimer.start(33);
    connect(&m_updateTimer, &QTimer::timeout, this, &Engine::UpdateTiles, Qt::DirectConnection);
}

void Engine::SetupWorkerThreads(int totalRows, int totalColumns){
    for(int i = 0; i < totalRows; ++i){
        for(int j = 0; j < totalColumns; ++j){
            m_workerThreads[QPoint(i, j)] = new Worker(m_mainThreadTileSet, totalRows, totalColumns, i, j, this);
        }
    }
    totalWorkerRows = totalRows;
    totalWorkerColumns = totalColumns;
}

Engine::~Engine(){
    // Determine if we should clean worker up properly or if it doesn't matter.
}

void Engine::SetEngineGraphicsItem(QGraphicsEngineItem* engineGraphicsItem){
    m_engineGraphicsItem = engineGraphicsItem;
    m_engineGraphicsItem->update();

    SetupWorkerThreads(2, 2);

    foreach(auto workerThread, m_workerThreads){
        QThread* thread = new QThread(this);
        workerThread->setup(thread, QString("Row: %0, Column: %1").arg(workerThread->assignedWorkerRow_).arg(workerThread->assignedWorkerColumn_));
        workerThread->moveToThread(thread);
        thread->start();
    }

    m_workersInitialized = true;
}

// Connected to the updateTimer::timeout to control update rates.
void Engine::UpdateTiles(){
    m_engineGraphicsItem->update();
}

void Engine::ClearTiles(){
    foreach(auto workerThread, m_workerThreads){
        workerThread->m_tileSet.ClearTiles();
    }

    m_mainThreadTileSet.ClearTiles();
}

void Engine::SetMaterial(Mat::Material material){
    m_currentMaterial = material;
}

// To encapsulate the threading location of each sub rect, figure out which
// tile's row and column corresponds wot which worker thread's.
void Engine::UserPlacedTile(Tile tile){
    int x = tile.position.x();
    int y = tile.position.y();

    int workerWidths  = m_width  / totalWorkerColumns;
    int workerHeights = m_height / totalWorkerRows;

    int workerRow = 0;
    while(workerRow < totalWorkerRows){
        if(y <= workerRow * workerHeights){
            break;
        }
        ++workerRow;
    }

    int workerColumn = 0;
    while(workerColumn < totalWorkerColumns){
        if(x <= workerColumn * workerWidths){
            break;
        }
        ++workerColumn;
    }

    --workerRow;
    --workerColumn;

    QPoint workerKey(workerRow, workerColumn);
    // We have to normalize the tile being placed into this worker.

    //xOffset_ = assignedWorkerColumn_ / totalWorkerColumns_  * m_resizeRequest.width();
    //yOffset_ = assignedWorkerRow_    / totalWorkerRows_     * m_resizeRequest.height();

    Worker* worker = m_workerThreads[workerKey];

    tile.position.setX(x - worker->xOffset_);
    tile.position.setY(y - worker->yOffset_);
    worker->m_tileSet.SetTile(tile);

}

// PhysicsWindow will invoke this on a resize event to make the m_tiles match the size of the window.
void Engine::ResizeTiles(int width, int height, bool initialization){
    if(width == m_width && height == m_height && !initialization) return;

    m_width  = width;
    m_height = height;

    if(m_workersInitialized){
        foreach(auto workerThread, m_workerThreads){
            workerThread->heightWidthMutex_.lockForWrite();
            workerThread->m_resizeRequest.setWidth(m_width / totalWorkerColumns);
            workerThread->m_resizeRequest.setHeight(m_height / totalWorkerRows);
            workerThread->needToResize = true;
            workerThread->heightWidthMutex_.unlock();
        }
    }

    m_mainThreadTileSet.ResizeTiles(width, height, initialization);

}
