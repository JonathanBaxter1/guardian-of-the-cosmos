#version 330 core

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 info;
layout (location = 2) in float rotation;

uniform float time;

uniform vec2 playerLocation;
uniform mat4 playerRotationMatrix;
uniform mat4 wormholeRotationMatrix;
uniform float aspectRatio;

uniform mat4 playerBulletRotationMatrices[64];
uniform vec4 enemyBulletLocations[128];
uniform mat4 enemyBulletRotationMatrices[128];

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
		gl_Position = playerRotationMatrix*position;
	} else if (type == 0.1) { // Enemy
		gl_Position = rotate(info[2])*position;
		gl_Position[0] += info[0] - playerLocation[0];
		gl_Position[1] += info[1] - playerLocation[1];
//		gl_Position[0] += enemyLocations[gl_InstanceID][0] - playerLocation[0];
//		gl_Position[1] += enemyLocations[gl_InstanceID][1] - playerLocation[1];
//		gl_Position[1] += enemyLocations[gl_InstanceID][3]*1000.0;
	} else if (type == 0.2) { // Wormhole
		gl_Position = wormholeRotationMatrix*position;
		gl_Position[0] += info[0] - playerLocation[0];
		gl_Position[1] += info[1] - playerLocation[1];
	} else if (type == 0.3) { // Relative to Player
		gl_Position = position;
		gl_Position[0] -= playerLocation[0];
		gl_Position[1] -= playerLocation[1];
	} else if (type == 0.4) { // Player Bullet
		gl_Position = rotate(info[2])*position;
		gl_Position[0] += info[0] - playerLocation[0];
		gl_Position[1] += info[1] - playerLocation[1];
//		gl_Position[0] += playerBulletLocations[gl_InstanceID][0] - playerLocation[0];
//		gl_Position[1] += playerBulletLocations[gl_InstanceID][1] - playerLocation[1];
//		gl_Position[1] += playerBulletLocations[gl_InstanceID][3]*1000.0;
	} else if (type == 0.5) { // Enemy Bullet
		gl_Position = enemyBulletRotationMatrices[gl_InstanceID]*position;
		gl_Position[0] += info[0] - playerLocation[0];
		gl_Position[1] += info[1] - playerLocation[1];
//		gl_Position[0] += enemyBulletLocations[gl_InstanceID][0] - playerLocation[0];
//		gl_Position[1] += enemyBulletLocations[gl_InstanceID][1] - playerLocation[1];
//		gl_Position[1] += enemyBulletLocations[gl_InstanceID][3]*1000.0;
	} else if (type == 0.7) { // Relative to Player Instanced (Asteroids)
		float rotationRate = (gl_InstanceID%16-8)/4.0;
		gl_Position = rotate(rotationRate*time + info[2])*position;
		gl_Position[0] += info[0] - playerLocation[0];
		gl_Position[1] += info[1] - playerLocation[1];
	}
	gl_Position[0] *= 1.0/aspectRatio;
};
