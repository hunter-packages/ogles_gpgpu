//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// See LICENSE file in project repository root for the license.
//

// Copyright (c) 2016-2017, David Hirvonen (this file)

/**
 * GPGPU Letterbox preview effect
 */
#ifndef OGLES_GPGPU_COMMON_PROC_LETTERBOX
#define OGLES_GPGPU_COMMON_PROC_LETTERBOX

#include "ogles_gpgpu/common/proc/base/filterprocbase.h"

#include <array>
#include <functional>

BEGIN_OGLES_GPGPU

/**
 * GPGPU cinema style letterbox filter effect that provides options for the
 * user to set the height of the centered visible pass-through band and the 
 * colors of the upper and lower letterbox bands.  Additionally, a callback is 
 * provided to support further per frame customizations.  Note that this effect 
 * is applied as an overlay on top of existing frames, so no transformation is 
 * applied to the input image.
 */
class LetterboxProc : public ogles_gpgpu::FilterProcBase {
public:

    /**
     * Alias for a display customization callback, which receives a
     * pointer to this.
     */
    using RenderDelegate = std::function<void(FilterProcBase* filter)>;

    /**
     * Constructor with optional height for center transparent portion of the effect.
     */    
    LetterboxProc(float height = 1.f);

    /**
     * Return the processors name.
     */    
    virtual const char* getProcName() {
        return "LetterboxProc";
    }

    /**
     * Set the height for the center transparent portion of the effect.
     */    
    void setHeight(float value) {
        height = value;
    }

    /**
     * Set the color of the upper and lower letterbox bands.
     */        
    void setColor(float r, float g, float b) {
        color = { { r, g, b } };
    }

    /**
     * Set the user defined callback for display customization.
     */            
    void setCallback(RenderDelegate& f) {
        func = f;
    }

    /**
     * Definition of draw step for filtering, which is responsible for
     * performing the display customization via the user provided callback.
     */                
    virtual void filterRenderDraw();

private:

    /**
     * Get the fragment shader source.
     */    
    virtual const char* getFragmentShaderSource() {
        return fshaderLetterboxSrc;
    }
    /**
     * Get uniform indices.
     */        
    virtual void getUniforms();
    
    /**
     * Set additional uniform.
     */            
    virtual void setUniforms();

protected:
    
    static const char* fshaderLetterboxSrc;
    float height = 1.f;
    std::array<float, 3> color;
    GLint shParamULetterboxHeight;
    GLint shParamULetterboxColor;
    RenderDelegate func;
};

END_OGLES_GPGPU

#endif // OGLES_GPGPU_COMMON_PROC_LETTERBOX
