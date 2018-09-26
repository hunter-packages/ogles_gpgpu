//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// See LICENSE file in project repository root for the license.
//

// Copyright (c) 2017-2018, David Hirvonen (this file)

#include "mesh.h"

BEGIN_OGLES_GPGPU

// clang-format off
const char * MeshShaderProc::vshaderMeshSrc = OG_TO_STR
(
 attribute vec4 aPos;
 attribute vec2 aTexCoord;
 varying vec2 vTexCoord;
 uniform mat4 transformMatrix;
 void main()
 {
    gl_Position = transformMatrix * vec4(aPos.xyz, 1.0);
    vTexCoord = aTexCoord;
 }
 );
// clang-format on

// clang-format off
const char * MeshShaderProc::fshaderMeshSrc =
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

MeshShaderProc::MeshShaderProc(const VertexBuffer& vertices, const CoordBuffer& coords)
    : texUnit(1)
    , texTarget(GL_TEXTURE_2D)
    , vertices(vertices)
    , coords(coords)
{
    for(int y = 0; y < 4; y++) {
        for(int x = 0; x < 4; x++) {
            MVP.data[y][x] = 0.f;
        }
        MVP.data[y][y] = 1.f;            
    }    

    // Compile utility shader:
    shader = std::make_shared<Shader>();
    if (!shader->buildFromSrc(vshaderMeshSrc, fshaderMeshSrc))
    {
        throw std::runtime_error("MeshShader: shader error");
    }

    shParamAPos = shader->getParam(ATTR, "aPos");
    shParamATexCoord = shader->getParam(ATTR, "aTexCoord");
    shParamUInputTex = shader->getParam(UNIF, "uInputTex");
    shParamUMVP = shader->getParam(UNIF, "transformMatrix");
    
    Tools::checkGLErr(getProcName(), "get uniforms and attributes");
}

void MeshShaderProc::setMesh(const VertexBuffer& verticesIn, const CoordBuffer& coordsIn) {
    vertices = verticesIn;
    coords = coordsIn;
}

void MeshShaderProc::setModelViewProjection(const Mat44f &mvp) {
    MVP = mvp;
}

void MeshShaderProc::setUniforms() {
    FilterProcBase::setUniforms();
    glUniformMatrix4fv(shParamUMVP, 1, 0, &MVP.data[0][0]);
    // TODO: Add bicubic
}

void MeshShaderProc::getUniforms() {
    FilterProcBase::getUniforms();
    shParamUMVP = shader->getParam(UNIF, "transformMatrix");
    // TODO: Add bicubic
}

void MeshShaderProc::filterRenderSetCoords() {
    // render to FBO
    if (fbo)
        fbo->bind();

    // set geometry
    glEnableVertexAttribArray(shParamAPos);
    glVertexAttribPointer(shParamAPos,
        OGLES_GPGPU_QUAD_COORDS_PER_VERTEX,
        GL_FLOAT,
        GL_FALSE,
        0,
        vertices.front().data());
        
    glVertexAttribPointer(shParamATexCoord,
        OGLES_GPGPU_QUAD_TEXCOORDS_PER_VERTEX,
        GL_FLOAT,
        GL_FALSE,
        0,
        coords.front().data());
    glEnableVertexAttribArray(shParamATexCoord);
    
    
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0, 0, 0, 1);
}

void MeshShaderProc::filterRenderDraw() {
    glDrawArrays(triangleKind, 0, static_cast<int>(vertices.size()));
}

void MeshShaderProc::setTriangleKind(GLenum kind) {
    triangleKind = kind;
}

GLenum MeshShaderProc::getTriangleKind() const {
    return triangleKind;
}

END_OGLES_GPGPU
