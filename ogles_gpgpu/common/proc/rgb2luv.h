//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// See LICENSE file in project repository root for the license.
//

// Copyright (c) 2014-2017, David Hirvonen (this file)

/**
 * GPGPU RGB to LUV conversion with built in shift to [0,1] range.
 */

#ifndef OGLES_GPGPU_COMMON_PROC_RGB2LUV
#define OGLES_GPGPU_COMMON_PROC_RGB2LUV

#include "ogles_gpgpu/common/proc/base/filterprocbase.h"

BEGIN_OGLES_GPGPU

/**
 * GPGPU filter for RGB to LUV color conversion.
 *
 * see: https://github.com/pdollar/toolbox/blob/master/channels/rgbConvert.m
 *
 * A per channel shift is performed within [0,1] range as follows:
 * L=L/270
 * u=(u+88)/270
 * v=(v+134)/270
 * luv.x = (luv.x/270.0);
 * luv.y = ((luv.y+88.0)/270.0);
 * luv.z = ((luv.z+134.0)/270.0);
 */
class Rgb2LuvProc : public FilterProcBase {
public:

    /**
     * Constructor.
     */    
    Rgb2LuvProc() {}

    /**
     * Return the processors name.
     */    
    virtual const char* getProcName() {
        return "Rgb2LuvProc";
    }

private:

    /**
     * Get the fragment shader source.
     */    
    virtual const char* getFragmentShaderSource() {
        return fshaderRgb2LuvSrc;
    }

    static const char* fshaderRgb2LuvSrc; // fragment shader source
};

END_OGLES_GPGPU

#endif // OGLES_GPGPU_COMMON_PROC_RGB2LUV
