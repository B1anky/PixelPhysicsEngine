#include "Elements.h"
#include "Engine.h"
#include "Tile.h"
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

    QPoint gravitatedPoint(parentTile->position.x(), parentTile->position.y() + yDirection);
    if(!engine->InBounds(gravitatedPoint)) return abidedToGravity;

    bool canSwap = engine->IsEmpty(gravitatedPoint)
                || ( ( engine->TileAt(gravitatedPoint).element->density < density ) && ( yDirection > 0 ) )  // We want to move down and we're more dense
                || ( ( engine->TileAt(gravitatedPoint).element->density > density ) && ( yDirection < 0 ) ); // We want to move up and we're less dense

    if(canSwap){

        heading = HeadingFromPointChange(parentTile->position, gravitatedPoint);

        //DeltaVelocityDueToGravity(velocity, parentTile->position, gravitatedPoint);

        engine->Swap(parentTile->position, gravitatedPoint);
        abidedToGravity = true;
    }
    return abidedToGravity;
}

bool PhysicalElement::SpreadUpdate(Engine* engine){

    bool spread = true;
    int x = parentTile->position.x();
    int y = parentTile->position.y();
    QPoint spreadPoint;

    bool canSpreadLeft                    = engine->IsEmpty(x - 1, y);
    bool canSpreadRight                   = engine->IsEmpty(x + 1, y);
    bool canSpreadBottomLeft              = engine->IsEmpty(x - 1, y + 1) && canSpreadLeft;
    bool canSpreadBottomRight             = engine->IsEmpty(x + 1, y + 1) && canSpreadRight;
    bool canSpreadBottomLeftDueToDensity  = engine->TileAt(x - 1, y + 1).element->density < density && canSpreadLeft;
    bool canSpreadBottomRightDueToDensity = engine->TileAt(x + 1, y + 1).element->density < density && canSpreadRight;

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
        heading = HeadingFromPointChange(parentTile->position, spreadPoint);
        DeltaVelocityDueToGravity(velocity, parentTile->position, spreadPoint);
        engine->Swap(parentTile->position, spreadPoint);
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
    int x = parentTile->position.x();
    int y = parentTile->position.y();

    bool canSpreadLeft                    = friction < 0.5 && engine->IsEmpty(x - 1, y);
    bool canSpreadRight                   = friction < 0.5 && engine->IsEmpty(x + 1, y);
    bool canSpreadDownLeft                = friction < 0.5 && engine->IsEmpty(x - 1, y + 1) && canSpreadLeft;
    bool canSpreadDownRight               = friction < 0.5 && engine->IsEmpty(x + 1, y + 1) && canSpreadRight;
    bool canSpreadBottomLeftDueToDensity  = friction < 0.5 && engine->TileAt(x - 1, y + 1).element->density < density && canSpreadLeft;
    bool canSpreadBottomRightDueToDensity = friction < 0.5 && engine->TileAt(x + 1, y + 1).element->density < density && canSpreadRight;
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
        engine->Swap(parentTile->position, spreadPoint);
    }

    return spread;
}

bool Liquid::Update(Engine* engine){
    bool dirtied = Liquid::GravityUpdate(engine);
    gravityUpdated = dirtied;
    dirtied |= Liquid::SpreadUpdate(engine);

    return dirtied;
}

bool Liquid::SpreadUpdate(Engine* engine){
    bool didSpread = PhysicalElement::SpreadUpdate(engine);

    if(!gravityUpdated && !didSpread && heading.x() == 0){
        int spread = engine->m_width;
        bool left = HorizontalDirectionFromHeading(heading);
        // Should compound the liquid's heading in its already moving direction
        heading.setX(left ? spread : -spread );
    }


    QPoint spreadPoint(parentTile->position.x(), parentTile->position.y());
    if(sign(heading.x()) < 0) {
        spreadPoint.setX(std::max(parentTile->position.x() + heading.x(), 0));
    }else if(sign(heading.x()) > 0){
        spreadPoint.setX(std::min(parentTile->position.x() + heading.x(), engine->m_width));
    }

    // We hit a non empty tile, stop moving
    // We have to loop over every point in betweem current location and target to see if something will stop us early.
    int finalSpreadOffset(0);
    int spreadDirection = sign(heading.x());
    for(; finalSpreadOffset < abs(heading.x()); ++finalSpreadOffset){
        QPoint potentialPoint(parentTile->position.x() + (spreadDirection * finalSpreadOffset), parentTile->position.y());
        if ( (!engine->InBounds(potentialPoint) || engine->TileAt(potentialPoint).element->density > density ) ){
            if(finalSpreadOffset > 0){
                --finalSpreadOffset;
            }
            spreadPoint = QPoint(parentTile->position.x() + (spreadDirection * finalSpreadOffset), parentTile->position.y());
            heading.setX(0);
            break;
        }else if(engine->IsEmpty(potentialPoint)){
            spreadPoint = potentialPoint;
            heading.setX(0);
            break;
        }
    }

    if(engine->IsEmpty(spreadPoint)){


        // Note: Compounding heading for liquids so they keep their direction for longer.
        //heading += HeadingFromPointChange(parentTile->position, spreadPoint);
        //DeltaVelocityDueToGravity(velocity, parentTile->position, spreadPoint);
        engine->Swap(parentTile->position, spreadPoint);
        didSpread = true;
    }

    return didSpread;
}
