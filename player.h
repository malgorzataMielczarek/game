#ifndef PLAYER_H
#define PLAYER_H

#include <QVector3D>
#include "gameobject.h"
#include "cmesh.h"

class Player:public GameObject
{
public:
    Player();

    QVector3D direction;
    float speed;

    void init();
    void render(GLWidget* glwidget);
    void update();

    CMesh m_mesh;
};

#endif // PLAYER_H
