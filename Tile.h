#ifndef TILE_H
#define TILE_H

#include <QObject>
#include <QPoint>
#include <memory>
#include "Elements.h"

class TileSet;
class Worker;
typedef QMap<QPoint, Worker*> WorkerMap;

struct Tile{

public:    

    Tile() : position(-1, -1), element(nullptr){
        SetElement(Mat::Material::EMPTY);
    }

    ~Tile(){ }

    Tile(const QPoint& positionIn, const std::shared_ptr<Element>& elementIn) : position(positionIn), element(elementIn){
        // This will take ownership of elementIn.
        if(element){
            element->active = true;
        }
    }

    Tile(int xPosIn, int yPosIn, Mat::Material material) : position(xPosIn, yPosIn), element(nullptr){
        SetElement(material);
    }

    Tile(const QPoint& positionIn, Element* elementIn) : position(positionIn), element(nullptr){
        SetElement(elementIn->material);
    }

    Tile(const Tile& tile) : position(tile.position), element(nullptr){
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

    void Update(TileSet& tilesToUpdateAgainst, Worker* bossWorker){
        element->Update(tilesToUpdateAgainst, bossWorker);
    }

    Tile& operator=(const Tile& tile){
        if( this != &tile ){
             position = tile.position;
             if(tile.element == nullptr){
                SetElement(tile.element->material);
             }else{
                element = tile.element;
                element->parentTile = this;
             }
        }
        return *this;
    }

    bool operator==(const Tile& tile) const{
        return position == tile.position
            && *element == *tile.element;
    }

    bool operator!=(const Tile& tile) const{
        return !(*this == tile);
    }

    bool IsEmpty() const{
        return element && element->material == Mat::Material::EMPTY;
    }

    QPoint position;
    std::shared_ptr<Element> element;
};

#endif // TILE_H
