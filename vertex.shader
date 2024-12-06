#version 330 core

layout(location = 0) in vec4 position;

uniform mat4 rotationMatrix;

void main()
{
	gl_Position = rotationMatrix*position;
	gl_Position[0] += (gl_InstanceID%160)/100.0 - 1.0;
};
