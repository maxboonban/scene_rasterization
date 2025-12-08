#define TINYOBJLOADER_IMPLEMENTATION

#include "tiny_obj_loader.h"

#include "sceneparser.h"
#include "scenefilereader.h"
#include <glm/gtx/transform.hpp>

#include <chrono>
#include <iostream>

bool SceneParser::parse(std::string filepath, RenderData &renderData) {
    ScenefileReader fileReader = ScenefileReader(filepath);
    bool success = fileReader.readJSON();
    if (!success) {
        return false;
    }

    // TODO: Use your Lab 5 code here
    renderData.globalData = fileReader.getGlobalData();
    renderData.cameraData = fileReader.getCameraData();

    sceneDFS(fileReader.getRootNode(), glm::mat4(1.0f), renderData.lights, renderData.shapes);

    return true;
}

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


void SceneParser::parseMesh(std::vector<RenderShapeData>& shapes, ScenePrimitive* shape, glm::mat4 ptm) {
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

void SceneParser::sceneDFS(SceneNode* root, glm::mat4 ptm, std::vector<SceneLightData>& lights, std::vector<RenderShapeData>& shapes) {
    foreach (SceneTransformation* trans, root->transformations) {
        ptm = ptm * parseSceneTransform(trans);
    }
    foreach (ScenePrimitive* shape, root->primitives) {
        if (shape->type == PrimitiveType::PRIMITIVE_MESH) parseMesh(shapes, shape, ptm);
        else shapes.push_back(RenderShapeData(*shape, ptm));
    }
    foreach (SceneLight* light, root->lights) {
        lights.push_back(SceneLightData(light->id, light->type, light->color, light->function, ptm * glm::vec4(0.f, 0.f, 0.f, 1.f), ptm * light->dir, light->penumbra, light->angle, light->width, light->height));
    }
    foreach (SceneNode* child, root->children) {
        sceneDFS(child, ptm, lights, shapes);
    }
}

glm::mat4 SceneParser::parseSceneTransform(SceneTransformation* trans) {
    switch (trans->type) {
    case TransformationType::TRANSFORMATION_TRANSLATE:
        return glm::translate(trans->translate);
    case TransformationType::TRANSFORMATION_SCALE:
        return glm::scale(trans->scale);
    case TransformationType::TRANSFORMATION_ROTATE:
        return glm::rotate(trans->angle, trans->rotate);
    case TransformationType::TRANSFORMATION_MATRIX:
        return trans->matrix;
    }
}
