
#define GLSL(src) "#version 330 core\n" #src
#include <GL/glew.h>

const GLchar* twod_vert = GLSL(
in vec3 position;
in vec2 texcoord;
in vec4 color;

out vec3 VPosition;
out vec2 VTexcoord;
out vec4 VColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() 
{
    VColor = color;
    VTexcoord = texcoord;
    VPosition = position;
    gl_Position = proj * view * model * vec4(position, 1.0);
    gl_PointSize = 5.0;
}
);

const GLchar* twod_frag = GLSL(
in vec3 VPosition;
in vec2 VTexcoord;
in vec4 VColor;

layout(location = 0) out vec4 outColor;

uniform int doTexture;
uniform sampler2D tex;

void main() 
{
    vec4 col = VColor;
    
    if(doTexture>0)
    {
        col *= texture(tex, VTexcoord);
    }

    outColor = col;
}
);

float v_box[] = 
{
    -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
     0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
     0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,

    -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
     0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
     0.5f, 0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    -0.5f, 0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    };
GLuint e_box[] = {0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7};

float v_horiz[] = 
{
    -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
     0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
     0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
};
GLuint e_horiz[] = {0,1,2, 3,0,2};