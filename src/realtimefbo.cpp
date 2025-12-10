#include "realtime.h"
#include "iostream"

void Realtime::makeFBO() {

    // Delete old textures if they exist
    if (m_fbo_texture != 0) glDeleteTextures(1, &m_fbo_texture);
    if (m_fbo_bloom_bright != 0) glDeleteTextures(1, &m_fbo_bloom_bright);
    if (m_fbo_depth != 0) glDeleteTextures(1, &m_fbo_depth);
    if (m_fbo != 0) glDeleteFramebuffers(1, &m_fbo);

    // Main scene color texture (attachment 0)
    glGenTextures(1, &m_fbo_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_fbo_width, m_fbo_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Bright colors texture for bloom (attachment 1)
    glGenTextures(1, &m_fbo_bloom_bright);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fbo_bloom_bright);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_fbo_width, m_fbo_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // depth texture
    glGenTextures(1, &m_fbo_depth);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fbo_depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, m_fbo_width, m_fbo_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // FBO
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // add texture and depth
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbo_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_fbo_bloom_bright, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_fbo_depth, 0);

    // Tell OpenGL which color attachments we'll use
    GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    // Sanity Check: Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer not complete!" << std::endl;
    }

    // unbind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
}

void Realtime::initializeFBO() {
    m_fbo_width = m_screen_width;
    m_fbo_height = m_screen_height;

    // fullscreen quad
    std::vector<GLfloat> fullscreen_quad_data =
        { //     POSITIONS    //
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f
        };

    // Generate and bind a VBO and a VAO for a fullscreen quad
    glGenBuffers(1, &m_fullscreen_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_fullscreen_vbo);
    glBufferData(GL_ARRAY_BUFFER, fullscreen_quad_data.size()*sizeof(GLfloat), fullscreen_quad_data.data(), GL_STATIC_DRAW);
    glGenVertexArrays(1, &m_fullscreen_vao);
    glBindVertexArray(m_fullscreen_vao);

    // attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));

    // Unbind the fullscreen quad's VBO and VAO and shader
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // create FBO
    makeFBO();
}


void Realtime::paintTexture(GLuint colorTexture, GLuint depthTexture) {
    glUseProgram(m_texture_shader);
    glBindVertexArray(m_fullscreen_vao);

    // bind color texture to slot 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glUniform1i(glGetUniformLocation(m_texture_shader, "colorTexture"), 0);

    // bind depth texture to slot 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(glGetUniformLocation(m_texture_shader, "depthTexture"), 1);

    // draw fullscreen quad
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glUseProgram(0);
}

// Create two horizontal/vertical blur passes
void Realtime::initializeBloomFBOs() {
    // Create ping-pong FBOs for blurring
    glGenFramebuffers(2, m_fbo_pingpong);
    glGenTextures(2, m_fbo_pingpong_texture);

    for (int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_pingpong[i]);

        glBindTexture(GL_TEXTURE_2D, m_fbo_pingpong_texture[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_fbo_width, m_fbo_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbo_pingpong_texture[i], 0);
    }

    // glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

}

void Realtime::renderBloom() {
    // Step 1: Extract bright colors to m_fbo_bloom_bright
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    glViewport(0, 0, m_fbo_width, m_fbo_height);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(m_bloom_extract_shader);
    glBindVertexArray(m_fullscreen_vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
    glUniform1i(glGetUniformLocation(m_bloom_extract_shader, "sceneTexture"), 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);

    // Blur the bright colors texture using ping-pong
    bool horizontal = true;
    bool first_iteration = true;
    int amount = 10; // Number of gaussian blur passes (more = stronger blur)

    glUseProgram(m_blur_shader);
    glBindVertexArray(m_fullscreen_vao);

    for (int i = 0; i < amount; i++) {
        // Bind the appropriate FBO
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_pingpong[horizontal]);

        // Set horizontal uniform
        glUniform1i(glGetUniformLocation(m_blur_shader, "horizontal"), horizontal ? 1 : 0);

        // Bind the source texture
        glActiveTexture(GL_TEXTURE0);
        if (first_iteration) {
            // First pass: blur the bright colors texture
            glBindTexture(GL_TEXTURE_2D, m_fbo_bloom_bright);
        } else {
            // Subsequent passes: blur the ping-pong texture
            glBindTexture(GL_TEXTURE_2D, m_fbo_pingpong_texture[!horizontal]);
        }
        glUniform1i(glGetUniformLocation(m_blur_shader, "image"), 0);

        // Render fullscreen quad
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Switch direction for next pass
        horizontal = !horizontal;
        if (first_iteration) first_iteration = false;
    }

    // Composite scene + bloom
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    // Set viewport for default framebuffer
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    glUseProgram(m_bloom_composite_shader);

    // Bind scene texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
    glUniform1i(glGetUniformLocation(m_bloom_composite_shader, "sceneTexture"), 0);

    // Bind blurred bloom texture (from last ping-pong pass)
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_fbo_pingpong_texture[!horizontal]);
    glUniform1i(glGetUniformLocation(m_bloom_composite_shader, "bloomTexture"), 1);

    // Render final result
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glUseProgram(0);
}
