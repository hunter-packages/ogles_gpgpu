//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// See LICENSE file in project repository root for the license.
//

// Copyright (c) 2017, David Hirvonen (this file)

/**
 * Framebuffer object handler.
 */
#ifndef OGLES_GPGPU_COMMON_GL_PBO
#define OGLES_GPGPU_COMMON_GL_PBO

#include "../common_includes.h"

namespace ogles_gpgpu {

/**
 * Input pixelbuffer object handler. Set up an OpenGL pixelbuffer for efficient
 * pack operations (gpu->cpu).  This can be used as an alternative to
 * glReadPixels on OpengL ES 3.0 platforms.
 */

class IPBO {
public:
    /**
     * Constructor.
     */
    IPBO(std::size_t width, std::size_t height);

    /**
     * Destructor.
     */
    ~IPBO();

    /**
     * Bind the PBO (GL_PIXEL_PACK_BUFFER)
     */
    void bind();

    /**
     * Unbind the PBO (GL_PIXEL_PACK_BUFFER).
     */
    void unbind();

    /**
     * Start read operation (asynchronous/non-blocking).
     */
    void start(); // asynchronous

    /**
     * Pack/read pixels to <buffer> (blocking call) after a call to start().
     */
    void finish(GLubyte* buffer); // asynchronous

    /**
     * Perform a blocking pack from the PBO.
     */
    void read(GLubyte* buffer); // synchronous (start, finish)

    /**
     * Returns current processing state for asynchronous read.
     */
    bool isReadingAsynchronously() const;

protected:
    bool isReadingAsynchronously_ = false; // read state (async API)
    std::size_t width; // width of PBO
    std::size_t height; // head of PBO
    GLuint pbo; // ID of created PBO
};

class OPBO {
public:
    /**
     * Constructor.
     */
    OPBO(std::size_t width, std::size_t height);

    /**
     * Destructor.
     */
    ~OPBO();

    /**
     * Bind the PBO (GL_PIXEL_UNPACK_BUFFER)
     */
    void bind();

    /**
     * Unbind the PBO (GL_PIXEL_UNPACK_BUFFER)
     */
    void unbind();

    /**
     * Unpack/write pixels in <buffer> to texture <texId>.
     */
    void write(const GLubyte* buffer, GLuint texId = 0);

protected:
    std::size_t width; // width of PBO
    std::size_t height; // head of PBO
    GLuint pbo; // ID of created PBO
};

} // ogles_gpgpu

#endif
