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

bool PhysicalElement::Update(TileSet& /*tilesToUpdateAgainst*/,  Worker* /*bossWorker*/){
    return false;
}

bool PhysicalElement::GravityUpdate(TileSet& tilesToUpdateAgainst, Worker* bossWorker){
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

    // Nice, efficient quick internal swapping logic. No neighbors needed. Most cases will fall into this.
    if(canSwap){

        heading = HeadingFromPointChange(parentTile->position, gravitatedPoint);
        //DeltaVelocityDueToGravity(velocity, parentTile->position, gravitatedPoint);
        tilesToUpdateAgainst.Swap(parentTile->position, gravitatedPoint);
        abidedToGravity = true;

    }
    // If going down goes out of bounds, attempt to swap with our bottom neighbor.
    else if(yDirection > 0 && !tilesToUpdateAgainst.InBounds(gravitatedPoint)){

        // Do we have a quadrant below us? If not, then we're actually on the ground.
        if(bossWorker->NeighborSwap(Worker::Direction::BOTTOM, parentTile->element)){
            active = false;
        }

    }
    // If going up goes out of bounds, attempt to swap with our top neighbor.
    else if(yDirection < 0 && !tilesToUpdateAgainst.InBounds(gravitatedPoint)){

        // Do we have a quadrant above us? If not, then we're actually on the ceiling.
        if(bossWorker->NeighborSwap(Worker::Direction::TOP, parentTile->element)){
            active = false;
        }

    }

    return abidedToGravity;
}

bool PhysicalElement::SpreadUpdate(TileSet& tilesToUpdateAgainst, Worker* bossWorker){

    bool spread = true;
    QPoint originalPoint(parentTile->position);
    int x = parentTile->position.x();
    int y = parentTile->position.y();
    QPoint spreadPoint;

    bool canSpreadLeft                    = ( tilesToUpdateAgainst.IsEmpty(x - 1, y)   ) || bossWorker->HasNeighbor(Worker::Direction::LEFT);
    bool canSpreadRight                   = ( tilesToUpdateAgainst.IsEmpty(x + 1, y)   ) || bossWorker->HasNeighbor(Worker::Direction::RIGHT);
    bool canSpreadBottomLeft              = ( tilesToUpdateAgainst.IsEmpty(x - 1, y + 1) || bossWorker->HasNeighbor(Worker::Direction::BOTTOM_LEFT) )  && canSpreadLeft;
    bool canSpreadBottomRight             = ( tilesToUpdateAgainst.IsEmpty(x + 1, y + 1) || bossWorker->HasNeighbor(Worker::Direction::BOTTOM_RIGHT) ) && canSpreadRight;
    bool canSpreadBottomLeftDueToDensity  = ( tilesToUpdateAgainst.TileAt (x - 1, y + 1).element->density < density || bossWorker->HasNeighbor(Worker::Direction::BOTTOM_LEFT) ) && canSpreadLeft;
    bool canSpreadBottomRightDueToDensity = ( tilesToUpdateAgainst.TileAt (x + 1, y + 1).element->density < density || bossWorker->HasNeighbor(Worker::Direction::BOTTOM_RIGHT) ) && canSpreadRight;

    Worker* targetNeighbor(nullptr);

    if (canSpreadBottomLeft && canSpreadBottomRight) { // bottom right or bottom left based on heading
        bool left = HorizontalDirectionFromHeading(heading);
        spreadPoint    = left ? QPoint(x - 1, y + 1) : QPoint(x + 1, y + 1);
        targetNeighbor = left ? bossWorker->bottomNeighbor_ : bossWorker->bottomNeighbor_;
    } else if (canSpreadBottomLeft) { // bottom left
        spreadPoint    = QPoint(x - 1, y + 1);
        targetNeighbor = bossWorker->bottomLeftNeighbor_;
    } else if (canSpreadBottomRight) { // bottom right
        spreadPoint    = QPoint(x + 1, y + 1);
        targetNeighbor = bossWorker->bottomRightNeighbor_;
    } else if (canSpreadBottomLeftDueToDensity && canSpreadBottomRightDueToDensity) { // bottom right or left based on heading (against delta density)
        bool left = HorizontalDirectionFromHeading(heading);
        spreadPoint    = left ? QPoint(x - 1, y + 1) : QPoint(x + 1, y + 1);
        targetNeighbor = left ? bossWorker->bottomNeighbor_ : bossWorker->bottomNeighbor_;
    } else if (canSpreadBottomLeftDueToDensity) { // bottom left (less dense)
        spreadPoint    = QPoint(x - 1, y + 1);
        targetNeighbor = bossWorker->bottomLeftNeighbor_;
    } else if (canSpreadBottomRightDueToDensity) { // bottom right (less dense)
        spreadPoint    = QPoint(x + 1, y + 1);
        targetNeighbor = bossWorker->bottomRightNeighbor_;
    } else if (canSpreadLeft && canSpreadRight) { // right or left based on heading
        bool left = HorizontalDirectionFromHeading(heading);
        spreadPoint    = left ? QPoint(x - 1, y) : QPoint(x + 1, y);
        targetNeighbor = left ? bossWorker->leftNeighbor_ : bossWorker->rightNeighbor_;
    } else if (canSpreadLeft) { // left
        spreadPoint    = QPoint(x - 1, y);
        targetNeighbor = bossWorker->leftNeighbor_;
    } else if (canSpreadRight) { // right
        spreadPoint = QPoint(x + 1, y);
        targetNeighbor = bossWorker->rightNeighbor_;
    }else{
        // Couldn't spread to any other quadrant...
        spread = false;
    }

    if(spread){
        //If spread point is in bounds, we can just do it, if not we need to try swapping with the neighbor.
        if(tilesToUpdateAgainst.IsEmpty(spreadPoint)){
            heading = HeadingFromPointChange(originalPoint, spreadPoint);
            DeltaVelocityDueToGravity(velocity, originalPoint, spreadPoint);
            tilesToUpdateAgainst.Swap(originalPoint, spreadPoint);
        }else if(!tilesToUpdateAgainst.InBounds(spreadPoint) && targetNeighbor != nullptr && bossWorker->NeighborSwap(bossWorker->NeighborDirection(targetNeighbor), this->parentTile->element)){
            heading = HeadingFromPointChange(originalPoint, spreadPoint);
            DeltaVelocityDueToGravity(velocity, originalPoint, spreadPoint);
            active = false;
        }
    }

    return spread;
}

bool MoveableSolid::Update(TileSet& tilesToUpdateAgainst, Worker* bossWorker){
    bool dirtied = MoveableSolid::GravityUpdate(tilesToUpdateAgainst, bossWorker);
    if(active){
        dirtied |= MoveableSolid::SpreadUpdate(tilesToUpdateAgainst, bossWorker);
    }
    return dirtied;
}

bool MoveableSolid::SpreadUpdate(TileSet& tilesToUpdateAgainst, Worker* bossWorker){
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

bool Liquid::Update(TileSet& tilesToUpdateAgainst, Worker* bossWorker){
    bool dirtied = Liquid::GravityUpdate(tilesToUpdateAgainst, bossWorker);
    //if(active){
    //    gravityUpdated = dirtied;
    //    dirtied |= Liquid::SpreadUpdate(tilesToUpdateAgainst, bossWorker);
    //}
    return dirtied;
}

bool Liquid::SpreadUpdate(TileSet& tilesToUpdateAgainst, Worker* bossWorker){
    bool didSpread = PhysicalElement::SpreadUpdate(tilesToUpdateAgainst, bossWorker);

    if(active && !gravityUpdated && !didSpread){

        if(heading.x() == 0){
            int spread = bossWorker->workerImage_.width();
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
//            if ( (outOfBounds || tilesToUpdateAgainst.TileAt(potentialPoint).element->density > density ) ){

//                // If specifically we're not in bounds towards the potential point, we should check to see if there's a quadrant to our left.
//                if(outOfBounds){

//                    // negative spreadDirection implies we should see if we have a quadrant to our left and if we can swap with its right side.
//                    if(spreadDirection < 0){

//                        Tile* swappableLeftQuadrantTile(nullptr);
//                        // If we can't go left anymore in our quadrant...
//                        Worker* leftWorker = allWorkers.value(tilesToUpdateAgainst.m_quadrant + QPoint(0, -1), nullptr);
//                        if(leftWorker != nullptr && leftWorker->m_tileSet.m_readWriteLock.tryLockForRead()){
//                            TileSet& leftTileSet   = leftWorker->m_tileSet;
//                            Tile& leftQuadrantTile = leftTileSet.TileAt(leftTileSet.width() - finalSpreadOffset, potentialPoint.y()); // tile left at current height in left quadrant
//                            if(leftQuadrantTile.IsEmpty() || leftQuadrantTile.element->density < density){
//                                swappableLeftQuadrantTile = &leftQuadrantTile;
//                            }
//                            leftWorker->m_tileSet.m_readWriteLock.unlock();
//                        }

//                        if(swappableLeftQuadrantTile != nullptr){
//                            heading.setX(0);
//                            parentTile->SwapElements(*swappableLeftQuadrantTile);
//                            return true;
//                        }

//                    }else if(spreadDirection > 0){
//                        Tile* swappableRightQuadrantTile(nullptr);
//                        // If we can't go left anymore in our quadrant...
//                        Worker* rightWorker = allWorkers.value(tilesToUpdateAgainst.m_quadrant + QPoint(0, 1), nullptr);
//                        if(rightWorker != nullptr && rightWorker->m_tileSet.m_readWriteLock.tryLockForRead()){
//                            TileSet& rightTileSet   = rightWorker->m_tileSet;
//                            Tile& rightQuadrantTile = rightTileSet.TileAt(finalSpreadOffset, potentialPoint.y()); // tile left at current height in left quadrant
//                            if(rightQuadrantTile.IsEmpty() || rightQuadrantTile.element->density < density){
//                                swappableRightQuadrantTile = &rightQuadrantTile;
//                            }
//                            rightWorker->m_tileSet.m_readWriteLock.unlock();
//                        }

//                        if(swappableRightQuadrantTile != nullptr){
//                            heading.setX(0);
//                            parentTile->SwapElements(*swappableRightQuadrantTile);
//                            return true;
//                        }
//                    }
//
//                }else{
//                    spreadPoint.setX(spreadPoint.x() + (spreadDirection * finalSpreadOffset));
//                    heading.setX(0);
//                }
//                break;
//            }else if(tilesToUpdateAgainst.IsEmpty(potentialPoint)){
//                spreadPoint = potentialPoint;
//                heading.setX(0);
//                break;
//            }
        }

        if(tilesToUpdateAgainst.IsEmpty(spreadPoint)){
            tilesToUpdateAgainst.Swap(parentTile->position, spreadPoint);
            didSpread = true;
        }
    }
    return didSpread;
}
