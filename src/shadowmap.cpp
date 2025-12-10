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
    numLights = m_renderData.lights.size();
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

// ================================================================
// Manual ORTHO matrix
// ================================================================
glm::mat4 manualOrtho(float left, float right,
                      float bottom, float top,
                      float near, float far)
{
    glm::mat4 M(1.0f);

    float rl = right - left;
    float tb = top - bottom;
    float fn = far - near;

    M[0][0] =  2.0f / rl;
    M[1][1] =  2.0f / tb;
    M[2][2] = -2.0f / fn;

    M[3][0] = -(right + left) / rl;
    M[3][1] = -(top + bottom) / tb;
    M[3][2] = -(far + near)  / fn;

    return M;
}

void Realtime::computeLightMVPs() {
    // Re-sync numLights with the actual lights in the scene,
    // in case the scene changed after initializeShadowDepths().
    numLights = std::min<int>((int)m_renderData.lights.size(), 8);

    if (numLights == 0) {
        m_light_MVPs.clear();
        return;
    }

    m_light_MVPs.assign(numLights, glm::mat4(1.0f));

    // Shadow near/far planes (separate from the camera's near/far)
    const float shadowNear = 0.1f;
    const float shadowFar  = 50.0f;

    // Rough "scene radius" for directional light coverage
    const float sceneRadius = 15.0f;
    const glm::vec3 sceneCenter(0.0f);

    std::vector<SceneLightData> &lights = m_renderData.lights;

    for (int i = 0; i < numLights; i++) {
        const SceneLightData &light = lights[i];

        glm::mat4 depthProjectionMatrix(1.0f);
        glm::mat4 depthViewMatrix(1.0f);

        switch (light.type) {
        case LightType::LIGHT_DIRECTIONAL: {
            // Directional light: direction in world space
            glm::vec3 dir = glm::normalize(glm::vec3(light.dir));

            // Position the "shadow camera" back along the light direction
            glm::vec3 eye = sceneCenter - dir * sceneRadius * 2.0f;

            glm::vec3 up(0.0f, 1.0f, 0.0f);
            if (fabs(glm::dot(dir, up)) > 0.99f) {
                up = glm::vec3(0.0f, 0.0f, 1.0f);
            }

            depthViewMatrix = glm::lookAt(eye, sceneCenter, up);

            depthProjectionMatrix = glm::ortho(
                -sceneRadius, sceneRadius,
                -sceneRadius, sceneRadius,
                shadowNear, shadowFar
                );
            break;
        }

        case LightType::LIGHT_SPOT: {
            glm::vec3 pos = glm::vec3(light.pos);
            glm::vec3 dir = glm::normalize(glm::vec3(light.dir));

            // Outer cone ~ angle + penumbra; full FOV = 2 * outer
            float outer = light.angle + light.penumbra;
            float fovy  = 2.0f * outer;
            // clamp to something sane
            fovy = glm::clamp(fovy, glm::radians(5.0f), glm::radians(170.0f));

            depthProjectionMatrix = glm::perspective(
                fovy,           // vertical FOV
                1.0f,           // square shadow map
                shadowNear,
                shadowFar
                );

            glm::vec3 up(0.0f, 1.0f, 0.0f);
            if (fabs(glm::dot(dir, up)) > 0.99f) {
                up = glm::vec3(0.0f, 0.0f, 1.0f);
            }

            depthViewMatrix = glm::lookAt(pos, pos + dir, up);
            break;
        }

        default:
            // We don't create shadow maps for point lights here
            depthProjectionMatrix = glm::mat4(1.0f);
            depthViewMatrix       = glm::mat4(1.0f);
            break;
        }

        // IMPORTANT: this is P * V ONLY (no model).
        m_light_MVPs[i] = depthProjectionMatrix * depthViewMatrix;
    }
}



void Realtime::paintShadows() {
    // No lights or no shadow maps → nothing to do
    if (numLights == 0 || m_shadow_maps.empty() || m_renderData.shapes.empty()) {
        return;
    }

    glUseProgram(m_shadow_shader);

    // Save current viewport/FBO
    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_shadow);
    glViewport(0, 0, m_shadow_size, m_shadow_size);
    glEnable(GL_DEPTH_TEST);

    int nLights = std::min<int>(numLights, static_cast<int>(m_renderData.lights.size()));

    for (int li = 0; li < nLights; li++) {
        // Attach depth texture for this light
        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D,
                               m_shadow_maps[li],
                               0);

        glClear(GL_DEPTH_BUFFER_BIT);

        // Upload the light's MVP (world → light clip)
        GLint locLightMVP = glGetUniformLocation(m_shadow_shader, "lightMVP");
        glUniformMatrix4fv(locLightMVP, 1, GL_FALSE, &m_light_MVPs[li][0][0]);

        // ==== Draw the SAME analytic primitives as paintGeometry() ====
        size_t objIdx = 0;
        for (const RenderShapeData &shape : m_renderData.shapes) {
            PrimitiveType t = shape.primitive.type;
            bool isAnalytic =
                (t == PrimitiveType::PRIMITIVE_CUBE   ||
                 t == PrimitiveType::PRIMITIVE_CONE   ||
                 t == PrimitiveType::PRIMITIVE_SPHERE ||
                 t == PrimitiveType::PRIMITIVE_CYLINDER);

            if (!isAnalytic) {
                continue; // meshes handled elsewhere, if you add them
            }

            if (objIdx >= m_objects.size()) break;
            GLShape &obj = m_objects[objIdx++];

            // SAME model matrix as in paintGeometry()
            glm::mat4 model = obj.model;

            GLint locModel = glGetUniformLocation(m_shadow_shader, "model");
            glUniformMatrix4fv(locModel, 1, GL_FALSE, &model[0][0]);

            glBindVertexArray(obj.vao);
            glDrawArrays(obj.mode, 0, obj.count);
        }
    }

    // Restore state
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
    glViewport(oldViewport[0], oldViewport[1],
               oldViewport[2], oldViewport[3]);
    glUseProgram(0);
}
