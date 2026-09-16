// =============================================================================
// CPSC 484 - Assignment 1 - OpenGL Mesh Viewer (STARTER)
// =============================================================================
// This file is NOT a fill-in-the-blanks starter. It gives you the parts that
// are the same in literally every OpenGL program -- opening a window and
// getting a context (Step 1), loading OpenGL's function pointers (Step 2),
// and the compile/link utility functions for turning GLSL text into a usable
// shader program -- because none of that is specific to THIS assignment, and
// you already built it once, understanding every line, in the Assignment 0
// demo (cpsc484_a1_demo_letterviewer.cpp / its line-by-line commented
// twin). Re-typing it here wouldn't teach you anything new.
//
// Everything below a "// TODO (2.x)" comment is yours to write from scratch.
// There are no predefined variables named cubeColor or lightPos waiting for
// you to fill in a value -- you decide what state your program needs and how
// to store it, the same way you'll have to on every assignment after this
// one. The section numbers in the TODOs match the Assignment 1 Instructions
// document; that's where the actual requirements and grading weights live.
// This file only tells you WHERE things go, not WHAT to write.
//
// Before you write a single line here, make sure you can explain -- to
// yourself, out loud -- every piece of the Assignment 0 demo: why GLFW hints
// have to be set before glfwCreateWindow(), what a VAO records versus what a
// VBO holds, why glVertexAttribPointer's stride/offset have to match your
// vertex layout exactly, and why compileShader() checks GL_COMPILE_STATUS.
// If any of those feel shaky, that demo (not this file) is where to go back
// to. This assignment assumes you already own that material.
// =============================================================================

#include "glad.h"          // OpenGL function loader -- must be included before glfw3.h
#include <GLFW/glfw3.h>    // window/context creation, input, timing
#include <cstdlib>         // std::getenv -- used by isRunningUnderWSL() below
#include <fstream>         // std::ifstream -- used by isRunningUnderWSL() below
#include <iostream>        // std::cerr / std::cout for error and debug messages
#include <string>          // std::string -- used by isRunningUnderWSL() below, and by the titleString you'll add next

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace glm;

// TODONE (2.1): declare your own window-title string here. See Assignment 1
// Instructions, Section 2.1. Something like:
std::string titleString = "Fall 2026 - Assignment 1 - Khoa Do";

// -----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
// C++ requires a function to be declared before it's used. main() (further
// down) calls these, so they're declared here and defined later in the file.
// -----------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height); // called by GLFW whenever the window is resized
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods); // called by GLFW on every key press/release/repeat
unsigned int compileShader(unsigned int type, const char* source);         // compiles one GLSL shader, returns its OpenGL ID
unsigned int createShaderProgram(const char* vertexSrc, const char* fragmentSrc); // compiles + links both shaders into one usable program

// Screen dimensions -- passed to glfwCreateWindow() below. Feel free to
// change these, but keep them as named constants rather than magic numbers
// scattered through the file.
const unsigned int SCR_WIDTH = 800;  // window width in pixels
const unsigned int SCR_HEIGHT = 600; // window height in pixels


const char* vertexShaderSource = R"GLSL(
#version 330 core
// "layout (location = N)" must match the glVertexAttribPointer(N, ...) calls
// in createLetterBuffer() below -- that's how the CPU-side vertex data gets
// matched up to these shader inputs.
layout (location = 0) in vec3 aPos;     // this vertex's position, straight from our VBO
layout (location = 1) in vec3 aNormal;  // this vertex's normal (which way its face points)

// "out" variables are computed once per vertex here, then automatically
// interpolated across each triangle before the fragment shader below sees
// them (that interpolation step is called rasterization).
out vec3 Normal;                        // will be picked up by "in vec3 Normal" in the fragment shader

out vec3 FragPos;                       // will be picked up by "in vec3 FragPos" in the fragment shader    

// A "uniform" is a value we set once per draw call from the CPU (see
// glUniformMatrix4fv in the render loop) that stays constant across every
// vertex/pixel of that draw call -- unlike aPos/aNormal, which are
// different for every vertex.
uniform mat4 transform;                 // this letter's combined rotate+scale+position matrix, set from the CPU

void main() {                           // GLSL entry point -- runs once per vertex

    vec4 worldPos = transform * vec4(aPos, 1.0); // transform this vertex's position into world space

    // mat3(transform) keeps only the rotation+scale part of the 4x4 matrix
    // (it drops the translation column), which is what you want when
    // transforming a *direction* like a normal instead of a *point*.
    Normal = mat3(transform) * aNormal; // rotate/scale this vertex's normal the same way the shape itself is rotated/scaled

    //FragPos = vec3(worldPos); // pass the world-space position to the fragment shader

    // gl_Position is a special built-in output: OpenGL reads it to know
    // where this vertex lands on screen (in clip space).
    gl_Position = transform * vec4(aPos, 1.0); // transform this vertex's position into its final on-screen location
}
)GLSL";

const char* fragmentShaderSource = R"GLSL(
#version 330 core
out vec4 FragColor;      // the final pixel color -- this is the only required output

int vec3 FragPos;          // interpolated from the vertex shader's "out vec3 FragPos" above

in vec3 Normal;          // interpolated from the vertex shader's "out vec3 Normal" above
uniform vec3 color;      // this letter's current color, set from the CPU each frame
uniform vec3 lightPos;   // the light's position, set from the CPU each frame

void main() {                                           // GLSL entry point -- runs once per pixel (fragment)
    vec3 N = normalize(Normal);                         // interpolation can shrink the length; renormalize to unit length
    //vec3 lightDir = normalize(vec3(0.4, 0.6, 1.0));      // a fixed light direction, never moves
    vec3 lightDir = normalize(lightPos - FragPos);               // a light direction that can be changed from the CPU each frame

    float ambient = 0.5;                                // a little light even on faces facing away from the light
    float diffuse = max(dot(N, lightDir), 0.0) * 1.2;    // brighter when a face points toward the light; clamp negative to 0

    FragColor = vec4(color * (ambient + diffuse), 1.0);  // scale this letter's color by the light amount; alpha = fully opaque
}
)GLSL";



// TODO (2.3): declare your vertex shader and fragment shader source here, as
// C++ raw string literals (see the Assignment 0 demo for the R"GLSL(...)GLSL"
// pattern and why it's safer than a plain R"(...)"). At minimum your vertex
// shader needs:
//   - a position input attribute and a normal input attribute
//   - a "transform" uniform (mat4) to place/rotate the mesh
// and your fragment shader needs:
//   - a "color" uniform (vec3) for the current cube color
//   - some simple ambient + diffuse shading using the light's position/
//     direction, so the cube's faces are visibly shaded differently rather
//     than being flat silhouettes (see the demo's fragment shader for one
//     way to do this -- yours doesn't have to match it exactly).
//
// const char* vertexShaderSource = R"GLSL(
// ...
// )GLSL";
//
// const char* fragmentShaderSource = R"GLSL(
// ...
// )GLSL";

// TODO (2.2): declare whatever state your mesh needs. At minimum you'll want
// somewhere to put your vertex data (positions + normals) and your index
// data once you've decided on a layout -- see Section 2.2 for the required
// float vertices[] / unsigned int indices[] shape. You'll also need VAO/VBO/
// EBO ids once you get to uploading that data to the GPU.


float vertices[] = {
    // Back face (-Z)
    -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,    0.0f, 0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,    0.0f, 0.0f, -1.0f,

    // Front face (+Z)
    -0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,

    // Left face (-X)
    -0.5f, -0.5f, -0.5f,   -1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,   -1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,   -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,   -1.0f, 0.0f, 0.0f,

    // Right face (+X)
    0.5f, -0.5f, -0.5f,    1.0f,  0.0f, 0.0f,
    0.5f, -0.5f,  0.5f,    1.0f,  0.0f, 0.0f,
    0.5f,  0.5f,  0.5f,    1.0f,  0.0f, 0.0f,
    0.5f,  0.5f, -0.5f,    1.0f,  0.0f, 0.0f,

    // Bottom face (-Y)
    -0.5f, -0.5f, -0.5f,    0.0f, -1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,    0.0f, -1.0f, 0.0f,
    0.5f, -0.5f,  0.5f,    0.0f, -1.0f, 0.0f,
    0.5f, -0.5f, -0.5f,    0.0f, -1.0f, 0.0f,

    // Top face (+Y)
    -0.5f,  0.5f, -0.5f,    0.0f,  1.0f, 0.0f,
    0.5f,  0.5f, -0.5f,    0.0f,  1.0f, 0.0f,
    0.5f,  0.5f,  0.5f,    0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,    0.0f,  1.0f, 0.0f
};

// Counter-clockwise winding as viewed from outside each face.
unsigned int indices[] = {
     0,  3,  2,   2,  1,  0,  // back
     4,  5,  6,   6,  7,  4,  // front
     8,  9, 10,  10, 11,  8,  // left
    12, 13, 14,  14, 15, 12,  // right
    16, 17, 18,  18, 19, 16,  // bottom
    20, 21, 22,  22, 23, 20   // top
};

std::vector<glm::vec3> presetColors = {
    {1.0f, 0.0f, 0.0f},  // red
    {0.0f, 1.0f, 0.0f},  // green
    {0.0f, 0.0f, 1.0f},  // blue
    {1.0f, 1.0f, 0.0f},  // yellow
    {1.0f, 0.0f, 1.0f},  // magenta
    {0.0f, 1.0f, 1.0f},  // cyan
    {1.0f, 0.5f, 0.0f},  // orange
    {1.0f, 1.0f, 1.0f}   // white
};

unsigned int VAO = 0;
unsigned int VBO = 0;
unsigned int EBO = 0;

unsigned int colorIndex = 0; // index of the current color in the presetColors vector
glm::vec3 cubeColor(0.0f, 0.0f, 0.0f);

float rotationAngleX = 0.0f; // rotation angle around the X-axis
float rotationAngleY = 0.0f; // rotation angle around the Y-axis

const float rotationStep = 5.0f; // degrees per key press
float rotationXSpeed = 1.0f; // degrees per second for continuous rotation
float rotationYSpeed = 1.0f; // degrees per second for continuous rotation

glm::vec3 lightPos(1.0f, 1.0f, -5.0f); // initial light position
const float lightStep = 0.5f; // step size for moving the light position

// TODO (2.4/2.5/2.6): declare whatever state your input handling needs to
// read and modify -- e.g. the cube's current color, a list of colors to
// cycle through, the light's position, and the mesh's current rotation
// angles. Nothing here is pre-named for you; pick names that make sense to
// you, since you're the one who has to keep using them.

// -----------------------------------------------------------------------------
// PLATFORM DETECTION (Linux/WSL only -- a no-op on Windows/macOS)
// -----------------------------------------------------------------------------
// Carried over from the Assignment 0 demo unchanged: under WSLg, GLFW's
// default Wayland backend has a known window-resize bug, so on WSL
// specifically we ask GLFW to use X11 instead. See the demo's own comments
// (right above its isRunningUnderWSL()) for the full explanation -- there's
// nothing assignment-specific to change here.
bool isRunningUnderWSL() {
    if (std::getenv("WSL_DISTRO_NAME") != nullptr) return true;
    if (std::getenv("WSL_INTEROP") != nullptr) return true;

    std::ifstream versionFile("/proc/version");
    if (versionFile) {
        std::string contents((std::istreambuf_iterator<char>(versionFile)),
                              std::istreambuf_iterator<char>());
        for (char& c : contents) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (contents.find("microsoft") != std::string::npos) return true;
    }
    return false;
}

/* *************************************************** */

int main() {
    // ---- Step 0: steer GLFW away from WSLg's buggy Wayland backend -----
#if defined(GLFW_VERSION_MAJOR) && (GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 4))
    if (isRunningUnderWSL() && glfwPlatformSupported(GLFW_PLATFORM_X11)) {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    }
#endif

    // ---- Step 1: create a window + OpenGL context via GLFW -------------
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    } 

    // Ask for an OpenGL 3.3 Core Profile context, same as Assignment 0.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE); // required on macOS to get a core-profile context at all; harmless no-op on Windows/Linux

    // TODO (2.1): pass your titleString.c_str() as the window title below
    // instead of the placeholder "Assignment 1" literal.
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, titleString.c_str(), nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Register our callbacks. GLFW calls these automatically -- we never
    // call them ourselves.
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    // ---- Step 2: load OpenGL function pointers via GLAD ----------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // TODO (2.3): compile + link your shaders here, once vertexShaderSource
    // and fragmentShaderSource exist above.
    // unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource); // compile + link our one shader program



    glEnable(GL_DEPTH_TEST); // near surfaces should hide far ones -- you'll want this once you have a 3D cube

    // TODO (2.2): build your cube's vertex/index data and upload it to the
    // GPU here (glGenVertexArrays / glGenBuffers / glBindBuffer /
    // glBufferData / glVertexAttribPointer / glEnableVertexAttribArray),
    // once you've declared the arrays and layout above. This happens once,
    // before the render loop -- not every frame.

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(indices),
        indices,
        GL_STATIC_DRAW
    );

    // Attribute 0: position -- first three floats in each six-float vertex.
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(float),
        reinterpret_cast<void*>(0)
    );
    glEnableVertexAttribArray(0);

    // Attribute 1: normal -- second three floats in each six-float vertex.
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(float),
        reinterpret_cast<void*>(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glBindVertexArray(0); // unbind VAO to avoid accidental modification

    // ---- Step 6 (numbering matches the Assignment 0 demo): render loop --
    while (!glfwWindowShouldClose(window)) {
        // TODO: poll any continuously-held keys here, if you're using that
        // input style for anything (see the demo's processInput() for the
        // pattern, and its INPUT HANDLING comment block for when polling is
        // the right tool vs. when the key_callback below is).

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // TODO (2.3/2.5/2.6/2.7): use your shader program, compute and
        // upload this frame's transform/color/light uniforms, bind your
        // cube's VAO, and issue the draw call. This is the part of the demo's
        // render loop that was specific to drawing letters -- yours will be
        // specific to drawing (and rotating, and recoloring, and relighting)
        // your cube instead.

        glUseProgram(shaderProgram); // "use this shader program for every draw call below"

        rotationAngleX += rotationXSpeed * 0.1f; // update rotation angle based on time
        rotationAngleY += rotationYSpeed * 0.1f; // update rotation angle based on time

        glm::mat4 transform = glm::mat4(1.0f); // identity matrix -- no translation, rotation, or scale
        
        transform = glm::rotate(transform, glm::radians(rotationAngleX), glm::vec3(0.0f, 1.0f, 0.0f)); // rotate around X-axis
        transform = glm::rotate(transform, glm::radians(rotationAngleY), glm::vec3(1.0f, 0.0f, 0.0f)); // rotate around Y-axis
        
        cubeColor = presetColors[colorIndex]; // set the cube color to the current preset color 

        int colorLoc = glGetUniformLocation(shaderProgram, "color"); // ask the shader program where its "color" uniform lives
        glUniform3fv(colorLoc, 1, glm::value_ptr(cubeColor));

        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));

        int transformLoc = glGetUniformLocation(shaderProgram, "transform"); // ask the shader program where its "transform" uniform lives
        
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));              // upload it -- GL_TRUE transposes, since we wrote it row-major above

        glBindVertexArray(VAO); // bind the VAO that records our vertex/index buffers and layout
        
        //sizeof(indices) / sizeof(indices[0]) = 36, which is the number of indices in the array
        glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(indices[0]), GL_UNSIGNED_INT, nullptr); // draw the cube using the index buffer 

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ---- Cleanup ---------------------------------------------------------
    // TODO: delete whatever VAOs/VBOs/EBOs and shader program you created
    // above, the same way the Assignment 0 demo cleans up its letter
    // buffers and shader program before glfwTerminate().
    glfwTerminate();
    return 0;
}

// Called every time the window is resized.
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

// Called by GLFW whenever a key is pressed, released, or repeated.
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;

    // TODO (2.4): handle ESC to close the window (see the Assignment 1
    // Instructions example), and add whatever other keys Sections 2.5
    // (color) and 2.6 (light position) and 2.7 (rotation) need. Remember:
    // action == GLFW_PRESS means "just went down this frame" -- check that
    // (or don't, depending on whether you want one-shot or repeat-while-
    // held behavior) the same way the Assignment 0 demo's key_callback does.

    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, true);
            break;

        case GLFW_KEY_C:
            colorIndex = (colorIndex + 1) % presetColors.size();
            break;
        
        case GLFW_KEY_LEFT:
            rotationAngleX -= rotationStep;
            break;
        
        case GLFW_KEY_RIGHT:
            rotationAngleX += rotationStep;
            break;

        case GLFW_KEY_UP:
            rotationAngleY += rotationStep;
            break;

        case GLFW_KEY_DOWN:
            rotationAngleY -= rotationStep;
            break;


        case GLFW_KEY_I:
            lightPos.y += lightStep;
            break;
        
        case GLFW_KEY_J:
            lightPos.y -= lightStep;
            break;

        case GLFW_KEY_K:
            lightPos.x += lightStep;
            break;

        case GLFW_KEY_L:
            lightPos.x -= lightStep;
            break;

        case GLFW_KEY_U:
            lightPos.z += lightStep;
            break;

        case GLFW_KEY_O:
            lightPos.z -= lightStep;
            break;
        }
    }

}

// -----------------------------------------------------------------------------
// SHADER COMPILE HELPERS
// -----------------------------------------------------------------------------
// Carried over from the Assignment 0 demo, unchanged -- this is reusable
// boilerplate, not something specific to this assignment's mesh. You WILL
// need to call createShaderProgram() with your own shader source strings
// (Section 2.3) -- that's the assignment-specific part.
//
// ALWAYS check compile/link status like this. When you get a blank screen
// later, this is almost always where the answer is -- read the console
// output before you touch anything else.

unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

unsigned int createShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSrc);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}
