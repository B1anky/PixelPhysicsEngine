#ifndef ENGINE_H
#define ENGINE_H

#include "Tile.h"
#include "TileSet.h"
#include <QObject>
#include <QTimer>
#include <QVector>
#include <QMetaEnum>
#include <QColor>
#include <QPoint>
#include <QSet>
#include <mutex>
#include <thread>
#include <atomic>
#include <QThread>
#include <QMutex>
#include <QReadWriteLock>
#include <QSize>
#include <QImage>

class QGraphicsEngineItem;
class Element;
class Engine;
class Worker;

typedef QMap<QPoint, Worker*> WorkerMap;

template<typename QEnum>
QString QtEnumToQString (const QEnum value)
{
  return QString(QMetaEnum::fromType<QEnum>().valueToKey(value));
}

class PhysicsWindow;

class Worker : public QObject{

    Q_OBJECT
    friend class Engine;
    friend class PhysicsWindow;

public:

    enum Direction{
          NONE = 0
        , TOP_LEFT
        , TOP
        , TOP_RIGHT
        , LEFT
        , RIGHT
        , BOTTOM_LEFT
        , BOTTOM
        , BOTTOM_RIGHT
    };

    // Helper struct to package contents into each neighbor's requestQueue_.
    struct WorkerPacket{
        Worker*                  sourceWorker_;
        QPoint                   destinationPosition_= QPoint(-1,-1);
        std::shared_ptr<Element> element_;

        WorkerPacket(Worker* sourceWorker, const QPoint& destinationPosition, const std::shared_ptr<Element>& element)
          : sourceWorker_       (sourceWorker)
          , destinationPosition_(destinationPosition)
          , element_            (element)
        { }

        WorkerPacket(const WorkerPacket& workerPacket)
            : sourceWorker_       (workerPacket.sourceWorker_)
            , destinationPosition_(workerPacket.destinationPosition_)
            , element_            (workerPacket.element_)
        { }

        WorkerPacket& operator=(const WorkerPacket& workerPacket){
            if(this != &workerPacket){
                sourceWorker_        = workerPacket.sourceWorker_;
                destinationPosition_ = workerPacket.destinationPosition_;
                element_             = workerPacket.element_;
            }

            return *this;
        }

        ~WorkerPacket(){ }

    };

    Worker(int totalWorkerRows, int totalWorkerColumns, int workerRow, int workerColumn, QImage& workerImage, WorkerMap& workerThreads, QObject* parent = nullptr)
        : QObject(parent)
        , needToResize(true)
        , needToClear(false)
        , workerImage_(workerImage)
        , totalWorkerRows_(totalWorkerRows)
        , totalWorkerColumns_(totalWorkerColumns)
        , assignedWorkerRow_(workerRow)
        , assignedWorkerColumn_(workerColumn)
        , m_tileSet(QPoint(workerRow, workerColumn))
        , killedByEngine(false)
        , safeToTerminate(false)
        , workerThreads_(workerThreads)
    {
        xOffset_ = assignedWorkerColumn_ * (workerImage.width()  / totalWorkerColumns_);
        yOffset_ = assignedWorkerRow_    * (workerImage.height() / totalWorkerRows_);
    }

    ~Worker(){
        emit finished(0);
    }

    void UpdateTiles();

    void FindNeighbors(){
        QPoint workerIndex(assignedWorkerColumn_, assignedWorkerRow_);

        topLeftNeighbor_     = workerThreads_.value(workerIndex + QPoint(-1, -1), nullptr);
        topNeighbor_         = workerThreads_.value(workerIndex + QPoint( 0, -1), nullptr);
        topRightNeighbor_    = workerThreads_.value(workerIndex + QPoint( 1, -1), nullptr);
        leftNeighbor_        = workerThreads_.value(workerIndex + QPoint(-1,  0), nullptr);
        rightNeighbor_       = workerThreads_.value(workerIndex + QPoint( 1,  0), nullptr);
        bottomLeftNeighbor_  = workerThreads_.value(workerIndex + QPoint(-1,  1), nullptr);
        bottomNeighbor_      = workerThreads_.value(workerIndex + QPoint( 0,  1), nullptr);
        bottomRightNeighbor_ = workerThreads_.value(workerIndex + QPoint( 1,  1), nullptr);
    }

    // Quick litmus test to determine if there exists a neighbor in a paricular direction.
    // This has no side-effects.
    bool HasNeighbor(Direction direction) const{
        switch(direction){
            case Direction::TOP_LEFT:
                return topLeftNeighbor_     != nullptr;
            case Direction::TOP:
                return topNeighbor_         != nullptr;
            case Direction::TOP_RIGHT:
                return topRightNeighbor_    != nullptr;
            case Direction::LEFT:
                return leftNeighbor_        != nullptr;
            case Direction::RIGHT:
                return rightNeighbor_       != nullptr;
            case Direction::BOTTOM_LEFT:
                return bottomLeftNeighbor_  != nullptr;
            case Direction::BOTTOM:
                return bottomNeighbor_      != nullptr;
            case Direction::BOTTOM_RIGHT:
                return bottomRightNeighbor_ != nullptr;
            default:
                return false;
        }
    }

    Direction NeighborDirection(Worker* neighbor) const{
        if(neighbor == topLeftNeighbor_){
            return Direction::TOP_LEFT;
        }else if(neighbor == topNeighbor_){
            return Direction::TOP;
        }else if(neighbor == topRightNeighbor_){
            return Direction::TOP_RIGHT;
        }else if(neighbor == leftNeighbor_){
            return Direction::LEFT;
        }else if(neighbor == rightNeighbor_){
            return Direction::RIGHT;
        }else if(neighbor == bottomLeftNeighbor_){
            return Direction::BOTTOM_LEFT;
        }else if(neighbor == bottomNeighbor_){
            return Direction::BOTTOM;
        }else if(neighbor == bottomRightNeighbor_){
            return Direction::BOTTOM_RIGHT;
        }

        return Direction::NONE;
    }

    // NeighborSwap encapsulates all logic required to enqueue a Tile's
    // element to be swapped to another neighbor. The owning tile loses its
    // element temporarily and becomes EMPTY. This will only occur if NeighborSwap
    // could even enqueue the element in the requested direction. If false,
    // it's the equivalent of InBounds returning false for the request.
    // Do not use NeighborSwap to check if a neighbor exists. This function DOES
    // have side effects and must lock the request queue of the neighbor if in a
    // valid direction. `this` is passed in just in case the
    bool NeighborSwap(Direction direction, const std::shared_ptr<Element>& element){

        bool swapped = false;

        // The coordinate would be the coordinate with respect we'd be checking that direction.
        // Edges are weird because only literally a single pixel could swap between corners.
        QPoint  neighborCoordinate(-1, -1);
        Worker* neighbor(nullptr);
        switch(direction){
            case Direction::TOP_LEFT:
                neighbor           = topLeftNeighbor_;     // requesting top-left's bottom right coordinate.
                neighborCoordinate = QPoint(workerImage_.width() / totalWorkerColumns_ - 1, workerImage_.height() / totalWorkerRows_ - 1);
                break;
            case Direction::TOP:
                neighbor           = topNeighbor_;         // requesting top's bottom coordinate, maintaining current x.
                neighborCoordinate = QPoint(element->parentTile->position.x(), workerImage_.height() / totalWorkerRows_ - 1);
                break;
            case Direction::TOP_RIGHT:
                neighbor           = topRightNeighbor_;    // requesting top-right's bottom left coordinate.
                neighborCoordinate = QPoint(0, workerImage_.height() / totalWorkerRows_ - 1);
                break;
            case Direction::LEFT:
                neighbor           = leftNeighbor_;        // requesting left's right coordinate, maintaining current y.
                neighborCoordinate = QPoint(workerImage_.width() / totalWorkerColumns_ - 1, element->parentTile->position.y());
                break;
            case Direction::RIGHT:
                neighbor           = rightNeighbor_;       // requesting right's left coordinate, maintaining current y.
                neighborCoordinate = QPoint(0, element->parentTile->position.y());
                break;
            case Direction::BOTTOM_LEFT:
                neighbor           = bottomLeftNeighbor_;  // requesting bottom-left's top right coordinate.
                neighborCoordinate = QPoint(workerImage_.width() / totalWorkerColumns_ - 1, 0);
                break;
            case Direction::BOTTOM:
                neighbor           = bottomNeighbor_;      // requesting bottom's top coordinate, maintaining current x.
                neighborCoordinate = QPoint(element->parentTile->position.x(), 0);
                break;
            case Direction::BOTTOM_RIGHT:
                neighbor           = bottomRightNeighbor_; // requesting bottom right's top left coordinate.
                neighborCoordinate = QPoint(0, 0);
                break;
            default:
                return false;
        }

        if(neighbor != nullptr){
            if(neighborCoordinate.x() < 0){
                neighborCoordinate.setX(0);
            }else if(neighborCoordinate.x() > workerImage_.width()){
                neighborCoordinate.setX(workerImage_.width() - 1);
            }
            if(neighborCoordinate.y() < 0){
                neighborCoordinate.setY(0);
            }else if(neighborCoordinate.y() > workerImage_.height()){
                neighborCoordinate.setY(workerImage_.height());
            }
            swapped = SendWorkerPacket(neighbor, neighborCoordinate, element);
        }

        return swapped;

    }

    // Do not call this unless you know what you're doing.
    // This should be managed primarily through NeighborSwap to ensure thread-safety.
    bool SendWorkerPacket(Worker* neighbor, const QPoint& neighborCoordinate, const std::shared_ptr<Element>& element){
        bool workerPacketEnqueued = false;
        //if(neighbor->m_tileSet.m_readWriteLock.tryLockForRead(1)){
            workerPacketEnqueued = neighbor->m_tileSet.IsEmpty(neighborCoordinate);
            //neighbor->m_tileSet.m_readWriteLock.unlock();
        //}

        // If the tile is empty in the neighbor, we can enqueue it to be processed.
        // If thread safety magically doesn't burn us, just enqueue it without locking.
        // Note: Add write locks here and read locks in the Update when polling from the
        // queue if aforementioned race conditions present themselves as random crashes.
        if(workerPacketEnqueued){
            if(neighbor->requestReadWriteLock_.tryLockForWrite()){
                neighbor->requestQueue_.push_back(WorkerPacket(this, neighborCoordinate, element));
                // Erase `this` from our parent tile somce the neighbor's queue will keep us alive.
                element->parentTile->SetElement(Mat::Material::EMPTY);
                neighbor->requestReadWriteLock_.unlock();
            }else{
                workerPacketEnqueued = false;
            }
        }

        return workerPacketEnqueued;
    }

    virtual void work(){
        srand(time(NULL));
        updateTimer = new QTimer(this);
        updateTimer->start(33);
        connect(updateTimer, &QTimer::timeout, this, &Worker::UpdateTiles, Qt::QueuedConnection);
    }

public slots:

    virtual void setup(QThread* thread, QString id){
        m_id = id;
        connect(thread, &QThread::started, this,   &Worker::work,  Qt::QueuedConnection);
        connect(this,   &Worker::finished, thread, &QThread::exit, Qt::QueuedConnection);
    }

signals:

    void finished(int);

    void started();

public:
    QTimer*   updateTimer;
    std::atomic<bool> needToResize;
    bool      needToClear;
    QSize     m_resizeRequest;
    QReadWriteLock heightWidthMutex_;
    QImage&        workerImage_;

//protected:
    // Used to determine the width and height implicitly
    int totalWorkerRows_;
    int totalWorkerColumns_;
    int assignedWorkerRow_;
    int assignedWorkerColumn_;
    int xOffset_;
    int yOffset_;

    TileSet m_tileSet;
    QString m_id;
    bool killedByEngine;
    bool safeToTerminate;

    // All worker threads.
    WorkerMap& workerThreads_;

    // Neighbors will write to the requestQueue in FIFO fashion.
    // Workers process their requestQueue as a special pre-update step.
    // This will help grab what other neighbors might be able to send
    // and process as soon as possible. If rejected during the processing
    // by the neighbor, it will immediately be enqueud back to `this`.
    QList<WorkerPacket> requestQueue_;
    QReadWriteLock requestReadWriteLock_;

    // All assignable neighbors.
    // When properly setup, they can be traversed like a graph.
    Worker* topLeftNeighbor_;
    Worker* topNeighbor_;
    Worker* topRightNeighbor_;
    Worker* leftNeighbor_;
    Worker* rightNeighbor_;
    Worker* bottomLeftNeighbor_;
    Worker* bottomNeighbor_;
    Worker* bottomRightNeighbor_;

};

class Engine : public QObject
{

    Q_OBJECT

    friend class PhysicsWindow;
    friend class Element;
    friend class GravityAbidingProperty;
    friend class SpreadAbidingProperty;
    friend class Liquid;

public:

    explicit Engine(int width, int height, QObject* parent = nullptr);

    ~Engine();

    void CleanupThreads();

    void SetupWorkerThreads(int totalRows, int totalColumns);

    void SetEngineGraphicsItem(QGraphicsEngineItem* engineGraphicsItem);

    // Clears all main and worker thread tiles to have an empty material at each tile.
    void ClearTiles();

    void SetMaterial(Mat::Material material);

    void UserPlacedTile(Tile tile);

protected:

    // Connected to the updateTimer::timeout to control update rates.
    void UpdateTiles();

    // PhysicsWindow will invoke this on a resize event to make the m_tiles match the size of the window.
    void ResizeTiles(int width, int height, bool initialization = false);

protected:

    int m_width;
    int m_height;

    // Used to determine the width and height implicitly
    int totalWorkerRows;
    int totalWorkerColumns;

    Mat::Material m_currentMaterial;
    QTimer  m_updateTimer;
    QGraphicsEngineItem* m_engineGraphicsItem;

    WorkerMap m_workerThreads;
    std::atomic<bool> m_workersInitialized;

    QImage m_workerImage;
    int workersKilledCount;
};

#endif // ENGINE_H
