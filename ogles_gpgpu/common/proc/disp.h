//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// Author: Markus Konrad <post@mkonrad.net>, Winter 2014/2015
// http://www.mkonrad.net
//
// See LICENSE file in project repository root for the license.
//

/**
 * Helper class to simply display an output.
 */
#ifndef OGLES_GPGPU_COMMON_PROC_DISP
#define OGLES_GPGPU_COMMON_PROC_DISP

#include "../common_includes.h"

#include "base/filterprocbase.h"

#include <functional>

namespace ogles_gpgpu {

/**
 * Helper class to simply display an output.
 * Render an input texture to a fullscreen quad.
 */
class Disp : public FilterProcBase {
public:
    
    using Callback = std::function<void()>;
    
    /**
     * Default constructor
     */
    Disp();
    
    /**
     * Constructor for renderbuffer based display
     */
    Disp(const Callback &renderbufferStorage);

    /**
     * Destructor
     */
    virtual ~Disp();
    
    /**
     * Output resolution of display.
     *
     * This is useful for setting glViewport correctly.  On Apple Retina displays, for example,
     * screen coordinates are not in pixels.
     */
    virtual void setDisplayResolution(float x, float y) {
        resolutionX = x;
        resolutionY = y;
    }

    /**
     * Output display offset.
     *
     * This is useful for a "letterbox" display for fixed aspect ratio images on a resized
     * canvas.
     */
    virtual void setOffset(float x, float y) {
        tx = x;
        ty = y;
    }

    /**
     * Return the processors name.
     */
    virtual const char* getProcName() {
        return "Disp";
    }

    /**
     * Init the processor for input frames of size <inW>x<inH> which is at
     * position <order> in the processing pipeline.
     */
    virtual int init(int inW, int inH, unsigned int order, bool prepareForExternalInput = false);

    /**
     * Render the output.
     */
    virtual int render(int position);

    /**
     * Not implemented - no output texture needed because Disp renders on screen.
     */
    virtual void createFBOTex(bool genMipmap) {}

    /**
     * Not implemented - no output is returned because Disp renders on screen.
     */
    virtual void getResultData(unsigned char* data = nullptr, int index = 0) const {} 

    /**
     * Not implemented - no output is returned because Disp renders on screen.
     */
    virtual void getResultData(const FrameDelegate& frameDelegate = {}, int index = 0) const {}

    /**
     * Not implemented - no MemTransferObj for output is set because Disp renders on screen.
     */
    virtual MemTransfer* getMemTransferObj() const {
        return NULL;
    }

    /**
     * Create an FBO for this processor. This will contain the result after rendering in its attached texture.
     *
     * (optional) create renderbuffer if renderbufferStorage callback was specified in constructor.
     */
    virtual void createFBO();
    
private:
    float tx = 0.f;
    float ty = 0.f;
    float resolutionX = 1.f;
    float resolutionY = 1.f;

    static const char* fshaderDispSrc; // fragment shader source
    
    GLuint renderbuffer; // (optional) renderbuffer based display
    Callback renderbufferStorage;
};
}

#endif
