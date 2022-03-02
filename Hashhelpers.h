#ifndef HASHHELPERS_H
#define HASHHELPERS_H

#include "Tile.h"
#include <QHash>

static inline uint qHash(const Tile &key, uint seed)
{
    return qHash(key.position.x() + key.position.y(), seed);
}

#endif // HASHHELPERS_H
