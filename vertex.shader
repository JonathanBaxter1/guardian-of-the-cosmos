#version 330 core

layout (location = 0) in vec4 position;
layout (location = 1) in vec2 offset;
layout (location = 2) in mat4 playerBulletRotationMatrix;

uniform vec4 enemyLocations[6];
uniform mat4 enemyRotationMatrices[6];
uniform vec2 wormholeLocations[3];
uniform vec2 playerLocation;
uniform mat4 playerRotationMatrix;
uniform mat4 wormholeRotationMatrix;
uniform float aspectRatio;

uniform vec4 playerBulletLocations[64];
uniform mat4 playerBulletRotationMatrices[64];
uniform vec4 enemyBulletLocations[128];
uniform mat4 enemyBulletRotationMatrices[128];

uniform vec2 asteroidLocations[256];


void main()
{
	float type = position[2];
	if (type == 0.0) { // Player
		gl_Position = playerRotationMatrix*position;
	} else if (type == 0.1) { // Enemy
		gl_Position = enemyRotationMatrices[gl_InstanceID]*position;
		gl_Position[0] += enemyLocations[gl_InstanceID][0] - playerLocation[0];
		gl_Position[1] += enemyLocations[gl_InstanceID][1] - playerLocation[1];
		gl_Position[1] += enemyLocations[gl_InstanceID][3]*1000.0;
	} else if (type == 0.2) { // Wormhole
		gl_Position = wormholeRotationMatrix*position;
		gl_Position[0] += wormholeLocations[gl_InstanceID][0] - playerLocation[0];
		gl_Position[1] += wormholeLocations[gl_InstanceID][1] - playerLocation[1];
	} else if (type == 0.3) { // Boundary
		gl_Position = position;
		gl_Position[0] -= playerLocation[0];
		gl_Position[1] -= playerLocation[1];
	} else if (type == 0.4) { // Player Bullet
		gl_Position = playerBulletRotationMatrix*position;
		gl_Position[0] += playerBulletLocations[gl_InstanceID][0] - playerLocation[0];
		gl_Position[1] += playerBulletLocations[gl_InstanceID][1] - playerLocation[1];
		gl_Position[1] += playerBulletLocations[gl_InstanceID][3]*1000.0;
	} else if (type == 0.5) { // Enemy Bullet
		gl_Position = enemyBulletRotationMatrices[gl_InstanceID]*position;
		gl_Position[0] += enemyBulletLocations[gl_InstanceID][0] - playerLocation[0];
		gl_Position[1] += enemyBulletLocations[gl_InstanceID][1] - playerLocation[1];
		gl_Position[1] += enemyBulletLocations[gl_InstanceID][3]*1000.0;
	} else if (type == 0.6) { // Asteroid
		gl_Position = position;
		gl_Position[0] += asteroidLocations[gl_InstanceID][0] - playerLocation[0];
		gl_Position[1] += asteroidLocations[gl_InstanceID][1] - playerLocation[1];
	} else if (type == 0.7) { // Test
		gl_Position = position;
		gl_Position[0] += offset[0];
		gl_Position[1] += offset[1];
	}
	gl_Position[0] *= 1.0/aspectRatio;
};
