#include "texturemanager.h"

std::map<std::string, QOpenGLTexture*> TextureManager::m_textures;
TextureManager::TextureManager()
{

}

void TextureManager::init()
{
    m_textures["brick"]=new QOpenGLTexture(QImage("resources/brick.jpg"));
}

QOpenGLTexture* TextureManager::getTexture(std::string name)
{
    return m_textures[name];
}
