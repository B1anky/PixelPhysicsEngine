#include "Elements.h"
#include "Tile.h"
#include "TileSet.h"
#include "Hashhelpers.h"
#include "Engine.h"
#include <QObject>
#include <random>

#include <QDebug>

template <typename T> int sign(T val) {
    return (T(0) < val) - (val < T(0));
}

QPoint HeadingFromPointChange(const QPoint& initialPoint, const QPoint& endPoint)
{
    QPoint heading(0, 0);

    if(initialPoint.x() < endPoint.x()){ // -> moved right
        heading.setX(1);
    }else if(initialPoint.x() > endPoint.x()){ // -> moved left
        heading.setX(-1);
    }

    if(initialPoint.y() < endPoint.y()){ // -> moved down
        heading.setY(1);
    }else if(initialPoint.y() > endPoint.y()){ // -> moved up
        heading.setY(-1);
    }

    return heading;
}

// Try to determine the y component of the velocity minus gravity.
// If falling with gravity, increment proportionally, else decrement to slow vertical ascent.
void DeltaVelocityDueToGravity(int& velocity, const QPoint& initialPoint, const QPoint& endPoint){
    // break up the component dx and dy components where velocity is the magnitude

    velocity = sign(initialPoint.y() - endPoint.y()); // - sign implies falling, 0 sign implies not falling, + sign implies rising

}

// True is left, false is right
bool HorizontalDirectionFromHeading(const QPoint& heading){
    return heading.x() != 0 ? sign(heading.x()) < 0 : ( rand() % 100 ) < 50;
}

bool PhysicalElement::Update(TileSet& /*tilesToUpdateAgainst*/, const WorkerMap& /*allWorkers*/){
    return false;
}

bool PhysicalElement::GravityUpdate(TileSet& tilesToUpdateAgainst, const WorkerMap& allWorkers){
    bool abidedToGravity = false;

    // a positive y implies gravity down, because it's so more dense than air.
    // a negative y implies gravity up, because it's less dense than air.
    int yDirection = 0;
    if(density != AMBIENT_DENSITY){
        yDirection = density > AMBIENT_DENSITY ? 1 : -1;
    }

    QPoint gravitatedPoint(parentTile->position.x(), parentTile->position.y() + yDirection);

    bool canSwap = tilesToUpdateAgainst.IsEmpty(gravitatedPoint)
                || ( ( tilesToUpdateAgainst.TileAt(gravitatedPoint).element->density < density ) && ( yDirection > 0 ) )  // We want to move down and we're more dense
                || ( ( tilesToUpdateAgainst.TileAt(gravitatedPoint).element->density > density ) && ( yDirection < 0 ) ); // We want to move up and we're less dense

    if(canSwap){

        heading = HeadingFromPointChange(parentTile->position, gravitatedPoint);

        //DeltaVelocityDueToGravity(velocity, parentTile->position, gravitatedPoint);

        tilesToUpdateAgainst.Swap(parentTile->position, gravitatedPoint);
        abidedToGravity = true;
    }else if(yDirection > 0 && !tilesToUpdateAgainst.InBounds(gravitatedPoint)){

        // Do we have a quadrant below us?
        Worker* workerBelow = allWorkers.value(tilesToUpdateAgainst.m_quadrant + QPoint(1, 0), nullptr);
        if(workerBelow != nullptr){

            gravitatedPoint.setY(0);

            //if(workerBelow->m_tileSet.m_readWriteLock.tryLockForRead(5)){
                // Swap tile with the other quadrant below.
                // Height 0 in the quadrant below would be the "bottom" of this quadrant
                Tile& otherQuadrantTile = workerBelow->m_tileSet.TileAt(gravitatedPoint);

                canSwap = otherQuadrantTile.IsEmpty()
                   || ( ( otherQuadrantTile.element->density < density ) && ( yDirection > 0 ) );  // We want to move down and we're more dense
                if(canSwap){
                    heading = HeadingFromPointChange(QPoint(parentTile->position.x(), -1), gravitatedPoint);

                    parentTile->SwapElements(otherQuadrantTile);

                    abidedToGravity = true;
                }
                //workerBelow->m_tileSet.m_readWriteLock.unlock();
            //}
        }
    }// Else if we're less dense and floating up, so check the workerAbove... TODO when we get to gases

    return abidedToGravity;
}

bool PhysicalElement::SpreadUpdate(TileSet& tilesToUpdateAgainst, const WorkerMap& allWorkers){

    bool spread = true;
    QPoint originalPoint(parentTile->position);
    int x = parentTile->position.x();
    int y = parentTile->position.y();
    QPoint spreadPoint;

    bool canSpreadLeft                    = tilesToUpdateAgainst.IsEmpty(x - 1, y);
    bool canSpreadRight                   = tilesToUpdateAgainst.IsEmpty(x + 1, y);
    bool canSpreadBottomLeft              = tilesToUpdateAgainst.IsEmpty(x - 1, y + 1) && canSpreadLeft;
    bool canSpreadBottomRight             = tilesToUpdateAgainst.IsEmpty(x + 1, y + 1) && canSpreadRight;
    bool canSpreadBottomLeftDueToDensity  = tilesToUpdateAgainst.TileAt(x - 1, y + 1).element->density < density && canSpreadLeft;
    bool canSpreadBottomRightDueToDensity = tilesToUpdateAgainst.TileAt(x + 1, y + 1).element->density < density && canSpreadRight;

    if (canSpreadBottomLeft && canSpreadBottomRight) { // bottom right or bottom left based on heading
        bool left = HorizontalDirectionFromHeading(heading);
        spreadPoint = left ? QPoint(x - 1, y + 1) : QPoint(x + 1, y + 1);
    } else if (canSpreadBottomLeft) { // bottom left
        spreadPoint = QPoint(x - 1, y + 1);
    } else if (canSpreadBottomRight) { // bottom right
        spreadPoint = QPoint(x + 1, y + 1);
    } else if (canSpreadBottomLeftDueToDensity && canSpreadBottomRightDueToDensity) { // bottom right or left based on heading (against delta density)
        bool left = HorizontalDirectionFromHeading(heading);
        spreadPoint = left ? QPoint(x - 1, y + 1) : QPoint(x + 1, y + 1);
    } else if (canSpreadBottomLeftDueToDensity) { // bottom left (less dense)
        spreadPoint = QPoint(x - 1, y + 1);
    } else if (canSpreadBottomRightDueToDensity) { // bottom right (less dense)
        spreadPoint = QPoint(x + 1, y + 1);
    } else if (canSpreadLeft && canSpreadRight) { // right or left based on heading
        bool left = HorizontalDirectionFromHeading(heading);
        spreadPoint = left ? QPoint(x - 1, y) : QPoint(x + 1, y);
    } else if (canSpreadLeft) { // left
        spreadPoint = QPoint(x - 1, y);
    } else if (canSpreadRight) { // right
        spreadPoint = QPoint(x + 1, y);
    }else{

        Tile* swappableLeftQuadrantTile(nullptr);
        Tile* swappableRightQuadrantTile(nullptr);
        Tile* swappableBottomLeftQuadrantTile(nullptr);
        Tile* swappableBottomRightQuadrantTile(nullptr);

        // If we can't go left anymore in our quadrant...
        if(!tilesToUpdateAgainst.InBounds(originalPoint + QPoint(-1, 0))){
            Worker* leftWorker = allWorkers.value(tilesToUpdateAgainst.m_quadrant + QPoint(0, -1), nullptr);
            if(leftWorker != nullptr && leftWorker->m_tileSet.m_readWriteLock.tryLockForRead()){
                TileSet& leftTileSet   = leftWorker->m_tileSet;
                Tile& leftQuadrantTile = leftTileSet.TileAt(leftTileSet.width() - 1, originalPoint.y()); // tile left at current height in left quadrant
                if(leftQuadrantTile.IsEmpty() || leftQuadrantTile.element->density < density){
                    heading = HeadingFromPointChange(originalPoint, originalPoint + QPoint(-1, 0));
                    DeltaVelocityDueToGravity(velocity, originalPoint, originalPoint + QPoint(-1, 0));
                    canSpreadLeft = true;
                    swappableLeftQuadrantTile = &leftQuadrantTile;
                }
                leftWorker->m_tileSet.m_readWriteLock.unlock();
            }
        }

        // If we can't go right anymore in our quadrant...
        if(!tilesToUpdateAgainst.InBounds(originalPoint + QPoint(1, 0))){
            Worker* rightWorker = allWorkers.value(tilesToUpdateAgainst.m_quadrant + QPoint(0, 1), nullptr);
            if(rightWorker != nullptr && rightWorker->m_tileSet.m_readWriteLock.tryLockForRead()){
                TileSet& rightTileSet  = rightWorker->m_tileSet;
                Tile& rightQuadrantTile = rightTileSet.TileAt(0, originalPoint.y()); // tile right at current height in right quadrant
                if(rightQuadrantTile.IsEmpty() || rightQuadrantTile.element->density < density){
                    heading = HeadingFromPointChange(originalPoint, originalPoint + QPoint(1, 0));
                    DeltaVelocityDueToGravity(velocity, originalPoint, originalPoint + QPoint(1, 0));
                    canSpreadRight = true;
                    swappableRightQuadrantTile = &rightQuadrantTile;
                }
                rightWorker->m_tileSet.m_readWriteLock.unlock();
            }
        }

        // We prioritize bottom left and right first, so since everything failed up above, do we have a quadrant below us to the left?
        // Also we have to be careful at the edge of 4 quadrants that we properly handle checking against the right tile since
        // going left could give us a Nth column quadrant and going right could be a Nth + 1 column quadrant.
        // If we can't go bottom left anymore in our quadrant...
        if(!tilesToUpdateAgainst.InBounds(originalPoint + QPoint(-1, 1))){
            Worker* bottomLeftWorker = allWorkers.value(tilesToUpdateAgainst.m_quadrant + QPoint(1, -1), nullptr);
            // This is very unlikely to occur...
            if(bottomLeftWorker != nullptr && bottomLeftWorker->m_tileSet.m_readWriteLock.tryLockForRead()){
                TileSet& bottomLeftTileSet = bottomLeftWorker->m_tileSet;
                Tile& bottomLeftQuadrantTile = bottomLeftTileSet.TileAt(bottomLeftTileSet.width() - 1, 0); // top right of the bottom left quadrant
                if(bottomLeftQuadrantTile.IsEmpty() || bottomLeftQuadrantTile.element->density < density){
                    heading = HeadingFromPointChange(originalPoint, originalPoint + QPoint(-1, 1));
                    DeltaVelocityDueToGravity(velocity, originalPoint, originalPoint + QPoint(-1, 1));
                    canSpreadBottomLeft = true;
                    swappableBottomLeftQuadrantTile = &bottomLeftQuadrantTile;
                }
                bottomLeftWorker->m_tileSet.m_readWriteLock.unlock();
            }
        }

        // If we can't go bottom right anymore in our quadrant...
        if(!tilesToUpdateAgainst.InBounds(originalPoint + QPoint(1, 1))){
            // Now the same logic, but for the bottom right quadrant...
            Worker* bottomRightWorker = allWorkers.value(tilesToUpdateAgainst.m_quadrant + QPoint(1, 1), nullptr);
            // This is very unlikely to occur as well...
            if(bottomRightWorker != nullptr && bottomRightWorker->m_tileSet.m_readWriteLock.tryLockForRead()){
                TileSet& bottomRightTileSet = bottomRightWorker->m_tileSet;
                Tile& bottomRightQuadrantTile = bottomRightTileSet.TileAt(0, 0); // top left of the bottom right quadrant
                if(bottomRightQuadrantTile.IsEmpty() || bottomRightQuadrantTile.element->density < density){
                    heading = HeadingFromPointChange(originalPoint, originalPoint + QPoint(1, 1));
                    DeltaVelocityDueToGravity(velocity, originalPoint, originalPoint + QPoint(1, 1));
                    canSpreadBottomRight = true;
                    swappableBottomRightQuadrantTile = &bottomRightQuadrantTile;
                }
                bottomRightWorker->m_tileSet.m_readWriteLock.unlock();
            }
        }

        if (canSpreadBottomLeft && canSpreadBottomRight) { // bottom right or bottom left based on heading
            bool left = HorizontalDirectionFromHeading(heading);
            left ? parentTile->SwapElements(*swappableBottomLeftQuadrantTile) : parentTile->SwapElements(*swappableBottomRightQuadrantTile);
            return true;
        } else if (canSpreadBottomLeft) { // bottom left
            parentTile->SwapElements(*swappableBottomLeftQuadrantTile);
            return true;
        } else if (canSpreadBottomRight) { // bottom right
            parentTile->SwapElements(*swappableBottomRightQuadrantTile);
            return true;
        } else if (canSpreadLeft && canSpreadRight) { // right or left based on heading
            bool left = HorizontalDirectionFromHeading(heading);
            left ? parentTile->SwapElements(*swappableLeftQuadrantTile) : parentTile->SwapElements(*swappableRightQuadrantTile);
            return true;
        } else if (canSpreadLeft) { // left
            parentTile->SwapElements(*swappableLeftQuadrantTile);
            return true;
        } else if (canSpreadRight) { // right
            parentTile->SwapElements(*swappableRightQuadrantTile);
            return true;
        }else{
            // Couldn't spread to any other quadrant...
            spread = false;
        }

    }

    if(spread){
        heading = HeadingFromPointChange(originalPoint, spreadPoint);
        DeltaVelocityDueToGravity(velocity, originalPoint, spreadPoint);
        tilesToUpdateAgainst.Swap(originalPoint, spreadPoint);
    }

    return spread;
}

bool MoveableSolid::Update(TileSet& tilesToUpdateAgainst, const WorkerMap& allWorkers){
    bool dirtied = MoveableSolid::GravityUpdate(tilesToUpdateAgainst, allWorkers);
    dirtied |= MoveableSolid::SpreadUpdate(tilesToUpdateAgainst, allWorkers);

    return dirtied;
}

bool MoveableSolid::SpreadUpdate(TileSet& tilesToUpdateAgainst, const WorkerMap& allWorkers){
    bool spread = true;
    int x = parentTile->position.x();
    int y = parentTile->position.y();

    bool canSpreadLeft                    = friction < 0.5 && tilesToUpdateAgainst.IsEmpty(x - 1, y);
    bool canSpreadRight                   = friction < 0.5 && tilesToUpdateAgainst.IsEmpty(x + 1, y);
    bool canSpreadDownLeft                = friction < 0.5 && tilesToUpdateAgainst.IsEmpty(x - 1, y + 1) && canSpreadLeft;
    bool canSpreadDownRight               = friction < 0.5 && tilesToUpdateAgainst.IsEmpty(x + 1, y + 1) && canSpreadRight;
    bool canSpreadBottomLeftDueToDensity  = friction < 0.5 && tilesToUpdateAgainst.TileAt(x - 1, y + 1).element->density < density && canSpreadLeft;
    bool canSpreadBottomRightDueToDensity = friction < 0.5 && tilesToUpdateAgainst.TileAt(x + 1, y + 1).element->density < density && canSpreadRight;
    QPoint spreadPoint;

    if(canSpreadDownLeft && canSpreadDownRight) {
        bool left = HorizontalDirectionFromHeading(heading);
        spreadPoint = left ? QPoint(x - 1, y + 1) : QPoint(x + 1, y + 1);
    } else if (canSpreadDownLeft) {
        spreadPoint = QPoint(x - 1, y + 1);
    } else if (canSpreadDownRight) {
        spreadPoint = QPoint(x + 1, y + 1);
    } else if(canSpreadBottomLeftDueToDensity && canSpreadBottomRightDueToDensity) {
        bool left = HorizontalDirectionFromHeading(heading);
        spreadPoint = left ? QPoint(x - 1, y + 1) : QPoint(x + 1, y + 1);
    } else if (canSpreadBottomLeftDueToDensity) {
        spreadPoint = QPoint(x - 1, y + 1);
    } else if (canSpreadBottomRightDueToDensity) {
        spreadPoint = QPoint(x + 1, y + 1);
    } else {
        spread = false;
    }

    if(spread){
        heading = HeadingFromPointChange(parentTile->position, spreadPoint);
        DeltaVelocityDueToGravity(velocity, parentTile->position, spreadPoint);
        tilesToUpdateAgainst.Swap(parentTile->position, spreadPoint);
    }

    return spread;
}

bool Liquid::Update(TileSet& tilesToUpdateAgainst, const WorkerMap& allWorkers){
    bool dirtied = Liquid::GravityUpdate(tilesToUpdateAgainst, allWorkers);
    gravityUpdated = dirtied;
    dirtied |= Liquid::SpreadUpdate(tilesToUpdateAgainst, allWorkers);

    return dirtied;
}

bool Liquid::SpreadUpdate(TileSet& tilesToUpdateAgainst, const WorkerMap& allWorkers){
    bool didSpread = PhysicalElement::SpreadUpdate(tilesToUpdateAgainst, allWorkers);

    if(!gravityUpdated && !didSpread){

        if(heading.x() == 0){
            int spread = tilesToUpdateAgainst.width() * allWorkers.value(tilesToUpdateAgainst.m_quadrant)->totalWorkerColumns_;
            bool left = HorizontalDirectionFromHeading(heading);
            // Should compound the liquid's heading in its already moving direction
            heading.setX(left ? spread : -spread );
        }

        QPoint spreadPoint(parentTile->position.x(), parentTile->position.y());

        // Efficiency check. Don't look for next spot if you're completely surrounded.
        // Note: This is a temporary fix and does have some visual side effects.
        //       TODO below emphasizes the expensive logic we're tyring to skip at all costs.
        //       Fluids are expensive to simulate...
        bool isSurrounded = true;
        for(int i = -1; i <= 1; ++i){
            for(int j = -1; j <= 1; ++j){
                QPoint offsetPoint(i,j);
                if(tilesToUpdateAgainst.IsEmpty(parentTile->position + offsetPoint)){
                    isSurrounded = false;
                    break;
                }
            }
        }

        if(isSurrounded){
            heading.setX(0);
            return didSpread;
        }

        // We hit a non empty tile, stop moving
        // We have to loop over every point in betweem current location and target to see if something will stop us early.
        // TODO: Do this logic in its own thread since it's very expensive.
        int spreadDirection = sign(heading.x());
        QPoint potentialPoint(0, 0);
        for(int finalSpreadOffset = 0; finalSpreadOffset < abs(heading.x()); ++finalSpreadOffset){
            potentialPoint.setX(spreadPoint.x() + (spreadDirection * finalSpreadOffset));
            potentialPoint.setY(spreadPoint.y());
            bool outOfBounds = !tilesToUpdateAgainst.InBounds(potentialPoint);
            if ( (outOfBounds || tilesToUpdateAgainst.TileAt(potentialPoint).element->density > density ) ){

                // If specifically we're not in bounds towards the potential point, we should check to see if there's a quadrant to our left.
                if(outOfBounds){

                    // negative spreadDirection implies we should see if we have a quadrant to our left and if we can swap with its right side.
                    if(spreadDirection < 0){

                        Tile* swappableLeftQuadrantTile(nullptr);
                        // If we can't go left anymore in our quadrant...
                        Worker* leftWorker = allWorkers.value(tilesToUpdateAgainst.m_quadrant + QPoint(0, -1), nullptr);
                        if(leftWorker != nullptr && leftWorker->m_tileSet.m_readWriteLock.tryLockForRead()){
                            TileSet& leftTileSet   = leftWorker->m_tileSet;
                            Tile& leftQuadrantTile = leftTileSet.TileAt(leftTileSet.width() - finalSpreadOffset, potentialPoint.y()); // tile left at current height in left quadrant
                            if(leftQuadrantTile.IsEmpty() || leftQuadrantTile.element->density < density){
                                swappableLeftQuadrantTile = &leftQuadrantTile;
                            }
                            leftWorker->m_tileSet.m_readWriteLock.unlock();
                        }

                        if(swappableLeftQuadrantTile != nullptr){
                            heading.setX(0);
                            parentTile->SwapElements(*swappableLeftQuadrantTile);
                            return true;
                        }

                    }else if(spreadDirection > 0){
                        Tile* swappableRightQuadrantTile(nullptr);
                        // If we can't go left anymore in our quadrant...
                        Worker* rightWorker = allWorkers.value(tilesToUpdateAgainst.m_quadrant + QPoint(0, 1), nullptr);
                        if(rightWorker != nullptr && rightWorker->m_tileSet.m_readWriteLock.tryLockForRead()){
                            TileSet& rightTileSet   = rightWorker->m_tileSet;
                            Tile& rightQuadrantTile = rightTileSet.TileAt(finalSpreadOffset, potentialPoint.y()); // tile left at current height in left quadrant
                            if(rightQuadrantTile.IsEmpty() || rightQuadrantTile.element->density < density){
                                swappableRightQuadrantTile = &rightQuadrantTile;
                            }
                            rightWorker->m_tileSet.m_readWriteLock.unlock();
                        }

                        if(swappableRightQuadrantTile != nullptr){
                            heading.setX(0);
                            parentTile->SwapElements(*swappableRightQuadrantTile);
                            return true;
                        }
                    }

                }else{
                    spreadPoint.setX(spreadPoint.x() + (spreadDirection * finalSpreadOffset));
                    heading.setX(0);
                }
                break;
            }else if(tilesToUpdateAgainst.IsEmpty(potentialPoint)){
                spreadPoint = potentialPoint;
                heading.setX(0);
                break;
            }
        }

        if(tilesToUpdateAgainst.IsEmpty(spreadPoint)){
            tilesToUpdateAgainst.Swap(parentTile->position, spreadPoint);
            didSpread = true;
        }
    }
    return didSpread;
}
