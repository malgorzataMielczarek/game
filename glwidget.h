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

    struct Triangle
    {
        QVector3D v1, v2, v3;
        QOpenGLTexture* texture;
        QVector3D n;
        float A, B, C, D;
        int groupSize;
    };

    std::vector<Triangle> collisionTriangles;
    CMesh collisionTrianglesMesh;
    void initCollisionTriangles();
    void addTriangleCollider(QVector3D v1, QVector3D v2, QVector3D v3, int groupSize = 1,
                             QVector2D uv1 = QVector2D(0,0), QVector2D uv2 = QVector2D(0,0),
                             QVector2D uv3 = QVector2D(0,0), QOpenGLTexture* texture = nullptr);
    void loadHeightMap(QString filepath, QVector3D center, QVector3D size,
                       QVector2D maxUV = QVector2D(0, 0), QOpenGLTexture* texture = nullptr);
};

#endif

