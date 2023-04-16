#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "camera.h"

namespace input
{
    extern Camera* camera;
    extern bool* spinning;
}

void processInput(GLFWwindow* window);
void mousePosCallback(GLFWwindow* window, double x, double y);
void mouseScrollCallback(GLFWwindow* window, double xOff, double yOff);
void resizeWindowCallback(GLFWwindow* window, int width, int height);