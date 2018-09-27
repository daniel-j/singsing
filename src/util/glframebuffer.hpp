#pragma once

#include <GL/glew.h>
#include "./glutils.hpp"

class Framebuffer {
 public:
    Framebuffer(unsigned int width = 0, unsigned int height = 0, GLenum format = GL_RGB)
        : m_width(width), m_height(height), m_format(format) {

        // create fbo texture
        GLCall(glGenTextures(1, &m_texture));
        GLCall(glBindTexture(GL_TEXTURE_2D, m_texture));
        GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GLCall(glTexImage2D(GL_TEXTURE_2D, 0, m_format, m_width, m_height, 0, m_format, GL_UNSIGNED_BYTE, nullptr));

        // create fbo and bind texture
        GLCall(glGenFramebuffers(1, &m_fbo));
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
        GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0));

        // GLenum drawBuffers[1]{GL_COLOR_ATTACHMENT0};
        // glDrawBuffers(1, drawBuffers);
        
        // unbind fbo and texture
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        GLCall(glBindTexture(GL_TEXTURE_2D, 0));
    }

    ~Framebuffer() {
        GLCall(glDeleteFramebuffers(1, &m_fbo));
        GLCall(glDeleteTextures(1, &m_texture));
    }

    void bind() {
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
        GLCall(glViewport(0, 0, m_width, m_height));
    }
    void unbind() {
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    }
    void resize(unsigned int width, unsigned int height) {
        if (m_width == width && m_height == height) return;
        m_width = width;
        m_height = height;
        GLCall(glBindTexture(GL_TEXTURE_2D, m_texture));
        GLCall(glTexImage2D(GL_TEXTURE_2D, 0, m_format, m_width, m_height, 0, m_format, GL_UNSIGNED_BYTE, nullptr));

        // unbind texture
        GLCall(glBindTexture(GL_TEXTURE_2D, 0));
    }

    const GLuint getHandle() const {
        return m_fbo;
    }
    const GLuint getTexture() const {
        return m_texture;
    }

 private:
    unsigned int m_width, m_height;
    GLenum m_format;
    GLuint m_fbo;
    GLuint m_texture;
};
