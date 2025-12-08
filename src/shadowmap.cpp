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

// ================================================================
// Manual PERSPECTIVE matrix
// fovy is in radians
// ================================================================
glm::mat4 manualPerspective(float fovy, float aspect, float near, float far)
{
    float t = tan(fovy * 0.5f);

    glm::mat4 M(0.0f);
    M[0][0] = 1.0f / (aspect * t);
    M[1][1] = 1.0f / t;
    M[2][2] = -(far + near) / (far - near);
    M[2][3] = -1.0f;
    M[3][2] = -(2.0f * far * near) / (far - near);
    return M;
}

// ================================================================
// Manual LOOKAT matrix
// ================================================================
glm::mat4 manualLookAt(glm::vec3 eye, glm::vec3 center, glm::vec3 up)
{
    glm::vec3 z = glm::normalize(eye - center);   // forward
    glm::vec3 x = glm::normalize(glm::cross(up, z)); // right
    glm::vec3 y = glm::cross(z, x);               // true up

    glm::mat4 M(1.0f);
    M[0][0] = x.x;  M[1][0] = x.y;  M[2][0] = x.z;  M[3][0] = -glm::dot(x, eye);
    M[0][1] = y.x;  M[1][1] = y.y;  M[2][1] = y.z;  M[3][1] = -glm::dot(y, eye);
    M[0][2] = z.x;  M[1][2] = z.y;  M[2][2] = z.z;  M[3][2] = -glm::dot(z, eye);

    return M;
}


void Realtime::computeLightMVPs() {
    if (!m_light_MVPs.empty()) {
        m_light_MVPs.clear();
    }

    std::vector<SceneLightData> &lights = m_renderData.lights;
    m_light_MVPs.resize(numLights);

    glm::mat4 depthModelMatrix = glm::mat4(1.0f);

    for (int i = 0; i < numLights; i++) {
        SceneLightData light = lights[i];

        glm::mat4 depthProj;
        glm::mat4 depthView;

        if (light.type == LightType::LIGHT_DIRECTIONAL) {

            glm::vec3 lightDir = -glm::vec3(light.dir);
            glm::vec3 eye = lightDir;   // directional lights use direction as "position"
            glm::vec3 center = glm::vec3(0,0,0);

            glm::vec3 up;
            if (fabs(glm::dot(glm::normalize(lightDir), glm::vec3(0,1,0))) > 0.99f)
                up = glm::vec3(0,0,1);
            else
                up = glm::vec3(0,1,0);

            // manual ortho
            depthProj = manualOrtho(-10.f, 10.f, -10.f, 10.f, settings.nearPlane, settings.farPlane);

            // manual lookAt
            depthView = manualLookAt(eye, center, up);
        }
        else if (light.type == LightType::LIGHT_SPOT) {

            glm::vec3 pos = glm::vec3(light.pos);
            glm::vec3 lightDir = -glm::vec3(light.dir);
            glm::vec3 center = pos - lightDir;

            glm::vec3 up;
            if (fabs(glm::dot(glm::normalize(lightDir), glm::vec3(0,1,0))) > 0.99f)
                up = glm::vec3(0,0,1);
            else
                up = glm::vec3(0,1,0);

            // manual perspective
            depthProj = manualPerspective(glm::radians(90.0f), 1.0f, settings.nearPlane, settings.farPlane);

            // manual lookAt
            depthView = manualLookAt(pos, center, up);
        }
        else {
            // ignore unsupported light types
            m_light_MVPs[i] = glm::mat4(1.0f);
            continue;
        }

        m_light_MVPs[i] = depthProj * depthView * depthModelMatrix;
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
