#include "realtime.h"
#include "settings.h"
#include <iostream>
#include <string>

// Helper to pass light and camera info to the shader for phong illumination
void Realtime::shader(const RenderShapeData& shape, const std::vector<SceneLightData>& lights) {
    SceneGlobalData global = m_metaData.globalData;
    SceneMaterial material = shape.primitive.material;
    glm::vec3 Ka = global.ka * glm::vec3(material.cAmbient);
    glm::vec3 Kd = global.kd * glm::vec3(material.cDiffuse);
    glm::vec3 Ks = global.ks * glm::vec3(material.cSpecular);
    glUniform3fv(glGetUniformLocation(m_shader, "ka"), 1, &Ka[0]);
    glUniform3fv(glGetUniformLocation(m_shader, "kd"), 1, &Kd[0]);
    glUniform3fv(glGetUniformLocation(m_shader, "ks"), 1, &Ks[0]);
    glUniform1f(glGetUniformLocation(m_shader, "shininess"), material.shininess);

    glm::vec3 cameraPos = glm::vec3(glm::inverse(m_view) * glm::vec4(0,0,0,1));
    glUniform3fv(glGetUniformLocation(m_shader, "cameraPos"), 1, &cameraPos[0]);

    // iterate through lights
    for (int i = 0; i < numLights; i++) {
        const SceneLightData& light = lights[i];

        // 0 = point, 1 = directional, 2 = spot
        glUniform1i(glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "].type").c_str()),
                    int(light.type));
        // light color
        glm::vec3 color = glm::vec3(light.color);
        glUniform3fv(glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "].color").c_str()),
                     1, &color[0]);
        //light position and direction
        glm::vec3 pos = glm::vec3(light.pos);
        glm::vec3 dir = glm::normalize(glm::vec3(light.dir));
        glUniform3fv(glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "].pos").c_str()),
                     1, &pos[0]);
        glUniform3fv(glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "].dir").c_str()),
                     1, &dir[0]);
        // attenuation function
        glm::vec3 function = glm::vec3(light.function);
        glUniform3fv(glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "].function").c_str()),
                     1, &function[0]);
        // spotlight cone angles
        glUniform1f(glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "].angle").c_str()),
                    light.angle);
        glUniform1f(glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "].penumbra").c_str()),
                    light.penumbra);
    }
}

void Realtime::paintGeometry() {
    // bind the shader
    glUseProgram(m_shader);

    // build the camera
    m_metaData.cameraData.pos = glm::vec4(m_camPos, 1.f);
    m_metaData.cameraData.look = glm::vec4(m_camLook, 1.f);
    m_metaData.cameraData.up   = glm::vec4(m_camUp, 0.f);
    Camera cam(m_metaData.cameraData, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);
    m_view = cam.getViewMatrix();
    m_proj = cam.getProjectionMatrix(settings.nearPlane, settings.farPlane);

    // pass view and projection matrices to the shader
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "view"), 1, GL_FALSE, &m_view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "proj"), 1, GL_FALSE, &m_proj[0][0]);

    //pass shadow size and soft shadow bool
    glUniform1i(glGetUniformLocation(m_shader, "shadowSize"), m_shadow_size);
    glUniform1i(glGetUniformLocation(m_shader, "softShadows"), settings.softShadows);

    if (numLights > 0) {
        glUniform1i(glGetUniformLocation(m_shader, "numLights"), numLights);
        for (int i = 0; i < numLights; i++) {
            // pass light MVPs uniform array
            glUniformMatrix4fv(glGetUniformLocation(m_shader, ("lightMVPs[" + std::to_string(i) + "]").c_str()),
                               1, GL_FALSE, &m_light_MVPs[i][0][0]);
            // bind and pass shadow maps
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, m_shadow_maps[i]);
            glUniform1i(glGetUniformLocation(m_shader, ("shadowMaps[" + std::to_string(i) + "]").c_str()), i);
        }
    }

    // pass height map and bump scale for bump mapping
    glActiveTexture(GL_TEXTURE0 + numLights);
    glBindTexture(GL_TEXTURE_2D, m_height_map);
    glUniform1i(glGetUniformLocation(m_shader, "heightMap"), numLights);
    float bumpScale = 2.f;//0.05f;
    glUniform1f(glGetUniformLocation(m_shader, "bumpScale"), bumpScale);

    // iterate through the scene primitives
    for (const RenderShapeData& shape : m_metaData.shapes) {
        // pass CTM and inverse transpose CTM to the shader
        m_model = shape.ctm;
        m_normalModel = shape.nrm;
        glUniformMatrix4fv(glGetUniformLocation(m_shader, "model"), 1, GL_FALSE, &m_model[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(m_shader, "normalModel"), 1, GL_FALSE, &m_normalModel[0][0]);

        // get primitive mesh
        PrimitiveMeshGL& mesh = getMesh(shape.primitive);

        // shader
        shader(shape, m_metaData.lights);

        // bind the VAO
        glBindVertexArray(mesh.vao);

        // Draw Command
        glDrawArrays(GL_TRIANGLES, 0, mesh.count);
    }

    // unbind the VAO
    glBindVertexArray(0);
    // unbind the shader
    glUseProgram(0);
}

void Realtime::loadHeightMap2D(const std::string &filename) {
    QImage img(QString::fromStdString(filename));
    if (img.isNull()) {
        std::cerr << "Failed to load heightmap: " << filename << std::endl;
        // return 0;
    } else {
        std::cout << "loaded heightmap" << std::endl;
    }
    m_image = img.convertToFormat(QImage::Format_RGBA8888).mirrored();

    glGenTextures(1, &m_height_map);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_height_map);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_image.width(), m_image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, m_image.bits());
    glGenerateMipmap(GL_TEXTURE_2D);

    // Texture filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Edge wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

