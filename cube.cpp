#include "cube.h"

Cube::Cube()
{
    m_name = "cube";
}

void Cube::init()
{
    m_mesh=CMesh::m_meshes["cube"];
    //scale=QVector3D(1.0f,1.0f,1.0f);
    //m_radius=sqrt(3.0f*pow(1.0f/2,2));
    m_name="Cube";
}

void Cube::render(GLWidget *glwidget)
{
    m_mesh->render(glwidget);
}

void Cube::update()
{
    position = position + energy;
    energy = energy/1.2f;
}
