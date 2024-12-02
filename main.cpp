#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "settings/settings.h"

struct agent {
  glm::vec2 position;
  float angle;
};


const int WIDTH = 1920;
const int HEIGHT = 1080;

const int NUM_AGENTS = 100000;
const int RADIUS = 256;

GLuint createComputeShader(const char *filename);
GLuint createProgram(GLuint computeShader);
GLuint createImageTexture();
GLuint createQuadVAO();
GLuint compileShader(const char *source, GLenum type);
GLuint createShaderProgram(const char *vertexSource,
                           const char *fragmentSource);

std::string loadShaderSource(const char *filename);

int main() {
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW." << std::endl;
    return -1;
  }

  GLFWwindow *window = glfwCreateWindow(
      WIDTH, HEIGHT, "OpenGL Compute Shader Example", NULL, NULL);
  if (!window) {
    std::cerr << "Failed to create GLFW window." << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW." << std::endl;
    return -1;
  }

  std::vector<agent> agents(NUM_AGENTS);
  for (int i = 0; i < NUM_AGENTS; ++i) {
    int r = rand() % RADIUS;
    float t = ((float)rand()) / INT_MAX * 2 * M_PI;
    agents[i].position = glm::vec2((float)WIDTH / 2 + r * cos(t),
                                   (float)HEIGHT / 2 + r * sin(t));
    agents[i].angle = -t*atan((float)HEIGHT/(float)WIDTH);
  }

  GLuint agentBuffer;
  glGenBuffers(1, &agentBuffer);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, agentBuffer);
  glBufferData(GL_SHADER_STORAGE_BUFFER, agents.size() * sizeof(agent),
               agents.data(), GL_STATIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, agentBuffer);

  GLuint outputImage = createImageTexture();

  GLuint agentComputeShader = createComputeShader("shaders/agent_compute_shader.glsl");
  GLuint agentProgram = createProgram(agentComputeShader);

  GLuint decayComputeShader = createComputeShader("shaders/decay_compute_shader.glsl");
  GLuint decayProgram = createProgram(decayComputeShader);

  
  glUseProgram(agentProgram);
  glBindImageTexture(1, outputImage, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
  glUniform1i(glGetUniformLocation(agentProgram, "num_agents"), NUM_AGENTS);
  glUniform1i(glGetUniformLocation(agentProgram, "senseRadius"), senseRadius);
  glUniform1f(glGetUniformLocation(agentProgram, "senseOffset"), senseOffset);
  glUniform1f(glGetUniformLocation(agentProgram, "senseAngle"), senseAngle);
  glUniform1f(glGetUniformLocation(agentProgram, "agentSpeed"), agentSpeed);
  glUniform1f(glGetUniformLocation(agentProgram, "agentRotateFactor"), agentRotateFactor);
  glUniform1f(glGetUniformLocation(agentProgram, "agentDepositAmount"), agentDepositAmount);
  glDispatchCompute((NUM_AGENTS + 255) / 256, 1, 1);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);


  glUseProgram(decayProgram);
  glUniform1f(glGetUniformLocation(decayProgram, "decayAmount"), decayAmount);
  glUniform1f(glGetUniformLocation(decayProgram, "blurAmount"), blurAmount);
  glDispatchCompute((WIDTH + 15) / 16, (HEIGHT + 15) / 16, 1);  
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);  

  
  std::string vertexShaderSource = loadShaderSource("shaders/quad_vertex_shader.glsl");
  std::string fragmentShaderSource =
      loadShaderSource("shaders/quad_fragment_shader.glsl");

  GLuint shaderProgram = createShaderProgram(vertexShaderSource.c_str(),
                                             fragmentShaderSource.c_str());

  
  GLuint quadVAO = createQuadVAO();

  
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(agentProgram);
    glDispatchCompute((NUM_AGENTS + 255) / 256, 1, 1);  
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glUseProgram(decayProgram);
    glDispatchCompute((WIDTH + 15) / 16, (HEIGHT + 15) / 16, 1);  
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);  


    glUseProgram(shaderProgram);
    glBindTexture(GL_TEXTURE_2D, outputImage);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  
  glDeleteProgram(shaderProgram);
  glDeleteBuffers(1, &agentBuffer);
  glDeleteTextures(1, &outputImage);
  glDeleteVertexArrays(1, &quadVAO);
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}


std::string loadShaderSource(const char *filename) {
  std::ifstream file(filename);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

GLuint createComputeShader(const char *filename) {
  std::string shaderCode = loadShaderSource(filename);
  const char *code = shaderCode.c_str();

  GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
  glShaderSource(shader, 1, &code, nullptr);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, nullptr, infoLog);
    std::cerr << "Compute Shader Compilation Failed: " << infoLog << std::endl;
  }

  return shader;
}

GLuint createProgram(GLuint computeShader) {
  GLuint program = glCreateProgram();
  glAttachShader(program, computeShader);
  glLinkProgram(program);

  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(program, 512, nullptr, infoLog);
    std::cerr << "Program Linking Failed: " << infoLog << std::endl;
  }

  return program;
}

GLuint createImageTexture() {
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, HEIGHT, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glBindTexture(GL_TEXTURE_2D, 0);
  return texture;
}

GLuint createQuadVAO() {
  GLfloat quadVertices[] = {
      
      -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, 
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 
      1.0f,  1.0f,  0.0f, 1.0f, 1.0f, 

      1.0f,  1.0f,  0.0f, 1.0f, 1.0f, 
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 
      1.0f,  -1.0f, 0.0f, 1.0f, 0.0f  
  };

  GLuint VAO, VBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                        (GLvoid *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                        (GLvoid *)(3 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
  return VAO;
}

GLuint createShaderProgram(const char *vertexSource,
                           const char *fragmentSource) {
  GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
  GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);

  GLuint program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(program, 512, nullptr, infoLog);
    std::cerr << "Shader Program Linking Failed: " << infoLog << std::endl;
  }

  return program;
}

GLuint compileShader(const char *source, GLenum type) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, nullptr, infoLog);
    std::cerr << "Shader Compilation Failed: " << infoLog << std::endl;
  }

  return shader;
}
