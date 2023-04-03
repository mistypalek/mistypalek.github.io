#include "input.h"

namespace input
{
    Camera* camera;
}
using namespace input;

bool pReleased = true;

void processInput(GLFWwindow* window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->move(FORWARD);
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->move(BACKWARD);
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->move(LEFT);
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->move(RIGHT);
    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera->move(UP);
    if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera->move(DOWN);
    
    if(glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && pReleased)
    {
        camera->isPerspective = !camera->isPerspective;
        pReleased = false;
    }
    if(glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE)
        pReleased = true;
}

bool firstMouse = true;
double lastX;
double lastY;

void mousePosCallback(GLFWwindow* window, double x, double y)
{
    if(firstMouse)
    {
        lastX = x;
        lastY = y;
        firstMouse = false;
    }

    glm::vec2 rot(x - lastX, lastY - y);

    lastX = x;
    lastY = y;

    camera->look(rot);
}

void mouseScrollCallback(GLFWwindow* window, double xOff, double yOff)
{
    camera->zoom(yOff);
}

void resizeWindowCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}