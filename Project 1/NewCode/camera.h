#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

enum CameraMovement
{
    FORWARD,
    BACKWARD,
    RIGHT,
    LEFT,
    UP,
    DOWN
};

class Camera
{
public:
    glm::vec3 pos;
    glm::vec2 rot;
    glm::vec3 front;
    float zoomLevel;
    bool isPerspective;

    Camera(int winWidth, int winHeight, glm::vec3 pos = glm::vec3());
    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();
    void move(CameraMovement direction);
    void look(glm::vec2 rot);
    void zoom(float off);

private:
    const int WINDOW_WIDTH;
    const int WINDOW_HEIGHT;

    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    void updateVectors();
};