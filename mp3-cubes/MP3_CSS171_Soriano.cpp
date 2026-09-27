/*
================================================================================
COURSE:     CSS171 - Computer Graphics and Visual Computing
TASK:       Machine Problem #3 (3D Modeling - Three Rotating Cubes)
FILENAME:   MP3_CSS171_Soriano.cpp
AUTHOR:     Willard Soriano
INSTRUCTOR: Polycarpio V. Cabalag II
DATE:       September 2026

DESCRIPTION:
Renders three nested cubes (a solid core inside two glass shells), each
spinning about a different axis, using the modern OpenGL 3.3 core pipeline
(GLFW + GLAD + GLM + GLSL shaders).

KEY FEATURES:
1. Efficient Geometry: ONE cube mesh of 8 shared vertices and 36 indices
   (exactly 12 triangles) lives in a single VAO/VBO/EBO and is drawn three
   times with different model matrices, so no vertex data is duplicated.
2. Flat Face Shading Without Extra Vertices: the fragment shader derives each
   face's normal from screen-space derivatives (dFdx/dFdy), so every face is
   lit distinctly even though the 8 corner vertices are shared.
3. Correct Transparency: the core is drawn opaque first, then the glass
   shells from inner to outer. Only each shell's front (camera-facing) walls
   are drawn, so the nested shells blend in back-to-front order and the far
   walls never show through as extra "phantom" cube outlines.
4. Always Visibly 3D: each cube is first tilted so its corner-to-corner
   (body) diagonal lies along its spin axis, and every spin axis lies in the
   screen plane (derived from the camera). A cube's faces are 54.7 degrees
   from its diagonal, so no face ever turns closer than 35.3 degrees to
   facing the camera - no cube ever collapses into a flat square.
5. Centered Layout: all cubes share the world origin, the camera looks at it,
   and the projection follows the window's aspect ratio on resize.

CONTROLS: ESC closes the window.
================================================================================
*/

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

// Window specifications
const int   WINDOW_WIDTH  = 1000;
const int   WINDOW_HEIGHT = 700;
const char *WINDOW_TITLE  = "CSS171 MP3 - Three Rotating Cubes (Willard Soriano)";

// Camera specifications
const float     FIELD_OF_VIEW_DEG = 45.0f;
const float     NEAR_PLANE        = 0.1f;
const float     FAR_PLANE         = 100.0f;
const glm::vec3 CAMERA_POSITION(0.0f, 1.2f, 7.5f);
const glm::vec3 CAMERA_TARGET(0.0f, 0.0f, 0.0f);
const glm::vec3 CAMERA_UP(0.0f, 1.0f, 0.0f);

// Light direction (points FROM the surface TOWARD the light), in view space
const glm::vec3 LIGHT_DIRECTION(0.4f, 0.7f, 0.6f);

// Background color: black, so the window never reads as an extra panel
// behind the cubes and the glass tints stand out
const glm::vec3 BACKGROUND_COLOR(0.0f, 0.0f, 0.0f);

// Unit cube: 8 corner vertices shared by all 6 faces (positions only)
const float CUBE_VERTICES[] = {
    -0.5f, -0.5f, -0.5f,  // 0: left  bottom back
     0.5f, -0.5f, -0.5f,  // 1: right bottom back
     0.5f,  0.5f, -0.5f,  // 2: right top    back
    -0.5f,  0.5f, -0.5f,  // 3: left  top    back
    -0.5f, -0.5f,  0.5f,  // 4: left  bottom front
     0.5f, -0.5f,  0.5f,  // 5: right bottom front
     0.5f,  0.5f,  0.5f,  // 6: right top    front
    -0.5f,  0.5f,  0.5f   // 7: left  top    front
};

// 6 faces x 2 triangles x 3 indices = 36 indices (12 triangles).
// Every triangle is wound counter-clockwise when seen from outside the cube,
// which face culling relies on to tell front faces from back faces.
const unsigned int CUBE_INDICES[] = {
    4, 5, 6,   6, 7, 4,  // front  (+Z)
    1, 0, 3,   3, 2, 1,  // back   (-Z)
    0, 4, 7,   7, 3, 0,  // left   (-X)
    5, 1, 2,   2, 6, 5,  // right  (+X)
    7, 6, 2,   2, 3, 7,  // top    (+Y)
    0, 1, 5,   5, 4, 0   // bottom (-Y)
};
const GLsizei CUBE_INDEX_COUNT = sizeof(CUBE_INDICES) / sizeof(CUBE_INDICES[0]);

// Per-cube appearance and motion
struct CubeSpec {
    float     size;             // edge length in world units
    glm::vec3 color;            // base RGB color
    float     alpha;            // 1.0 = opaque, < 1.0 = glass
    glm::vec2 spinOnScreen;     // spin axis as a screen direction (x = right, y = up)
    float     degreesPerSecond; // spin speed; negative spins the other way
};

// Ordered inner to outer: the opaque core must be drawn before the glass
// shells so the shells can blend over it. Each cube tumbles about a
// different screen direction: diagonal, horizontal and vertical.
const CubeSpec CUBES[] = {
    { 1.0f, glm::vec3(1.00f, 0.62f, 0.20f), 1.00f, glm::vec2(1.0f, 1.0f),  90.0f }, // amber core
    { 1.9f, glm::vec3(0.20f, 0.85f, 0.75f), 0.40f, glm::vec2(1.0f, 0.0f), -45.0f }, // teal glass
    { 2.9f, glm::vec3(0.55f, 0.45f, 1.00f), 0.32f, glm::vec2(0.0f, 1.0f),  30.0f }  // violet glass
};
const int CUBE_COUNT = sizeof(CUBES) / sizeof(CUBES[0]);

// Vertex shader: transforms positions and passes view-space position on
const char *VERTEX_SHADER_SOURCE = R"(
#version 330 core
layout (location = 0) in vec3 aPosition;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vViewPosition;

void main() {
    vec4 viewPosition = uView * uModel * vec4(aPosition, 1.0);
    vViewPosition = viewPosition.xyz;
    gl_Position = uProjection * viewPosition;
}
)";

// Fragment shader: flat Lambert lighting from a derivative-based face normal
const char *FRAGMENT_SHADER_SOURCE = R"(
#version 330 core
in vec3 vViewPosition;

uniform vec3  uColor;
uniform float uAlpha;
uniform vec3  uLightDirection;

out vec4 FragColor;

void main() {
    // The derivatives of position across the screen span the triangle's
    // plane, so their cross product is the face normal (constant per face).
    vec3 faceNormal = normalize(cross(dFdx(vViewPosition), dFdy(vViewPosition)));

    float ambient = 0.30;
    float diffuse = max(dot(faceNormal, normalize(uLightDirection)), 0.0);
    FragColor = vec4(uColor * (ambient + 0.75 * diffuse), uAlpha);
}
)";

// Resize the viewport whenever the framebuffer changes size
void framebufferSizeCallback(GLFWwindow *, int width, int height) {
    glViewport(0, 0, width, height);
}

// Report GLFW errors instead of failing silently
void glfwErrorCallback(int code, const char *description) {
    std::cerr << "GLFW error " << code << ": " << description << "\n";
}

// Close the window when ESC is pressed
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

// Compile one shader stage; returns 0 and prints the log on failure
GLuint compileShader(GLenum stage, const char *source, const char *stageName) {
    GLuint shader = glCreateShader(stage);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << stageName << " shader compilation failed:\n" << log << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// Compile and link the shader program; returns 0 on failure
GLuint createShaderProgram() {
    GLuint vertexShader   = compileShader(GL_VERTEX_SHADER, VERTEX_SHADER_SOURCE, "Vertex");
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_SOURCE, "Fragment");
    if (vertexShader == 0 || fragmentShader == 0) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // The stages are copied into the program, so they can be released now
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "Shader program linking failed:\n" << log << "\n";
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

// Upload the shared cube mesh into one VAO (with its VBO and EBO)
GLuint createCubeMesh(GLuint &vbo, GLuint &ebo) {
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTICES), CUBE_VERTICES, GL_STATIC_DRAW);

    // The EBO binding is recorded in the VAO, so bind it while the VAO is bound
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(CUBE_INDICES), CUBE_INDICES, GL_STATIC_DRAW);

    // Attribute 0: vec3 position, tightly packed
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    return vao;
}

// Fixed orientation of one cube's spin, worked out once at startup
struct CubeSpin {
    glm::vec3 axis;  // world-space spin axis, unit length, in the screen plane
    glm::mat4 tilt;  // turns the cube so its body diagonal lies on the axis
};

// Turn a screen direction into a world axis and the tilt that puts the
// cube's (1,1,1) body diagonal onto it. Keeping the axis perpendicular to
// the line of sight is what guarantees no face ever turns flat to the camera.
CubeSpin makeCubeSpin(const CubeSpec &cube) {
    glm::vec3 toCamera    = glm::normalize(CAMERA_POSITION); // cubes sit at the origin
    glm::vec3 screenRight = glm::normalize(glm::cross(CAMERA_UP, toCamera));
    glm::vec3 screenUp    = glm::cross(toCamera, screenRight);

    CubeSpin spin;
    spin.axis = glm::normalize(cube.spinOnScreen.x * screenRight + cube.spinOnScreen.y * screenUp);

    const glm::vec3 bodyDiagonal = glm::normalize(glm::vec3(1.0f));
    glm::vec3 tiltAxis = glm::cross(bodyDiagonal, spin.axis);
    float tiltAngle = std::acos(glm::clamp(glm::dot(bodyDiagonal, spin.axis), -1.0f, 1.0f));
    // A zero-length tilt axis means the diagonal is already (anti)parallel to
    // the spin axis; either way spinning about it is correct, so no tilt.
    spin.tilt = glm::length(tiltAxis) > 1e-6f
                    ? glm::rotate(glm::mat4(1.0f), tiltAngle, glm::normalize(tiltAxis))
                    : glm::mat4(1.0f);
    return spin;
}

// Model matrix for one cube at the given time: scale, tilt, then spin
glm::mat4 cubeModelMatrix(const CubeSpec &cube, const CubeSpin &spin, float timeSeconds) {
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(cube.degreesPerSecond * timeSeconds), spin.axis);
    model = model * spin.tilt;
    model = glm::scale(model, glm::vec3(cube.size));
    return model;
}

// Draw every cube in table order: the opaque core, then the glass shells
// from inner to outer. Back faces are culled for all of them.
void drawCubes(GLuint program, GLuint vao, const CubeSpin spins[], float timeSeconds) {
    GLint modelLocation = glGetUniformLocation(program, "uModel");
    GLint colorLocation = glGetUniformLocation(program, "uColor");
    GLint alphaLocation = glGetUniformLocation(program, "uAlpha");

    glBindVertexArray(vao);

    for (int i = 0; i < CUBE_COUNT; ++i) {
        const CubeSpec &cube = CUBES[i];
        glm::mat4 model = cubeModelMatrix(cube, spins[i], timeSeconds);
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(colorLocation, 1, glm::value_ptr(cube.color));
        glUniform1f(alphaLocation, cube.alpha);

        // Glass keeps testing depth but stops writing it, so a shell never
        // hides anything drawn after it. Its far walls are culled: seen
        // through the glass, their smaller perspective outline would read
        // as an extra cube.
        bool isGlass = cube.alpha < 1.0f;
        glDepthMask(isGlass ? GL_FALSE : GL_TRUE);
        glDrawElements(GL_TRIANGLES, CUBE_INDEX_COUNT, GL_UNSIGNED_INT, (void *)0);
    }
    glDepthMask(GL_TRUE);

    glBindVertexArray(0);
}

int main() {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return EXIT_FAILURE;
    }

    // Request an OpenGL 3.3 core profile context with 4x multisampling
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    GLuint program = createShaderProgram();
    if (program == 0) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint vao = createCubeMesh(vbo, ebo);

    // Fixed pipeline state: depth testing, face culling, alpha blending, MSAA
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);

    // The camera and light never move, so their uniforms are set once
    glUseProgram(program);
    glm::mat4 view = glm::lookAt(CAMERA_POSITION, CAMERA_TARGET, CAMERA_UP);
    glUniformMatrix4fv(glGetUniformLocation(program, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glm::vec3 lightInViewSpace = glm::mat3(view) * LIGHT_DIRECTION;
    glUniform3fv(glGetUniformLocation(program, "uLightDirection"), 1, glm::value_ptr(lightInViewSpace));
    GLint projectionLocation = glGetUniformLocation(program, "uProjection");

    CubeSpin spins[CUBE_COUNT];
    for (int i = 0; i < CUBE_COUNT; ++i) {
        spins[i] = makeCubeSpin(CUBES[i]);
    }

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        // Rebuild the projection each frame so resizing keeps cubes centered
        // and undistorted; skip rendering while minimized (0-height window).
        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        if (framebufferWidth > 0 && framebufferHeight > 0) {
            float aspect = (float)framebufferWidth / (float)framebufferHeight;
            glm::mat4 projection = glm::perspective(glm::radians(FIELD_OF_VIEW_DEG), aspect,
                                                    NEAR_PLANE, FAR_PLANE);
            glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

            glClearColor(BACKGROUND_COLOR.r, BACKGROUND_COLOR.g, BACKGROUND_COLOR.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            drawCubes(program, vao, spins, (float)glfwGetTime());
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
