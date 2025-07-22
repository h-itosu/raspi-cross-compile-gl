#include "ShaderUtils.h"
#include <fstream>
#include <sstream>
#include <iostream>

GLuint createShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Shader compile error (" << (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment") << "):\n"
                  << log << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint createProgram(const char *vertexSource, const char *fragmentSource)
{
    GLuint vertexShader = createShader(GL_VERTEX_SHADER, vertexSource);
    if (!vertexShader)
        return 0;

    GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fragmentShader)
    {
        glDeleteShader(vertexShader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        char log[512];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "Program link error:\n"
                  << log << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

GLuint createProgramFromFiles(const char *vertexPath, const char *fragmentPath)
{
    auto readFile = [](const char *path) -> std::string
    {
        std::ifstream file(path);
        if (!file)
        {
            std::cerr << "Failed to open shader file: " << path << std::endl;
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    };

    std::string vs = readFile(vertexPath);
    std::string fs = readFile(fragmentPath);

    if (vs.empty() || fs.empty())
    {
        std::cerr << "Shader source is empty. Vertex: " << vs.empty() << ", Fragment: " << fs.empty() << std::endl;
        return 0;
    }

    return createProgram(vs.c_str(), fs.c_str());
}
