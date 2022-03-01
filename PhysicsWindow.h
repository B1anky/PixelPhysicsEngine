#ifndef PHYSICSWINDOW_H
#define PHYSICSWINDOW_H

#include "Engine.h"
#include "QGraphicsPixelItem.h"
#include <QWidget>
#include <QGraphicsScene>
#include <QVBoxLayout>
#include <QComboBox>
#include <QGraphicsView>
#include <QSlider>
#include <QLabel>

class QEvent;
class QResizeEvent;

class PhysicsWindow : public QWidget
{
    Q_OBJECT
public:
    explicit PhysicsWindow(QWidget* parent = nullptr);

protected:

    bool eventFilter(QObject* target, QEvent* event);

    void resizeEvent(QResizeEvent* resizeEvent) override;

    void MaterialComboBoxValueChanged(const QString& newMaterialString);

    void RadiusSliderValueChanged(int value);

    void SetScale(double scale);

protected:

    QVBoxLayout        m_mainVLayout;
    QGraphicsScene     m_scene;
    QGraphicsView      m_view;
    Engine             m_engine;
    QGraphicsPixelItem m_graphicsItem;
    QGraphicsLineItem  m_lineOverlayItem;
    QLineF             m_lineOverlayLine;
    QComboBox          m_materialComboBox;
    QSlider            m_radiusSlider;
    QLabel             m_radiusValueLabel;
    bool               m_leftMousePressed;
    bool               m_rightMousePressed;
    QPointF            m_lastMousePosition;
    int                m_radius;
    double             m_scale;
};

#endif // PHYSICSWINDOW_H
