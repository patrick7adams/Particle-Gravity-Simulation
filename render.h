#ifndef RENDER_C
#define RENDER_C

#include "glad.h"
#include "glfw3.h"

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

const float pi = 3.14159265f;

void error_callback(int error, const char* description);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void checkError();
void draw(unsigned int program, unsigned int VAO, int num_circles);
GLFWwindow* init();
unsigned int programInit();
void circleInit(float* data, unsigned int* indices, int index, float center_x, float center_y, float radius);
void dataInit(unsigned int* VAO, int num_circles, float* center_x, float* center_y, float* radii);
int render(GLFWwindow* window, unsigned int* VAO, unsigned int program, int num_circles, float* center_x, float* center_y, float* radii);
#endif