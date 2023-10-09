#include "render.h"
#include "obj/shader_constants.h"

const int resX = 1000;
const int resY = 1000;

int num_sectors = 50;

void error_callback(int error, const char* description) {
    fprintf(stderr, "Error %d: %s\n", error, description);
}
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

void checkError() {
    int error = glGetError();
    if(error != 0) {
        fprintf(stderr, "Error found! Code: %d\n", error);
        exit(1);
    }
}

void draw(unsigned int program, unsigned int VAO, int num_circles) {
    float time = glfwGetTime();
    glClearColor(0.0, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    int resolutionLocation = glGetUniformLocation(program, "resolution");
    int timeLocation = glGetUniformLocation(program, "time");

    glUseProgram(program);

    glUniform1f(timeLocation, time);
    glUniform2i(resolutionLocation, resX, resY);

    // glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, num_sectors*num_circles*3, GL_UNSIGNED_INT, 0);

    // glFlush();
}

GLFWwindow* init() {
    if(!glfwInit()) {
        fprintf(stderr, "GLFW failed to initialize!\n");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(resX, resY, "GRAVITY!", NULL, NULL);
    if(!window) {
        fprintf(stderr, "Window failed to create!\n");
    }

    glfwSetErrorCallback(error_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);

    glfwSetTime(0.0);
    return window;
}

unsigned int programInit() {
    const char *vertex = (char*) shaders_vertex_glsl;
    const char *fragment = (char*) shaders_fragment_glsl;

    // vertex shader

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertex, NULL);
    glCompileShader(vertexShader);

    int success;
	char log[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    glGetProgramInfoLog(vertexShader, 512, NULL, log);
    printf("Vertex shader info log: %s\n", log);
	if (!success) {
		fprintf(stderr, "Vertex shader failed to compile!\n");
		glDeleteShader(vertexShader);
	}

    // Fragment shader

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragment, NULL);
    glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    glGetProgramInfoLog(fragmentShader, 512, NULL, log);
    printf("Fragment shader info log: %s\n", log);
	if (!success) {
		fprintf(stderr, "Fragment shader failed to compile.\n");
		glDeleteShader(fragmentShader);
	}

    // Shader program

    unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindAttribLocation(shaderProgram, 0, "vertices");
	glBindAttribLocation(shaderProgram, 0, "colors");
	glLinkProgram(shaderProgram);

    glGetShaderiv(shaderProgram, GL_LINK_STATUS, &success);
    glGetProgramInfoLog(shaderProgram, 512, NULL, log);
    printf("Program info log: %s\n", log);
	if (!success) {
		fprintf(stderr, "Shader program failed to link.\n");
		glDeleteShader(shaderProgram);
	}

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void circleInit(float* data, unsigned int* indices, int index, float center_x, float center_y, float radius) {
    int data_offset = index*2*(num_sectors+1);
    int ebo_offset = index*(num_sectors+1);
    int indices_offset = index*num_sectors*3;
    data[data_offset] = center_x;
    data[data_offset+1] = center_y;
    for(int sector = 1; sector <= num_sectors; sector++) {
        float angle = 2.0f*pi*((float) sector/num_sectors);
        data[data_offset+sector*2] = radius * cosf(angle) + center_x;
        data[data_offset+sector*2+1] = radius * sinf(angle) + center_y;
    }
    for(int sector = 0; sector < num_sectors; sector++) {
        indices[indices_offset+sector*3] = ebo_offset;
        indices[indices_offset+sector*3+1] = sector+1+ebo_offset;
        if(sector+2 > num_sectors) {
            indices[indices_offset+sector*3+2] = 1+ebo_offset;
        } else {
            indices[indices_offset+sector*3+2] = sector+2+ebo_offset;
        }
    }
}

void dataInit(unsigned int* VAO, int num_circles, float* center_x, float* center_y, float* radii) {
    // float data[] = {
    // //  vertices     colors            texcoords
    //     1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // tr
    //     1.0f, -1.0f, 0.0f, 0.0f, 0.f, 1.0f, 0.0f, // br
    //     -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // bl
    //     -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f // tl
    // };
    int data_size = 2*(num_sectors+1)*num_circles*sizeof(float);
    int indices_size = 3*num_sectors*num_circles*sizeof(unsigned int);
    float* data = (float*) malloc(data_size);
    unsigned int* indices = (unsigned int*) malloc(indices_size);

    for (int i = 0; i < num_circles; i++) {
        circleInit(data, indices, i, center_x[i], center_y[i], radii[i]);
    }
    // for (int i = 0; i < 3*num_sectors*num_circles; i+=3) {
    //     printf("Index %d: (%d, %d, %d)\n", i/3, indices[i], indices[i+1], indices[i+2]);
    // }

    // for (int i = 0; i < 2*(num_sectors+1)*num_circles; i+=2) {
    //     printf("Index %d: (%f, %f)\n", i/2, data[i], data[i+1]);
    // }
    
    unsigned int VBO, EBO;

    glGenVertexArrays(1, VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(*VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, data_size, data, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(2*sizeof(float)));
    // glEnableVertexAttribArray(1);
    // glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(5*sizeof(float)));
    // glEnableVertexAttribArray(2);
    free(data);
    free(indices);
}

// unsigned int genTextures() {
//     int width, height, channelCount;
//     unsigned char *data = stbi_load("image1.png", &width, &height, &channelCount, 0);

//     if(!data) {
//         fprintf(stderr, "Failed to load texture");
//     }

//     unsigned int texture;
//     glGenTextures(1, &texture);
//     glBindTexture(GL_TEXTURE_2D, texture);

//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
//     glGenerateMipmap(GL_TEXTURE_2D);

//     stbi_image_free(data);
//     return texture;
// }

int render(GLFWwindow* window, unsigned int* VAO, unsigned int program, int num_circles, float* center_x, float* center_y, float* radii) {
    dataInit(VAO, num_circles, center_x, center_y, radii);
    draw(program, *VAO, num_circles);
    glfwSwapBuffers(window);
    glfwPollEvents();
    return 0;
}