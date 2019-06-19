#ifndef BULLET_H
#define BULLET_H

#include "gameobject.h"
#include "cmesh.h"

class Bullet:public GameObject
{
public:
    Bullet();

    void init();
    void render(GLWidget* glwidget);
    void update();

    CMesh* m_mesh;
};

#endif // BULLET_H
