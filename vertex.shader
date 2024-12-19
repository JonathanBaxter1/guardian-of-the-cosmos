#version 330 core

layout(location = 0) in vec4 position;

uniform vec4 enemyLocations[6];
uniform vec2 playerLocation;
uniform mat4 rotationMatrix;
uniform mat4 wormholeRotationMatrix;
uniform mat4 matrices[6];
uniform float aspectRatio;

void main()
{
	float type = position[2];
	if (type == 0.0) { // Player
		gl_Position = rotationMatrix*position;
	} else if (type == 0.1) { // Enemy
		gl_Position = matrices[gl_InstanceID]*position;
		gl_Position[0] += enemyLocations[gl_InstanceID][0] - playerLocation[0];
		gl_Position[1] += enemyLocations[gl_InstanceID][1] - playerLocation[1];
	} else if (type == 0.2) { // Wormhole
		gl_Position = wormholeRotationMatrix*position;
		gl_Position[0] -= playerLocation[0];
		gl_Position[1] -= playerLocation[1];
	}
	gl_Position[0] *= 1.0/aspectRatio;
};
