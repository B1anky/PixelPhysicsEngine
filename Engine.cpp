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


template <typename T> int sign(T val) {
    return (T(0) < val) - (val < T(0));
}

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
            localYOffset = j + yOffset_ - 1;
            QRgb* pixelRow = (QRgb*)(workerImage_.bits() + (localYOffset * workerImage_.bytesPerLine()));
            for (int i = 0; i < m_tileSet.width(); ++i) {
                localXOffset = i + xOffset_ - 1;
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

    auto ProcessRequestQueue = [this](){
        // Empty our queue for requests or send them back to the sender.
        for(int i = 0; i < requestQueue_.size(); ++i){
            requestReadWriteLock_.lockForWrite();
            WorkerPacket& currentWorkerPacket = requestQueue_[i];
            // Take ownership of the element
            QPoint defaultDestinationPosition(currentWorkerPacket.destinationPosition_);
            bool dealtWith(false);

            QPoint potentialPointOffset(defaultDestinationPosition.x() + currentWorkerPacket.element_->heading.x(), defaultDestinationPosition.y());
            // If the heading value is greater than 1, it's probably due to liquid. We don't want to push liquid into the middle of another neighbor...
            if(m_tileSet.IsEmpty(defaultDestinationPosition)/* && abs(currentWorkerPacket.element_->heading.x()) <= 1*/){
                m_tileSet.SetTile(Tile(defaultDestinationPosition, currentWorkerPacket.element_));
                m_tileSet.TileAt(defaultDestinationPosition).Update(m_tileSet, this);
                dealtWith = true;
            }else if(potentialPointOffset.x() > 0){
                // Pass it to the right neighbor
                dealtWith = NeighborSwap(Direction::RIGHT, currentWorkerPacket.element_);
            }else if(potentialPointOffset.x() < 0){
                // Pass it to the left neighbor
                dealtWith = NeighborSwap(Direction::LEFT, currentWorkerPacket.element_);
            }

            // Can we find the next empty spot down, without hitting something more dense?
            //if(!dealtWith){
            //    bool someValidPointFound = m_tileSet.IsEmpty(defaultDestinationPosition);
            //    // Liquid / gas Physics helping
            //    if(someValidPointFound && abs(currentWorkerPacket.element_->heading.x()) > 1 ){
            //        while(m_tileSet.TileAt(potentialPointOffset).element->density < currentWorkerPacket.element_->density){
            //            potentialPointOffset.setY(potentialPointOffset.y() + 1);
            //            if(!m_tileSet.IsEmpty(potentialPointOffset) || (abs(currentWorkerPacket.element_->heading.x()) > 1 )){
            //                potentialPointOffset.setY(potentialPointOffset.y() - 1);
            //                break;
            //            }
            //        }
            //    }
            //
            //    if(someValidPointFound){
            //        //m_tileSet.m_readWriteLock.lockForWrite();
            //        m_tileSet.SetTile(Tile(potentialPointOffset, currentWorkerPacket.element_));
            //        m_tileSet.TileAt(potentialPointOffset).Update(m_tileSet, this);
            //        //m_tileSet.m_readWriteLock.unlock();
            //        dealtWith = true;
            //    }
            //}
            //
            if(dealtWith){
                requestQueue_.removeAt(i);
                --i;
            }
            else if(NeighborSwap(NeighborDirection(currentWorkerPacket.sourceWorker_), currentWorkerPacket.element_)){
                requestQueue_.removeAt(i);
                --i;
                dealtWith = true;
            }
            requestReadWriteLock_.unlock();
        }
    };

    // Try to service any resize requests from the main thread.
    if(needToResize){
        m_tileSet.ResizeTiles(m_resizeRequest.width(), m_resizeRequest.height());
        xOffset_ = ( assignedWorkerColumn_ * (workerImage_.width()  / totalWorkerColumns_)) + 1;
        yOffset_ = ( assignedWorkerRow_    * (workerImage_.height() / totalWorkerRows_   )) + 1;
        m_resizeRequest.setWidth(0);
        m_resizeRequest.setHeight(0);

        needToResize = false;

        UpdateImage();
    }

    ProcessRequestQueue();

    if(needToClear){
        requestQueue_.clear();
        m_tileSet.ClearTiles();
        needToClear = false;
    }

    QVector<int> localWidths(QVector<int>(m_tileSet.width()));
    std::iota(localWidths.begin(), localWidths.end(), 0);
    std::random_shuffle(localWidths.begin(), localWidths.end());

    for (int i = 0; i < m_tileSet.width(); ++i) {
        for (int j = m_tileSet.height() - 1; j >= 0; --j) {
            m_tileSet.m_readWriteLock.lockForWrite();
            Tile& tile = m_tileSet.TileAt(localWidths.at(i), j);
            if(!m_tileSet.IsEmpty(tile)){
                tile.Update(m_tileSet, this);
            }
            m_tileSet.m_readWriteLock.unlock();
        }
    }

    ProcessRequestQueue();

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

    SetupWorkerThreads(1, 2);

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

    int workerWidths  = m_width  / static_cast<double>(totalWorkerColumns);
    int workerHeights = m_height / static_cast<double>(totalWorkerRows);

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
        tile.position.setX(x - worker->xOffset_);
        tile.position.setY(y - worker->yOffset_);
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
