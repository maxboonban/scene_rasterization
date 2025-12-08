#include "sceneparser.h"
#include "scenefilereader.h"
#include <glm/gtx/transform.hpp>

#include <chrono>
#include <iostream>

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
        RenderShapeData shapeData;
        shapeData.ctm = ctm;
        shapeData.primitive = *shape;
        renderData.shapes.push_back(shapeData);
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
