#include "Elements.h"
#include "Tile.h"
#include "TileSet.h"
#include "Hashhelpers.h"
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

bool PhysicalElement::Update(TileSet& /*tilesToUpdateAgainst*/){
    return false;
}

bool PhysicalElement::GravityUpdate(TileSet& tilesToUpdateAgainst){
    bool abidedToGravity = false;

    // a positive y implies gravity down, because it's so more dense than air.
    // a negative y implies gravity up, because it's less dense than air.
    int yDirection = 0;
    if(density != AMBIENT_DENSITY){
        yDirection = density > AMBIENT_DENSITY ? 1 : -1;
    }

    QPoint gravitatedPoint(parentTile->position.x(), parentTile->position.y() + yDirection);
    if(!tilesToUpdateAgainst.InBounds(gravitatedPoint)) return abidedToGravity;

    bool canSwap = tilesToUpdateAgainst.IsEmpty(gravitatedPoint)
                || ( ( tilesToUpdateAgainst.TileAt(gravitatedPoint).element->density < density ) && ( yDirection > 0 ) )  // We want to move down and we're more dense
                || ( ( tilesToUpdateAgainst.TileAt(gravitatedPoint).element->density > density ) && ( yDirection < 0 ) ); // We want to move up and we're less dense

    if(canSwap){

        heading = HeadingFromPointChange(parentTile->position, gravitatedPoint);

        //DeltaVelocityDueToGravity(velocity, parentTile->position, gravitatedPoint);

        tilesToUpdateAgainst.Swap(parentTile->position, gravitatedPoint);
        abidedToGravity = true;
    }
    return abidedToGravity;
}

bool PhysicalElement::SpreadUpdate(TileSet& tilesToUpdateAgainst){

    bool spread = true;
    QPoint originalPost(parentTile->position);
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
    }
    else{
        spread = false;
    }

    if(spread){
        heading = HeadingFromPointChange(originalPost, spreadPoint);
        DeltaVelocityDueToGravity(velocity, originalPost, spreadPoint);
        tilesToUpdateAgainst.Swap(originalPost, spreadPoint);
    }

    return spread;
}

bool MoveableSolid::Update(TileSet& tilesToUpdateAgainst){
    bool dirtied = MoveableSolid::GravityUpdate(tilesToUpdateAgainst);
    dirtied |= MoveableSolid::SpreadUpdate(tilesToUpdateAgainst);

    return dirtied;
}

bool MoveableSolid::SpreadUpdate(TileSet& tilesToUpdateAgainst){
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

bool Liquid::Update(TileSet& tilesToUpdateAgainst){
    bool dirtied = Liquid::GravityUpdate(tilesToUpdateAgainst);
    gravityUpdated = dirtied;
    dirtied |= Liquid::SpreadUpdate(tilesToUpdateAgainst);

    return dirtied;
}

bool Liquid::SpreadUpdate(TileSet& tilesToUpdateAgainst){
    bool didSpread = PhysicalElement::SpreadUpdate(tilesToUpdateAgainst);

    if(!gravityUpdated && !didSpread){

        if(heading.x() == 0){
            int spread = tilesToUpdateAgainst.width();
            bool left = HorizontalDirectionFromHeading(heading);
            // Should compound the liquid's heading in its already moving direction
            heading.setX(left ? spread : -spread );
        }

        QPoint spreadPoint(parentTile->position.x(), parentTile->position.y());

        // Efficiency check. Don't look for next spot if you're completely surrounded.
        // Note: This is a temporary fix and does have some visual side effects.
        //       TODO below emphasizes the expensive logic we're tyring to skip at all costs.
        //       Fluids are expensive to simulate...
        //bool isSurrounded = true;
        //for(int i = -1; i <= 1; ++i){
        //    for(int j = -1; j <= 1; ++j){
        //        QPoint offsetPoint(i,j);
        //        if(engine->IsEmpty(parentTile->position + offsetPoint, tilesToUpdateAgainst)){
        //            isSurrounded = false;
        //            break;
        //        }
        //    }
        //}
        //
        //if(isSurrounded){
        //    heading.setX(0);
        //    return didSpread;
        //}

        // We hit a non empty tile, stop moving
        // We have to loop over every point in betweem current location and target to see if something will stop us early.
        // TODO: Do this logic in its own thread since it's very expensive.
        int spreadDirection = sign(heading.x());
        QPoint potentialPoint(0, 0);
        for(int finalSpreadOffset = 0; finalSpreadOffset < abs(heading.x()); ++finalSpreadOffset){
            potentialPoint.setX(spreadPoint.x() + (spreadDirection * finalSpreadOffset));
            potentialPoint.setY(spreadPoint.y());
            if ( (!tilesToUpdateAgainst.InBounds(potentialPoint) || tilesToUpdateAgainst.TileAt(potentialPoint).element->density > density ) ){
                spreadPoint.setX(spreadPoint.x() + (spreadDirection * finalSpreadOffset));
                heading.setX(0);
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
