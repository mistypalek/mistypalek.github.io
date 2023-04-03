#define _USE_MATH_DEFINES
#include <iostream>         
#include <cstdlib>          
#include <GL/glew.h>        
#include <GLFW/glfw3.h>     
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std; 

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif


namespace
{
    const char* const WINDOW_TITLE = "Misty Palek 7-5 Final"; 

    
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    
    struct GLMesh
    {
        GLuint vao;         
        GLuint vbos[2];     
        GLuint nIndices;    
    };

   
    GLFWwindow* gWindow = nullptr;

    GLMesh remoteMesh;
    GLMesh planeMesh;
    GLMesh speakerMesh;
    GLMesh fanMesh;
    GLMesh fanBaseMesh;
    GLMesh fanPoleMesh;
    GLMesh pyramidMesh;
    GLMesh pyramidBottomMesh;
    
    GLuint gProgramId;

    float rot = 0;
}

// intializes program, sets window size
// redraws graphis to set window size
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePosCallback(GLFWwindow* window, double x, double y);
glm::mat4 UGetCameraMatrix();
void UMouseScrollCallback(GLFWwindow* window, double xOff, double yOff);
void URender();
void RenderMesh(GLMesh& mesh, glm::vec3 scale, glm::vec3 rotation, glm::vec3 translation, bool useTexture = false, int texId = 0);
void CreateCube(GLMesh& mesh, glm::vec3 color = glm::vec3(0));
void CreatePlane(GLMesh& mesh, glm::vec3 color = glm::vec3(0));
void CreateCylinder(GLMesh& mesh, glm::vec3 color = glm::vec3(0));
void CreateSphere(GLMesh& mesh, glm::vec3 color = glm::vec3(0));
void CreatePyramid(GLMesh& mesh, glm::vec3 color = glm::vec3(0));
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* fileName, GLuint& id);
void UDestroyTexture(GLuint texId);
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);

// Vertex Shader
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 normal;
    layout(location = 2) in vec4 color;  
    layout(location = 3) in vec2 texCoords;

    out vec3 vertexNormal;
    out vec3 vertexFragmentPos;
    out vec4 vertexColor;
    out vec2 vertexTexCoords;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f);

        vertexFragmentPos = vec3(model * vec4(position, 1.0f));
        vertexNormal = mat3(transpose(inverse(model))) * normal;

        vertexColor = color;
        vertexTexCoords = texCoords;
    }
);

// Fragment Shader
const GLchar* fragmentShaderSource = GLSL(440,
    in vec3 vertexNormal;
    in vec3 vertexFragmentPos;
    in vec4 vertexColor; 
    in vec2 vertexTexCoords;

    uniform vec3 lightColor;
    uniform vec3 lightPos;
    uniform vec3 viewPosition;
    out vec4 fragmentColor;

    uniform sampler2D uTexture;
    uniform bool uUseTexture;

    void main()
    {
        float ambientStrength = 0.1f; // Set ambient or global lighting strength
        vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

        //Calculate Diffuse lighting*/
        vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
        vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
        float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
        vec3 diffuse = impact * lightColor; // Generate diffuse light color

        //Calculate Specular lighting*/
        float specularIntensity = 0.4f; // Set specular light strength
        float highlightSize = 8.0f; // Set specular highlight size
        vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
        vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
        //Calculate specular component
        float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
        vec3 specular = specularIntensity * specularComponent * lightColor;

        vec4 textureColor;

        if (uUseTexture)
            textureColor = texture(uTexture, vertexTexCoords);
        else
            textureColor = vec4(vertexColor);

        // Calculate phong result
        vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

        fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
    }
);

float speed = 0.01, xRot = 90, yRot = 0;
int mouseLastX, mouseLastY;
bool firstMouse = true;

glm::vec3 Pos(-2, 10, -10);
glm::vec3 Front;
glm::vec3 Right;
glm::vec3 Up;

bool isPerspective = true;
bool pRelease = true;

GLuint remoteTexId;
GLuint fanTexId;
GLuint pyramidTexId;

glm::vec3 lightColor(1, 1, 1);
glm::vec3 lightPos(-2, 7, -3);

int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    CreateCube(remoteMesh);
    CreatePlane(planeMesh, glm::vec3(0.7f, 1.0f, 0.7f));
    CreateCylinder(speakerMesh, glm::vec3(0.15f));
    CreateSphere(fanMesh);
    CreateCylinder(fanBaseMesh, glm::vec3(0.15f));
    CreateCylinder(fanPoleMesh, glm::vec3(0.15f));
    CreatePyramid(pyramidMesh);
    CreateCube(pyramidBottomMesh);

    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    if (!UCreateTexture("C:/Misty Palek - 7-5 Final/images/remote.png", remoteTexId))
    {
        cout << "Failed to load texture remote.png" << endl;
        return EXIT_FAILURE;
    }
    
    if (!UCreateTexture("C:/Misty Palek - 7-5 Final/images/fan.png", fanTexId))
    {
        cout << "Failed to load texture fan.png" << endl;
        return EXIT_FAILURE;
    }

    if (!UCreateTexture("C:/Misty Palek - 7-5 Final/images/pyramid.png", pyramidTexId))
    {
        cout << "Failed to load texture pyramid.png" << endl;
        return EXIT_FAILURE;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, remoteTexId);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, fanTexId);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, pyramidTexId);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    while (!glfwWindowShouldClose(gWindow))
    {
        UProcessInput(gWindow);

        URender();

        glfwPollEvents();
    }

    UDestroyMesh(remoteMesh);
    UDestroyMesh(planeMesh);
    UDestroyMesh(speakerMesh);

    UDestroyTexture(remoteTexId);

    UDestroyShaderProgram(gProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, glfwGetPrimaryMonitor(), NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePosCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);

    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}

// query GLFW to react accordingly to input
void UProcessInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        Pos.z += speed;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        Pos.z -= speed;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        Pos.x += speed;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        Pos.x -= speed;

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        Pos.y += speed;

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        Pos.y -= speed;

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        if(pRelease)
        {
            isPerspective = !isPerspective;
            pRelease = false;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE)
        pRelease = true;
}

void UMousePosCallback(GLFWwindow* window, double x, double y)
{
    if (firstMouse)
    {
        mouseLastX = x;
        mouseLastY = y;
        firstMouse = false;
    }

    float xOffset = x - mouseLastX;
    float yOffset = mouseLastY - y;

    mouseLastX = x;
    mouseLastY = y;

    float sensitivity = 0.5;

    xOffset *= sensitivity;
    yOffset *= sensitivity;

    xRot += xOffset;
    yRot += yOffset;

    if (yRot > 89)
        yRot = 89;

    if (yRot < -89)
        yRot = -89;

    glm::vec3 front;

    front.x = cos(glm::radians(xRot) * cos(glm::radians(yRot)));
    front.y = sin(glm::radians(yRot));
    front.z = sin(glm::radians(xRot) * cos(glm::radians(yRot)));

    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, glm::vec3(0, 1, 0)));
    Up = glm::normalize(glm::cross(Right, Front));
}

glm::mat4 UGetCameraMatrix()
{
    return glm::lookAt(Pos, Pos + Front, Up);
}

void UMouseScrollCallback(GLFWwindow* window, double xOff, double yOff)
{
    speed += yOff * 0.01;
    if (speed < 0)
        speed = 0;
}


void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// Functioned called to render a frame
void URender()
{
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RenderMesh(speakerMesh, glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(90.0f, 0.0f, 0.0f), glm::vec3(-2.0f, 1.0f, 2.0f));
    RenderMesh(planeMesh, glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f), glm::vec3(0.0f));
    RenderMesh(remoteMesh, glm::vec3(0.6f, 0.2f, 1.5f), glm::vec3(0.0f, -90.0f, 0.0f), glm::vec3(-2.0f, 0.1f, -2.0f), true, 0);
    RenderMesh(fanMesh, glm::vec3(4.0f, 4.0f, 1.2f), glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(2.0f, 5.0f, 2.0f), true, 1);
    RenderMesh(fanBaseMesh, glm::vec3(2.5f, 1.25f, 0.2f), glm::vec3(90.0f, 0.0f, -45.0f), glm::vec3(2.5f, 0.1f, 2.5f));
    RenderMesh(fanPoleMesh, glm::vec3(0.5f, 0.5f, 3.0f), glm::vec3(90.0f, 0.0f, 0.0f), glm::vec3(2.5f, 3.2f, 2.5f));
    RenderMesh(pyramidMesh, glm::vec3(0.7f, 1.0f, 0.7f), glm::vec3(0.0f, -140.0f, 0.0f), glm::vec3(2.15f, 0.2f, -1.85f), true, 2);
    RenderMesh(pyramidBottomMesh, glm::vec3(0.7f, 0.2f, 0.9f), glm::vec3(0.0f, -140.0f, 0.0f), glm::vec3(2.0f, 0.0f, -2.0f), true, 2);

    rot += 0.25;

    glfwSwapBuffers(gWindow);  
}

// Renders a GLMesh
void RenderMesh(GLMesh &mesh, glm::vec3 scale, glm::vec3 rotation, glm::vec3 translation, bool useTexture, int texId)
{
    // Creates rotations matrices
    glm::mat4 rotateX = glm::rotate(glm::radians(rotation.x), glm::vec3(1, 0, 0));
    glm::mat4 rotateY = glm::rotate(glm::radians(rotation.y), glm::vec3(0, 1, 0));
    glm::mat4 rotateZ = glm::rotate(glm::radians(rotation.z), glm::vec3(0, 0, 1));

    // Creates transform matrix
    glm::mat4 model = glm::translate(translation) * (rotateX * rotateY * rotateZ) * glm::scale(scale);

    // Creates projection matrix
    glm::mat4 projection;
    if (isPerspective)
        projection = glm::perspective(45.0f, (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    else
        projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Gemotry shader parameters
    glUniformMatrix4fv(glGetUniformLocation(gProgramId, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(gProgramId, "view"), 1, GL_FALSE, glm::value_ptr(UGetCameraMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(gProgramId, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Lighting shader parameters
    glUniform3f(glGetUniformLocation(gProgramId, "lightColor"), lightColor.r, lightColor.g, lightColor.b);
    glUniform3f(glGetUniformLocation(gProgramId, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(glGetUniformLocation(gProgramId, "viewPosition"), Pos.x, Pos.y, Pos.z);

    glUniform1i(glGetUniformLocation(gProgramId, "uUseTexture"), useTexture);

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(mesh.vao);

    // Set texture
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), texId);

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, mesh.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
}

// Creates a cylinder GLMesh
void CreateCylinder(GLMesh& mesh, glm::vec3 color)
{
    const int slices = 16;
    const float angle = 2 * M_PI / slices;

    const int secCircOffset = slices + 1;
    const int triangleSize = 3;

    glm::vec3 vertex[(slices + 1) * 2];

    const int numVerts = sizeof(vertex) / sizeof(vertex[0]) * 12;
    const int numIndices = slices * triangleSize * 4;

    GLfloat vao[numVerts];
    GLushort indices[numIndices];

    for (int i = 0; i <= slices; i++)
    {
        if (i == slices)
        {
            vertex[i] = glm::vec3(0, 0, 1);
            vertex[i + secCircOffset] = glm::vec3(0, 0, -1);
        }
        else
        {
            vertex[i] = glm::vec3(glm::cos(angle * i), glm::sin(angle * i), 1);
            vertex[i + secCircOffset] = glm::vec3(glm::cos(angle * i), glm::sin(angle * i), -1);
        }
    }

    for (int i = 0; i < numVerts / 12; i++)
    {
        int pointer = i * 12;

        vao[pointer] = vertex[i].x;
        vao[pointer + 1] = vertex[i].y;
        vao[pointer + 2] = vertex[i].z;

        vao[pointer + 3] = vertex[i].x;
        vao[pointer + 4] = vertex[i].y;
        vao[pointer + 5] = vertex[i].z;

        vao[pointer + 6] = color.r;
        vao[pointer + 7] = color.g;
        vao[pointer + 8] = color.b;
        vao[pointer + 9] = 1;

        vao[pointer + 10] = vertex[i].x;
        vao[pointer + 11] = vertex[i].y;
    }

    for (int i = 0; i < slices; i++)
    {
        int pointer = i * triangleSize;

        indices[pointer] = i;
        indices[pointer + 1] = slices;
        indices[pointer + 2] = i + 1 < slices ? i + 1 : 0;

        pointer += slices * triangleSize;

        indices[pointer] = i + secCircOffset;
        indices[pointer + 1] = slices + secCircOffset;
        indices[pointer + 2] = i + 1 < slices ? i + 1 + secCircOffset : secCircOffset;

        pointer += slices * triangleSize;

        indices[pointer] = i;
        indices[pointer + 1] = i + secCircOffset;
        indices[pointer + 2] = i + 1 < slices ? i + 1 : 0;

        pointer += slices * triangleSize;

        indices[pointer] = i + secCircOffset;
        indices[pointer + 1] = i + 1 < slices ? i + 1 : 0;
        indices[pointer + 2] = i + 1 < slices ? i + 1 + secCircOffset : secCircOffset;
    }

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerColor = 4;
    const GLuint floatsPerTexture = 2;

    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vao), vao, GL_STATIC_DRAW);

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerColor + floatsPerTexture);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float)* floatsPerVertex) + (sizeof(float) * floatsPerNormal));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float)* floatsPerVertex) + (sizeof(float) * floatsPerNormal) + (sizeof(float) * floatsPerColor));
    glEnableVertexAttribArray(3);
}

// Creates a plane GLMesh
void CreatePlane(GLMesh& mesh, glm::vec3 color)
{
    // Position and Color data
    GLfloat verts[] = {
             // Vertex Positions                        // Colors (r,g,b,a)
            -1.0f, 0.0f, -1.0f,     0.0f, 1.0f, 0.0f,   color.r, color.g, color.b, 1.0f,   1.0f, 0.0f,
            -1.0f, 0.0f,  1.0f,     0.0f, 1.0f, 0.0f,   color.r, color.g, color.b, 1.0f,   1.0f, 0.0f,
             1.0f, 0.0f,  1.0f,     0.0f, 1.0f, 0.0f,   color.r, color.g, color.b, 1.0f,   1.0f, 0.0f,
             1.0f, 0.0f, -1.0f,     0.0f, 1.0f, 0.0f,   color.r, color.g, color.b, 1.0f,   1.0f, 0.0f
    };

    // Index data to share position data
    GLushort indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerColor = 4;
    const GLuint floatsPerTexture = 2;

    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerColor + floatsPerTexture);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex) + (sizeof(float) * floatsPerNormal));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex) + (sizeof(float) * floatsPerNormal) + (sizeof(float) * floatsPerColor));
    glEnableVertexAttribArray(3);
}

// Creates a cube GLMesh
void CreateCube(GLMesh& mesh, glm::vec3 color)
{
    // Position and Color data
    GLfloat verts[] = {
        // Vertex Positions                             // Colors (r,g,b,a)
       -1.0f,  1.0f, -1.0f,      -1.0f, 1.0f, -1.0f,    color.r, color.g, color.b, 1.0f,   0.0f, 0.0f,
       -1.0f,  1.0f,  1.0f,      -1.0f, 1.0f, 1.0f,     color.r, color.g, color.b, 1.0f,   0.0f, 1.0f,
        1.0f,  1.0f,  1.0f,      1.0f, 1.0f, 1.0f,      color.r, color.g, color.b, 1.0f,   1.0f, 1.0f,
        1.0f,  1.0f, -1.0f,      1.0f, 1.0f, -1.0f,     color.r, color.g, color.b, 1.0f,   1.0f, 0.0f,
       -1.0f, -1.0f, -1.0f,      -1.0f, -1.0f, -1.0f,   color.r, color.g, color.b, 1.0f,   0.0f, 0.0f,
       -1.0f, -1.0f,  1.0f,      -1.0f, -1.0f, 1.0f,    color.r, color.g, color.b, 1.0f,   0.0f, 1.0f,
        1.0f, -1.0f,  1.0f,      1.0f, -1.0f, 1.0f,     color.r, color.g, color.b, 1.0f,   1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,      1.0f, -1.0f, -1.0f,    color.r, color.g, color.b, 1.0f,   1.0f, 0.0f
    };

    // Index data to share position data
    GLushort indices[] = {
        0, 1, 2,
        0, 2, 3,
        4, 5, 6,
        4, 6, 7,
        0, 1, 5,
        0, 5, 4,
        1, 2, 6,
        1, 6, 5,
        2, 3, 7,
        2, 7, 6,
        3, 0, 4,
        3, 4, 7
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerColor = 4;
    const GLuint floatsPerTexture = 2;

    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerColor + floatsPerTexture);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex) + (sizeof(float) * floatsPerNormal));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex) + (sizeof(float) * floatsPerNormal) + (sizeof(float) * floatsPerColor));
    glEnableVertexAttribArray(3);
}

// Creates a sphere GLMesh
void CreateSphere(GLMesh& mesh, glm::vec3 color)
{
    const int complexity = 32;

    const float latAngle = 2 * M_PI / complexity;
    const float logAngle = M_PI / complexity;

    glm::vec3 vertex[complexity + 1][complexity + 1];

    const int numVerts = (complexity + 1) * (complexity + 1) * 12;
    const int numIndices = numVerts / 12 * 2 * 3;

    GLfloat vao[numVerts];
    GLushort indices[numIndices];

    for (int i = 0; i < complexity + 1; i++)
    {
        float lat = (((i - 0) * (M_PI - -M_PI)) / (complexity - 0)) + -M_PI;

        for (int j = 0; j < complexity + 1; j++)
        {
            float log = (((j - 0) * (M_PI_2 - -M_PI_2)) / (complexity - 0)) + -M_PI_2;

            vertex[i][j] = glm::vec3
            (
                glm::sin(lat) * glm::cos(log),
                glm::sin(lat) * glm::sin(log),
                glm::cos(lat)
            );

            int pointer = ((i * (complexity + 1)) + j) * 12;

            vao[pointer] = vertex[i][j].x;
            vao[pointer + 1] = vertex[i][j].y;
            vao[pointer + 2] = vertex[i][j].z;

            vao[pointer + 3] = vertex[i][j].x;
            vao[pointer + 4] = vertex[i][j].y;
            vao[pointer + 5] = vertex[i][j].z;

            vao[pointer + 6] = color.r;
            vao[pointer + 7] = color.g;
            vao[pointer + 8] = color.b;
            vao[pointer + 9] = 1;

            vao[pointer + 10] = vertex[i][j].x / 2 + 0.5;
            vao[pointer + 11] = vertex[i][j].y / 2 + 0.5;

            pointer = ((i * (complexity + 1)) + j) * 6;

            indices[pointer] = i * (complexity + 1) + j;
            indices[pointer + 1] = (i + 1) * (complexity + 1) + j;
            indices[pointer + 2] = i * (complexity + 1) + (j + 1);

            indices[pointer + 3] = (i + 1) * (complexity + 1) + (j + 1);
            indices[pointer + 4] = (i + 1) * (complexity + 1) + j;
            indices[pointer + 5] = i * (complexity + 1) + (j + 1);
        }
    }

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerColor = 4;
    const GLuint floatsPerTexture = 2;

    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vao), vao, GL_STATIC_DRAW);

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerColor + floatsPerTexture);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex) + (sizeof(float) * floatsPerNormal));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex) + (sizeof(float) * floatsPerNormal) + (sizeof(float) * floatsPerColor));
    glEnableVertexAttribArray(3);
}

// Creates a pyramid GLMesh
void CreatePyramid(GLMesh& mesh, glm::vec3 color)
{
    // Position and Color data
    GLfloat verts[] = {
        // Vertex Positions                        // Colors (r,g,b,a)
       -1.0f, 0.0f, -1.0f,     -1.0f, 0.5f, -1.0f,   color.r, color.g, color.b, 1.0f,   0.0f, 0.0f,
       -1.0f, 0.0f,  1.0f,     -1.0f, 0.5f, 1.0f,   color.r, color.g, color.b, 1.0f,   0.0f, 1.0f,
        1.0f, 0.0f,  1.0f,     1.0f, 0.5f, 1.0f,   color.r, color.g, color.b, 1.0f,   1.0f, 1.0f,
        1.0f, 0.0f, -1.0f,     1.0f, 0.5f, -1.0f,   color.r, color.g, color.b, 1.0f,   1.0f, 0.0f,

        0.0f, 1.0f, 0.0f,     0.0f, 1.0f, 0.0f,   color.r, color.g, color.b, 1.0f,   0.5f, 0.5f
    };

    // Index data to share position data
    GLushort indices[] = {
        0, 1, 2,
        0, 2, 3,
        0, 4, 1,
        1, 4, 2,
        2, 4, 3,
        3, 4, 0
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerColor = 4;
    const GLuint floatsPerTexture = 2;

    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerColor + floatsPerTexture);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex) + (sizeof(float) * floatsPerNormal));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex) + (sizeof(float) * floatsPerNormal) + (sizeof(float) * floatsPerColor));
    glEnableVertexAttribArray(3);
}

void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(2, mesh.vbos);
}

bool UCreateTexture(const char* fileName, GLuint &id)
{
    int width, height, channels;
    unsigned char* image = stbi_load(fileName, &width, &height, &channels, 0);

    if (image)
    {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        // Set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // Set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}

void UDestroyTexture(GLuint texId)
{
    glGenTextures(1, &texId);
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    int success = 0;
    char infoLog[512];

    programId = glCreateProgram();

    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    glCompileShader(vertexShaderId); 
  
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); 
    
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   
    
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}