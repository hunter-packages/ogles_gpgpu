//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// See LICENSE file in project repository root for the license.
//

// Copyright (c) 2017, David Hirvonen (this file)

#include "pbo.h"
#include "ogles_gpgpu/platform/opengl/gl_includes.h"

#include <iostream>
#include <sstream>
#include <cstring> // memcpy

#include <stdlib.h>

using namespace std;
using namespace ogles_gpgpu;

// ::: input/read  :::

IPBO::IPBO(std::size_t width, std::size_t height)
    : width(width)
    , height(height)
    , isReadingAsynchronously_(false) {

    glGenBuffers(1, &pbo);
    Tools::checkGLErr("IPBO::IPBO", "glGenBuffers()");

    std::size_t pbo_size = width * height * 4;
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    Tools::checkGLErr("IPBO::IPBO", "glBindBuffer()");

    glBufferData(GL_PIXEL_PACK_BUFFER, pbo_size, 0, GL_DYNAMIC_READ);
    Tools::checkGLErr("IPBO::IPBO", "glBufferData()");

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    Tools::checkGLErr("IPBO::IPBO", "glBindBuffer()");
}

IPBO::~IPBO() {
    if (pbo > 0) {
        glDeleteBuffers(1, &pbo);
        Tools::checkGLErr("IPBO::~IPBO", "glDeleteBuffers()");
        pbo = 0;
    }
}

bool IPBO::isReadingAsynchronously() const {
    return isReadingAsynchronously_;
}

void IPBO::bind() {
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    Tools::checkGLErr("IPBO::bind", "glBindBuffer()");
}

void IPBO::unbind() {
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    Tools::checkGLErr("IPBO::unbind", "glBindBuffer()");
}

void IPBO::start() {

    if (!isReadingAsynchronously_) {
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        Tools::checkGLErr("IPBO::start", "glReadBuffer()");

        // Note glReadPixels last argument == 0 for PBO reads
        glReadPixels(0, 0, width, height, OGLES_GPGPU_TEXTURE_FORMAT, GL_UNSIGNED_BYTE, 0);
        Tools::checkGLErr("IPBO::start", "glReadPixels()");

        isReadingAsynchronously_ = true;
    }
}

void IPBO::finish(GLubyte* buffer) {

    if (isReadingAsynchronously_) {
        std::size_t pbo_size = width * height * 4;
#if defined(OGLES_GPGPU_OSX)
        // Note: glMapBufferRange does not seem to work in OS X
        GLubyte* ptr = static_cast<GLubyte*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));
#else
        GLubyte* ptr = static_cast<GLubyte*>(glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, pbo_size, GL_MAP_READ_BIT));
#endif
        Tools::checkGLErr("IPBO::finish", "glMapBufferRange()");

        if (ptr) {
            memcpy(buffer, ptr, pbo_size);

            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            Tools::checkGLErr("IPBO::finish", "glUnmapBuffer()");
        }

        isReadingAsynchronously_ = false;
    }
}

void IPBO::read(GLubyte* buffer) {
    start(); // Use start() and
    finish(buffer); // finish() pair for consistent internal state
}

// ::: output/write  :::

OPBO::OPBO(std::size_t width, std::size_t height)
    : width(width)
    , height(height) {

    glGenBuffers(1, &pbo);
    Tools::checkGLErr("OPBO::OPBO", "glGenBuffers()");

    std::size_t pbo_size = width * height * 4;
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    Tools::checkGLErr("OPBO::OPBO", "glBindBuffer()");

    glBufferData(GL_PIXEL_UNPACK_BUFFER, pbo_size, 0, GL_STREAM_DRAW);
    Tools::checkGLErr("OPBO::OPBO", "glBufferData()");

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    Tools::checkGLErr("OPBO::OPBO", "glBindBuffer()");
}

OPBO::~OPBO() {
    if (pbo > 0) {
        glDeleteBuffers(1, &pbo);
        Tools::checkGLErr("OPBO::~OPBO", "glDeleteBuffers()");
        pbo = 0;
    }
}

void OPBO::bind() {
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    Tools::checkGLErr("OPBO::bind", "glBindBuffer()");
}

void OPBO::unbind() {
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    Tools::checkGLErr("OPBO::unbind", "glBindBuffer()");
}

void OPBO::write(const GLubyte* buffer, GLuint texId) {
    std::size_t pbo_size = width * height * 4;

    glBufferData(GL_PIXEL_UNPACK_BUFFER, pbo_size, NULL, GL_STREAM_DRAW);
    Tools::checkGLErr("OPBO::write", "glBufferData()");

#if defined(OGLES_GPGPU_OSX)
    // TODO: glMapBufferRange does not seem to work in OS X
    GLubyte* ptr = static_cast<GLubyte*>(glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));
    Tools::checkGLErr("OPBO::write", "glMapBuffer()");
#else
    GLubyte* ptr = static_cast<GLubyte*>(glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, pbo_size, GL_MAP_WRITE_BIT));
    Tools::checkGLErr("OPBO::write", "glMapBufferRange()");
#endif

    if (ptr) {
        memcpy(ptr, buffer, pbo_size);

        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        Tools::checkGLErr("OPBO::write", "glUnmapBuffer()");

        glBindTexture(GL_TEXTURE_2D, texId);
        Tools::checkGLErr("OPBO::write", "glBindTexture()");

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, OGLES_GPGPU_TEXTURE_FORMAT, GL_UNSIGNED_BYTE, 0);
        Tools::checkGLErr("OPBO::write", "glTexSubImage2D()");
    }
}
