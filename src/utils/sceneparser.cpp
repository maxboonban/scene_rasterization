#define TINYOBJLOADER_IMPLEMENTATION

#include "tiny_obj_loader.h"

#include "sceneparser.h"
#include "scenefilereader.h"
#include <glm/gtx/transform.hpp>

#include <chrono>
#include <iostream>

void insertVec3(std::vector<GLfloat> &data, glm::vec3 v)
{
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}

void load_triangles(const tinyobj::shape_t &obj, std::vector<glm::vec3> &vertices, std::vector<glm::vec3> &normals, std::vector<RenderShapeData> &shapes, ScenePrimitive *shape, glm::mat4 ptm)
{
    const std::vector<tinyobj::index_t> &indices = obj.mesh.indices;
    const std::vector<int> &mat_ids = obj.mesh.material_ids;

    std::cout << "Loading " << mat_ids.size() << " triangles..." << std::endl;

    std::vector<GLfloat> tris;
    for (size_t face_ind = 0; face_ind < mat_ids.size(); face_ind++)
    {
        insertVec3(tris, vertices[indices[3 * face_ind].vertex_index]);
        insertVec3(tris, normals[indices[3 * face_ind].normal_index]);
        insertVec3(tris, vertices[indices[3 * face_ind + 1].vertex_index]);
        insertVec3(tris, normals[indices[3 * face_ind + 1].normal_index]);
        insertVec3(tris, vertices[indices[3 * face_ind + 2].vertex_index]);
        insertVec3(tris, normals[indices[3 * face_ind + 2].normal_index]);
    }
    shapes.push_back(RenderShapeData(*shape, ptm, tris));
}


void parseMesh(std::vector<RenderShapeData>& shapes, ScenePrimitive* shape, glm::mat4 ptm) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> objs;
    std::vector<tinyobj::material_t> objmaterials;
    std::string err, warn;

    std::string meshfile_dir = "";

    bool success = tinyobj::LoadObj(&attrib, &objs, &objmaterials, &warn, &err,
        shape->meshfile.c_str(), //model to load
        meshfile_dir.c_str(), //directory to search for materials
        true); //enable triangulation

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;

    for (size_t vec_start = 0; vec_start < attrib.vertices.size(); vec_start += 3) {
        vertices.emplace_back(
            attrib.vertices[vec_start],
            attrib.vertices[vec_start + 1],
            attrib.vertices[vec_start + 2]);
    }

    for (size_t norm_start = 0; norm_start < attrib.normals.size(); norm_start += 3) {
        normals.emplace_back(
            attrib.normals[norm_start],
            attrib.normals[norm_start + 1],
            attrib.normals[norm_start + 2]);
    }

    for(auto obj = objs.begin(); obj < objs.end(); ++obj) {
        load_triangles(*obj, vertices, normals, shapes, shape, ptm);
    }
}

void parseAux(SceneNode* node, glm::mat4 ctm, RenderData &renderData) {
    if (node == nullptr) return;
    for (SceneTransformation* transform : node->transformations) {
        if (transform->type == TransformationType::TRANSFORMATION_TRANSLATE) {
            ctm = ctm * glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, transform->translate[0], transform->translate[1], transform->translate[2], 1);
        } else if (transform->type == TransformationType::TRANSFORMATION_SCALE) {
            ctm = ctm * glm::mat4(transform->scale[0], 0, 0, 0, 0, transform->scale[1], 0, 0, 0, 0, transform->scale[2], 0, 0, 0, 0, 1);
        } else if (transform->type == TransformationType::TRANSFORMATION_ROTATE) {
            glm::vec3 rotationAxis = glm::normalize(transform->rotate);
            glm::mat4 rotationMatrix = glm::mat4(cos(transform->angle) + rotationAxis[0] * rotationAxis[0] * (1 - cos(transform->angle)),
                                                 rotationAxis[1] * rotationAxis[0] * (1 - cos(transform->angle)) + rotationAxis[2] * sin(transform->angle),
                                                 rotationAxis[2] * rotationAxis[0] * (1 - cos(transform->angle)) - rotationAxis[1] * sin(transform->angle),
                                                 0,
                                                 rotationAxis[0] * rotationAxis[1] * (1 - cos(transform->angle)) - rotationAxis[2] * sin(transform->angle),
                                                 cos(transform->angle) + rotationAxis[1] * rotationAxis[1] * (1 - cos(transform->angle)),
                                                 rotationAxis[2] * rotationAxis[1] * (1 - cos(transform->angle)) + rotationAxis[0] * sin(transform->angle),
                                                 0,
                                                 rotationAxis[0] * rotationAxis[2] * (1 - cos(transform->angle)) + rotationAxis[1] * sin(transform->angle),
                                                 rotationAxis[1] * rotationAxis[2] * (1 - cos(transform->angle)) - rotationAxis[0] * sin(transform->angle),
                                                 cos(transform->angle) + rotationAxis[2] * rotationAxis[2] * (1 - cos(transform->angle)),
                                                 0,
                                                 0, 0, 0, 1);
            ctm = ctm * rotationMatrix;
        } else if (transform->type == TransformationType::TRANSFORMATION_MATRIX) ctm = ctm * transform->matrix;
    }

    for (ScenePrimitive* shape : node->primitives) {
        if (shape->type == PrimitiveType::PRIMITIVE_MESH) {
            parseMesh(renderData.shapes, shape, ctm);
        }
        else {
            RenderShapeData shapeData;
            shapeData.ctm = ctm;
            shapeData.primitive = *shape;
            renderData.shapes.push_back(shapeData);
        }
    }

    for (SceneLight* light : node->lights) {
        SceneLightData lightData;
        lightData.type = light->type;
        lightData.color = light->color;
        lightData.function = light->function;
        if (!(light->type == LightType::LIGHT_POINT)) lightData.dir = light->dir;
        if (!(light->type == LightType::LIGHT_DIRECTIONAL)) lightData.pos = ctm * glm::vec4(0, 0, 0, 1);
        if (light->type == LightType::LIGHT_SPOT) {
            lightData.penumbra = light->penumbra;
            lightData.angle = light->angle;
        }
        renderData.lights.push_back(lightData);
    }

    for (SceneNode* child : node->children) parseAux(child, ctm, renderData);
    return;
}

bool SceneParser::parse(std::string filepath, RenderData &renderData) {
    ScenefileReader fileReader = ScenefileReader(filepath);
    bool success = fileReader.readJSON();
    if (!success) {
        return false;
    }

    renderData.cameraData = fileReader.getCameraData();
    renderData.globalData = fileReader.getGlobalData();

    renderData.shapes.clear();
    renderData.lights.clear();
    SceneNode* rootNode = fileReader.getRootNode();
    glm::mat4 ctm = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    parseAux(rootNode, ctm, renderData);

    return true;
}
