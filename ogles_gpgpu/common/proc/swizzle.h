//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// See LICENSE file in project repository root for the license.
//

// Copyright (c) 2016-2017, David Hirvonen (this file)

/**
 * GPGPU single texture swizzling (i.e., RGBA to BGRA).
 */
#ifndef OGLES_GPGPU_COMMON_PROC_SWIZZLE
#define OGLES_GPGPU_COMMON_PROC_SWIZZLE

#include "ogles_gpgpu/common/proc/base/filterprocbase.h"

BEGIN_OGLES_GPGPU

/**
 * A simple filter for single image texture swizzling.  
 */

class SwizzleProc : public ogles_gpgpu::FilterProcBase {
public:

    /**
     * Define swizzle conversion types (relative to input RGBA)
     */    
    enum SwizzleKind {
        kSwizzleRGBA,
        kSwizzleBGRA,
        kSwizzleARGB,
        kSwizzleABGR,
        kSwizzleGRAB
    };

    /**
     * Constructor with optional swizzle conversion type.
     */    
    SwizzleProc(SwizzleKind swizzleKind = kSwizzleBGRA)
        : swizzleKind(swizzleKind) {
    }

    /**
     * Return the processor's name.
     */        
    virtual const char* getProcName() {
        return "SwizzleProc";
    }

    /**
     * Set the swizzle operation to be performed.
     */            
    void setSwizzleType(SwizzleKind kind) {
        swizzleKind = kind;
    }

    /**
     * Get the fragment shader source for active swizzle type.
     */                
    virtual const char* getFragmentShaderSource();

protected:
    
    static const char* fshaderRGBASrc;
    static const char* fshaderBGRASrc;
    static const char* fshaderARGBSrc;
    static const char* fshaderABGRSrc;
    static const char* fshaderGRABSrc;
    SwizzleKind swizzleKind = kSwizzleBGRA;
};

END_OGLES_GPGPU

#endif // OGLES_GPGPU_COMMON_PROC_SWIZZLE
