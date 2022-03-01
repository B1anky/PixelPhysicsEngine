#include "Elements.h"
#include "Engine.h"
#include "Tile.h"
#include "Hashhelpers.h"
#include <QObject>
#include <random>

#include <QDebug>

bool PhysicalElement::Update(Engine* /*engine*/){
    return false;
}

bool PhysicalElement::GravityUpdate(Engine* engine){
    bool abidedToGravity = false;

    // a positive y implies gravity down, because it's so more dense than air.
    // a negative y implies gravity up, because it's less dense than air.
    int yDirection = 0;
    if(density != AMBIENT_DENSITY){
        yDirection = density > AMBIENT_DENSITY ? 1 : -1;
    }

    QPoint gravitatedPoint(parentTile->xPos, parentTile->yPos + yDirection);
    if(!engine->InBounds(gravitatedPoint)) return abidedToGravity;

    bool canSwap = engine->IsEmpty(gravitatedPoint)
                || ( ( engine->TileAt(gravitatedPoint).element->density < density ) && ( yDirection > 0 ) )  // We want to move down and we're more dense
                || ( ( engine->TileAt(gravitatedPoint).element->density > density ) && ( yDirection < 0 ) ); // We want to move up and we're less dense

    if(canSwap){
        engine->Swap(parentTile->xPos, parentTile->yPos, gravitatedPoint.x(), gravitatedPoint.y());
        abidedToGravity = true;
    }
    return abidedToGravity;
}

bool PhysicalElement::SpreadUpdate(Engine* engine){

    bool spread = true;
    int x = parentTile->xPos;
    int y = parentTile->yPos;
    QPoint spreadPoint;

    bool canSpreadLeft  = engine->IsEmpty(x - 1, y);
    bool canSpreadRight = engine->IsEmpty(x + 1, y);
    bool canSpreadBottomLeft  = engine->IsEmpty(x - 1, y + 1);
    bool canSpreadBottomRight = engine->IsEmpty(x + 1, y + 1);
    bool canSpreadBottomLeftDueToDensity  = engine->TileAt(x - 1, y + 1).element->density < density;
    bool canSpreadBottomRightDueToDensity = engine->TileAt(x + 1, y + 1).element->density < density;

    if (canSpreadBottomLeft && canSpreadBottomRight) { // bottom right or bottom left randomly
        bool left = ( rand() % 100 ) < 50;
        spreadPoint = left ? QPoint(x - 1, y + 1) : QPoint(x + 1, y + 1);
    } else if (canSpreadBottomLeft) { // bottom left
        spreadPoint = QPoint(x - 1, y + 1);
    } else if (canSpreadBottomRight) { // bottom right
        spreadPoint = QPoint(x + 1, y + 1);
    } else if (canSpreadBottomLeftDueToDensity && canSpreadBottomRightDueToDensity) { // bottom right or left randomly (less dense)
        bool left = ( rand() % 100 ) < 50;
        spreadPoint = left ? QPoint(x - 1, y + 1) : QPoint(x + 1, y + 1);
    } else if (canSpreadBottomLeftDueToDensity) { // bottom left (less dense)
        spreadPoint = QPoint(x - 1, y + 1);
    } else if (canSpreadBottomRightDueToDensity) { // bottom right (less dense)
        spreadPoint = QPoint(x + 1, y + 1);
    } else if (canSpreadLeft && canSpreadRight) { // right or left randomly
        bool left = ( rand() % 100 ) < 50;
        spreadPoint = left ? QPoint(x - 1, y) : QPoint(x + 1, y);
    } else if (canSpreadLeft) { // left
        spreadPoint = QPoint(x - 1, y);
    } else if (canSpreadRight) { // right
        spreadPoint = QPoint(x + 1, y);
    }
    else{
        spread = false;
    }

    if(spread){
        engine->Swap(x, y, spreadPoint.x(), spreadPoint.y());
    }

    return spread;
}

bool MoveableSolid::Update(Engine* engine){
    bool dirtied = MoveableSolid::GravityUpdate(engine);
    dirtied |= MoveableSolid::SpreadUpdate(engine);

    return dirtied;
}

bool MoveableSolid::SpreadUpdate(Engine* engine){
    bool spread = true;
    int x = parentTile->xPos;
    int y = parentTile->yPos;

    bool canSpreadDownLeft  = friction < 0.5 && engine->IsEmpty(x - 1, y + 1);
    bool canSpreadDownRight = friction < 0.5 && engine->IsEmpty(x + 1, y + 1);
    bool canSwapDownLeft    = friction < 0.5 && engine->TileAt(x - 1, y + 1).element->density < density;
    bool canSwapDownRight   = friction < 0.5 && engine->TileAt(x + 1, y + 1).element->density < density;
    QPoint spreadPoint;

    if(canSpreadDownLeft && canSpreadDownRight) {
        bool left = ( rand() % 100 ) < 50;
        spreadPoint = left ? QPoint(x - 1, y + 1) : QPoint(x + 1, y + 1);
    } else if (canSpreadDownLeft) {
        spreadPoint = QPoint(x - 1, y + 1);
    } else if (canSpreadDownRight) {
        spreadPoint = QPoint(x + 1, y + 1);
    } else if(canSwapDownLeft && canSwapDownRight) {
        bool left = ( rand() % 100 ) < 50;
        spreadPoint = left ? QPoint(x - 1, y + 1) : QPoint(x + 1, y + 1);
    } else if (canSwapDownLeft) {
        spreadPoint = QPoint(x - 1, y + 1);
    } else if (canSwapDownRight) {
        spreadPoint = QPoint(x + 1, y + 1);
    } else {
        spread = false;
    }

    if(spread){
        engine->Swap(x, y, spreadPoint.x(), spreadPoint.y());
    }

    return spread;
}

bool Liquid::Update(Engine* engine){
    bool dirtied = Liquid::GravityUpdate(engine);
    dirtied |= Liquid::SpreadUpdate(engine);

    return dirtied;
}

bool Liquid::SpreadUpdate(Engine* engine){
    bool didSpread = PhysicalElement::SpreadUpdate(engine);

    if(!didSpread){
        int spread = 1;
        bool left = true;
        if (heading == 0) {
            left = ( rand() % 100 ) < 50;
            heading = left ? -spread : spread;
        }
        QPoint targetPosition(parentTile->xPos + heading, parentTile->yPos);
        if(engine->InBounds(targetPosition)){
            // We hit a non empty tile, stop moving
            if ( (!engine->InBounds(targetPosition) || engine->TileAt(targetPosition).element->density > density )) {
                heading = 0;
            }
            // its empty move there
            else{
                engine->Swap(parentTile->xPos, parentTile->yPos, targetPosition.x(), targetPosition.y());
                didSpread = true;
            }
        }
    }

    return didSpread;
}
