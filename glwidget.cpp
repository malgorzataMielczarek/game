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

    m_program->setUniformValue(m_lightLoc.position, QVector3D(0.0f, 0.0f, 15.0f));
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
                else
                {
                    v.normalize();
                    float energySum=obj->energy.length()+obj2->energy.length();
                    obj->energy=v*energySum/2;
                    obj2->energy=-v*energySum/2;
                }
            }
        }
        obj->update();
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
