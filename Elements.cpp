#include "Elements.h"
#include "Engine.h"
#include "Tile.h"
#include "Hashhelpers.h"
#include <QObject>
#include <random>

#include <QDebug>

bool PhysicalElement::Update(Engine* engine){
    //return GravityUpdate(engine);
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
    bool canSpreadBottomLeftDueToDensity  = engine->TileAt(x - 1, y + 1).element->density <= density;
    bool canSpreadBottomRightDueToDensity = engine->TileAt(x + 1, y + 1).element->density <= density;

    if (canSpreadLeft && canSpreadRight) { // right or left randomly
        bool left = ( rand() % 100 ) < 50;
        spreadPoint = left ? QPoint(x - 1, y) : QPoint(x + 1, y);
    } else if (canSpreadLeft && !canSpreadRight) { // left
        spreadPoint = QPoint(x - 1, y);
    } else if (!canSpreadLeft && canSpreadRight) { // right
        spreadPoint = QPoint(x + 1, y);
    } else if (canSpreadBottomLeft && canSpreadBottomRight) { // bottom right or bottom left randomly
        bool left = ( rand() % 100 ) < 50;
        spreadPoint = left ? QPoint(x - 1, y + 1) : QPoint(x + 1, y + 1);
    } else if (canSpreadBottomLeft && !canSpreadBottomRight) { // bottom left
        spreadPoint = QPoint(x - 1, y + 1);
    } else if (!canSpreadBottomLeft && canSpreadBottomRight) { // bottom right
        spreadPoint = QPoint(x + 1, y + 1);
    } else if (canSpreadBottomLeftDueToDensity && canSpreadBottomRightDueToDensity) { // bottom right or left randomly (less dense)
        bool left = ( rand() % 100 ) < 50;
        spreadPoint = left ? QPoint(x - 1, y + 1) : QPoint(x + 1, y + 1);
    } else if (canSpreadBottomLeftDueToDensity && !canSpreadBottomRightDueToDensity) { // bottom left (less dense)
        spreadPoint = QPoint(x - 1, y + 1);
    } else if (!canSpreadBottomLeftDueToDensity && canSpreadBottomRightDueToDensity) { // bottom right (less dense)
        spreadPoint = QPoint(x + 1, y + 1);
    } else{
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

    if (canSpreadDownLeft && !canSpreadDownRight) {
        spreadPoint = QPoint(x - 1, y + 1);
    } else if (!canSpreadDownLeft && canSpreadDownRight) {
        spreadPoint = QPoint(x + 1, y + 1);
    } else if(canSpreadDownLeft && canSpreadDownRight) {
        bool left = ( rand() % 100 ) < 50;
        spreadPoint = left ? QPoint(x - 1, y + 1) : QPoint(x + 1, y + 1);
    } else if (canSwapDownLeft && !canSwapDownRight) {
        spreadPoint = QPoint(x - 1, y + 1);
    } else if (!canSwapDownLeft && canSwapDownRight) {
        spreadPoint = QPoint(x + 1, y + 1);
    } else if(canSwapDownLeft && canSwapDownRight) {
        bool left = ( rand() % 100 ) < 50;
        spreadPoint = left ? QPoint(x - 1, y + 1) : QPoint(x + 1, y + 1);
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
    //if(!dirtied){
        dirtied = Liquid::SpreadUpdate(engine);
    //}

    return dirtied;
}

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

bool Liquid::SpreadUpdate(Engine* engine){
    bool didSpread = PhysicalElement::SpreadUpdate(engine);

    if(!didSpread){
        int xPos = parentTile->xPos;
        int yPos = parentTile->yPos;
        // If both of our tiles to our left or right aren't empty, don't do this expensive logic.
        if(!engine->IsEmpty(xPos - 1, yPos) && !engine->IsEmpty(xPos + 1, yPos) ){
            return didSpread;
        }

        //Tile* currentTile = this;
        int spread = 1;
        bool left = true;
        if (heading == 0) {
            spread = 1;
            left = rand() % 100 < 50;
        }
        QPoint updatedPosition(parentTile->xPos + spread, parentTile->yPos);
        while(engine->InBounds(updatedPosition)){
            heading = left ? -spread : spread;
            // Check the tile in the direction we are heading
            QPoint spreadPoint(parentTile->xPos + heading, parentTile->yPos);
            // We hit a non empty tile, stop moving
            if ( (!engine->InBounds(spreadPoint) || engine->TileAt(spreadPoint).element->density > density )) {
                heading = 0;
                return didSpread;
            }
            // its empty move there
            engine->Swap(parentTile->xPos, parentTile->yPos, spreadPoint.x(), spreadPoint.y());
            updatedPosition = spreadPoint;
        }
    }

    return didSpread;
}
