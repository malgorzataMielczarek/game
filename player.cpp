#include "player.h"

Player::Player()
{
    position = QVector3D(0,0,0);
    direction = QVector3D(0,0,-1);
    speed = 0.01f;
}

void Player::init()
{
    m_mesh=CMesh::m_meshes["bunny"];
    scale = QVector3D(0.1f,0.1f,0.1f);
    m_radius = 0.1f;
    m_name = "Player";
}

void Player::render(GLWidget *glwidget)
{
    m_mesh->render(glwidget);
}

void Player::update()
{
    position = position + energy;
    energy = energy/1.2f;
}
