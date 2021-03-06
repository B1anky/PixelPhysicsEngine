#ifndef ELEMENTS_H
#define ELEMENTS_H

#include <QMetaEnum>
#include <QMap>
#include <QColor>
#include <limits>
#include <QPoint>

#define AMBIENT_TEMP     20.0  // Celsius
#define DEFAULT_LIFETIME 1000  // ms
#define AMBIENT_DENSITY  1.225 // kg/m^3

class Tile;
class Engine;

namespace Mat{
    Q_NAMESPACE
    enum Material{
        EMPTY = 0,
        SAND  = 1 << 0,
        WATER = 1 << 2,
        WOOD  = 1 << 3,
    };
    Q_ENUM_NS(Material)

    static inline QMap<Mat::Material, QColor> MaterialToColorMap = { { Mat::Material::EMPTY, QColor( 64,  64,  64) }
                                                                   , { Mat::Material::SAND,  QColor(189, 183, 107) }
                                                                   , { Mat::Material::WATER, QColor(  0,   0, 255) }
                                                                   , { Mat::Material::WOOD,  QColor( 55,  25,   0) }
                                                                   };

}

struct Element
{
    Element(Tile* parentTileIn) :
        material(Mat::Material::EMPTY)
      , temperature(AMBIENT_TEMP)
      , lifetime(DEFAULT_LIFETIME)
      , density(1.0)
      , onFire(false)
      , velocity(0)
      , heading(QPoint(0,0))
      , parentTile(parentTileIn) { }

    virtual ~Element(){}

    Element(const Element& element) :
        material(element.material)
      , temperature(element.temperature)
      , lifetime(element.lifetime)
      , density(element.density)
      , onFire(element.onFire)
      , velocity(element.velocity)
      , heading(element.heading){ }

    Element& operator=(const Element& element){
        if(this != &element){
            material    = element.material;
            temperature = element.temperature;
            lifetime    = element.lifetime;
            density     = element.density;
            onFire      = element.onFire;
            velocity    = element.velocity;
            heading     = element.heading;
        }
        return *this;
    }

    bool operator==(const Element& element) const{
        return   material    == element.material
              && temperature == element.temperature
              && lifetime    == element.lifetime
              && density     == element.density
              && onFire      == element.onFire
              && velocity    == element.velocity
              && heading     == element.heading;
    }

    bool operator!=(const Element& element) const{
        return !(*this == element);
    }

    Element(Tile* parentTileIn, Mat::Material materialIn) :
        material(materialIn)
      , temperature(AMBIENT_TEMP)
      , lifetime(DEFAULT_LIFETIME)
      , density(1.0)
      , onFire(false)
      , velocity(0)
      , heading(QPoint(0, 0))
      , parentTile(parentTileIn) { }

    virtual bool Update(Engine* /*engine*/){
        return false;
    }

    Mat::Material material;
    double        temperature;
    int           lifetime;
    double        density;
    bool          onFire;
    int           velocity;
    QPoint        heading; // x, y heading
    Tile*         parentTile;
};

struct PhysicalElement : public Element{

    PhysicalElement(Tile* parentTileIn) :
        Element(parentTileIn) { }

    virtual ~PhysicalElement(){}

    PhysicalElement(const PhysicalElement& physicalElement) :
        Element(physicalElement) { }

    PhysicalElement& operator=(const PhysicalElement& physicalElement)
    {
        Element::operator=(physicalElement);
        return *this;
    }

    bool operator==(const PhysicalElement& physicalElement) const
    {
        return Element::operator==(physicalElement);
    }

    bool operator!=(const PhysicalElement& physicalElement) const
    {
        return !(*this == physicalElement);
    }

    virtual bool Update(Engine* engine);
    virtual bool GravityUpdate(Engine* engine);
    virtual bool SpreadUpdate(Engine* engine);

};

struct Solid : public PhysicalElement
{

public:
    Solid(Tile* parentTileIn) :
        PhysicalElement(parentTileIn)
      , friction(1.0)
    { }

    ~Solid(){}

    Solid(const Solid& solid) :
        PhysicalElement(solid)
      , friction(solid.friction)
    { }

    Solid& operator=(const Solid& solid)
    {
        PhysicalElement::operator=(solid);
        if(this != &solid){
            friction = solid.friction;
        }
        return *this;
    }

    bool operator==(const Solid& solid) const
    {
        return PhysicalElement::operator==(solid)
            && friction == solid.friction;
    }

    bool operator!=(const Solid& solid) const
    {
        return !(*this == solid);
    }

    double friction;

};

struct Wood : public Solid
{

public:
    Wood(Tile* parentTileIn) :
        Solid(parentTileIn)
    {
        material = Mat::WOOD;
        density = std::numeric_limits<double>::max();
    }

    ~Wood(){}

    Wood(const Wood& wood) : Solid(wood) { }

    Wood& operator=(const Wood& wood)
    {
        Solid::operator=(wood);
        return *this;
    }

    bool operator==(const Wood& wood) const
    {
        return Solid::operator==(wood);
    }

    bool operator!=(const Wood& wood) const
    {
        return !(*this == wood);
    }

};

struct MoveableSolid : public Solid{

    MoveableSolid(Tile* parentTileIn) :
        Solid(parentTileIn)
    { }

    ~MoveableSolid(){}

    MoveableSolid(const MoveableSolid& moveableSolid) :
        Solid(moveableSolid)
    { }

    MoveableSolid& operator=(const MoveableSolid& moveableSolid)
    {
        Solid::operator=(moveableSolid);
        return *this;
    }

    bool operator==(const MoveableSolid& moveableSolid) const
    {
        return Solid::operator==(moveableSolid);
    }

    bool operator!=(const MoveableSolid& moveableSolid) const
    {
        return !(*this == moveableSolid);
    }

    bool Update(Engine* engine)        override;
    bool GravityUpdate(Engine *engine) override{ return PhysicalElement::GravityUpdate(engine); }
    bool SpreadUpdate(Engine* engine)  override;

};

struct Sand : public MoveableSolid
{

public:
    Sand(Tile* parentTileIn) : MoveableSolid(parentTileIn){
        material = Mat::Material::SAND;
        density = 1520.0;
        friction = 0.0;
    }

    ~Sand(){}

    Sand(const Sand& sand) : MoveableSolid(sand) { }

    Sand& operator=(const Sand& sand)
    {
        MoveableSolid::operator=(sand);
        return *this;
    }

    bool operator==(const Sand& sand) const
    {
        return MoveableSolid::operator==(sand);
    }

    bool operator!=(const Sand& sand) const
    {
        return !(*this == sand);
    }

    bool Update(Engine *engine) override{
        return MoveableSolid::Update(engine);
    }

    bool GravityUpdate(Engine* engine) override{
        return MoveableSolid::Update(engine);
    }

    bool SpreadUpdate(Engine* engine){
        return MoveableSolid::SpreadUpdate(engine);
    }

};

struct Ice : public Solid
{

public:
    Ice(Tile* parentTileIn) : Solid(parentTileIn){ }

};

struct Liquid : public PhysicalElement
{

public:
    Liquid(Tile* parentTileIn) : PhysicalElement(parentTileIn), gravityUpdated(false){ }

    ~Liquid(){}

    Liquid(const Liquid& liquid) :
        PhysicalElement(liquid)
      , gravityUpdated(false)
    { }

    Liquid& operator=(const Liquid& liquid)
    {
        if(this != &liquid){
            Element::operator=(liquid);
            gravityUpdated = liquid.gravityUpdated;
        }

        return *this;
    }

    bool operator==(const Liquid& liquid) const
    {
        return Element::operator==(liquid)
             && gravityUpdated == liquid.gravityUpdated;
    }

    bool operator!=(const Liquid& liquid) const
    {
        return !(*this == liquid);
    }

    bool Update(Engine* engine)        override;
    bool GravityUpdate(Engine* engine) override{ return PhysicalElement::GravityUpdate(engine); }
    bool SpreadUpdate(Engine* engine)  override;

    bool gravityUpdated;

};

struct Water : public Liquid
{

public:
    Water(Tile* parentTileIn) : Liquid(parentTileIn){
        material = Mat::Material::WATER;
        density = 997.0;
    }

    ~Water(){}

    Water(const Water& water) : Liquid(water) {
        material = Mat::Material::WATER;
    }

    Water& operator=(const Water& water)
    {
        Liquid::operator=(water);
        return *this;
    }

    bool operator==(const Water& water) const
    {
        return Liquid::operator==(water);
    }

    bool operator!=(const Water& water) const
    {
        return !(*this == water);
    }

    bool Update(Engine* engine)        override{ return Liquid::Update(engine);        }
    bool GravityUpdate(Engine* engine) override{ return Liquid::GravityUpdate(engine); }
    bool SpreadUpdate(Engine* engine)  override{ return Liquid::SpreadUpdate(engine);  }

};

struct Gas : public PhysicalElement
{

public:
    Gas(Tile* parentTileIn) : PhysicalElement(parentTileIn){ }

};

struct Steam : public Gas
{

public:
    Steam(Tile* parentTileIn) : Gas(parentTileIn){ }

};

#endif // ELEMENTS_H
