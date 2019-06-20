#include "glwidget.h"
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <math.h>
#include <iostream>
#include <qstack.h>
#include "bullet.h"
#include "cube.h"
#include "texturemanager.h"

using namespace std;

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      m_program(nullptr)
{
    setMouseTracking(true);
    QCursor c = cursor();
    c.setShape(Qt::CursorShape::BlankCursor);
    setCursor(c);
    setFocusPolicy(Qt::StrongFocus);
}

GLWidget::~GLWidget()
{
    cleanup();
}

QSize GLWidget::sizeHint() const
{
    return QSize(1000, 800);
}

void GLWidget::addObject(GameObject *obj)
{
    obj->init();
    m_gameObjects.push_back(obj);
}

void GLWidget::cleanup()
{
    if (m_program == nullptr)
        return;
    makeCurrent();

    delete m_program;
    m_program = nullptr;
    doneCurrent();
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.1f, 0.2f, 0.3f, 1);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    CMesh::loadAllMeshes();

    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "resources/shader.vs");
    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "resources/shader.fs");
    m_program->bindAttributeLocation("vertex", 0);
    m_program->bindAttributeLocation("normal", 1);
    m_program->link();

    m_program->bind();
    m_projMatrixLoc = m_program->uniformLocation("projMatrix");
    m_viewMatrixLoc = m_program->uniformLocation("viewMatrix");
    m_modelMatrixLoc = m_program->uniformLocation("modelMatrix");
    m_modelColorLoc = m_program->uniformLocation("modelColor");
    m_hasTextureLoc = m_program->uniformLocation("hasTexture");
    m_lightLoc.position = m_program->uniformLocation("light.position");
    m_lightLoc.ambient = m_program->uniformLocation("light.ambient");
    m_lightLoc.diffuse = m_program->uniformLocation("light.diffuse");

    m_program->release();

    lastUpdateTime = 0;
    timer.start();
    FPS=60;

    TextureManager::init();
    initCollisionTriangles();

    addObject((&m_player));
    for(int i = 0; i < 5; i++)
    {
        for(int j = 0; j < 7; j++)
        {
            Cube* cube = new Cube();

            cube->position.setX(j * 1 - 3);
            cube->position.setY(0);
            cube->position.setZ(i * 1 - 6);

            cube->material_color.setX(i * 0.2f);
            cube->material_color.setY(0.5f);
            cube->material_color.setZ(j * 0.1f);

            cube->scale = QVector3D(0.3f,0.3f,0.3f);

            cube->m_radius = 0.5f * sqrt(3 * cube->scale.x() * cube->scale.x());
            cube->m_texture = TextureManager::getTexture("brick");

            addObject(cube);
        }
    }
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    QStack<QMatrix4x4> worldMatrixStack;

    m_program->bind();

    m_program->setUniformValue(m_lightLoc.position, m_player.position - m_player.direction);//QVector3D(0.0f, 0.0f, 15.0f));
    m_program->setUniformValue(m_lightLoc.ambient, QVector3D(0.1f, 0.1f, 0.1f));
    m_program->setUniformValue(m_lightLoc.diffuse, QVector3D(0.9f, 0.9f, 0.9f));

    m_camera.setToIdentity();

    m_world.setToIdentity();


    if(cameraType == 'f')
    {
    //kamera FPP
        m_camera.lookAt(
                m_player.position,
                m_player.position + m_player.direction,
                QVector3D(0,1,0));
    }
    else if(cameraType == 't')
    {
    //kamera TPP
    m_camera.lookAt(
            m_player.position - m_camDistance * m_player.direction,
            m_player.position,
            QVector3D(0,1,0));
    }

    for(int i = 0; i < m_gameObjects.size(); i++)
    {
        GameObject* obj = m_gameObjects[i];

        m_program->setUniformValue(m_modelColorLoc, obj->material_color);

        if(obj->m_texture!=nullptr)
        {
            m_program->setUniformValue(m_hasTextureLoc, 1);
            obj->m_texture->bind();
        }
        else
        {
            m_program->setUniformValue(m_hasTextureLoc, 0);
        }
        worldMatrixStack.push(m_world);
            m_world.translate(obj->position);
            m_world.rotate(obj->rotation.x(),1,0,0);
            m_world.rotate(obj->rotation.y(),0,1,0);
            m_world.rotate(obj->rotation.z(),0,0,1);
            m_world.scale(obj->scale);
            setTransforms();
            obj->render(this);
            m_world = worldMatrixStack.pop();
    }

    for(int i = 0; i < collisionTriangles.size(); )
    {
        Triangle triangle = collisionTriangles[i];

        m_program->setUniformValue(m_modelColorLoc, QVector3D(1, 1, 1));

        if(triangle.texture != nullptr)
        {
            m_program->setUniformValue(m_hasTextureLoc, 1);
            triangle.texture->bind();
        }
        else
        {
            m_program->setUniformValue(m_hasTextureLoc, 0);
        }

        worldMatrixStack.push(m_world);
            m_world.translate(QVector3D(0, 0, 0));
            m_world.rotate(0, 1, 0, 0);
            m_world.rotate(0, 0, 1, 0);
            m_world.rotate(0, 0, 0, 1);
            m_world.scale(QVector3D(1, 1, 1));
            setTransforms();
            collisionTrianglesMesh.render(this, i * 3, triangle.groupSize * 3);
        m_world = worldMatrixStack.pop();

        i += triangle.groupSize;
    }

    m_program->release();

    float timerTime = timer.elapsed() * 0.001f;
    float deltaTime = timerTime - lastUpdateTime;
    if(deltaTime >= (1.0f/FPS))
    {
        updateGL();
        lastUpdateTime = timerTime;
    }

    update();
}

void GLWidget::updateGL()
{
    for(int i = 0; i < m_gameObjects.size(); i++)
    {
        GameObject* obj = m_gameObjects[i];

        obj->previousPosition = obj->position;

        for(int j = 0; j < m_gameObjects.size(); j++)
        {
            if(i == j) continue;

            GameObject* obj2 = m_gameObjects[j];

            QVector3D v = obj->position - obj2->position;
            float d = v.length();

            if(d < (obj->m_radius + obj2->m_radius))
            {
                std::string name1=obj->m_name;
                std::string name2=obj->m_name;
                GameObject* o1=obj;
                GameObject* o2=obj2;
                if(strcmp(name1.c_str(),name2.c_str())>0)
                {
                    o1=obj2;
                    o2=obj;
                    v=-v;
                }
                if(!o1->m_name.compare("Player")&&!o2->m_name.compare("bullet"))
                {

                }
                else if(!o1->m_name.compare("bullet") && !o2->m_name.compare("cube"))
                {
                    o1->isAlive = false;
                    o2->isAlive = false;
                }
                else
                {
                    o1->position = o1->position + v * (d/2);
                    o2->position = o2->position - v * (d/2);
                    v.normalize();
                    float energySum = o1->energy.length() + o2->energy.length();
                    o1->energy = v * energySum/2;
                    o2->energy = -v * energySum/2;
                }
            }
        }
        obj->energy.setY(obj->energy.y() - 0.02f);
        obj->update();
    }

    for(unsigned int i = 0; i < m_gameObjects.size(); i++)
    {
        GameObject* obj = m_gameObjects[i];

        for(int j = 0; j < collisionTriangles.size(); j++)
        {
            Triangle tr = collisionTriangles[j];

            float currDist = tr.A * obj->position.x() + tr.B * obj->position.y() + tr.C * obj->position.z() + tr.D;
            float prevDist = tr.A * obj->previousPosition.x() + tr.B * obj->previousPosition.y() + tr.C *obj->position.z() + tr.D;

            if((currDist * prevDist < 0) || (abs(currDist) < obj->m_radius))
            {
                QVector3D p = obj->position - tr.n * currDist;
                QVector3D r = (tr.v1 + tr.v2 + tr.v3) * (1.0f/3.0f) - p;
                r = r.normalized();
                p = p + r * obj->m_radius;

                QVector3D v0 = tr.v2 - tr.v1,
                        v1 = tr.v3 - tr.v1,
                        v2 = p - tr.v1;
                float d00 = QVector3D::dotProduct(v0, v0),
                        d01 = QVector3D::dotProduct(v0, v1),
                        d11 = QVector3D::dotProduct(v1, v1),
                        d20 = QVector3D::dotProduct(v2, v0),
                        d21 = QVector3D::dotProduct(v2, v1),
                        denom = d00 * d11 - d01 * d01,
                        v = (d11 * d20 - d01 * d21)/denom,
                        w = (d00 * d21 - d01 * d20)/denom,
                        u = 1.0f - v - w;
                if(v >= 0 && w >= 0 && (v + w) <= 1)
                {
                    float d = obj->m_radius - currDist;
                    obj->position = obj->position + tr.n * d;
                    obj->energy = obj->energy - tr.n * QVector3D::dotProduct(tr.n, obj->energy) * 2;
                }
            }
        }
    }

    QCursor::setPos(mapToGlobal(QPoint(width()/2,height()/2)));
    if(m_keyState[Qt::Key_W])
    {
        m_player.energy.setX(m_player.energy.x() + m_player.direction.x() * m_player.speed);
        m_player.energy.setZ(m_player.energy.z() + m_player.direction.z() * m_player.speed);
    }
    if(m_keyState[Qt::Key_S])
    {
        m_player.energy.setX(m_player.energy.x() - m_player.direction.x() * m_player.speed);
        m_player.energy.setZ(m_player.energy.z() - m_player.direction.z() * m_player.speed);
    }
    if(m_keyState[Qt::Key_A])
    {
        m_player.energy.setX(m_player.energy.x() + m_player.direction.z() * m_player.speed);
        m_player.energy.setZ(m_player.energy.z() - m_player.direction.x() * m_player.speed);
    }
    if(m_keyState[Qt::Key_D])
    {
        m_player.energy.setX(m_player.energy.x() - m_player.direction.z() * m_player.speed);
        m_player.energy.setZ(m_player.energy.z() + m_player.direction.x() * m_player.speed);
    }
    if(m_keyState[Qt::Key_Q])
    {
        float phi = atan2(m_player.direction.z(),m_player.direction.x());
        phi = phi - 0.05f;
        m_player.direction.setX(cos(phi));
        m_player.direction.setZ(sin(phi));
    }
    if(m_keyState[Qt::Key_E])
    {
        float phi = atan2(m_player.direction.z(),m_player.direction.x());
        phi = phi + 0.05f;
        m_player.direction.setX(cos(phi));
        m_player.direction.setZ(sin(phi));
    }
    if(m_keyState[Qt::Key_F])
    {
        cameraType = 'f';
    }
    if(m_keyState[Qt::Key_T])
    {
        cameraType = 't';
    }
    for(int i=0; i<m_gameObjects.size();)
    {
        GameObject* obj=m_gameObjects[i];
        if(obj->isAlive==false)
        {
            m_gameObjects.erase(m_gameObjects.begin()+i);
            delete obj;
        }
        else
        {
            i++;
        }
    }
}

void GLWidget::setTransforms(void)
{
    m_program->setUniformValue(m_projMatrixLoc, m_proj);
    m_program->setUniformValue(m_viewMatrixLoc, m_camera);
    m_program->setUniformValue(m_modelMatrixLoc, m_world);
}

void GLWidget::initCollisionTriangles()
{
    /*addTriangleCollider(
                QVector3D(25, 0, -25),
                QVector3D(-25, 0, -25),
                QVector3D(25, 0, 25),
                1,
                QVector2D(1, 1),
                QVector2D(0, 1),
                QVector2D(1, 0),
                TextureManager::getTexture("grass"));
    addTriangleCollider(
                QVector3D(-25, 0, -25),
                QVector3D(-25, 0, 25),
                QVector3D(25, 0, 25),
                1,
                QVector2D(0, 1),
                QVector2D(0, 0),
                QVector2D(1, 0),
                TextureManager::getTexture("grass"));
*/
/*
    addTriangleCollider(QVector3D(30, 0, -30), QVector3D(-30, 0, -30), QVector3D(30, 0, 30), 1, QVector2D(1, 1), QVector2D(0, 1), QVector2D(1, 0), TextureManager::getTexture("grass"));
    addTriangleCollider(QVector3D(-30, 0, -30), QVector3D(-30, 0, 30), QVector3D(30, 0, 30), 1, QVector2D(0, 1), QVector2D(0, 0), QVector2D(1, 0), TextureManager::getTexture("grass"));

    addTriangleCollider(QVector3D(-15, 0, 15), QVector3D(-15, 15, 15), QVector3D(15, 15, 15), 1, QVector2D(1, 0), QVector2D(1, 1), QVector2D(0, 1), TextureManager::getTexture("brick"));
    addTriangleCollider(QVector3D(15, 15, 15), QVector3D(15, 0, 15), QVector3D(-15, 0, 15), 1, QVector2D(0, 1), QVector2D(0, 0), QVector2D(1, 0), TextureManager::getTexture("brick"));
    addTriangleCollider(QVector3D(-15, 15, 15), QVector3D(-15, 0, 15), QVector3D(-15, 15, 20), 1, QVector2D(0, 1), QVector2D(0, 0), QVector2D(1, 1), TextureManager::getTexture("brick"));
    addTriangleCollider(QVector3D(-15, 0, 15), QVector3D(-15, 0, 20), QVector3D(-15, 15, 20), 1, QVector2D(0, 0), QVector2D(1, 0), QVector2D(1, 1), TextureManager::getTexture("brick"));
    addTriangleCollider(QVector3D(-35, 0, 20), QVector3D(-35, 15, 20), QVector3D(-15, 15, 20), 1, QVector2D(1, 0), QVector2D(1, 1), QVector2D(0, 1), TextureManager::getTexture("brick"));
    addTriangleCollider(QVector3D(-15, 15, 20), QVector3D(-15, 0, 20), QVector3D(-35, 0, 20), 1, QVector2D(0, 1), QVector2D(0, 0), QVector2D(1, 0), TextureManager::getTexture("brick"));
    addTriangleCollider(QVector3D(-35, 15, 20), QVector3D(-35, 0, 20), QVector3D(-35, 15, -30), 1, QVector2D(0, 1), QVector2D(0, 0), QVector2D(1, 1), TextureManager::getTexture("brick"));
    addTriangleCollider(QVector3D(-35, 0, 20), QVector3D(-35, 0, -30), QVector3D(-35, 15, -30), 1, QVector2D(0, 0), QVector2D(1, 0), QVector2D(1, 1), TextureManager::getTexture("brick"));
    addTriangleCollider(QVector3D(25, 0, -30), QVector3D(25, 15, -30), QVector3D(-35, 15, -30), 1, QVector2D(1, 0), QVector2D(1, 1), QVector2D(0, 1), TextureManager::getTexture("brick"));
    addTriangleCollider(QVector3D(-35, 15, -30), QVector3D(-35, 0, -30), QVector3D(25, 0, -30), 1, QVector2D(0, 1), QVector2D(0, 0), QVector2D(1, 0), TextureManager::getTexture("brick"));
    addTriangleCollider(QVector3D(25, 15, -30), QVector3D(25, 0, -30), QVector3D(25, 15, 25), 1, QVector2D(0, 1), QVector2D(0, 0), QVector2D(1, 1), TextureManager::getTexture("brick"));
    addTriangleCollider(QVector3D(25, 0, -30), QVector3D(25, 0, 25), QVector3D(25, 15, 25), 1, QVector2D(0, 0), QVector2D(1, 0), QVector2D(1, 1), TextureManager::getTexture("brick"));
    addTriangleCollider(QVector3D(25, 15, 25), QVector3D(25, 0, 25), QVector3D(15, 15, 15), 1, QVector2D(0, 1), QVector2D(0, 0), QVector2D(1, 1), TextureManager::getTexture("brick"));
    addTriangleCollider(QVector3D(25, 0, 25), QVector3D(15, 0, 15), QVector3D(15, 15, 15), 1, QVector2D(0, 0), QVector2D(1, 0), QVector2D(1, 1), TextureManager::getTexture("brick"));

    addTriangleCollider(QVector3D(5, 3, -21), QVector3D(-5, 3, -21), QVector3D(5, 3, -13), 1, QVector2D(1, 1), QVector2D(0, 1), QVector2D(1, 0), TextureManager::getTexture("wood"));
    addTriangleCollider(QVector3D(-5, 3, -21), QVector3D(-5, 3, -13), QVector3D(5, 3, -13), 1, QVector2D(0, 1), QVector2D(0, 0), QVector2D(1, 0), TextureManager::getTexture("wood"));
    addTriangleCollider(QVector3D(-5, 3, -21), QVector3D(-10, 0, -21), QVector3D(-5, 3, -13), 1, QVector2D(1, 1), QVector2D(0, 1), QVector2D(1, 0), TextureManager::getTexture("wood"));
    addTriangleCollider(QVector3D(-10, 0, -21), QVector3D(-10, 0, -13), QVector3D(-5, 3, -13), 1, QVector2D(0, 1), QVector2D(0, 0), QVector2D(1, 0), TextureManager::getTexture("wood"));
    addTriangleCollider(QVector3D(10, 0, -21), QVector3D(5, 3, -21), QVector3D(10, 0, -13), 1, QVector2D(1, 1), QVector2D(0, 1), QVector2D(1, 0), TextureManager::getTexture("wood"));
    addTriangleCollider(QVector3D(5, 3, -21), QVector3D(5, 3, -13), QVector3D(10, 0, -13), 1, QVector2D(0, 1), QVector2D(0, 0), QVector2D(1, 0), TextureManager::getTexture("wood"));
    addTriangleCollider(QVector3D(5, 3, -13), QVector3D(-5, 3, -13), QVector3D(5, 0, -13), 1, QVector2D(1, 1), QVector2D(0, 1), QVector2D(1, 0), TextureManager::getTexture("wood"));
    addTriangleCollider(QVector3D(-5, 3, -13), QVector3D(-5, 0, -13), QVector3D(5, 0, -13), 1, QVector2D(0, 1), QVector2D(0, 0), QVector2D(1, 0), TextureManager::getTexture("wood"));
    addTriangleCollider(QVector3D(-5, 3, -13), QVector3D(-10, 0, -13), QVector3D(-5, 0, -13), 1, QVector2D(1, 1), QVector2D(0, 0), QVector2D(1, 0), TextureManager::getTexture("wood"));
    addTriangleCollider(QVector3D(5, 3, -13), QVector3D(5, 0, -13), QVector3D(10, 0, -13), 1, QVector2D(0, 1), QVector2D(0, 0), QVector2D(1, 0), TextureManager::getTexture("wood"));
    addTriangleCollider(QVector3D(-5, 3, -21), QVector3D(5, 3, -21), QVector3D(-5, 0, -21), 1, QVector2D(1, 1), QVector2D(0, 1), QVector2D(1, 0), TextureManager::getTexture("wood"));
    addTriangleCollider(QVector3D(5, 3, -21), QVector3D(5, 0, -21), QVector3D(-5, 0, -21), 1, QVector2D(0, 1), QVector2D(0, 0), QVector2D(1, 0), TextureManager::getTexture("wood"));
    addTriangleCollider(QVector3D(-5, 3, -21), QVector3D(-5, 0, -21), QVector3D(-10, 0, -21), 1, QVector2D(1, 1), QVector2D(1, 0), QVector2D(0, 0), TextureManager::getTexture("wood"));
    addTriangleCollider(QVector3D(5, 3, -21), QVector3D(10, 0, -21), QVector3D(5, 0, -21), 1, QVector2D(0, 1), QVector2D(1, 0), QVector2D(0, 0), TextureManager::getTexture("wood"));
*/
    loadHeightMap("resources/heightmap.png", QVector3D(0.0f, -5.0f, 0.0f), QVector3D(200.0f, 20.0f, 200.0f),
                  QVector2D(10, 10), TextureManager::getTexture("grass"));

    collisionTrianglesMesh.m_primitive = GL_TRIANGLES;
    collisionTrianglesMesh.initVboAndVao();
}

void GLWidget::addTriangleCollider(QVector3D v1, QVector3D v2, QVector3D v3, int groupSize, QVector2D uv1, QVector2D uv2, QVector2D uv3, QOpenGLTexture *texture)
{
    Triangle t;
    t.v1 = v1;
    t.v2 = v2;
    t.v3 = v3;
    t.texture = texture;
    t.groupSize = groupSize;
    t.n = QVector3D::crossProduct(v1-v3, v2-v1).normalized();
    t.A = t.n.x();
    t.B = t.n.y();
    t.C = t.n.z();
    t.D = -(t.A * v1.x() + t.B * v1.y() + t.C * v1.z());

    collisionTriangles.push_back(t);

    collisionTrianglesMesh.add(t.v1, t.n, uv1);
    collisionTrianglesMesh.add(t.v2, t.n, uv2);
    collisionTrianglesMesh.add(t.v3, t.n, uv3);
}

void GLWidget::loadHeightMap(QString filepath, QVector3D center, QVector3D size, QVector2D maxUV, QOpenGLTexture *texture)
{
    QImage img;
    img.load(filepath);

    float boxSizeX = size.x()/img.width();
    float boxSizeZ = size.z()/img.height();

    float offsetX = -boxSizeX * img.width()/2 + center.x();
    float offsetZ = -boxSizeZ * img.height()/2;

    float minY = center.y() - size.y()/2;
    float maxY = center.y() + size.y()/2;

    int groupSize = (img.width() - 1) * (img.height() - 1) * 2;

    for(int i = 0; i < img.height() - 1; i++)
    {
        for(int j = 0; j < img.width() - 1; j++)
        {
            float X1 = offsetX + boxSizeX * i;
            float X2 = offsetX + boxSizeX * (i + 1);
            float Z1 = offsetZ + boxSizeZ * j;
            float Z2 = offsetZ + boxSizeZ * (j + 1);

            float Y00 = QColor(img.pixel(j, i)).red()/255.0f * (maxY - minY) + minY;
            float Y01 = QColor(img.pixel(j + 1, i)).red()/255.0f * (maxY - minY) + minY;
            float Y10 = QColor(img.pixel(j, i + 1)).red()/255.0f * (maxY - minY) + minY;
            float Y11 = QColor(img.pixel(j + 1, i + 1)).red()/255.0f * (maxY - minY) + minY;

            QVector2D texCoord00(maxUV.x() * i/img.width(), maxUV.y() * j/img.height());
            QVector2D texCoord01(maxUV.x() * (i + 1)/img.width(), maxUV.y() * j/img.height());
            QVector2D texCoord10(maxUV.x() * i/img.width(), maxUV.y() * (j + 1)/img.height());
            QVector2D texCoord11(maxUV.x() * (i + 1)/img.width(), maxUV.y() * (j + 1)/img.height());

            addTriangleCollider(QVector3D(X1, Y00, Z1), QVector3D(X1, Y01, Z2), QVector3D(X2, Y11, Z2),
                                groupSize, texCoord00, texCoord10, texCoord11, texture);
            addTriangleCollider(QVector3D(X2, Y11, Z2), QVector3D(X2, Y10, Z1), QVector3D(X1, Y00, Z1),
                                groupSize, texCoord11, texCoord01, texCoord00, texture);
        }
    }
}

void GLWidget::resizeGL(int w, int h)
{
    m_proj.setToIdentity();
    m_proj.perspective(60.0f, GLfloat(w) / h, 0.01f, 100.0f);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    m_lastPos = event->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    float phi = atan2(m_player.direction.z(),m_player.direction.x());
    float theta = acos(m_player.direction.y());

    int dx = event->x() - width()/2;
    int dy = event->y() - height()/2;

    phi = phi + dx * 0.01f;
    theta = theta + dy * 0.01f;
    if(theta<0.01f)theta=0.01f;
    if(theta>3.14f)theta=3.14f;

    m_player.direction.setX(sin(theta) * cos(phi));
    m_player.direction.setY(cos(theta));
    m_player.direction.setZ(sin(theta)*sin(phi));
}

void GLWidget::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape)
        exit(0);
    else if(e->key()==Qt::Key_Space)
    {
        Bullet* bullet=new Bullet();
        bullet->position=m_player.position+m_player.direction*0.7f;
        bullet->position.setY(0);
        bullet->scale=QVector3D(0.5f,0.5f,0.5f);
        bullet->m_radius=0.5f;
        bullet->energy=3*m_player.direction;
        bullet->energy.setY(0);
        addObject(bullet);
    }
    else
        QWidget::keyPressEvent(e);

    if(e->key() >= 0 && e->key() <= 255)
        m_keyState[e->key()] = true;
}

void GLWidget::keyReleaseEvent(QKeyEvent *e)
{
    if(e->key() >= 0 && e->key() <= 255)
        m_keyState[e->key()] = false;
}
