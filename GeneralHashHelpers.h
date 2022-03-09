#ifndef GENERALHASHHELPERS_H
#define GENERALHASHHELPERS_H

#include <QPoint>

static inline bool operator<(const QPoint& p1,const QPoint& p2)
{
    return p1.x() < p2.x() || p1.y() < p2.y();
}

static inline uint qHash(const QPoint& key, uint seed)
{
    return qHash((53 + qHash(key.x())) * 53 + qHash(key.y()), seed);
}

#endif // GENERALHASHHELPERS_H
