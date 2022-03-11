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

bool Element::NeighborSwapped(Worker* bossWorker, const QPoint& spreadPoint, TileSet& tilesToUpdateAgainst, int yDirection){

    bool swapped = false;
    Worker* targetNeighbor(nullptr);

    // Y = [0, height) -> Left or Right neighbor, regardless of density.
    if(spreadPoint.y() <= tilesToUpdateAgainst.height() && spreadPoint.y() >= 0){
        if(spreadPoint.x() < 0){
            targetNeighbor = bossWorker->leftNeighbor_;
        }else if(spreadPoint.x() >= tilesToUpdateAgainst.width()){
            targetNeighbor = bossWorker->rightNeighbor_;
        }
    }
    // Y >= height -> Bottom left, Bottom right, or bottom neighbor if denser than ambient gravity.
    else if(yDirection > 0){
        if(spreadPoint.x() < 0){
            targetNeighbor = bossWorker->bottomLeftNeighbor_;
        }else if(spreadPoint.x() > tilesToUpdateAgainst.width()){
            targetNeighbor = bossWorker->bottomRightNeighbor_;
        }else{
            targetNeighbor = bossWorker->bottomNeighbor_;
        }
    }
    // Y <= 0 -> Top left, Top right, or Top neighbor if less dense than ambient gravity.
    else if(yDirection < 0){
        if(spreadPoint.x() < 0){
            targetNeighbor = bossWorker->topLeftNeighbor_;
        }else if(spreadPoint.x() > tilesToUpdateAgainst.width()){
            targetNeighbor = bossWorker->topRightNeighbor_;
        }else{
            targetNeighbor = bossWorker->topNeighbor_;
        }
    }

    if( targetNeighbor != nullptr){
        if(bossWorker->NeighborSwap(bossWorker->NeighborDirection(targetNeighbor), this->parentTile->element)){
            active  = false;
            swapped = true;
        }
    }
    return swapped;
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

    int yDirection = 0;
    if(density != AMBIENT_DENSITY){
        yDirection = density > AMBIENT_DENSITY ? 1 : -1;
    }

    bool spread = false;
    bool canSpreadLocally = true;
    QPoint originalPoint(parentTile->position);
    int x = parentTile->position.x();
    int y = parentTile->position.y();
    QPoint spreadPoint;

    bool canSpreadLeft                    = tilesToUpdateAgainst.IsEmpty(x - 1, y);
    bool canSpreadRight                   = tilesToUpdateAgainst.IsEmpty(x + 1, y);
    bool canSpreadBottomLeft              = tilesToUpdateAgainst.IsEmpty(x - 1, y + 1);
    bool canSpreadBottomRight             = tilesToUpdateAgainst.IsEmpty(x + 1, y + 1);
    bool canSpreadBottomLeftDueToDensity  = tilesToUpdateAgainst.TileAt (x - 1, y + 1).element->density < density && canSpreadLeft;
    bool canSpreadBottomRightDueToDensity = tilesToUpdateAgainst.TileAt (x + 1, y + 1).element->density < density && canSpreadRight;

    if (canSpreadBottomLeft && canSpreadBottomRight) { // bottom right or bottom left based on heading
        bool left   = HorizontalDirectionFromHeading(heading);
        spreadPoint = left ? QPoint(x - 1, y + 1) : QPoint(x + 1, y + 1);
    } else if (canSpreadBottomLeft) { // bottom left
        spreadPoint = QPoint(x - 1, y + 1);
    } else if (canSpreadBottomRight) { // bottom right
        spreadPoint = QPoint(x + 1, y + 1);
    } else if (canSpreadBottomLeftDueToDensity && canSpreadBottomRightDueToDensity) { // bottom right or left based on heading (against delta density)
        bool left   = HorizontalDirectionFromHeading(heading);
        spreadPoint = left ? QPoint(x - 1, y + 1) : QPoint(x + 1, y + 1);
    } else if (canSpreadBottomLeftDueToDensity) { // bottom left (less dense)
        spreadPoint = QPoint(x - 1, y + 1);
    } else if (canSpreadBottomRightDueToDensity) { // bottom right (less dense)
        spreadPoint = QPoint(x + 1, y + 1);
    } else if (canSpreadLeft && canSpreadRight) { // right or left based on heading
        bool left   = HorizontalDirectionFromHeading(heading);
        spreadPoint = left ? QPoint(x - 1, y) : QPoint(x + 1, y);
    } else if (canSpreadLeft) { // left
        spreadPoint = QPoint(x - 1, y);
    } else if (canSpreadRight) { // right
        spreadPoint = QPoint(x + 1, y);
    }else{
        canSpreadLocally = false;
    }

    //If spread point is in bounds, we can just do it, if not we need to try swapping with the neighbor.
    if(canSpreadLocally){
        heading = HeadingFromPointChange(originalPoint, spreadPoint);
        //DeltaVelocityDueToGravity(velocity, originalPoint, spreadPoint);
        tilesToUpdateAgainst.Swap(originalPoint, spreadPoint);
        spread = true;
    }

    // If we couldn't spread locally, then try to find, with the same rule priority, the closest neighbor.
    if(!canSpreadLocally){

        bool left   = HorizontalDirectionFromHeading(heading);
        spreadPoint = left ? QPoint(x - 1, y + 1) : QPoint(x + 1, y + 1);

        if(NeighborSwapped(bossWorker, spreadPoint, tilesToUpdateAgainst, yDirection)){
            heading = HeadingFromPointChange(originalPoint, spreadPoint);
            //DeltaVelocityDueToGravity(velocity, originalPoint, spreadPoint);
            active = false;
            spread = true;
        }

        // If we couldn't swap bottom left or bottom right, try left or right
        // Note: Code seems dead, but not sure why. There definitely should be cases where
        //       the above NeighboreSwapped can fail.
        if(!spread){
            spreadPoint = left ? QPoint(x - 1, y) : QPoint(x + 1, y);
            if(NeighborSwapped(bossWorker, spreadPoint, tilesToUpdateAgainst, yDirection)){
                heading = HeadingFromPointChange(originalPoint, spreadPoint);
                //DeltaVelocityDueToGravity(velocity, originalPoint, spreadPoint);
                active = false;
                spread = true;
            }
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
    if(active){
        gravityUpdated = dirtied;
        dirtied |= Liquid::SpreadUpdate(tilesToUpdateAgainst, bossWorker);
    }
    return dirtied;
}

bool Liquid::SpreadUpdate(TileSet& tilesToUpdateAgainst, Worker* bossWorker){
    bool didSpread = PhysicalElement::SpreadUpdate(tilesToUpdateAgainst, bossWorker);

    if(active && !gravityUpdated && !didSpread){

        //if(heading.x() == 0){
            int spread = bossWorker->workerImage_.width();
            bool left  = HorizontalDirectionFromHeading(heading);
            heading.setX(left ? spread : -spread );
        //}

        QPoint spreadPoint(parentTile->position.x(), parentTile->position.y());

        // We hit a non empty tile, stop moving
        // We have to loop over every point in betweem current location and target to see if something will stop us early.
        int spreadDirection = sign(heading.x());
        QPoint potentialPoint(0, 0);
        for(int finalSpreadOffset = 0; finalSpreadOffset < abs(heading.x()); ++finalSpreadOffset){
            potentialPoint.setX(spreadPoint.x() + (spreadDirection * finalSpreadOffset));
            potentialPoint.setY(spreadPoint.y());
            bool outOfBounds = !tilesToUpdateAgainst.InBounds(potentialPoint);
            if ( outOfBounds || ( tilesToUpdateAgainst.TileAt(potentialPoint).element->density > density ) ){

                // If specifically we're not in bounds towards the potential point, we should check to see if there's a quadrant to our left.
                if(outOfBounds){

                    int yDirection = 0;
                    if(density != AMBIENT_DENSITY){
                        yDirection = density > AMBIENT_DENSITY ? 1 : -1;
                    }

                    if(NeighborSwapped(bossWorker, potentialPoint, tilesToUpdateAgainst, yDirection)){
                        //heading.setX(0);
                        active = false;
                        break;
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

        if(active && tilesToUpdateAgainst.IsEmpty(spreadPoint)){
            tilesToUpdateAgainst.Swap(parentTile->position, spreadPoint);
            didSpread = true;
        }
    }

    return didSpread;

}
