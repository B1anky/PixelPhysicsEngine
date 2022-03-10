#include "Engine.h"
#include "QGraphicsEngineItem.h"
#include "QGraphicsPixelItem.h"
#include "Elements.h"
#include "Hashhelpers.h"
#include "TileSet.h"
#include <QApplication>
#include <QDebug>

#include <QPoint>
#include <random>
#include <time.h>

#define POLLING 0

#if POLLING
static qint64 averageElapsed = 0;
static qint64 totalElapsed = 0;
static qint64 numRuns = 0;
static QMutex timerMutex;
#endif


void Worker::UpdateTiles(){

    auto UpdateImage = [this](){

        {
        #if POLLING
        QElapsedTimer timer;
        timer.start();
        #endif
        }
        int localXOffset = 0;
        int localYOffset = 0;

        for (int j = 0; j < m_tileSet.height(); ++j) {
            localYOffset = j + yOffset_;// + (yOffset_ > 0 ? -1 : 0);
            QRgb* pixelRow = (QRgb*)(workerImage_.bits() + (localYOffset * workerImage_.bytesPerLine()));
            for (int i = 0; i < m_tileSet.width(); ++i) {
                localXOffset = i + xOffset_;
                pixelRow[localXOffset] = Mat::MaterialToColorMap[m_tileSet.TileAt(i, j).element->material].rgba();
            }
        }

        {
        #if POLLING
        timerMutex.lock();
        totalElapsed   += timer.nsecsElapsed();
        averageElapsed = totalElapsed / ++numRuns;
        qDebug() << averageElapsed;
        timerMutex.unlock();
        #endif
        }
    };

    // Empty our queue for requests or send them back to the sender.
    requestReadWriteLock_.lockForRead();
    //qDebug() << m_id << "Queue size:" << requestQueue_.size();
    while( !requestQueue_.empty() ) {
        const WorkerPacket& topWorkerPacket = requestQueue_.front();
        // Take ownership of the element
        if( m_tileSet.IsEmpty(topWorkerPacket.destinationPosition_) ){
            //qDebug() << topWorkerPacket.destinationPosition_;
            m_tileSet.SetTile(Tile(topWorkerPacket.destinationPosition_, topWorkerPacket.element_));
        }else{
            NeighborSwap(NeighborDirection(topWorkerPacket.sourceWorker_), topWorkerPacket.element_);
        }
        requestQueue_.pop_front();
    }
    requestReadWriteLock_.unlock();

    // Try to service any resize requests from the main thread.
    if(needToResize){
        m_tileSet.ResizeTiles(m_resizeRequest.width(), m_resizeRequest.height());
        xOffset_ = assignedWorkerColumn_ * m_resizeRequest.width();
        yOffset_ = assignedWorkerRow_    * m_resizeRequest.height();
        m_resizeRequest.setWidth(0);
        m_resizeRequest.setHeight(0);

        needToResize = false;

        UpdateImage();
    }

    if(needToClear){
        m_tileSet.ClearTiles();
        needToClear = false;
    }

    QVector<int> localWidths(QVector<int>(m_tileSet.width()));
    std::iota(localWidths.begin(), localWidths.end(), 0);
    std::random_shuffle(localWidths.begin(), localWidths.end());

    for (int i = 0; i < m_tileSet.width(); ++i) {
        for (int j = m_tileSet.height() - 1; j >= 0; --j) {
            Tile& tile = m_tileSet.TileAt(localWidths.at(i), j);
            if(!m_tileSet.IsEmpty(tile)){
                m_tileSet.m_readWriteLock.lockForWrite();
                tile.Update(m_tileSet, this);
                m_tileSet.m_readWriteLock.unlock();
            }
        }
    }

    if(!needToResize){
        UpdateImage();
    }

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
    TileSet::InvalidTile.element->density = std::numeric_limits<double>::max();

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
            Worker* newWorker = new Worker(totalRows, totalColumns, i, j, m_workerImage, m_workerThreads);
            m_workerThreads[QPoint(j, i)] = newWorker;
        }
    }
    totalWorkerRows    = totalRows;
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

    foreach(auto workerThread, m_workerThreads){
        workerThread->FindNeighbors();
    }

    m_workersInitialized = true;
}

// Connected to the updateTimer::timeout to control update rates.
void Engine::UpdateTiles(){
    m_engineGraphicsItem->update();
}

void Engine::ClearTiles(){
    foreach(auto workerThread, m_workerThreads){
        workerThread->needToClear = true;
    }
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

    QPoint workerKey(workerColumn, workerRow);
    // We have to normalize the tile being placed into this worker.
    Worker* worker = m_workerThreads.value(workerKey, nullptr);
    // Can be a nullptr if the user is like half off of the view or scene with a big radius
    if(worker != nullptr){
        worker->m_tileSet.m_readWriteLock.lockForWrite();
        tile.position.setX(x - worker->xOffset_ - 1);
        tile.position.setY(y - worker->yOffset_ - 1);
        worker->m_tileSet.SetTile(tile);
        worker->m_tileSet.m_readWriteLock.unlock();
    }
}

// PhysicsWindow will invoke this on a resize event to make the m_tiles match the size of the window.
void Engine::ResizeTiles(int width, int height, bool initialization){
    if(width == m_width && height == m_height && !initialization) return;

    m_width  = width;
    m_height = height;

    m_workerImage = QImage(m_width, m_height, QImage::Format_RGB32);
    m_workerImage.fill(Mat::MaterialToColorMap[Mat::Material::EMPTY]);

    if(m_workersInitialized){
        foreach(auto workerThread, m_workerThreads){
            workerThread->m_resizeRequest.setWidth (m_width  / totalWorkerColumns);
            workerThread->m_resizeRequest.setHeight(m_height / totalWorkerRows);
            workerThread->needToResize = true;
        }
    }

}
