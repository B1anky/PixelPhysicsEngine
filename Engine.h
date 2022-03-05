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

class QGraphicsEngineItem;
class Element;
class Engine;

template<typename QEnum>
QString QtEnumToQString (const QEnum value)
{
  return QString(QMetaEnum::fromType<QEnum>().valueToKey(value));
}

class PhysicsWindow;
static inline QReadWriteLock m_tile_mutex;

class Worker : public QObject{

    Q_OBJECT
    friend class Engine;
    friend class PhysicsWindow;

public:

    Worker(TileSet& allTiles, int totalWorkerRows, int totalWorkerColumns, int workerRow, int workerColumn, QObject* parent = nullptr)
        : QObject(parent)
        , mainThreadTiles(allTiles)
        , totalWorkerRows_(totalWorkerRows)
        , totalWorkerColumns_(totalWorkerColumns)
        , assignedWorkerRow_(workerRow)
        , assignedWorkerColumn_(workerColumn)
        , m_tileSet(allTiles)
    {
        xOffset_ = assignedWorkerColumn_ * m_resizeRequest.width();
        yOffset_ = assignedWorkerRow_    * m_resizeRequest.height();
    }

    ~Worker(){
        emit finished(0);
    }

    bool IsEmpty(const Tile& tile) const{
        return tile.element->material == Mat::Material::EMPTY;
    }

    void UpdateTiles();

    virtual void work(){
        srand(time(NULL));
        QTimer* updateTimer = new QTimer(this);
        updateTimer->start(33);
        connect(updateTimer, &QTimer::timeout, this, &Worker::UpdateTiles, Qt::DirectConnection);
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

    TileSet&  mainThreadTiles;
    std::atomic<bool> needToResize;
    QReadWriteLock heightWidthMutex_;

protected:
    // Used to determine the width and height implicitly
    int totalWorkerRows_;
    int totalWorkerColumns_;
    int assignedWorkerRow_;
    int assignedWorkerColumn_;
    int xOffset_;
    int yOffset_;

    TileSet                  m_tileSet;
    QString                  m_id;
    static inline QMutex     m_mutex = QMutex();
    QSize m_resizeRequest;
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
    TileSet m_mainThreadTileSet;
    QTimer  m_updateTimer;
    QGraphicsEngineItem* m_engineGraphicsItem;

    QMap<QPoint, Worker*> m_workerThreads;
    std::atomic<bool> m_workersInitialized;
};

#endif // ENGINE_H
