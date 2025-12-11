
#include "../Header/Util.h"   

#define _CRT_SECURE_NO_WARNINGS
#include <fstream>
#include <sstream>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "../Header/stb_image.h"



int endProgram(std::string message) {
    std::cout << message << std::endl;
    glfwTerminate();
    return -1;
}


unsigned int compileShader(GLenum type, const char* filePath)
{
    
    std::ifstream file(filePath);
    std::stringstream ss;

    if (file.is_open()) {
        ss << file.rdbuf();
        std::cout << "Successfully read shader: " << filePath << std::endl;
    }
    else {
        std::cout << "ERROR: Failed to read shader at: " << filePath << std::endl;
        return 0;
    }

    std::string src = ss.str();
    const char* sourceCode = src.c_str();

    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &sourceCode, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);

        std::cout
            << ((type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT")
            << " SHADER COMPILATION ERROR:\n" << infoLog << std::endl;

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}


unsigned int createShader(const char* vsPath, const char* fsPath)
{
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vsPath);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fsPath);

    if (!vs || !fs) {
        std::cout << "Shader creation failed.\n";
        return 0;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);

    glLinkProgram(program);

    
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cout << "PROGRAM LINK ERROR:\n" << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

// -----------------------------------------------------
// Load image into OpenGL texture
// -----------------------------------------------------
unsigned loadImageToTexture(const char* filePath)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);   

    unsigned char* data = stbi_load(filePath, &width, &height, &channels, STBI_rgb_alpha);

    if (!data) {
        std::cout << "ERROR: Failed to load texture: " << filePath << std::endl;
        return 0;
    }

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, data);

    glGenerateMipmap(GL_TEXTURE_2D);

    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    return texture;
}


GLFWcursor* loadImageToCursor(const char* filePath) {

    int w, h, ch;
    unsigned char* pixels = stbi_load(filePath, &w, &h, &ch, 0);

    if (!pixels) {
        std::cout << "ERROR: Cursor image load failed: " << filePath << std::endl;
        return nullptr;
    }

    GLFWimage img;
    img.width = w;
    img.height = h;
    img.pixels = pixels;

    int hotspotX = w / 5;
    int hotspotY = h / 5;

    GLFWcursor* cursor = glfwCreateCursor(&img, hotspotX, hotspotY);
    stbi_image_free(pixels);

    return cursor;
}


void preprocessTexture(unsigned& texture, const char* filepath)
{
    texture = loadImageToTexture(filepath);

    glBindTexture(GL_TEXTURE_2D, texture);

    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
}
