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

    if( heightWidthMutex_.tryLockForRead(5) ){
        if(needToResize){
            m_tileSet.ResizeTiles(m_resizeRequest.width(), m_resizeRequest.height());
            xOffset_ = assignedWorkerColumn_ * m_resizeRequest.width();
            yOffset_ = assignedWorkerRow_    * m_resizeRequest.height();
            m_resizeRequest.setWidth(0);
            m_resizeRequest.setHeight(0);
            needToResize = false;
        }
        heightWidthMutex_.unlock();
    }

    if(m_tileSet.m_readWriteLock.tryLockForWrite(5)){
        QVector<int> localWidths(QVector<int>(m_tileSet.width()));
        std::iota(localWidths.begin(), localWidths.end(), 0);
        std::random_shuffle(localWidths.begin(), localWidths.end());

        for (int i = 0; i < m_tileSet.width(); ++i) {
            for (int j = m_tileSet.height() - 1; j >= 0; --j) {
                Tile& tile = m_tileSet.TileAt(localWidths.at(i), j);
                if(!m_tileSet.IsEmpty(tile)){
                    tile.Update(m_tileSet);
                }
            }
        }
        m_tileSet.m_readWriteLock.unlock();
    }

    // If we need to resize and we're shrinking, we could go out of bounds.
    if(!needToResize){
        if(mainThreadTiles.m_readWriteLock.tryLockForWrite(5) ){
            m_tileSet.m_readWriteLock.lockForRead();
            // Based on current width and heights, determine where
            // we index into the mainThreadTiles.
            for (int i = 0; i < m_tileSet.width(); ++i) {
                for (int j = m_tileSet.height() - 1; j >= 0; --j) {
                    mainThreadTiles.SetTileBulkUpdate(Tile(i + xOffset_, j + yOffset_, m_tileSet.TileAt(i, j).element->material));
                }
            }
            m_tileSet.m_readWriteLock.unlock();
            mainThreadTiles.m_readWriteLock.unlock();
        }
    }
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
    connect(&m_updateTimer, &QTimer::timeout, this, &Engine::UpdateTiles, Qt::QueuedConnection);
}

void Engine::SetupWorkerThreads(int totalRows, int totalColumns){
    for(int i = 0; i < totalRows; ++i){
        for(int j = 0; j < totalColumns; ++j){
            m_workerThreads[QPoint(i, j)] = new Worker(m_mainThreadTileSet, totalRows, totalColumns, i, j);
        }
    }
    totalWorkerRows = totalRows;
    totalWorkerColumns = totalColumns;
}

Engine::~Engine(){
    foreach(auto workerThread, m_workerThreads){
        workerThread->updateTimer->stop();
        emit workerThread->finished(0);
        delete workerThread;
    }
}

void Engine::SetEngineGraphicsItem(QGraphicsEngineItem* engineGraphicsItem){
    m_engineGraphicsItem = engineGraphicsItem;
    m_engineGraphicsItem->update();

    SetupWorkerThreads(2, 4);

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
    Worker* worker = m_workerThreads.value(workerKey, nullptr);
    // Can be a nullptr if the user is like half off of the view or scene with a big radius
    if(worker != nullptr){
        if(worker->m_tileSet.m_readWriteLock.tryLockForWrite(33)){
            tile.position.setX(x - worker->xOffset_);
            tile.position.setY(y - worker->yOffset_);
            worker->m_tileSet.SetTileBulkUpdate(tile);
            worker->m_tileSet.m_readWriteLock.unlock();
        }
    }
}

// PhysicsWindow will invoke this on a resize event to make the m_tiles match the size of the window.
void Engine::ResizeTiles(int width, int height, bool initialization){
    if(width == m_width && height == m_height && !initialization) return;

    m_width  = width;
    m_height = height;

    m_mainThreadTileSet.ResizeTiles(width, height, initialization);

    if(m_workersInitialized){
        foreach(auto workerThread, m_workerThreads){
            if(workerThread->heightWidthMutex_.tryLockForWrite(5)){
                workerThread->m_resizeRequest.setWidth(m_width / totalWorkerColumns);
                workerThread->m_resizeRequest.setHeight(m_height / totalWorkerRows);
                workerThread->needToResize = true;
                workerThread->heightWidthMutex_.unlock();
            }
        }
    }

}
