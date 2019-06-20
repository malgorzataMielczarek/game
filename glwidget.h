#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QKeyEvent>
#include <QMap>
#include <QElapsedTimer>
#include <vector>
#include "cmesh.h"
#include "player.h"

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    GLWidget(QWidget *parent = nullptr);
    ~GLWidget();

    QSize sizeHint() const override;
    void addObject(GameObject* obj);

    friend CMesh;

public slots:
    void cleanup();

signals:

protected:
    void initializeGL() override;
    void paintGL() override;
    void updateGL();
    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    void setTransforms(void);

private:

    struct LightLocStruct
    {
        int position;
        int ambient;
        int diffuse;
    };

    QPoint m_lastPos;
    QOpenGLShaderProgram *m_program;
    int m_projMatrixLoc;
    int m_viewMatrixLoc;
    int m_modelMatrixLoc;
    int m_modelColorLoc;
    int m_hasTextureLoc;
    LightLocStruct m_lightLoc;

    QMatrix4x4 m_proj;
    QMatrix4x4 m_camera;
    char cameraType = 'f';
    QMatrix4x4 m_world;

    Player m_player;

    std::vector<GameObject*> m_gameObjects;

    bool m_keyState[256];

    float m_camDistance = 1.5f;

    QElapsedTimer timer;
    float lastUpdateTime;
    float FPS;
};

#endif

