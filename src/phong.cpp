#include "realtime.h"
#include "settings.h"

// ======================================================================
// Lighting + material uniforms helper (Phong)
// ======================================================================

void Realtime::shader(const RenderShapeData& shape,
                      const std::vector<SceneLightData>& lights)
{
    SceneGlobalData global   = m_renderData.globalData;
    SceneMaterial   material = shape.primitive.material;

    glm::vec3 Ka = global.ka * glm::vec3(material.cAmbient);
    glm::vec3 Kd = global.kd * glm::vec3(material.cDiffuse);
    glm::vec3 Ks = global.ks * glm::vec3(material.cSpecular);

    glUniform3fv(glGetUniformLocation(m_shader, "ka"), 1, &Ka[0]);
    glUniform3fv(glGetUniformLocation(m_shader, "kd"), 1, &Kd[0]);
    glUniform3fv(glGetUniformLocation(m_shader, "ks"), 1, &Ks[0]);
    glUniform1f(glGetUniformLocation(m_shader, "shininess"),
                material.shininess);

    // Camera position - keep your working version
    glm::vec3 camPos = glm::vec3(m_camera.pos);
    glUniform3fv(glGetUniformLocation(m_shader, "cameraPos"),
                 1, glm::value_ptr(camPos));

    // Upload lights exactly as before
    int n = std::min<int>(lights.size(), numLights);
    for (int i = 0; i < n; i++) {
        const SceneLightData &light = lights[i];
        std::string base = "lights[" + std::to_string(i) + "].";

        glUniform1i(glGetUniformLocation(m_shader, (base + "type").c_str()),
                    int(light.type));

        glm::vec3 color = glm::vec3(light.color);
        glUniform3fv(glGetUniformLocation(m_shader, (base + "color").c_str()),
                     1, &color[0]);

        glm::vec3 pos = glm::vec3(light.pos);
        glm::vec3 dir = glm::normalize(glm::vec3(light.dir));
        glUniform3fv(glGetUniformLocation(m_shader, (base + "pos").c_str()),
                     1, &pos[0]);
        glUniform3fv(glGetUniformLocation(m_shader, (base + "dir").c_str()),
                     1, &dir[0]);

        glm::vec3 function = glm::vec3(light.function);
        glUniform3fv(glGetUniformLocation(m_shader, (base + "function").c_str()),
                     1, &function[0]);

        glUniform1f(glGetUniformLocation(m_shader, (base + "angle").c_str()),
                    light.angle);
        glUniform1f(glGetUniformLocation(m_shader, (base + "penumbra").c_str()),
                    light.penumbra);
    }
}


// ======================================================================
// paintGeometry  (Phong + optional soft shadows + bump mapping)
// ======================================================================

void Realtime::paintGeometry() {
    glUseProgram(m_shader);

    int fbWidth  = width()  * m_devicePixelRatio;
    int fbHeight = height() * m_devicePixelRatio;
    float aspect = float(fbWidth) / float(fbHeight);

    // Camera transforms
    glm::mat4 view = m_camera.getViewMatrix();
    glm::mat4 proj = m_camera.getProjectionMatrix(
        aspect, settings.nearPlane, settings.farPlane
        );

    m_view = view;
    m_proj = proj;

    glUniformMatrix4fv(glGetUniformLocation(m_shader, "view"),
                       1, GL_FALSE, &m_view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "proj"),
                       1, GL_FALSE, &m_proj[0][0]);

    glm::vec3 camPos = glm::vec3(m_camera.pos);
    glUniform3fv(glGetUniformLocation(m_shader, "cameraPos"),
                 1, glm::value_ptr(camPos));

    // ===============================================================
    // SHADOWS — FIXED TEXTURE UNIT ASSIGNMENT
    // ===============================================================

    bool useDepthShadows = (settings.extraCredit4 != 0);

    glUniform1i(glGetUniformLocation(m_shader, "shadowSize"),
                useDepthShadows ? m_shadow_size : 0);
    glUniform1i(glGetUniformLocation(m_shader, "softShadows"),
                useDepthShadows ? 1 : 0);

    int nLights = std::min<int>(numLights, m_renderData.lights.size());
    glUniform1i(glGetUniformLocation(m_shader, "numLights"), nLights);

    int SHADOW_BASE = 1;  // shadow maps will use texture units 1,2,3,...

    if (useDepthShadows) {
        for (int i = 0; i < nLights; i++) {

            // ---- upload light MVP ----
            std::string mvpName = "lightMVPs[" + std::to_string(i) + "]";
            glUniformMatrix4fv(glGetUniformLocation(m_shader, mvpName.c_str()),
                               1, GL_FALSE, &m_light_MVPs[i][0][0]);

            // ---- bind shadow map texture ----
            int unit = SHADOW_BASE + i; // texture unit 1,2,3,...

            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, m_shadow_maps[i]);

            std::string smName = "shadowMaps[" + std::to_string(i) + "]";
            glUniform1i(glGetUniformLocation(m_shader, smName.c_str()), unit);
        }
    }

    // ===============================================================
    // --- BUMP MAP (height map) — uses unit AFTER shadow maps -------
    // ===============================================================

    if (m_height_map != 0) {
        int BUMP_UNIT = SHADOW_BASE + nLights;  // e.g., if 2 lights → unit 3

        glActiveTexture(GL_TEXTURE0 + BUMP_UNIT);
        glBindTexture(GL_TEXTURE_2D, m_height_map);

        glUniform1i(glGetUniformLocation(m_shader, "heightMap"), BUMP_UNIT);

        float bumpScale = 2.f;
        glUniform1f(glGetUniformLocation(m_shader, "bumpScale"), bumpScale);
    }

    // ===============================================================
    // DRAW ANALYTIC PRIMITIVES (unchanged, but texture-unit–safe)
    // ===============================================================

    size_t objIdx = 0;
    for (const RenderShapeData &shape : m_renderData.shapes) {
        if (objIdx >= m_objects.size()) break;

        GLShape &obj = m_objects[objIdx++];

        m_model       = obj.model;
        m_normalModel = glm::transpose(glm::inverse(obj.model));

        glUniformMatrix4fv(glGetUniformLocation(m_shader, "model"),
                           1, GL_FALSE, &m_model[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(m_shader, "normalModel"),
                           1, GL_FALSE, &m_normalModel[0][0]);

        // ===========================================================
        // DIFFUSE / ALBEDO TEXTURE — ALWAYS TEXTURE UNIT 0
        // ===========================================================

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_mesh_texture);
        glUniform1i(glGetUniformLocation(m_shader, "sampler"), 0);

        if (shape.primitive.material.textureMap.isUsed) {
            glUniform1i(glGetUniformLocation(m_shader, "useTexture"), 1);

            QImage img = shape.primitive.material.textureMap.texture;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                         img.width(), img.height(),
                         0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
        } else {
            glUniform1i(glGetUniformLocation(m_shader, "useTexture"), 0);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // material + lights
        shader(shape, m_renderData.lights);

        // === DRAW ===
        glBindVertexArray(obj.vao);
        glDrawArrays(obj.mode, 0, obj.count);
    }

    glBindVertexArray(0);
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
