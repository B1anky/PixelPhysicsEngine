#ifndef TILE_H
#define TILE_H

#include <QObject>
#include <QPoint>
#include <QDebug>
#include <memory>
#include "Elements.h"

class Engine;

struct Tile{

public:

    Tile() : xPos(-1), yPos(-1), element(nullptr){
        SetElement(Mat::Material::EMPTY);
    }

    ~Tile(){
        element.reset();
    }

    Tile(int xPosIn, int yPosIn, Mat::Material material) : xPos(xPosIn), yPos(yPosIn), element(nullptr){
        SetElement(material);
    }

    Tile(const QPoint& position, Element* elementIn) : xPos(position.x()), yPos(position.y()), element(nullptr){
        SetElement(elementIn->material);
    }

    Tile(const Tile& tile) : xPos(tile.xPos), yPos(tile.yPos), element(nullptr){
        SetElement(tile.element->material);
    }

    void SwapElements(Tile& otherTile){
        if(otherTile.element && element){
            element.swap(otherTile.element);            
            element->parentTile = this;
            otherTile.element->parentTile = &otherTile;
        }
    }

    void SetElement(Mat::Material material){
        // For dynamic dispatching on Update
        switch(material){
            case Mat::Material::EMPTY:
                element = std::make_shared<PhysicalElement>(this);
                break;
            case Mat::Material::SAND:
                element = std::make_shared<Sand>(this);
                break;
            case Mat::Material::WATER:
                element = std::make_shared<Water>(this);
                break;
            case Mat::Material::WOOD:
                element = std::make_shared<Wood>(this);
                break;
            default:
                element.reset();
                break;
        }
    }

    void Update(Engine* engine){
        element->Update(engine);
    }

    Tile& operator=(const Tile& tile){
        if( this != &tile ){
             xPos    = tile.xPos;
             yPos    = tile.yPos;
             element = tile.element;
             element->parentTile = this;
        }
        return *this;
    }

    bool operator==(const Tile& tile) const{
        return xPos     == tile.xPos
            && yPos     == tile.yPos
            && *element == *tile.element;
    }

    bool operator!=(const Tile& tile) const{
        return !(*this == tile);
    }

    int xPos;
    int yPos;
    //Element* element;
    std::shared_ptr<PhysicalElement> element;
};

#endif // TILE_H
