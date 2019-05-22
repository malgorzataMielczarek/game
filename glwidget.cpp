#include "glwidget.h"
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <math.h>
#include <iostream>
#include <qstack.h>
using namespace std;

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      m_program(nullptr)
{
    setFocusPolicy(Qt::StrongFocus);
}

GLWidget::~GLWidget()
{
    for(auto it = m_meshes.begin() ; it != m_meshes.end(); it++)
        delete it.value();

    cleanup();
}

QSize GLWidget::sizeHint() const
{
    return QSize(1000, 800);
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

void GLWidget::qNormalizeAngle(float &angle)
{
    while (angle < 0)
        angle += 360;
    while (angle > 360)
        angle -= 360;
}

void GLWidget::setXRotation(float angle)
{
    qNormalizeAngle(angle);
    if (abs(angle - m_camXRot) > 0.001f) {
        m_camXRot = angle;
        emit xRotationChanged(angle);
        update();
    }
}

void GLWidget::setYRotation(float angle)
{
    qNormalizeAngle(angle);
    if (abs(angle - m_camYRot) > 0.001f) {
        m_camYRot = angle;
        emit yRotationChanged(angle);
        update();
    }
}

void GLWidget::setZRotation(float angle)
{
    qNormalizeAngle(angle);
    if (abs(angle - m_camZRot) > 0.001f) {
        m_camZRot = angle;
        emit zRotationChanged(angle);
        update();
    }
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.1f, 0.2f, 0.3f, 1);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

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
    m_lightLoc.position = m_program->uniformLocation("light.position");
    m_lightLoc.ambient = m_program->uniformLocation("light.ambient");
    m_lightLoc.diffuse = m_program->uniformLocation("light.diffuse");

    m_meshes.insert("Cube", new CMesh);
    m_meshes["Cube"]->generateCube(1.0f, 1.0f, 1.0f);

    m_meshes.insert("Sphere", new CMesh);
    m_meshes["Sphere"]->generateSphere(0.5f, 24);

    m_meshes.insert("Bunny", new CMesh);
    m_meshes["Bunny"]->generateMeshFromObjFile("resources/bunny.obj");

    m_program->release();
	
	m_timer = 0;

    m_robotPosition = QVector3D(0.0f, 0.0f, 0.0f);
	
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
    m_camera.translate(0, 0, -m_camDistance);

    m_world.setToIdentity();
    m_world.rotate(m_camXRot, 1, 0, 0);
    m_world.rotate(m_camYRot, 0, 1, 0);
    m_world.rotate(m_camZRot, 0, 0, 1);

    // Bunny
    worldMatrixStack.push(m_world);
        m_world.translate(0.7f, 0.0f, 0.2f);
        m_world.scale(QVector3D(0.1f, 0.1f, 0.1f));
        setTransforms();
        m_program->setUniformValue(m_modelColorLoc,QVector3D(1.0f, 1.0, 1.0));
        m_meshes["Bunny"]->render(this);
    m_world = worldMatrixStack.pop();

    // Robot
    worldMatrixStack.push(m_world);

        m_world.translate(m_robotPosition);

    // Robot's Head
        worldMatrixStack.push(m_world);
            m_world.translate(0.0f, 0.20f, 0.0f);
            m_world.scale(QVector3D(0.1f, 0.1f, 0.1f));
            setTransforms();
            m_program->setUniformValue(m_modelColorLoc, QVector3D(0.0f, 1.0, 0.0));
            m_meshes["Cube"]->render(this);
        m_world = worldMatrixStack.pop();

    // Robot's Body
        worldMatrixStack.push(m_world);
            m_world.translate(0.0f, 0.0f, 0.0f);
            m_world.scale(QVector3D(0.25f, 0.3f, 0.15f));
            setTransforms();
            m_program->setUniformValue(m_modelColorLoc, QVector3D(0.0f, 0.0, 1.0));
            m_meshes["Cube"]->render(this);
        m_world = worldMatrixStack.pop();

    // Robot's Left Arm
        worldMatrixStack.push(m_world);
            m_world.translate(-0.12f, 0.05f, 0.0f);
            m_world.rotate(15.0f, 0, 0, 1);
            m_world.translate(-0.08f, 0.0f, 0.0f);
            m_world.scale(QVector3D(0.16f, 0.05f, 0.05f));
            setTransforms();
            m_program->setUniformValue(m_modelColorLoc, QVector3D(1.0f, 0.0, 0.0));
            m_meshes["Cube"]->render(this);
        m_world = worldMatrixStack.pop();

    // Robot's Right Arm
        worldMatrixStack.push(m_world);
            m_world.translate(0.2f, 0.05f, 0.0f);
            m_world.rotate(-15.0f, 0, 0, 1);
            m_world.scale(QVector3D(0.16f, 0.05f, 0.05f));
            setTransforms();
            m_program->setUniformValue(m_modelColorLoc, QVector3D(1.0f, 0.0, 0.0));
            m_meshes["Cube"]->render(this);
        m_world = worldMatrixStack.pop();

    // Robot's Left Leg
        worldMatrixStack.push(m_world);
            m_world.translate(-0.1f, -0.2f, 0.0f);
            m_world.rotate(-15.0f, 0, 0, 1);
            m_world.scale(QVector3D(0.05f, 0.16f, 0.05f));
            setTransforms();
            m_program->setUniformValue(m_modelColorLoc, QVector3D(1.0f, 0.0, 0.0));
            m_meshes["Cube"]->render(this);
        m_world = worldMatrixStack.pop();

    // Robot's Right Leg
        worldMatrixStack.push(m_world);
            m_world.translate(0.1f, -0.2f, 0.0f);
            m_world.rotate(15.0f, 0, 0, 1);
            m_world.scale(QVector3D(0.05f, 0.16f, 0.05f));
            setTransforms();
            m_program->setUniformValue(m_modelColorLoc, QVector3D(1.0f, 0.0, 0.0));
            m_meshes["Cube"]->render(this);
        m_world = worldMatrixStack.pop();

    m_world = worldMatrixStack.pop();

    // Circle of spheres and cubes
    for(int i = 0 ; i < 15 ; i++)
    {
        QVector3D innerPosition;
        QVector3D outerPosition;

        float r1 = 1;
        float r2 = 2;
        float theta1 = float(i) / 15 * 2 * float(M_PI);
        float theta2 = float(i+0.5f) / 15 * 2 * float(M_PI);

        innerPosition.setX(r1 * cos(theta1));
        innerPosition.setY(0);
        innerPosition.setZ(r1 * sin(theta1));

        outerPosition.setX(r2 * cos(theta2));
        outerPosition.setY(0);
        outerPosition.setZ(r2 * sin(theta2));

        worldMatrixStack.push(m_world);
            m_world.translate(innerPosition);
            m_world.scale(QVector3D(0.2f, 0.2f, 0.2f));
            setTransforms();
            m_program->setUniformValue(m_modelColorLoc, QVector3D(cos(theta1) * 0.5f + 0.5f, sin(theta1) * 0.5f + 0.5f, 0.0));
            m_meshes["Sphere"]->render(this);
        m_world = worldMatrixStack.pop();

        worldMatrixStack.push(m_world);
            m_world.translate(outerPosition);
            m_world.scale(QVector3D(0.2f, 0.2f, 0.2f));
            setTransforms();
            m_program->setUniformValue(m_modelColorLoc, QVector3D(0.02f, cos(theta1) * 0.5f + 0.5f, sin(theta1) * 0.5f + 0.5f));
            m_meshes["Cube"]->render(this);
        m_world = worldMatrixStack.pop();
    }

    m_program->release();

    m_timer++;

    if(m_keyState[Qt::Key_Z]) m_camDistance += 0.005f;
    if(m_keyState[Qt::Key_X]) m_camDistance -= 0.005f;

    if(m_keyState[Qt::Key_W]) m_robotPosition.setZ(m_robotPosition.z() - 0.005f);
    if(m_keyState[Qt::Key_S]) m_robotPosition.setZ(m_robotPosition.z() + 0.005f);
    if(m_keyState[Qt::Key_A]) m_robotPosition.setX(m_robotPosition.x() - 0.005f);
    if(m_keyState[Qt::Key_D]) m_robotPosition.setX(m_robotPosition.x() + 0.005f);

    update();
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
    int dx = event->x() - m_lastPos.x();
    int dy = event->y() - m_lastPos.y();

    if (event->buttons() & Qt::LeftButton) {
        setXRotation(m_camXRot + 0.5f * dy);
        setYRotation(m_camYRot + 0.5f * dx);
    } else if (event->buttons() & Qt::RightButton) {
        setXRotation(m_camXRot + 0.5f * dy);
        setZRotation(m_camZRot + 0.5f * dx);
    }
    m_lastPos = event->pos();
}

void GLWidget::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape)
        exit(0);
    else if (e->key() == Qt::Key_Q)
        exit(0);
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
