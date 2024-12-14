#version 330 core

layout(location = 0) in vec4 position;

uniform vec4 enemyLocations[6];
uniform mat4 rotationMatrix;

void main()
{
	gl_Position = rotationMatrix*position;
	gl_Position[0] += enemyLocations[gl_InstanceID][0];
	gl_Position[1] += enemyLocations[gl_InstanceID][1];
};
