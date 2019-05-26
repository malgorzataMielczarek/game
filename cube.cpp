#include "cube.h"

Cube::Cube()
{
    m_name = "cube";
}

void Cube::init()
{
    m_mesh.generateCube(1.0f,1.0f,1.0f);
}

void Cube::render(GLWidget *glwidget)
{
    m_mesh.render((glwidget));
}

void Cube::update()
{
    position = position + energy;
    energy = energy/1.2f;
}
