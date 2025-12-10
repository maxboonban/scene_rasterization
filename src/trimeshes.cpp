#include "shapes/Cube.h"
#include "shapes/Sphere.h"
#include "shapes/Cone.h"
#include "shapes/Cylinder.h"
#include "settings.h"
#include "realtime.h"

#include "utils/tiny_obj_loader.h"

// ============================================================================
// buildVertices (unchanged except for PRIMITIVE_MESH handled in getMesh)
// ============================================================================
std::vector<float> Realtime::buildVertices(const ScenePrimitive& primitive) const {
    int p1 = std::max(1, settings.shapeParameter1);
    int p2 = std::max(3, settings.shapeParameter2);

    switch (primitive.type) {

    case PrimitiveType::PRIMITIVE_CUBE: {
        Cube cube;
        cube.updateParams(p1);
        return cube.generateShape();           // 6 floats/vertex
    }

    case PrimitiveType::PRIMITIVE_SPHERE: {
        Sphere sphere;
        p1 = std::max(2, p1);
        sphere.updateParams(p1, p2);
        return sphere.generateShape();         // 6 floats/vertex
    }

    case PrimitiveType::PRIMITIVE_CONE: {
        Cone cone;
        cone.updateParams(p1, p2);
        return cone.generateShape();           // 6 floats/vertex
    }

    case PrimitiveType::PRIMITIVE_CYLINDER: {
        Cylinder cyl;
        cyl.updateParams(p1, p2);
        return cyl.generateShape();            // 6 floats/vertex
    }

    default:
        return {};
    }
}

// ============================================================================
// loadOBJ — returns interleaved 8-float vertices: pos(3), normal(3), uv(2)
// ============================================================================
std::vector<float> Realtime::loadOBJ(const std::string& filename) {
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;

    // Attach directory for MTL search
    {
        size_t slash = filename.find_last_of("/\\");
        if (slash != std::string::npos)
            config.mtl_search_path = filename.substr(0, slash + 1);
    }

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(filename, config)) {
        if (!reader.Error().empty())
            std::cerr << "TinyObjReader ERROR: " << reader.Error() << std::endl;
        return {};
    }
    if (!reader.Warning().empty())
        std::cerr << "TinyObjReader WARNING: " << reader.Warning() << std::endl;

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();

    std::vector<float> data;
    data.reserve(10000); // grows dynamically anyway

    for (const auto& shape : shapes) {
        size_t idx_offset = 0;

        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            unsigned char fv = shape.mesh.num_face_vertices[f]; // should be 3

            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[idx_offset + v];

                // ---- Position ----
                glm::vec3 pos(0);
                if (idx.vertex_index >= 0) {
                    int vi = 3 * idx.vertex_index;
                    pos.x = attrib.vertices[vi + 0];
                    pos.y = attrib.vertices[vi + 1];
                    pos.z = attrib.vertices[vi + 2];
                }

                // ---- Normal ----
                glm::vec3 nor(0,1,0);
                if (idx.normal_index >= 0) {
                    int ni = 3 * idx.normal_index;
                    nor.x = attrib.normals[ni + 0];
                    nor.y = attrib.normals[ni + 1];
                    nor.z = attrib.normals[ni + 2];
                }

                // ---- UV ----
                glm::vec2 uv(0);
                if (idx.texcoord_index >= 0) {
                    int ti = 2 * idx.texcoord_index;
                    uv.x = attrib.texcoords[ti + 0];
                    uv.y = attrib.texcoords[ti + 1];
                }

                // position
                data.push_back(pos.x);
                data.push_back(pos.y);
                data.push_back(pos.z);

                // normal
                data.push_back(nor.x);
                data.push_back(nor.y);
                data.push_back(nor.z);

                // uv
                data.push_back(uv.x);
                data.push_back(uv.y);
            }

            idx_offset += fv;
        }
    }

    return data;
}

// ============================================================================
// getMesh — automatically detects 6-float vs 8-float formats
// ============================================================================
PrimitiveMeshGL& Realtime::getMesh(const ScenePrimitive& primitive) {

    int p1 = settings.shapeParameter1,
        p2 = settings.shapeParameter2;

    if (primitive.type == PrimitiveType::PRIMITIVE_CUBE)
        p2 = 0;

    PrimitiveMeshKey key{ primitive.type, p1, p2 };

    if (!m_meshes.count(key)) {

        std::vector<float> data;

        bool isOBJ = (primitive.type == PrimitiveType::PRIMITIVE_MESH);

        if (isOBJ)
            data = loadOBJ(primitive.meshfile);  // returns 8 floats per vertex
        else
            data = buildVertices(primitive);     // returns 6 floats per vertex

        PrimitiveMeshGL mesh{};

        // --- VBO ---
        glGenBuffers(1, &mesh.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     data.size() * sizeof(GLfloat),
                     data.data(),
                     GL_STATIC_DRAW);

        // --- VAO ---
        glGenVertexArrays(1, &mesh.vao);
        glBindVertexArray(mesh.vao);

        if (isOBJ) {
            // ---- OBJ format: 8 floats ----
            glEnableVertexAttribArray(0); // position
            glEnableVertexAttribArray(1); // normal
            glEnableVertexAttribArray(2); // uv

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));

            mesh.count = static_cast<GLsizei>(data.size() / 8);

        } else {
            // ---- Primitive shapes: 6 floats ----
            glEnableVertexAttribArray(0); // position
            glEnableVertexAttribArray(1); // normal

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));

            mesh.count = static_cast<GLsizei>(data.size() / 6);
        }

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        m_meshes.emplace(key, mesh);
    }

    return m_meshes.at(key);
}
