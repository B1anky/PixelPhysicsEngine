#ifndef HASHHELPERS_H
#define HASHHELPERS_H

#include "Tile.h"
#include <QHash>

static inline bool operator<(const QPoint& p1,const QPoint& p2)
{
    return p1.x() < p2.x() || p1.y() < p2.y();
}

static inline uint qHash(const Tile &key, uint seed)
{
    return qHash(key.position.x() + key.position.y(), seed);
}

#endif // HASHHELPERS_H
