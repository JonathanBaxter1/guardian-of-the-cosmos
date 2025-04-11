#version 330 core

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 info;
//layout (location = 2) in float rotation;

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
	int type = int(position[2]*10.0);
	switch (type) {
	case 0: // Player
		gl_Position = rotate(playerAngle)*position;
		break;
	case 1: // Relative to player
		gl_Position = rotate(info[2])*position;
		gl_Position[0] += info[0] - playerLocation[0];
		gl_Position[1] += info[1] - playerLocation[1];
		break;
	case 2: // Wormhole
		gl_Position = rotate(time*64.0)*position;
		gl_Position[0] += info[0] - playerLocation[0];
		gl_Position[1] += info[1] - playerLocation[1];
		break;
	case 3: // Relative to player (single instance)
		gl_Position = position;
		gl_Position[0] -= playerLocation[0];
		gl_Position[1] -= playerLocation[1];
		break;
	case 4: // Asteroids
		float rotationRate = (gl_InstanceID%16-8)/4.0;
		gl_Position = rotate(rotationRate*time + info[2])*position;
		gl_Position[0] += info[0] - playerLocation[0];
		gl_Position[1] += info[1] - playerLocation[1];
		break;
	}
	gl_Position[0] *= 1.0/aspectRatio;
};
