#include "camera.h"

Camera::Camera(int winWidth, int winHeight, glm::vec3 pos) : WINDOW_WIDTH(winWidth), WINDOW_HEIGHT(winHeight), pos(pos)
{
    isPerspective = true;
    
    rot.x = 90;
    zoomLevel = 45;
    worldUp = glm::vec3(0, 1, 0);

    updateVectors();
}

glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(pos, pos + front, up);
}

glm::mat4 Camera::getProjectionMatrix()
{
    if (isPerspective)
        return glm::perspective(zoomLevel, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
    else
        return glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
}

void Camera::move(CameraMovement direction)
{
    if(direction == FORWARD)
        pos += glm::vec3(front.x, 0, front.z) * 0.1f;
    if(direction == BACKWARD)
        pos -= glm::vec3(front.x, 0, front.z) * 0.1f;
    if(direction == RIGHT)
        pos += right * 0.1f;
    if(direction == LEFT)
        pos -= right * 0.1f;
    if(direction == UP)
        pos += worldUp * 0.1f;
    if(direction == DOWN)
        pos -= worldUp * 0.1f;
}

void Camera::look(glm::vec2 rot)
{
    this->rot += rot * 0.1f;

    if(this->rot.y > 89)
        this->rot.y = 89;
    if(this->rot.y < -89)
        this->rot.y = -89;
    
    updateVectors();
}

void Camera::zoom(float off)
{
    zoomLevel -= off * 0.1f;
    if(zoomLevel < 1)
        zoomLevel = 1;
    if(zoomLevel > 45)
        zoomLevel = 45;
}

void Camera::updateVectors()
{
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(rot.x)) * cos(glm::radians(rot.y));
    newFront.y = sin(glm::radians(rot.y));
    newFront.z = sin(glm::radians(rot.x)) * cos(glm::radians(rot.y));

    front = glm::normalize(newFront);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}