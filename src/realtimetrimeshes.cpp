#include "shapes/Cube.h"
#include "shapes/Sphere.h"
#include "shapes/Cone.h"
#include "shapes/Cylinder.h"
#include "settings.h"
#include "realtime.h"
#include <fstream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "utils/tiny_obj_loader.h"

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
        std::vector<float> primitiveData;
        if (primitive.type == PrimitiveType::PRIMITIVE_MESH) {
            primitiveData = loadOBJ(primitive.meshfile);
        } else {
            primitiveData = buildVertices(primitive);
        }
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
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(0));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(6 * sizeof(GLfloat)));

        // Unbind VBO and VAO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // count
        mesh.count = static_cast<GLsizei>(primitiveData.size()/8);

        // Add mesh to unordered map
        m_meshes.emplace(key, mesh);
    }

    return m_meshes.at(key);
}

std::vector<float> Realtime::loadOBJ(const std::string& filename) {
    tinyobj::ObjReaderConfig config;
    config.triangulate = true; // only get triangles

    // set MTL search path to directory of OBJ
    std::string base_dir;
    {
        auto slash_pos = filename.find_last_of("/\\");
        if (slash_pos != std::string::npos) {
            base_dir = filename.substr(0, slash_pos + 1);
            config.mtl_search_path = base_dir;
        }
    }

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(filename, config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error() << std::endl;
        }
        return {};
    }

    if (!reader.Warning().empty()) {
        std::cerr << "TinyObjReader warning: " << reader.Warning() << std::endl;
    }

    const tinyobj::attrib_t& attrib = reader.GetAttrib();
    const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();
    // const std::vector<tinyobj::material_t>& materials = reader.GetMaterials();

    std::vector<float> data;
    data.reserve(shapes.size() * 3 * 8); // rough guess; will grow as needed

    // Iterate over all shapes and faces
    for (const auto& shape : shapes) {
        size_t index_offset = 0;

        // Each face is a triangle
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            unsigned char fv = shape.mesh.num_face_vertices[f];
            // Should be 3 if triangulated
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                // ---- Position ----
                glm::vec3 pos(0.0f);
                if (idx.vertex_index >= 0) {
                    int vi = 3 * idx.vertex_index;
                    pos.x = attrib.vertices[vi + 0];
                    pos.y = attrib.vertices[vi + 1];
                    pos.z = attrib.vertices[vi + 2];
                }

                // ---- Normal ----
                glm::vec3 nor(0.0f, 1.0f, 0.0f); // default up if missing
                if (idx.normal_index >= 0) {
                    int ni = 3 * idx.normal_index;
                    nor.x = attrib.normals[ni + 0];
                    nor.y = attrib.normals[ni + 1];
                    nor.z = attrib.normals[ni + 2];
                }

                // ---- UV ----
                glm::vec2 uv(0.0f);
                if (idx.texcoord_index >= 0) {
                    int ti = 2 * idx.texcoord_index;
                    uv.x = attrib.texcoords[ti + 0];
                    uv.y = attrib.texcoords[ti + 1];
                }

                // Interleaved: pos(3), normal(3), uv(2) = 8 floats
                data.push_back(pos.x);
                data.push_back(pos.y);
                data.push_back(pos.z);

                data.push_back(nor.x);
                data.push_back(nor.y);
                data.push_back(nor.z);

                data.push_back(uv.x);
                data.push_back(uv.y);
            }

            index_offset += fv;
        }
    }

    return data;
}

// std::vector<float> Realtime::loadOBJ(const std::string& filename) {
//     std::ifstream file(filename);
//     if (!file.is_open()) {
//         std::cerr << "Could not load OBJ: " << filename << std::endl;
//         return {};
//     }

//     std::vector<glm::vec3> positions;
//     std::vector<glm::vec3> normals;
//     std::vector<glm::vec2> uvs;
//     std::vector<float>      data;
//     std::vector<std::string> materials;
//     std::string material;

//     std::string line;
//     while (std::getline(file, line)) {
//         std::stringstream ss(line);
//         std::string type;
//         ss >> type;

//         if (type == "v") {
//             glm::vec3 p;
//             ss >> p.x >> p.y >> p.z;
//             positions.push_back(p);
//         }
//         else if (type == "vn") {
//             glm::vec3 n;
//             ss >> n.x >> n.y >> n.z;
//             normals.push_back(n);
//         }
//         else if (type == "vt") {
//             glm::vec2 uv;
//             ss >> uv.x >> uv.y;
//             uvs.push_back(uv);
//         }
//         else if (type == "usemtl") {
//             ss >> material;
//         }
//         else if (type == "f") {
//             // We assume triangles (Blender will triangulate for you)
//             for (int i = 0; i < 3; ++i) {
//                 std::string vert;
//                 ss >> vert;
//                 if (vert.empty()) continue;

//                 int pIndex = 0;
//                 int tIndex = 0;
//                 int nIndex = 0;

//                 // Try v/vt/vn
//                 int count = std::sscanf(vert.c_str(), "%d/%d/%d", &pIndex, &tIndex, &nIndex);
//                 if (count != 3) {
//                     // Try v//vn
//                     count = std::sscanf(vert.c_str(), "%d//%d", &pIndex, &nIndex);
//                     if (count == 2) {
//                         tIndex = 0; // no uv
//                     } else {
//                         // Try v/vt
//                         count = std::sscanf(vert.c_str(), "%d/%d", &pIndex, &tIndex);
//                         if (count != 2) {
//                             std::cerr << "Unrecognized face format: " << vert << std::endl;
//                             continue;
//                         }
//                         nIndex = 0; // no normal
//                     }
//                 }

//                 // OBJ indices are 1-based
//                 glm::vec3 pos(0.0f);
//                 glm::vec3 nor(0.0f, 1.0f, 0.0f);
//                 glm::vec2 uv(0.0f);

//                 if (pIndex > 0 && static_cast<size_t>(pIndex) <= positions.size())
//                     pos = positions[pIndex - 1];

//                 if (nIndex > 0 && static_cast<size_t>(nIndex) <= normals.size())
//                     nor = normals[nIndex - 1];

//                 if (tIndex > 0 && static_cast<size_t>(tIndex) <= uvs.size())
//                     uv = uvs[tIndex - 1];

//                 // Push interleaved vertex data: pos(3), normal(3), uv(2)
//                 data.push_back(pos.x);
//                 data.push_back(pos.y);
//                 data.push_back(pos.z);

//                 data.push_back(nor.x);
//                 data.push_back(nor.y);
//                 data.push_back(nor.z);

//                 data.push_back(uv.x);
//                 data.push_back(uv.y);

//                 materials.push_back(material);
//             }
//         }
//     }

//     return data;
// }

// std::vector<float> Realtime::loadOBJ(const std::string& filename) {
// // std::map<std::string, std::vector<float>> Realtime::loadOBJ(const std::string& filename) {
//     std::ifstream file(filename);
//     if (!file.is_open()) {
//         std::cerr << "Could not load OBJ: " << filename << std::endl;
//         return {};
//     }

//     std::vector<glm::vec3> positions;
//     std::vector<glm::vec3> normals;
//     std::vector<glm::vec2> uvs;
//     std::vector<float> data;
//     std::vector<std::string> materials;
//     std::string material;


//     std::vector<unsigned int> posIdx, normIdx;

//     std::string line;
//     while (std::getline(file, line)) {
//         std::stringstream ss(line);
//         std::string type;
//         ss >> type;

//         if (type == "v") {
//             glm::vec3 p; ss >> p.x >> p.y >> p.z;
//             positions.push_back(p);
//         } else if (type == "vn") {
//             glm::vec3 n; ss >> n.x >> n.y >> n.z;
//             normals.push_back(n);
//         } else if (type == "vt"){
//             glm::vec2 uv; ss >> uv.x >> uv.y;
//             uvs.push_back(uv);
//         } else if (type == "f") {
//             // format: v//vn
//             for (int i = 0; i < 3; i++) {
//                 std::string vert;
//                 ss >> vert;

//                 int p, n;
//                 sscanf(vert.c_str(), "%d//%d", &p, &n);

//                 glm::vec3 pos = positions[p - 1];
//                 glm::vec3 nor = normals[n - 1];

//                 data.push_back(pos.x);
//                 data.push_back(pos.y);
//                 data.push_back(pos.z);

//                 data.push_back(nor.x);
//                 data.push_back(nor.y);
//                 data.push_back(nor.z);

//                 materials.push_back(material);
//             }
//         } else if (type == std::string("usemtl")) {
//             ss >> material;
//         }
//     }

//     return data;
// }

