//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// Author: Markus Konrad <post@mkonrad.net>, Winter 2014/2015
// http://www.mkonrad.net
//
// See LICENSE file in project repository root for the license.
//

#include "disp.h"
#include "../common_includes.h"

using namespace std;
using namespace ogles_gpgpu;

// clang-format off
const char *Disp::fshaderDispSrc =
#if defined(OGLES_GPGPU_OPENGLES)
OG_TO_STR(precision mediump float;)
#endif
OG_TO_STR(
varying vec2 vTexCoord;
uniform sampler2D uInputTex;
void main()
{
    gl_FragColor = vec4(texture2D(uInputTex, vTexCoord).rgba);
}
);
// clang-format on

Disp::Disp() = default;

Disp::Disp(const Callback &renderbufferStorage)
    : renderbufferStorage(renderbufferStorage)
{
    
}

Disp::~Disp()
{
    if (renderbufferStorage)
    {
        glDeleteRenderbuffers(1, &renderbuffer);
    }
}

int Disp::init(int inW, int inH, unsigned int order, bool prepareForExternalInput) {
    OG_LOGINF(getProcName(), "initialize");

    if(renderbufferStorage)
    {
        createFBO();
    }
    
    // ProcBase init - set defaults
    baseInit(inW, inH, order, prepareForExternalInput, procParamOutW, procParamOutH, procParamOutScale);

    // FilterProcBase init - create shaders, get shader params, set buffers for OpenGL
    filterInit(vshaderDefault, fshaderDispSrc);

    return 1;
}

int Disp::render(int position) {
    OG_LOGINF(getProcName(), "input tex %d, target %d, framebuffer of size %dx%d", texId, texTarget, outFrameW, outFrameH);

    filterRenderPrepare();
    glViewport(tx, ty, outFrameW * resolutionX, outFrameH * resolutionY); // override
    Tools::checkGLErr(getProcName(), "render prepare");

    filterRenderSetCoords();
    Tools::checkGLErr(getProcName(), "render set coords");

    filterRenderDraw();
    Tools::checkGLErr(getProcName(), "render draw");
    
    if(renderbufferStorage)
    {
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        Tools::checkGLErr(getProcName(), "glBindRenderbuffer");
    }

    filterRenderCleanup();
    Tools::checkGLErr(getProcName(), "render cleanup");

    return 0;
}

void Disp::createFBO()
{
    FilterProcBase::createFBO();
    
    if (renderbufferStorage) {
        
        fbo->bind();
        Tools::checkGLErr(getProcName(), "glBindFramebuffer");
        
        // Create a Renderbuffer
        glGenRenderbuffers(1, &renderbuffer);
        Tools::checkGLErr(getProcName(), "glGenRenderbuffers");
    
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        Tools::checkGLErr(getProcName(), "glBindRenderbuffer");
    
        renderbufferStorage();
        Tools::checkGLErr(getProcName(), "renderbufferStorage");
        
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer);
        Tools::checkGLErr(getProcName(), "glFramebufferRenderbuffer");
    
        auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
            std::stringstream ss;
            ss << "Framebuffer incomplete :" << int(fboStatus);
            throw std::runtime_error(ss.str());
        }
    }
}
