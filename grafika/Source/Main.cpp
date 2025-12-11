
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../Header/Util.h"
#include "../Header/CreateQuad.h"
#include "../Header/MapDetails.h"

#include <cstdio>
#include <thread>
#include <chrono>
#include <string>
#include <cmath>
#include <iostream>


GLuint vaoFull = 0, vaoNDC = 0;
GLuint mapTex = 0, pinTex = 0, infoTex = 0, digitsTex = 0, iconWalkingTex = 0, iconMeasuringTex = 0, dotTex = 0;
GLuint mapShader = 0, texShader = 0, lineShader = 0;

GLint loc_map_uTex = -1, loc_map_camCenter = -1, loc_map_zoom = -1;
GLint loc_tex_uTex = -1, loc_tex_pos = -1, loc_tex_size = -1, loc_tex_alpha = -1;
GLint loc_tex_uvScale = -1, loc_tex_uvOffset = -1;

const double targetFrameTime = 1.0 / 75.0; 


int endProgram(std::string message);

void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW ERROR " << error << ": " << description << std::endl;
}

GLFWwindow* initWindow() {
    
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) return nullptr;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* vmode = monitor ? glfwGetVideoMode(monitor) : nullptr;
    if (vmode) {
        screenWidth = vmode->width;
        screenHeight = vmode->height;
    }
    else {
        screenWidth = 800;
        screenHeight = 800;
    }

    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Map", NULL, NULL);
    if (!window) {
       
        std::cerr << "Window creation with GL 3.1 failed, trying GL 2.1 fallback..." << std::endl;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);

        window = glfwCreateWindow(800, 800, "Map (GL 2.1)", NULL, NULL);
        if (!window) return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    screenWidth = w;
    screenHeight = h;
    glViewport(0, 0, screenWidth, screenHeight);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return nullptr;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* wnd, int newW, int newH) {
        screenWidth = newW; screenHeight = newH;
        glViewport(0, 0, newW, newH);
        });

    return window;
}


void resetUVProps() {
    if (loc_tex_uvScale >= 0) glUniform2f(loc_tex_uvScale, 1.0f, 1.0f);
    if (loc_tex_uvOffset >= 0) glUniform2f(loc_tex_uvOffset, 0.0f, 0.0f);
}

void drawMeasureLines()
{
    if (measurePoints.size() < 2) return;

    glUseProgram(lineShader);
    std::vector<float> verts;
    verts.reserve(measurePoints.size() * 2);

    for (auto& p : measurePoints)
    {
        
        float x = p.x * 2.0f - 1.0f;
        float y = p.y * 2.0f - 1.0f;
        verts.push_back(x);
        verts.push_back(y);
    }

    GLuint vbo = 0, vao = 0;
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glLineWidth(4.0f);
    glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)measurePoints.size());

    
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void drawMeasureDots()
{
    if (dotTex == 0) return;
    glUseProgram(texShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, dotTex);
    if (loc_tex_uTex >= 0) glUniform1i(loc_tex_uTex, 0);

    for (auto& p : measurePoints)
    {
        float x = p.x * 2.0f - 1.0f;
        float y = p.y * 2.0f - 1.0f;

        if (loc_tex_pos >= 0) glUniform2f(loc_tex_pos, x, y);
        if (loc_tex_size >= 0) glUniform2f(loc_tex_size, 0.03f, 0.05f);
        if (loc_tex_alpha >= 0) glUniform1f(loc_tex_alpha, 1.0f);
        resetUVProps();

        glBindVertexArray(vaoNDC);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
}

void drawMap() {
    glUseProgram(mapShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mapTex);
    if (loc_map_uTex >= 0) glUniform1i(loc_map_uTex, 0);

    if (mode == MODE_WALKING) {
        if (loc_map_camCenter >= 0) glUniform2f(loc_map_camCenter, camX, camY);
        if (loc_map_zoom >= 0) glUniform1f(loc_map_zoom, zoomVal);
    }
    else {
        if (loc_map_camCenter >= 0) glUniform2f(loc_map_camCenter, 0.5f, 0.5f);
        if (loc_map_zoom >= 0) glUniform1f(loc_map_zoom, 1.0f);
    }
    glBindVertexArray(vaoFull);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void drawPin() {
    glUseProgram(texShader);
    resetUVProps();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pinTex);
    if (loc_tex_uTex >= 0) glUniform1i(loc_tex_uTex, 0);

    float aspect = (float)screenWidth / (float)screenHeight;
    float pinH = 0.24f;
    float pinW = 0.12f;

    if (loc_tex_size >= 0) glUniform2f(loc_tex_size, pinW * aspect * 0.5f, pinH * 0.5f * aspect * 0.5f);
    if (loc_tex_pos >= 0) glUniform2f(loc_tex_pos, 0.0f, 0.0f);
    if (loc_tex_alpha >= 0) glUniform1f(loc_tex_alpha, 1.0f);

    glBindVertexArray(vaoNDC);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void drawModeIcon(unsigned int modeTex) {
    glUseProgram(texShader);
    resetUVProps();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, modeTex);
    if (loc_tex_uTex >= 0) glUniform1i(loc_tex_uTex, 0);

    float aspect = (float)screenWidth / (float)screenHeight;
    float iconSize = 0.25f;

    float posX = 0.85f;
    float posY = 0.85f;

    if (loc_tex_size >= 0) glUniform2f(loc_tex_size, (iconSize) * 0.5f * aspect * 0.5f, iconSize * 0.5f * aspect * 0.5f);
    if (loc_tex_pos >= 0) glUniform2f(loc_tex_pos, posX, posY);
    if (loc_tex_alpha >= 0) glUniform1f(loc_tex_alpha, 1.0f);

    glBindVertexArray(vaoNDC);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void drawDistanceNumbers() {
    glUseProgram(texShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, digitsTex);
    if (loc_tex_uTex >= 0) glUniform1i(loc_tex_uTex, 0);
    if (loc_tex_alpha >= 0) glUniform1f(loc_tex_alpha, 1.0f);

    int distInt = (mode == MODE_WALKING) ? (int)totalDistance : (int)measureDistance;
    std::string s = std::to_string(distInt);

    float aspect = (float)screenWidth / (float)screenHeight;
    float charHeight = 0.1f;
    float charWidth = charHeight * aspect * 0.5f;

    float startX = -0.9f;
    float startY = 0.8f;

    if (loc_tex_uvScale >= 0) glUniform2f(loc_tex_uvScale, 0.1f, 1.0f);

    for (int i = 0; i < (int)s.length(); ++i) {
        char c = s[i];
        int digit = c - '0';
        if (digit < 0 || digit > 9) continue;

        if (loc_tex_uvOffset >= 0) glUniform2f(loc_tex_uvOffset, (float)digit * 0.1f, 0.0f);
        if (loc_tex_size >= 0) glUniform2f(loc_tex_size, charWidth * 0.4f, charHeight * 0.5f);
        if (loc_tex_pos >= 0) glUniform2f(loc_tex_pos, startX + (i * charWidth), startY);

        glBindVertexArray(vaoNDC);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
}


int main() {
    GLFWwindow* window = initWindow();
    if (!window) return endProgram("Window init failed.");

    
    GLFWcursor* cursor = loadImageToCursor("Resources/compass.png");
    if (cursor) glfwSetCursor(window, cursor);

    
    vaoFull = createFullscreenQuad();
    vaoNDC = createNDCQuad();

    preprocessTexture(mapTex, "Resources/map_angeles.jpg");
    preprocessTexture(pinTex, "Resources/pin.png");
    preprocessTexture(digitsTex, "Resources/digits.png");
    preprocessTexture(iconWalkingTex, "Resources/walk_icon.png");
    preprocessTexture(iconMeasuringTex, "Resources/ruler.png");
    preprocessTexture(dotTex, "Resources/dot.png");

    auto clampTex = [](GLuint tex) {
        if (tex == 0) return;
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        };
    clampTex(mapTex);
    clampTex(pinTex);
    clampTex(infoTex);
    clampTex(digitsTex);
    clampTex(iconWalkingTex);
    clampTex(iconMeasuringTex);
    clampTex(dotTex);

  
    mapShader = createShader("Shaders/map.vert", "Shaders/map.frag");
    texShader = createShader("Shaders/tex.vert", "Shaders/tex.frag");
    lineShader = createShader("Shaders/line.vert", "Shaders/line.frag");

    if (mapShader) {
        loc_map_uTex = glGetUniformLocation(mapShader, "uTexture");
        loc_map_camCenter = glGetUniformLocation(mapShader, "uCamCenter");
        loc_map_zoom = glGetUniformLocation(mapShader, "uZoom");
    }

    if (texShader) {
        loc_tex_uTex = glGetUniformLocation(texShader, "uTexture");
        loc_tex_pos = glGetUniformLocation(texShader, "uPos");
        loc_tex_size = glGetUniformLocation(texShader, "uSize");
        loc_tex_alpha = glGetUniformLocation(texShader, "uAlpha");
        loc_tex_uvScale = glGetUniformLocation(texShader, "uUVScale");
        loc_tex_uvOffset = glGetUniformLocation(texShader, "uUVOffset");
    }

    if (texShader) {
        glUseProgram(texShader);
        if (loc_tex_uvScale >= 0) glUniform2f(loc_tex_uvScale, 1.0f, 1.0f);
        if (loc_tex_uvOffset >= 0) glUniform2f(loc_tex_uvOffset, 0.0f, 0.0f);
        if (loc_tex_alpha >= 0) glUniform1f(loc_tex_alpha, 1.0f);
    }

    
    while (!glfwWindowShouldClose(window)) {
        double frameStart = glfwGetTime();

        processKeyboardInput(window);

        glClearColor(0.12f, 0.14f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glViewport(0, 0, screenWidth, screenHeight);

        drawMap();
        if (mode == MODE_WALKING) {
            drawModeIcon(iconWalkingTex);
            drawPin();
            drawDistanceNumbers();
        }
        else {
            if (mode == MODE_MEASURING) {
                drawModeIcon(iconMeasuringTex);
                drawMeasureLines();
                drawMeasureDots();
                drawDistanceNumbers();
            }
        }

        processMouse(window);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // Frame limiter: sleep a bit to avoid busy spin
        double elapsed = glfwGetTime() - frameStart;
        double toWait = targetFrameTime - elapsed;
        if (toWait > 0.001) {
            std::this_thread::sleep_for(std::chrono::duration<double>(toWait));
        }
    }

    glDeleteVertexArrays(1, &vaoFull);
    glDeleteVertexArrays(1, &vaoNDC);
    if (mapShader) glDeleteProgram(mapShader);
    if (texShader) glDeleteProgram(texShader);
    if (lineShader) glDeleteProgram(lineShader);
    if (cursor) glfwDestroyCursor(cursor);

    if (mapTex) glDeleteTextures(1, &mapTex);
    if (pinTex) glDeleteTextures(1, &pinTex);
    if (infoTex) glDeleteTextures(1, &infoTex);
    if (digitsTex) glDeleteTextures(1, &digitsTex);
    if (iconWalkingTex) glDeleteTextures(1, &iconWalkingTex);
    if (iconMeasuringTex) glDeleteTextures(1, &iconMeasuringTex);
    if (dotTex) glDeleteTextures(1, &dotTex);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
