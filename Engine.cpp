#include "Engine.h"
#include "QGraphicsEngineItem.h"
#include "QGraphicsPixelItem.h"
#include "Elements.h"
#include "Hashhelpers.h"
#include "TileSet.h"
#include <QApplication>

#include <QPoint>
#include <random>
#include <time.h>

void Worker::UpdateTiles(){

    auto paintImage = [this](){
        int localXOffset = 0;
        int localYOffset = 0;
        for (int i = 0; i < m_tileSet.width(); ++i) {
            localXOffset = i + xOffset_;
            for (int j = m_tileSet.height() - 1; j >= 0; --j) {
                localYOffset = j + yOffset_;
                if(localXOffset < workerImage_.width() && localYOffset < workerImage_.height()){
                    workerImage_.setPixelColor(localXOffset, localYOffset, Mat::MaterialToColorMap[m_tileSet.TileAt(i, j).element->material]);
                }
            }
        }
    };

    // Try to service any resize requests from the main thread.
    if(needToResize){
        m_tileSet.ResizeTiles(m_resizeRequest.width(), m_resizeRequest.height());
        xOffset_ = assignedWorkerColumn_ * m_resizeRequest.width();
        yOffset_ = assignedWorkerRow_    * m_resizeRequest.height();
        m_resizeRequest.setWidth(0);
        m_resizeRequest.setHeight(0);

        paintImage();

        needToResize = false;
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

    paintImage();

    if(killedByEngine){
        updateTimer->stop();
        safeToTerminate = true;
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
    workersKilledCount = 0;
    for(int i = 0; i < totalRows; ++i){
        for(int j = 0; j < totalColumns; ++j){
            Worker* newWorker = new Worker(m_mainThreadTileSet, totalRows, totalColumns, i, j, m_workerImage);
            m_workerThreads[QPoint(i, j)] = newWorker;
        }
    }
    totalWorkerRows = totalRows;
    totalWorkerColumns = totalColumns;
}

void Engine::CleanupThreads(){

    // Simulating a thread join.
    foreach(auto workerThread, m_workerThreads){
        workerThread->killedByEngine = true;
    }

    while(!m_workerThreads.empty()){
        for(auto workerThreadIter = m_workerThreads.begin(); workerThreadIter != m_workerThreads.end(); ++workerThreadIter ){
            if(workerThreadIter.value()->safeToTerminate){
                workerThreadIter.value()->thread()->quit();
                delete workerThreadIter.value();
                m_workerThreads.erase(workerThreadIter);
                break;
            }
        }
    }
}

Engine::~Engine(){
    CleanupThreads();
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
        if(worker->m_tileSet.m_readWriteLock.tryLockForWrite(5)){
            tile.position.setX(x - worker->xOffset_);
            tile.position.setY(y - worker->yOffset_);
            worker->m_tileSet.SetTile(tile);
            worker->m_tileSet.m_readWriteLock.unlock();
        }
    }
}

// PhysicsWindow will invoke this on a resize event to make the m_tiles match the size of the window.
void Engine::ResizeTiles(int width, int height, bool initialization){
    if(width == m_width && height == m_height && !initialization) return;

    m_width  = width;
    m_height = height;

    //m_mainThreadTileSet.m_readWriteLock.lockForWrite();
    //m_mainThreadTileSet.ResizeTiles(width, height, initialization);
    //m_mainThreadTileSet.m_readWriteLock.unlock();

    m_workerImage = QImage(m_width, m_height, QImage::Format_ARGB32);

    if(m_workersInitialized){
        foreach(auto workerThread, m_workerThreads){
            //if(workerThread->heightWidthMutex_.tryLockForWrite(5)){
                workerThread->m_resizeRequest.setWidth(m_width / totalWorkerColumns);
                workerThread->m_resizeRequest.setHeight(m_height / totalWorkerRows);
                workerThread->needToResize = true;
                //workerThread->heightWidthMutex_.unlock();
            //}
        }
    }

}
