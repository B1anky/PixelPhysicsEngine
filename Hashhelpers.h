#ifndef HASHHELPERS_H
#define HASHHELPERS_H

#include "Tile.h"
#include "GeneralHashHelpers.h"
#include <QHash>

static inline uint qHash(const Tile& key, uint seed)
{
    return qHash((53 + qHash(key.position.x())) * 53 + qHash(key.position.y()), seed);
}


static inline uint qHash(const Tile* key, uint seed)
{
    return qHash((53 + qHash(key->position.x())) * 53 + qHash(key->position.y()), seed);
}

#endif // HASHHELPERS_H
