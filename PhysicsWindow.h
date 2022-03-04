#ifndef PHYSICSWINDOW_H
#define PHYSICSWINDOW_H

#include "Engine.h"
#include "QGraphicsEngineItem.h"
#include "QGraphicsPixelItem.h"
#include <QWidget>
#include <QGraphicsScene>
#include <QVBoxLayout>
#include <QComboBox>
#include <QGraphicsView>
#include <QSlider>
#include <QLabel>
#include <QPushButton>

class QEvent;
class QResizeEvent;
class QKeyEvent;

class PhysicsWindow : public QWidget
{
    Q_OBJECT
public:
    explicit PhysicsWindow(QWidget* parent = nullptr);

protected:

    bool eventFilter(QObject* target, QEvent* event);

    void resize();

    void resizeEvent(QResizeEvent* resizeEvent) override;

    void keyPressEvent(QKeyEvent* keyEvent) override;

    void keyReleaseEvent(QKeyEvent* keyEvent) override;

    void MaterialComboBoxValueChanged(const QString& newMaterialString);

    void SetPenRadius(int value);

    void SetScale(double scale);

    // Helper functions for drawing
    void CircleAt( std::function<void(int,int)> f );

    void LineAt();

    void PreviewPixelsAt();

    void ClearTiles();

protected:

    // Core engine
    Engine m_engine;

    // Widgets and layouts
    QVBoxLayout    m_mainVLayout;
    QGraphicsScene m_scene;
    QGraphicsView  m_view;
    QComboBox      m_materialComboBox;
    QSlider        m_radiusSlider;
    QLabel         m_radiusValueLabel;
    QSlider        m_scaleSlider;
    QLabel         m_scaleValueLabel;
    QPushButton    m_clearButton;


    // Drawing
    QGraphicsEngineItem m_engineGraphicsItem;

    QGraphicsLineItem   m_lineOverlayItem;
    QLineF              m_lineOverlayLine;

    QGraphicsPixelItem  m_previewPixelItem;
    QVector<QPoint>     m_previewPixels;

    // States
    bool    m_leftMousePressed;
    bool    m_rightMousePressed;
    bool    m_shiftKeyPressed;
    bool    m_controlKeyPressed;
    QPointF m_lastMousePosition;
    int     m_radius;
    double  m_scale;

};

#endif // PHYSICSWINDOW_H
