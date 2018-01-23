//
// ogles_gpgpu project - GPGPU for mobile devices and embedded systems using OpenGL ES 2.0
//
// See LICENSE file in project repository root for the license.
//

// Copyright (c) 2017-2018, David Hirvonen (this file)

#ifndef OGLES_GPGPU_COMMON_PROC_MESH
#define OGLES_GPGPU_COMMON_PROC_MESH

#include <ogles_gpgpu/common/proc/base/filterprocbase.h>

#include <array>
#include <memory>

BEGIN_OGLES_GPGPU

class MeshShaderProc : public FilterProcBase {
public:
    using VertexBuffer = std::vector<std::array<float,3>>;
    using CoordBuffer = std::vector<std::array<float,2>>;

    /**
     * Constructor.
     */
    MeshShaderProc(const VertexBuffer& vertices, const CoordBuffer& coords);

    /**
     * Return the processors name.
     */
    virtual const char* getProcName() { 
        return "MeshShaderProc";
    }

    /**
     * Get the fragment shader source.
     */
    virtual const char* getFragmentShaderSource() {
        return fshaderMeshSrc;
    }
    
    /**
     * Get the vertex shader source.
     */
    virtual const char* getVertexShaderSource() {
        return vshaderMeshSrc;
    }

    /**
     * Set the mesh vertex coordinates and texture coordinates.
     */
    void setMesh(const VertexBuffer& verticesIn, const CoordBuffer& coordsIn);

    /**
     * Set the the OpenGL model view projection.
     */
    void setModelViewProjection(const Mat44f &mvp);
    
    /**
     * Get uniform values from shaders.
     */
    void getUniforms();
    
    /**
     * Set uniform values from shaders.
     */
    void setUniforms();
    
    /**
     * Set triangle kind: GL_TRIANGLES, GL_TRIANGLE_STRIP
     */
    void setTriangleKind(GLenum kind);
    
    /**
     * Get triangle kind: GL_TRIANGLES, GL_TRIANGLE_STRIP
     */
    GLenum getTriangleKind() const;

protected:

    virtual void filterRenderSetCoords();
    virtual void filterRenderDraw();

    std::shared_ptr<Shader> shader;

    static const char* vshaderMeshSrc;
    static const char* fshaderMeshSrc;

    GLuint texUnit;
    GLuint texTarget;

    GLint shParamAPos;
    GLint shParamATexCoord;
    GLint shParamUInputTex;
    GLint shParamUMVP;

    VertexBuffer vertices;
    CoordBuffer coords;

    GLenum triangleKind = GL_TRIANGLE_STRIP;
    Mat44f MVP;
};

END_OGLES_GPGPU

#endif // __drishti_graphics_MeshShader_h__
