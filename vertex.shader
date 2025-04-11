#version 330 core

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 info;
layout (location = 2) in float rotation;

uniform float time;
uniform float aspectRatio;

uniform vec2 playerLocation;
uniform float playerAngle;

mat4 rotate(float angle)
{
	return mat4(
		cos(angle), -sin(angle), 0.0, 0.0,
		sin(angle), cos(angle), 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0
	);
}


void main()
{
	float type = position[2];
	if (type == 0.0) { // Player
		gl_Position = rotate(playerAngle)*position;
	} else if (type == 0.1) { // Relative to Player
		gl_Position = rotate(info[2])*position;
		gl_Position[0] += info[0] - playerLocation[0];
		gl_Position[1] += info[1] - playerLocation[1];
	} else if (type == 0.2) { // Wormhole
		gl_Position = rotate(time*64.0)*position;
		gl_Position[0] += info[0] - playerLocation[0];
		gl_Position[1] += info[1] - playerLocation[1];
	} else if (type == 0.3) { // Relative to Player (single instance)
		gl_Position = position;
		gl_Position[0] -= playerLocation[0];
		gl_Position[1] -= playerLocation[1];
	} else if (type == 0.4) { // Asteroids
		float rotationRate = (gl_InstanceID%16-8)/4.0;
		gl_Position = rotate(rotationRate*time + info[2])*position;
		gl_Position[0] += info[0] - playerLocation[0];
		gl_Position[1] += info[1] - playerLocation[1];
	}
	gl_Position[0] *= 1.0/aspectRatio;
};
