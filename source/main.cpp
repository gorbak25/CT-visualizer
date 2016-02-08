#define GLEW_STATIC

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw_gl3.h"
#include "twod_shad.h"
#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>

using namespace std;

namespace Twood
{
static bool TwoodReady = false;
static GLuint prog;
static GLuint vert, frag;
static GLint model, view, proj, doTexture;

class Twood
{
private:    
    GLuint vao;
    GLuint vbo;
    GLuint ebo;

public:
    Twood()
    {
        if(!TwoodReady)
        {
            //create shaders
            vert = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vert, 1, &twod_vert, NULL);
            glCompileShader(vert);

            frag = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(frag, 1, &twod_frag, NULL);
            glCompileShader(frag);

            //assemble pipeline
            prog = glCreateProgram();
            glAttachShader(prog, vert);
            glAttachShader(prog, frag);
            glLinkProgram(prog);
            glUseProgram(prog);

            //generate uniforms
            model = glGetUniformLocation(prog, "model");
            view = glGetUniformLocation(prog, "view");
            proj = glGetUniformLocation(prog, "proj");
            doTexture = glGetUniformLocation(prog, "doTexture");
            TwoodReady = true;
        }

        //generate VBO, VAO and EBO
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

        

        //configure pipeline
        GLint position = glGetAttribLocation(prog, "position");
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, 9*sizeof(float), 0);

        GLint texcoord = glGetAttribLocation(prog, "texcoord");
        glEnableVertexAttribArray(texcoord);
        glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE, 9*sizeof(float), (void*)(3*sizeof(float)));

        GLint color = glGetAttribLocation(prog, "color");
        glEnableVertexAttribArray(color);
        glVertexAttribPointer(color, 4, GL_FLOAT, GL_FALSE, 9*sizeof(float), (void*)(5*sizeof(float)));
    }

    ~Twood()
    {
        if(TwoodReady)
        {
            glUseProgram(0);
            glDetachShader(prog, vert);
            glDetachShader(prog, frag);
            glDeleteShader(vert);
            glDeleteShader(frag);
            glDeleteProgram(prog);
            TwoodReady = false;
        }
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glDeleteVertexArrays(1, &vao);
    }

    static void setModel(glm::mat4 mod)
    {
        glUniformMatrix4fv(model, 1, GL_FALSE, glm::value_ptr(mod));
    }
    static void setView(glm::mat4 vie)
    {
        glUniformMatrix4fv(view, 1, GL_FALSE, glm::value_ptr(vie));
    }
    static void setProj(glm::mat4 pro)
    {
        glUniformMatrix4fv(proj, 1, GL_FALSE, glm::value_ptr(pro));
    }
    static void setDoTexture(bool dec)
    {
        glUniform1i(doTexture, (dec==true?1:0));
    }

    //x,y,z,u,v,r,g,b,a
    void uploadVertex(float* tab, size_t size, GLenum memory_type = GL_STATIC_DRAW)
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, size, tab, memory_type);
    }

    void uploadEBO(GLuint* tab, size_t size, GLenum memory_type = GL_STATIC_DRAW)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, tab, memory_type);
    }

    static void activate_pipeline()
    {
        if(TwoodReady) glUseProgram(prog);
    }

    void bind()
    {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    }

    void draw(GLenum type, int vertex_count)
    {
        glDrawElements(type, vertex_count, GL_UNSIGNED_INT, 0);
    }
};
}

glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
GLfloat yaw   = -90.0f; 
GLfloat pitch =   0.0f;
bool keys[1024];
bool firstMouse = true;
double lastX, lastY;
double curX, curY;
bool leftMouseButton = false;
int windowWidth = 1280;
int windowHeight = 720;
float fov = 45.0f;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mode);
    if(action == GLFW_PRESS)
      keys[key] = true;
    else if(action == GLFW_RELEASE)
      keys[key] = false;  
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    ImGui_ImplGlfwGL3_MouseButtonCallback(window, button, action, mods);
    if(button==GLFW_MOUSE_BUTTON_1 && action==GLFW_PRESS)
    {
        leftMouseButton = true;
        lastX = curX;
        lastY = curY;
    }
    else if(button==GLFW_MOUSE_BUTTON_1 && action==GLFW_RELEASE) leftMouseButton = false;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    curX = xpos; curY = ypos;
    if(leftMouseButton)
    {
        if(firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }
      
        GLfloat xoffset = xpos - lastX;
        GLfloat yoffset = lastY - ypos; 
        lastX = xpos;
        lastY = ypos;

        GLfloat sensitivity = 0.1;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

       yaw   += xoffset;
        pitch += yoffset;

        if(pitch > 89.0f)
            pitch = 89.0f;
        if(pitch < -89.0f)
            pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }
}  

void window_size_callback(GLFWwindow* window, int width, int height)
{
    windowHeight = height;
    windowWidth = width;
    glViewport(0,0,windowWidth,windowHeight);
    Twood::Twood::setProj(glm::perspective(glm::radians(fov), float(windowWidth) / windowHeight, 0.1f, 10.0f));
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGui_ImplGlfwGL3_ScrollCallback(window, xoffset, yoffset);
    if(fov >= 1.0f && fov <= 45.0f)
        fov -= yoffset;
    if(fov <= 1.0f)
        fov = 1.0f;
    if(fov >= 45.0f)
        fov = 45.0f;

    Twood::Twood::setProj(glm::perspective(glm::radians(fov), float(windowWidth) / windowHeight, 0.1f, 10.0f));
}

void do_movement()
{
    // Camera controls
    GLfloat cameraSpeed = 5.0f * deltaTime;
    if(keys[GLFW_KEY_W])
        cameraPos += cameraSpeed * cameraFront;
    if(keys[GLFW_KEY_S])
        cameraPos -= cameraSpeed * cameraFront;
    if(keys[GLFW_KEY_A])
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if(keys[GLFW_KEY_D])
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if(keys[GLFW_KEY_Q])
        cameraPos += cameraSpeed * cameraUp;
    if(keys[GLFW_KEY_E])
        cameraPos -= cameraSpeed * cameraUp;
    if(cameraPos.x > 3.0) cameraPos.x = 3.0;
    if(cameraPos.y > 3.5) cameraPos.y = 3.5;
    if(cameraPos.z > 3.0) cameraPos.z = 3.0;
    if(cameraPos.x < -3.0) cameraPos.x = -3.0;
    if(cameraPos.y < -3.5) cameraPos.y = -3.5;
    if(cameraPos.z < -3.0) cameraPos.z = -3.0;
}

void error_callback(int error, const char* description)
{
   fprintf(stderr, "Error %d: %s\n", error, description);
}

void PrepareImages();

const unsigned int img_width = 512;
vector<int16_t*> data;
unsigned char** images = NULL;
cv::Mat* cv_images;
cv::Mat* cv_edges;
GLuint* slice_textures;
GLuint* slice_edges_textures;
int** vertex_map;

void loadData(string name)
{
    ifstream file;
    file.open(name.c_str(), ios_base::binary);
    if(!file.good()) throw string("Cannot Load File: ")+name;
    file.seekg(0);
    
    char align;
    file.read(&align, sizeof(char));
        
    while(true)
    {
        int16_t* slice = new int16_t[img_width*img_width];    
        
        file.read(reinterpret_cast<char*>(slice), img_width*img_width*sizeof(int16_t));
        data.push_back(slice);
        //cout << slice[0] << "  " << slice[1] << endl;
        if(file.eof()) break;
    }

    file.close();

    images = new unsigned char*[data.size()];
    vertex_map = new int*[data.size()];
    for(int i = 0; i<data.size(); i++)
    {
        images[i] = new unsigned char[4*img_width*img_width];
        vertex_map[i] = new int[img_width*img_width];
    }

    cv_images = new cv::Mat[data.size()];
    cv_edges = new cv::Mat[data.size()];
}

const unsigned int num_of_colors = 5;
ImVec4 gradient_colors[num_of_colors] = {
    ImColor(0,0,255),
    ImColor(135,206,250),
    ImColor(0,70,0),
    ImColor(255,255,0),
    ImColor(255,0,0)
};

float LinearInterpolate(float a, float b, float weight)
{
    return (1-weight)*a + weight*b;
}

ImVec4 LinearInterpolate(ImVec4 a, ImVec4 b, float weight)
{
    ImVec4 res;
    res.x = LinearInterpolate(a.x, b.x, weight);
    res.y = LinearInterpolate(a.y, b.y, weight);
    res.z = LinearInterpolate(a.z, b.z, weight);
    return res;
}

int16_t max_img_val;
int16_t min_img_val;
int img_crop[2];
float cannyParams[2] = {100, 3};

void PrepareImage(unsigned int slice)
{
    //cout << data.size();
    if(slice >= data.size()) throw string("Slice Out Of Bounds");

    int16_t* tmp = data[slice];
    max_img_val = tmp[0];
    min_img_val = tmp[0];

    for(unsigned int i = 1; i<img_width*img_width; i++)
    {
        max_img_val = max(max_img_val, tmp[i]);
        min_img_val = min(min_img_val, tmp[i]);
    }

   // cout << max_val << endl;

    for(unsigned int i = 0; i<img_width*img_width; i++)
    {
        if(tmp[i]>=img_crop[0] && tmp[i]<=img_crop[1])
        {
            float dist = float(tmp[i]-img_crop[0])/(img_crop[1]-img_crop[0]);
            float step = 1.0f/(num_of_colors-1);
            unsigned int col_num = floor(dist/step);
            
            ImVec4 cur_col = LinearInterpolate(gradient_colors[col_num], gradient_colors[col_num+1], (dist-step*col_num)/step);
            
            //cout << dist-step*col_num << endl;

            images[slice][4*i + 0] = cur_col.x*255;
            images[slice][4*i + 1] = cur_col.y*255;
            images[slice][4*i + 2] = cur_col.z*255;
            images[slice][4*i + 3] = 255;
        }
        else
        {
            images[slice][4*i + 0] = images[slice][4*i + 1] = images[slice][4*i + 2] = 0;
            images[slice][4*i + 3] = 0;
        }
    }

    //images[0][0]=255;

    cv_images[slice] = cv::Mat(img_width, img_width, CV_8UC4, images[slice]);

    cv::Mat grey_img, edges;
    cv_edges[slice].create( cv_images[slice].size(), cv_images[slice].type());
    cv::cvtColor(cv_images[slice], grey_img, CV_RGBA2GRAY);
    cv::blur(grey_img, edges, cv::Size(2,2));
    cv::Canny(edges, edges, cannyParams[0], cannyParams[0]*cannyParams[1], 3);
    cv_edges[slice] = cv::Scalar::all(0);
    cv_images[slice].copyTo(cv_edges[slice], edges);
}

void PrepareImages()
{
    for(unsigned int i = 0; i<data.size(); i++)
        PrepareImage(i);
}

vector<float> vertexes;
vector<GLuint> vertexes_ebo;

void GenerateVertexes()
{
    int count = 0;
    for(int i = 0; i<data.size(); i++)
    {
        cv::Mat* ref = &cv_edges[i];
        unsigned char* tmp = ref->ptr();

        for(int y = 0; y<img_width; y++)
        {
            for(int x = 0; x<img_width; x++)
            {
                if(
                    tmp[(y*img_width + x)*4 + 0] != 0 && 
                    tmp[(y*img_width + x)*4 + 1] != 0 && 
                    tmp[(y*img_width + x)*4 + 2] != 0 )
                {
                    vertex_map[i][y*img_width + x] = count;
                    //x, y, z
                    vertexes.push_back(float(-x)/img_width+0.5f);
                    vertexes.push_back(float(i)/data.size()-0.5f);
                    vertexes.push_back(float(-y)/img_width+0.5f);

                    //u, v
                    vertexes.push_back(0.0f);
                    vertexes.push_back(0.0f);
                    //r, g, b, a
                    vertexes.push_back(float(tmp[(y*img_width + x)*4 + 0])/255);
                    vertexes.push_back(float(tmp[(y*img_width + x)*4 + 1])/255);
                    vertexes.push_back(float(tmp[(y*img_width + x)*4 + 2])/255);
                    vertexes.push_back(float(tmp[(y*img_width + x)*4 + 3])/255);

                    //tmp
                    vertexes_ebo.push_back(count);
                    count++;

                }
                else
                {
                    vertex_map[i][y*img_width + x] = -1;
                }
            }
        }
    }
}

int main(int argn, char** args)
{
    if(argn != 2)
    {
        cout << "Usage: ./visualize [vnd_data_file]" << endl;
        return 0;
    }
    try{
    loadData(string(args[1]));
    //cout << data[0][0] << endl;
    // Setup window
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Visualize!", NULL, NULL);

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, &key_callback);
    glfwSetCursorPosCallback(window, &mouse_callback);
    glfwSetMouseButtonCallback(window, &mouse_button_callback);
    glfwSetCharCallback(window, &ImGui_ImplGlfwGL3_CharCallback);
    glfwSetScrollCallback(window, &scroll_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
	glewInit();

    // Setup ImGui binding
    ImGui_ImplGlfwGL3_Init(window, false);

    //Setup Textures
    slice_textures = new GLuint[data.size()];
    slice_edges_textures = new GLuint[data.size()];
    glGenTextures(data.size(), slice_textures);
    glGenTextures(data.size(), slice_edges_textures);
    for(int i = 0; i<data.size(); i++)
    {
        glBindTexture(GL_TEXTURE_2D, slice_textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, slice_edges_textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_width, 0, GL_RGBA, GL_UNSIGNED_BYTE, cv_edges[i].ptr());
    }
    PrepareImage(0);
    img_crop[0] = min_img_val;
    img_crop[1] = max_img_val;
    PrepareImages();

    for(int i = 0; i<data.size(); i++)
    {
        glBindTexture(GL_TEXTURE_2D, slice_textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_width, 0, GL_RGBA, GL_UNSIGNED_BYTE, images[i]);

        glBindTexture(GL_TEXTURE_2D, slice_edges_textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_width, 0, GL_RGBA, GL_UNSIGNED_BYTE, cv_edges[i].ptr());
    }

    // Setup wireframe
    Twood::Twood wireframe;
    wireframe.bind();
    wireframe.uploadEBO(e_box, sizeof(e_box));
    wireframe.uploadVertex(v_box, sizeof(v_box));

    //Setup slice
    Twood::Twood slice;
    slice.bind();
    slice.uploadEBO(e_horiz, sizeof(e_horiz));
    slice.uploadVertex(v_horiz, sizeof(v_horiz));

    //Setup points
    GenerateVertexes();
    Twood::Twood mesh;
    mesh.bind();
    mesh.uploadVertex(vertexes.data(), vertexes.size()*sizeof(float));
    mesh.uploadEBO(vertexes_ebo.data(), vertexes_ebo.size()*sizeof(GLuint));

    Twood::Twood::setDoTexture(false);
    Twood::Twood::setModel(glm::mat4());
    Twood::Twood::setProj(glm::perspective(glm::radians(45.0f), float(windowWidth) / windowHeight, 0.1f, 10.0f));
    Twood::Twood::setView(glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp));


    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_POINT_SIZE);
    glEnable(GL_POINT_SMOOTH);
    //glBlendFunc (GL_ONE, GL_ONE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_ONE_MINUS_DST_ALPHA,GL_DST_ALPHA);
    //glDepthFunc(GL_ALWAYS);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    //Setup Framebuffer
    GLuint neuralFramebuffer;
    glGenFramebuffers(1, &neuralFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, neuralFramebuffer);

    const int neuralNetworkBufferWidth = 512;

    GLuint neuralTexture;
    glGenTextures(1, &neuralTexture);
    glBindTexture(GL_TEXTURE_2D, neuralTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, neuralNetworkBufferWidth, neuralNetworkBufferWidth, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    GLuint neuralDepthBuffer;
    glGenRenderbuffers(1, &neuralDepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, neuralDepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, neuralNetworkBufferWidth, neuralNetworkBufferWidth);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, neuralDepthBuffer);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, neuralTexture, 0);
    GLenum neuralDrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, neuralDrawBuffers);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw string("Failed to configure framebuffer");

    glViewport(0,0,neuralNetworkBufferWidth,neuralNetworkBufferWidth);

    unsigned char img_tmp[4*neuralNetworkBufferWidth*neuralNetworkBufferWidth];

    bool one_time = false;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;  
        do_movement();
        
        ImGui_ImplGlfwGL3_NewFrame();

        static float rot = 0.0f;
        static int s_cur = 1.0f;
        static bool display_canny = false;
        static bool all_canny = false;
        static bool hide_slice = false;
        static int how_many = vertexes_ebo.size();
        {
            ImGui::Begin("Params");
            
            ImGui::SliderFloat("Rotation", &rot, 0.0f, 180.0f);
            ImGui::SliderInt("Slice", &s_cur, 1, int(data.size())-1);
            ImGui::SliderInt2("Image Crop", (int*)img_crop, min_img_val, max_img_val);
            ImGui::SliderFloat("Low Threshold", &cannyParams[0], 0.0f, 100.0f);
            ImGui::SliderFloat("Threshold Ratio", &cannyParams[1], 1.0f, 5.0f);
            ImGui::SliderInt("Point count", &how_many, 0, vertexes_ebo.size());
            
            ImGui::Value("camPos.x", cameraPos.x);
            ImGui::Value("camPos.y", cameraPos.y);
            ImGui::Value("camPos.z", cameraPos.z);

            ImGui::Value("camFront.x", cameraFront.x);
            ImGui::Value("camFront.y", cameraFront.y);
            ImGui::Value("camFront.z", cameraFront.z);

            if(ImGui::Button("One time")) one_time = false;
            if(ImGui::Button("Orig/Canny")) display_canny ^= 1;
            if(ImGui::Button("All Canny's")) all_canny ^= 1;
            if(ImGui::Button("Hide Slice")) hide_slice ^= 1;
            if(ImGui::Button("Recalculate"))
            {
                PrepareImages();
                for(int i = 0; i<data.size(); i++)
                {
                    glBindTexture(GL_TEXTURE_2D, slice_textures[i]);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_width, 0, GL_RGBA, GL_UNSIGNED_BYTE, images[i]);

                    glBindTexture(GL_TEXTURE_2D, slice_edges_textures[i]);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_width, 0, GL_RGBA, GL_UNSIGNED_BYTE, cv_edges[i].ptr());
                }
            }

            if(display_canny) glBindTexture(GL_TEXTURE_2D, slice_edges_textures[s_cur]);
            else glBindTexture(GL_TEXTURE_2D, slice_textures[s_cur]);

            for(int i = 0; i<num_of_colors; i++)
                ImGui::ColorEdit3(to_string(i+1).c_str(), (float*)&gradient_colors[i]);
            
            if(ImGui::GetWindowIsFocused() && ImGui::IsAnyItemActive())
            {
                leftMouseButton = false;
            }

            ImGui::End();
        }
        glm::mat4 trans;
            trans = glm::rotate(
            trans,
            glm::radians(rot),
            glm::vec3(0.0f, 1.0f, 0.0f));

        // Rendering
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        Twood::Twood::activate_pipeline();
        Twood::Twood::setView(glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp));
        glEnable(GL_DEPTH_TEST);

        wireframe.bind();
        Twood::Twood::setDoTexture(false);
        Twood::Twood::setModel(glm::scale(trans,glm::vec3(2,2,2)));
        wireframe.draw(GL_LINES, 24);

        if(!hide_slice)
        {
            slice.bind();
            Twood::Twood::setDoTexture(true);
            glActiveTexture(GL_TEXTURE0);
            if(all_canny)
            {
                glDisable(GL_DEPTH_TEST);
                for(int i = 0; i<s_cur; i++)
                {
                    Twood::Twood::setModel(glm::translate(glm::scale(trans,glm::vec3(2,2,2)),glm::vec3(0,float(i)/data.size(),0)));
                    glBindTexture(GL_TEXTURE_2D, slice_edges_textures[i]);
                    slice.draw(GL_TRIANGLES, 6);
                }
            }
            else
            {
                Twood::Twood::setModel(glm::translate(glm::scale(trans,glm::vec3(2,2,2)),glm::vec3(0,float(s_cur)/data.size(),0)));
                slice.draw(GL_TRIANGLES, 6);
            }
        }
        if(hide_slice)
        {
            mesh.bind();
            glEnable(GL_POINT_SIZE);
            glEnable(GL_POINT_SMOOTH);
            Twood::Twood::setDoTexture(false);
            Twood::Twood::setModel(glm::scale(trans,glm::vec3(2,2,2)));
            mesh.draw(GL_POINTS, how_many);
        }

        ImGui::Render();

        //render mesh to buffer
        if(!one_time)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, neuralFramebuffer);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glViewport(0,0,neuralNetworkBufferWidth,neuralNetworkBufferWidth);
            glClearColor(0.0f,0.0f,0.0f,1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            mesh.bind();
            Twood::Twood::setDoTexture(false);
            Twood::Twood::setModel(glm::scale(trans,glm::vec3(2,2,2)));
            Twood::Twood::setView(glm::lookAt(glm::vec3(-0.042f, 3.5f,  0.042f), glm::vec3(-0.042f, 2.5f,  0.025f), cameraUp));
            Twood::Twood::setProj(glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 10.0f));
            mesh.draw(GL_POINTS, how_many);

            glBindTexture(GL_TEXTURE_2D, neuralTexture);
            glReadPixels(0, 0, neuralNetworkBufferWidth, neuralNetworkBufferWidth, GL_BGRA, GL_UNSIGNED_BYTE, img_tmp);
            //glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_tmp);
            cv::Mat cv_img_tmp = cv::Mat(neuralNetworkBufferWidth,neuralNetworkBufferWidth,CV_8UC4,img_tmp);
            cv::flip(cv_img_tmp, cv_img_tmp, 0);
            cv::imwrite("test.bmp", cv_img_tmp);
            one_time=true;

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            Twood::Twood::setProj(glm::perspective(glm::radians(45.0f), float(windowWidth) / windowHeight, 0.1f, 10.0f));
            glViewport(0,0,windowWidth,windowHeight);
        }

        glfwSwapBuffers(window);        
    }

    // Cleanup
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &neuralFramebuffer);
    glDeleteTextures(1, &neuralTexture);
    glDeleteTextures(1, &neuralDepthBuffer);

    glDeleteTextures(data.size(), slice_textures);
    glDeleteTextures(data.size(), slice_edges_textures);
    delete [] slice_textures;
    delete [] slice_edges_textures;

    for(int i = 0; i<data.size(); i++)
    {
        delete [] images[i];
        delete [] vertex_map[i];
    }
    delete [] images;
    delete [] vertex_map;
    delete [] cv_images;
    delete [] cv_edges;

    ImGui_ImplGlfwGL3_Shutdown();
    glfwTerminate();
    
    }
    catch(string err)
    {
        cerr << err << endl;
    }

    return 0;
}
