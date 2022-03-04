#include "PhysicsWindow.h"
#include <QEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QGraphicsSceneMouseEvent>
#include <math.h>
#include <QPainterPathStroker>
#include <QDebug>

template <typename T> int sign(T val) {
    return (T(0) < val) - (val < T(0));
}

PhysicsWindow::PhysicsWindow(QWidget* parent) :
    QWidget(parent)
  , m_engine(500, 500, this)
  , m_radiusSlider(Qt::Orientation::Horizontal)
  , m_scaleSlider(Qt::Orientation::Horizontal)
  , m_clearButton("Clear View")
  , m_engineGraphicsItem(m_engine.m_tiles)
  , m_previewPixelItem(m_previewPixels, m_engine.m_currentMaterial)
  , m_leftMousePressed(false)
  , m_rightMousePressed(false)
  , m_shiftKeyPressed(false)
  , m_controlKeyPressed(false)
  , m_radius(1)
  , m_scale(1.0)
{
    setLayout(&m_mainVLayout);
    m_mainVLayout.addWidget(&m_view);
    m_view.setScene(&m_scene);
    m_scene.installEventFilter(this);
    m_view.setAlignment(Qt::AlignTop|Qt::AlignLeft);
    m_view.setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    m_view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    SetScale(5.0);

    m_engine.SetEngineGraphicsItem(&m_engineGraphicsItem);

    m_view.setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustToContents);

    QStringList materialList;
    QMetaEnum materialMetaEnum = QMetaEnum::fromType<Mat::Material>();
    for( int i = 0; i < materialMetaEnum.keyCount(); ++i ){
        materialList.append(materialMetaEnum.key(i));
    }
    m_materialComboBox.addItems(materialList);
    m_mainVLayout.addWidget(&m_materialComboBox);
    QWidget* radiuSliderWidget = new QWidget;
    QHBoxLayout* radiusSliderHLayout = new QHBoxLayout;
    radiuSliderWidget->setLayout(radiusSliderHLayout);
    radiusSliderHLayout->addWidget(&m_radiusValueLabel);
    radiusSliderHLayout->addWidget(&m_radiusSlider);
    m_mainVLayout.addWidget(radiuSliderWidget);
    m_radiusSlider.setRange(1, 20);

    QWidget* scaleSliderWidget = new QWidget;
    QHBoxLayout* scaleSliderHLayout = new QHBoxLayout;
    scaleSliderWidget->setLayout(scaleSliderHLayout);
    scaleSliderHLayout->addWidget(&m_scaleValueLabel);
    scaleSliderHLayout->addWidget(&m_scaleSlider);
    m_mainVLayout.addWidget(scaleSliderWidget);
    m_scaleSlider.setRange(5, 20);

    m_mainVLayout.addWidget(&m_clearButton);

    connect(&m_materialComboBox, &QComboBox::currentTextChanged, this, &PhysicsWindow::MaterialComboBoxValueChanged, Qt::DirectConnection);
    connect(&m_radiusSlider,     &QSlider::valueChanged,         this, &PhysicsWindow::SetPenRadius,                 Qt::DirectConnection);
    connect(&m_scaleSlider,      &QSlider::valueChanged,         this, &PhysicsWindow::SetScale,                     Qt::DirectConnection);
    connect(&m_clearButton,      &QPushButton::clicked,          this, &PhysicsWindow::ClearTiles,                   Qt::DirectConnection);

    m_radiusSlider.setValue(m_radius);
    SetPenRadius(m_radius);

    m_scaleSlider.setValue(m_scale);

    QColor alphaMaterialColor(Mat::MaterialToColorMap[m_engine.m_currentMaterial]);
    alphaMaterialColor.setAlpha(128);
    m_lineOverlayItem.setPen(QPen(alphaMaterialColor, m_radius));

    m_lineOverlayItem.hide();
    m_scene.addItem(&m_engineGraphicsItem);
    m_scene.addItem(&m_lineOverlayItem);
    m_scene.addItem(&m_previewPixelItem);

    m_view.setMouseTracking(true);

}

void PhysicsWindow::CircleAt( std::function<void(int,int)> f ){
    if(m_lastMousePosition.x() > 0 && m_lastMousePosition.y() > 0){
        int y = floor(m_lastMousePosition.y());
        int x = floor(m_lastMousePosition.x());
        int top    = y - m_radius;
        int bottom = y + m_radius;
        for (int j = top; j <= bottom; j++) {
            int yd     = j - y;
            int xd     = sqrt( (m_radius * m_radius) - ( yd * yd ) );
            int left   = ceil(x - xd);
            int right  = floor(x + xd);
            for (int i = left; i <= right; i++) {
                f(i,j);
            }
        }
    }
}

void PhysicsWindow::LineAt(){
    QPainterPath linePath(m_lineOverlayLine.p1());
    linePath.lineTo(m_lineOverlayLine.p2());

    QPainterPathStroker stroker;
    stroker.setWidth(m_radius);

    QPainterPath strokedPath = stroker.createStroke(linePath);
    QRectF boundingRect(strokedPath.boundingRect());
    for(int x = boundingRect.left(); x < boundingRect.right(); ++x){
        for(int y = boundingRect.top(); y < boundingRect.bottom(); ++y){
            QPointF point(x, y);
            if(strokedPath.contains(point)){
                m_engine.SetTile(Tile(point.x(), point.y(), m_engine.m_currentMaterial), m_engine.m_tiles);
            }
        }
    }
}

void PhysicsWindow::PreviewPixelsAt(){
    m_previewPixels.clear();
    CircleAt( [this](int i, int j){ m_previewPixels.append(QPoint(i, j)); } );
    m_previewPixelItem.update();
}

bool PhysicsWindow::eventFilter(QObject* target, QEvent* event)
{
    auto PlaceAt = [this](int i, int j){
        m_engine.SetTile(Tile(i, j, m_engine.m_currentMaterial), m_engine.m_workerThread.dirtyTiles_);
    };

    auto PlaceCircle = [this, PlaceAt](){
        CircleAt(PlaceAt);
    };



    if (target == &m_scene){

        if(event->type() == QEvent::GraphicsSceneMouseMove){
            const QGraphicsSceneMouseEvent* const mouseEvent = static_cast<const QGraphicsSceneMouseEvent*>(event);
            m_lastMousePosition = mouseEvent->scenePos();
            PreviewPixelsAt();
        }

        if (event->type() == QEvent::GraphicsSceneMousePress){
            const QGraphicsSceneMouseEvent* const mouseEvent = static_cast<const QGraphicsSceneMouseEvent*>(event);
            if( (mouseEvent->buttons() & Qt::LeftButton) == Qt::LeftButton && !m_leftMousePressed ){
                m_leftMousePressed  = true;
                m_lastMousePosition = mouseEvent->scenePos();
            }
            if( (mouseEvent->buttons() & Qt::RightButton) == Qt::RightButton && !m_rightMousePressed ){
                m_rightMousePressed  = true;
                m_lastMousePosition = mouseEvent->scenePos();
                m_lineOverlayLine.setP1(m_lastMousePosition);
                m_lineOverlayItem.show();
            }
        }
        else if(event->type() == QEvent::GraphicsSceneMouseRelease){
            const QGraphicsSceneMouseEvent* const mouseEvent = static_cast<const QGraphicsSceneMouseEvent*>(event);
            if((mouseEvent->button() & Qt::LeftButton) == Qt::LeftButton){
                m_leftMousePressed  = false;
                m_lastMousePosition = QPointF(-1, -1);
            }else if((mouseEvent->button() & Qt::RightButton) == Qt::RightButton){
                m_rightMousePressed = false;
                LineAt();
                m_lineOverlayLine.setPoints(QPointF(-1,-1), QPointF(-2,-2));
                m_lineOverlayItem.hide();
                m_lineOverlayItem.setLine(m_lineOverlayLine);
            }
        }else if(event->type() == QEvent::GraphicsSceneMouseMove && ( m_leftMousePressed || m_rightMousePressed ) ){
            if(m_rightMousePressed){
                m_lineOverlayLine.setP2(m_lastMousePosition);
                m_lineOverlayItem.setLine(m_lineOverlayLine);
            }
            if(m_shiftKeyPressed){
                LineAt();
            }
        }else if(event->type() == QEvent::GraphicsSceneDragLeave){
            m_leftMousePressed  = false;
        }else if(event->type() == QEvent::GraphicsSceneDragEnter){
            const QGraphicsSceneMouseEvent* const mouseEvent = static_cast<const QGraphicsSceneMouseEvent*>(event);
            m_leftMousePressed  = ( mouseEvent->buttons() & Qt::LeftButton );
            m_rightMousePressed = ( mouseEvent->buttons() & Qt::RightButton );
        }else if(event->type() == QEvent::GraphicsSceneWheel){
            const QGraphicsSceneWheelEvent* const wheelEvent = static_cast<const QGraphicsSceneWheelEvent*>(event);
            if( m_controlKeyPressed ){
                SetScale( m_scale + sign( wheelEvent->delta() ) );
            }else{
                SetPenRadius( m_radius + sign( wheelEvent->delta() ) );
            }
        }
    }

    if(m_leftMousePressed){
        PlaceCircle();
        m_previewPixelItem.hide();
    }else{
        m_previewPixelItem.show();
    }

    m_previewPixelItem.setVisible(m_view.underMouse() && !m_leftMousePressed);

    return QWidget::eventFilter(target, event);
}

void PhysicsWindow::resize(){
    int scaledWidth  = ( m_view.contentsRect().width()  / m_scale ) + 1;
    int scaledheight = ( m_view.contentsRect().height() / m_scale ) + 1;

    m_scene.setSceneRect(0, 0, scaledWidth, scaledheight);

    // These widths and heights are needed for the QGraphicsItem::boundingRect overrides.
    m_engineGraphicsItem.width  = scaledWidth;
    m_engineGraphicsItem.height = scaledheight;

    m_previewPixelItem.width  = scaledWidth;
    m_previewPixelItem.height = scaledheight;

    // Reserve or decrease space on the m_tiles.
    m_engine.ResizeTiles(scaledWidth, scaledheight);
}

void PhysicsWindow::resizeEvent(QResizeEvent* resizeEvent){
    resize();
    QWidget::resizeEvent(resizeEvent);
}

void PhysicsWindow::keyPressEvent(QKeyEvent* keyEvent){
    if(m_rightMousePressed && ( keyEvent->modifiers() & Qt::ShiftModifier )){
        m_shiftKeyPressed = true;
        LineAt();
    }

    if( keyEvent->modifiers() & Qt::ControlModifier ){
        m_controlKeyPressed = true;
    }

    Qt::Key key = static_cast<Qt::Key>(keyEvent->key());

    if(Mat::KeyToMaterialMap.contains(key)){
        QMetaEnum materialMetaEnum = QMetaEnum::fromType<Mat::Material>();
        m_materialComboBox.setCurrentText( materialMetaEnum.valueToKey(Mat::KeyToMaterialMap[key]) );
    }
}

void PhysicsWindow::keyReleaseEvent(QKeyEvent* keyEvent){
    if( (keyEvent->modifiers() & Qt::ShiftModifier ) == 0){
        m_shiftKeyPressed = false;
    }

    if( (keyEvent->modifiers() & Qt::ControlModifier) == 0){
        m_controlKeyPressed = false;
    }
}

void PhysicsWindow::MaterialComboBoxValueChanged(const QString& newMaterialString){
    QMetaEnum materialMetaEnum = QMetaEnum::fromType<Mat::Material>();
    m_engine.SetMaterial(static_cast<Mat::Material>(materialMetaEnum.keyToValue(newMaterialString.toStdString().c_str())));

    QColor alphaMaterialColor(Mat::MaterialToColorMap[m_engine.m_currentMaterial]);
    alphaMaterialColor.setAlpha(128);
    m_lineOverlayItem.setPen(QPen(alphaMaterialColor, m_radius));
}

void PhysicsWindow::SetPenRadius(int value){
    if(value < m_radiusSlider.minimum() || m_radiusSlider.maximum() < value ) return;
    if(sender() != &m_radiusSlider){
        m_radiusSlider.setValue(value);
    }

    m_radius = value;
    m_radiusValueLabel.setText(QString("Radius: %0").arg(QString::number(m_radius)));
    m_lineOverlayItem.setPen(QPen(m_lineOverlayItem.pen().color(), m_radius));
    PreviewPixelsAt();
}

void PhysicsWindow::SetScale(double scale){
    if(scale < m_scaleSlider.minimum() || m_scaleSlider.maximum() < scale ) return;
    m_scale = scale;
    m_scaleValueLabel.setText(QString("Scale: %0").arg(QString::number(m_scale)));
    if(sender() != &m_scaleSlider)
        m_scaleSlider.setValue(m_scale);


    int originalWidth  = width();
    int originalheight = height();

    resize();

    m_view.scale(m_scale, m_scale);
    m_view.translate(originalWidth - width(), originalheight - height());

    m_view.fitInView(m_scene.sceneRect());

}

void PhysicsWindow::ClearTiles(){
    m_engine.ClearTiles(m_engine.m_workerThread.dirtyTiles_);
}
