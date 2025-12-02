#include "sceneparser.h"
#include "scenefilereader.h"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <chrono>
#include <iostream>

bool SceneParser::parse(std::string filepath, RenderData &renderData) {
    ScenefileReader fileReader = ScenefileReader(filepath);
    bool success = fileReader.readJSON();
    if (!success) {
        return false;
    }

    // TODO: Use your Lab 5 code here
    // Get global data and camera data
    renderData.globalData = fileReader.getGlobalData();
    renderData.cameraData = fileReader.getCameraData();

    // Traverse the scene graph to build primitives and lights lists
    renderData.shapes.clear();
    renderData.lights.clear();
    SceneNode* root = fileReader.getRootNode();
    glm::mat4 ctm = glm::mat4(1.0f);
    dfsSceneGraph(root, ctm, renderData);

    return true;
}

glm::mat4 SceneParser::nodeTransformation(SceneNode* node) {
    glm::mat4 M = glm::mat4(1.0f);
    for (SceneTransformation* transformation : node->transformations) {
        switch (transformation->type) {
        case TransformationType::TRANSFORMATION_TRANSLATE:
            M = glm::translate(M, transformation->translate); break;
        case TransformationType::TRANSFORMATION_ROTATE:
            M = glm::rotate(M, transformation->angle, transformation->rotate); break;
        case TransformationType::TRANSFORMATION_SCALE:
            M = glm::scale(M, transformation->scale); break;
        case TransformationType::TRANSFORMATION_MATRIX:
            M = M * transformation->matrix; break;
        }
    }
    return M;
}

void SceneParser::dfsSceneGraph(SceneNode* node, glm::mat4 &parentCTM, RenderData &renderData) {
    glm::mat4 ctm = parentCTM * nodeTransformation(node);

    // Primitives
    for (ScenePrimitive* primitive : node->primitives) {
        RenderShapeData shapeData;
        shapeData.primitive = *primitive;
        shapeData.ctm = ctm;
        shapeData.inv = glm::inverse(ctm);
        shapeData.nrm = glm::transpose(shapeData.inv);
        renderData.shapes.push_back(shapeData);
    }

    // Lights
    for (SceneLight* light : node->lights) {
        SceneLightData lightData;
        lightData.id = light->id;
        lightData.type = light->type;
        lightData.color = light->color;
        lightData.function = light->function;
        lightData.penumbra = light->penumbra;
        lightData.angle    = light->angle;
        if (light->type != LightType::LIGHT_DIRECTIONAL) {
            lightData.pos = ctm * glm::vec4(0,0,0,1);
        }
        if (light->type != LightType::LIGHT_POINT) {
            lightData.dir = ctm * light->dir;
        }
        renderData.lights.push_back(lightData);
    }

    // Recursively iterate through children
    for (SceneNode* child : node->children) {
        dfsSceneGraph(child, ctm, renderData);
    }
}
