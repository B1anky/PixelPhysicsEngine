#ifndef ENGINE_H
#define ENGINE_H

#include "Tile.h"
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

class QGraphicsEngineItem;
class Element;

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

    Worker(QVector<QVector<Tile>>& allTiles, QVector<int>& randomWidths, Engine* engine, QObject* parent = nullptr)
        : QObject(parent)
        , allTiles_(allTiles)
        , randomWidths_(randomWidths)
        , engine_(engine)
        , dirtyTiles_(allTiles)
    {
    }

    ~Worker(){
        emit finished(0);
    }

    bool IsEmpty(const Tile& tile) const{
        return tile.element->material == Mat::Material::EMPTY;
    }

    void UpdateTiles();

    virtual void work(){
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

    QVector<QVector<Tile>>&  allTiles_;
    QVector<int>&            randomWidths_;
    Engine*                  engine_;

protected:
    QVector<QVector<Tile>>   dirtyTiles_;
    QString                  m_id;
    //QTimer                   m_updateTimer;
    static inline QMutex     m_mutex = QMutex();
    //static inline QSemaphore m_lockAcquirer = QSemaphore(0);
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

    static Tile InvalidTile;

    explicit Engine(int width, int height, QObject* parent = nullptr);

    ~Engine();

    void SetEngineGraphicsItem(QGraphicsEngineItem* engineGraphicsItem);

    // Returns whether the tile is a valid coordinate to check against.
    bool InBounds(int xPos, int yPos, QVector<QVector<Tile> >& tileToCheckAgainst);

    // Returns whether the tile is a valid coordinate to check against.
    bool InBounds(const QPoint& position, QVector<QVector<Tile> >& tileToCheckAgainst);

    // Returns whether the tile is a valid coordinate to check against.
    bool InBounds(const Tile& tile, QVector<QVector<Tile> >& tileToCheckAgainst);

    // Returns whether the tile at location x, y's material is empty.
    bool IsEmpty(int xPos, int yPos, QVector<QVector<Tile> >& tileToCheckAgainst);

    // Returns whether the tile at location x, y's material is empty.
    bool IsEmpty(const QPoint& position, QVector<QVector<Tile> >& tileToCheckAgainst);

    // Returns whether the tile at location x, y's material is empty.
    bool IsEmpty(const Tile& tile);

    // Controls setting tiles at a particular location. This will add it to the dirty set.
    void SetTile( const Tile& tile, QVector<QVector<Tile> >& tileToCheckAgainst);

    // Controls setting tiles at a particular location. This will add it to the dirty set.
    void SetTile( Tile* tile, QVector<QVector<Tile> >& tileToCheckAgainst);

    // Sets the material that will be inserted on the next mouse-left-click event.
    void SetMaterial(Mat::Material material);

    // Convenience for getting a tile at a position.
    Tile& TileAt(int xPos, int yPos, QVector<QVector<Tile> >& tileToCheckAgainst);

    // Convenience for getting a tile at a position.
    Tile& TileAt(const QPoint& position, QVector<QVector<Tile> >& tileToCheckAgainst);

    void Swap(int xPos1, int yPos1, int xPos2, int yPos2, QVector<QVector<Tile> >& tileToCheckAgainst);
    void Swap(const QPoint& pos1, const QPoint& pos2, QVector<QVector<Tile> >& tileToCheckAgainst);
    void Swap(const Tile& tile1, const Tile& tile2, QVector<QVector<Tile> >& tileToCheckAgainst);

    void ClearTiles(QVector<QVector<Tile> >& tileToCheckAgainst);

protected:

    // Connected to the updateTimer::timeout to control update rates.
    void UpdateTiles();

    // PhysicsWindow will invoke this on a resize event to make the m_tiles match the size of the window.
    void ResizeTiles(int width, int height, bool initialization = false);

    void UpdateLoop();

protected:

    int m_width;
    int m_height;
    Mat::Material m_currentMaterial;
    QVector<QVector<Tile>> m_tiles;
    QVector<int> randomWidths;
    QTimer m_updateTimer;
    QGraphicsEngineItem* m_engineGraphicsItem;
    std::unique_lock<std::mutex> m_tile_shared_lock;
    //std::mutex m_tile_mutex;
    Worker m_workerThread;
    std::atomic<bool> m_initialized;
};

#endif // ENGINE_H
