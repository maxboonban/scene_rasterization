#include "realtime.h"
#include "settings.h"

void Realtime::initializeShadowFBO() {
   // create FBO
    glGenFramebuffers(1, &m_fbo_shadow);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_shadow);

    // no color buffer
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
}

void Realtime::initializeShadowDepths() {
    // delete existing shadow textures
    if (!m_shadow_maps.empty()) {
        glDeleteTextures(m_shadow_maps.size(), m_shadow_maps.data());
        m_shadow_maps.clear();
    }

    // Keep track of the number of lights
    numLights = m_metaData.lights.size();
    numLights = (int)std::min((float)numLights, 8.f);

    m_shadow_maps.resize(numLights);

    // depth textures for each directional/spot light
    glGenTextures(numLights, m_shadow_maps.data());
    for (int i = 0; i < numLights; i++) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_shadow_maps[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, m_shadow_size, m_shadow_size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // add texture
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_shadow);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_shadow_maps[i], 0);
        glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
    }
    // unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Realtime::computeLightMVPs() {
    if (!m_light_MVPs.empty()) {
        m_light_MVPs.clear();
    }

    std::vector<SceneLightData> &lights = m_metaData.lights;
    m_light_MVPs.resize(numLights);

    glm::mat4 depthProjectionMatrix;
    glm::mat4 depthViewMatrix;
    glm::mat4 depthModelMatrix = glm::mat4(1.0);;
    glm::vec3 lightInvDir;
    glm::vec3 lightPos;
    for (int i = 0; i < numLights; i++) {
        SceneLightData light = lights[i];
        switch (light.type) {
        case LightType::LIGHT_DIRECTIONAL:
            lightInvDir = -glm::vec3(light.dir);
            // depthProjectionMatrix = glm::ortho<float>(-10.f,10.f,-10.f,10.f,settings.nearPlane,settings.farPlane);
            depthProjectionMatrix = glm::ortho<float>(-30.f,30.f,-30.f,30.f,settings.nearPlane,settings.farPlane);
            if (fabs(glm::dot(glm::normalize(lightInvDir), glm::vec3(0,1,0))) > 0.99f) {
                depthViewMatrix = glm::lookAt(lightInvDir, glm::vec3(0,0,0), glm::vec3(0,0,1));
            } else {
                depthViewMatrix = glm::lookAt(lightInvDir, glm::vec3(0,0,0), glm::vec3(0,1,0));
            }
            break;
        case LightType::LIGHT_SPOT:
            lightPos = glm::vec3(light.pos);
            lightInvDir = -glm::vec3(light.dir);
            depthProjectionMatrix = glm::perspective<float>(glm::radians(90.0f), 1.0f, settings.nearPlane, settings.farPlane);
            if (fabs(glm::dot(glm::normalize(lightInvDir), glm::vec3(0,1,0))) > 0.99f) {
                depthViewMatrix = glm::lookAt(lightPos, lightPos-lightInvDir, glm::vec3(0,0,1));
            } else {
                depthViewMatrix = glm::lookAt(lightPos, lightPos-lightInvDir, glm::vec3(0,1,0));
            }
            break;
        default:
            break;
        }
        glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
        m_light_MVPs[i] = depthMVP;
    }
}

void Realtime::paintShadows() {
    if (numLights > 0) {
        glCullFace(GL_FRONT);
        // iterate through lights
        for (int i = 0; i < numLights; i++) {
            glUseProgram(m_shadow_shader);
            glEnable(GL_DEPTH_TEST);
            glViewport(0, 0, m_shadow_size, m_shadow_size);
            glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_shadow);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadow_maps[i], 0);
            glClear(GL_DEPTH_BUFFER_BIT);
            glUniformMatrix4fv(glGetUniformLocation(m_shadow_shader, "lightMVP"), 1, GL_FALSE, &m_light_MVPs[i][0][0]);

            // Draw scene with shadow depth shader
            for (const RenderShapeData &shape : m_metaData.shapes) {
                glm::mat4 model = shape.ctm;
                glUniformMatrix4fv(glGetUniformLocation(m_shadow_shader, "model"), 1, GL_FALSE, &model[0][0]);

                PrimitiveMeshGL &mesh = getMesh(shape.primitive);
                glBindVertexArray(mesh.vao);
                glDrawArrays(GL_TRIANGLES, 0, mesh.count);
            }
        }
        // unbind
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
        glUseProgram(0);
        glCullFace(GL_BACK);
    }
}
