#ifndef HASHHELPERS_H
#define HASHHELPERS_H

#include "Tile.h"
#include <QHash>

static inline uint qHash(const Tile &key, uint seed)
{
    return qHash(key.xPos + key.yPos, seed);
}

#endif // HASHHELPERS_H
