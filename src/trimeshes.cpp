#include "shapes/Cube.h"
#include "shapes/Sphere.h"
#include "shapes/Cone.h"
#include "shapes/Cylinder.h"
#include "settings.h"
#include "realtime.h"

// Helper to build vertices for a given primitive
std::vector<float> Realtime::buildVertices(const ScenePrimitive& primitive) const {
    // Parameters 1 and 2 for tessellation levels
    int p1 = std::max(1, settings.shapeParameter1);  //p1 must be at least 1
    int p2 = std::max(3, settings.shapeParameter2);  //p2 must be at least 3

    switch (primitive.type) {
    case PrimitiveType::PRIMITIVE_CUBE: {
        Cube cube;
        cube.updateParams(p1);
        return cube.generateShape();
    }
    case PrimitiveType::PRIMITIVE_SPHERE: {
        Sphere sphere;
        p1 = std::max(2, p1);                        //p1 must be at least 2 for spheres
        sphere.updateParams(p1, p2);
        return sphere.generateShape();
    }
    case PrimitiveType::PRIMITIVE_CONE: {
        Cone cone;
        cone.updateParams(p1, p2);
        return cone.generateShape();
    }
    case PrimitiveType::PRIMITIVE_CYLINDER: {
        Cylinder cylinder;
        cylinder.updateParams(p1, p2);
        return cylinder.generateShape();
    }
    default:
        return {};
    }
}

// Helper to get mesh for the current primitive and tessellation parameters
// Builds vertices if does not already exist
PrimitiveMeshGL& Realtime::getMesh(const ScenePrimitive& primitive) {
    // Parameters 1 and 2 for tessellation levels
    int p1 = settings.shapeParameter1, p2 = settings.shapeParameter2;
    // Param 2 should not affect cube mesh creation
    if (primitive.type == PrimitiveType::PRIMITIVE_CUBE) { p2 = 0; }

    // key
    PrimitiveMeshKey key{primitive.type, p1, p2};

    // Build mesh if it does not exist
    if (!m_meshes.count(key)) {
        // build vertices
        std::vector<float> primitiveData = buildVertices(primitive);
        // create PrimitiveMeshGL for the new data
        PrimitiveMeshGL mesh{};

        // generate and bind VBO
        glGenBuffers(1, &mesh.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
        // pass data to VBO
        glBufferData(GL_ARRAY_BUFFER, primitiveData.size() * sizeof(GLfloat), primitiveData.data(), GL_STATIC_DRAW);

        // generate and bind VAO
        glGenVertexArrays(1, &mesh.vao);
        glBindVertexArray(mesh.vao);
        // VAO attributes
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(0));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));

        // Unbind VBO and VAO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // count
        mesh.count = static_cast<GLsizei>(primitiveData.size()/6);

        // Add mesh to unordered map
        m_meshes.emplace(key, mesh);
    }

    return m_meshes.at(key);
}
