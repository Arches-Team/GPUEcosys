#version 430 core

#ifdef VERTEX_SHADER
in vec4 Position;

layout(location = 0) uniform mat4 ViewMatrix;
layout(location = 1) uniform mat4 ProjMatrix;

void main()
{
    gl_Position = ProjMatrix * ViewMatrix * Position;
}
#endif

#ifdef FRAGMENT_SHADER
out vec4 color;

layout (location = 2) uniform vec4 FragColor;

void main()
{
	color = FragColor;
}
#endif