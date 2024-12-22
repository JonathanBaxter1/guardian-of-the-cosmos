#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <alloca.h>

#define PI 3.14159265358979323846
#define WINDOW_NAME "Guardian of the Cosmos"

// Maximum number of each object type
// Update the vertex shader when any of these values are changed
#define NUM_ENEMIES 6
#define NUM_WORMHOLES 3
#define NUM_PLAYER_BULLETS 64
#define NUM_ENEMY_BULLETS 2048

#define BOUNDARY_SIDES 256
#define BOUNDARY_RADIUS 5.0
#define BULLETS_PER_SECOND 10.0
#define PLAYER_HITBOX_RAD 0.04
#define ENEMY_HITBOX_RAD 0.055
#define PLAYER_BULLET_HITBOX_RAD 0.03

#define Render() do {\
	/* Set background to black */\
	glClear(GL_COLOR_BUFFER_BIT); \
\
	/* Render objects */\
	glDrawElements(GL_LINE_LOOP, sizeof(playerInd)/sizeof(unsigned int), GL_UNSIGNED_INT, (void*)(long)(indOffsets[0])); \
	glDrawElementsInstanced(GL_LINES, sizeof(enemyInd)/sizeof(unsigned int), GL_UNSIGNED_INT, (void*)(long)(indOffsets[1]), NUM_ENEMIES); \
	glDrawElementsInstanced(GL_LINE_LOOP, sizeof(wormholeInd)/sizeof(unsigned int), GL_UNSIGNED_INT, (void*)(long)(indOffsets[2]), NUM_WORMHOLES); \
	glDrawElements(GL_LINE_LOOP, BOUNDARY_SIDES, GL_UNSIGNED_INT, (void*)(long)(indOffsets[3])); \
	glDrawElementsInstanced(GL_LINES, sizeof(playerBulletInd)/sizeof(unsigned int), GL_UNSIGNED_INT, (void*)(long)(indOffsets[4]), NUM_PLAYER_BULLETS); \
\
	/* Swap front and back buffers */ \
	glfwSwapBuffers(window); \
	} while(0)

GLFWwindow* window;
int* nullptr = NULL;

float playerX = 0.0;
float playerY = 0.0;
double playerAngle = 0.0;
float playerVelocityX = 0.0;
float playerVelocityY = 0.0;
float playerHealth = 1.0;

double playerRotationRate = 0.0;
int isShooting = 0;


static unsigned int compileShader(unsigned int type, const char* source)
{
	unsigned int id = glCreateShader(type);
	glShaderSource(id, 1, &source, nullptr);
	glCompileShader(id);

	// Error Checking
	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE) {
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		char* message = (char*)alloca(length*sizeof(char));
		glGetShaderInfoLog(id, length, &length, message);
		printf("Failed to compile shader\n");
		printf("%s\n", message);
		glDeleteShader(id);
		return 0;
	}

	return id;
}

static unsigned int createShader(const char* vertexShader, const char* fragmentShader)
{
	unsigned int program = glCreateProgram();
	unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShader);
	unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glValidateProgram(program);
	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}

static unsigned int createShaderFromFiles(char* vertexFileName, char* fragmentFileName)
{
	FILE *vertexFile = fopen(vertexFileName, "r");
	if (vertexFile == NULL) {
		printf("Could not read vertex shader file: %s\n", vertexFileName);
		glfwTerminate();
		exit(-1);
	}
	char* vertexShader = NULL;
	size_t vertexShaderLen;
	ssize_t vertexBytesRead = getdelim(&vertexShader, &vertexShaderLen, '\0', vertexFile);
	if (vertexBytesRead == -1) {
		printf("Error reading vertex shader file: %s\n", vertexFileName);
		glfwTerminate();
		exit(-1);
	}

	FILE *fragmentFile = fopen(fragmentFileName, "r");
	if (fragmentFile == NULL) {
		printf("Could not read fragment shader file: %s\n", fragmentFileName);
		glfwTerminate();
		exit(-1);
	}
	char* fragmentShader = NULL;
	size_t fragmentShaderLen;
	ssize_t fragmentBytesRead = getdelim(&fragmentShader, &fragmentShaderLen, '\0', fragmentFile);
	if (fragmentBytesRead == -1) {
		printf("Error reading fragment shader file: %s\n", fragmentFileName);
		glfwTerminate();
		exit(-1);
	}

	unsigned int shader = createShader(vertexShader, fragmentShader);
	free(vertexShader);
	free(fragmentShader);
	return shader;
}

int isKeyDown(int key)
{
	return glfwGetKey(window, key) == GLFW_PRESS;
}

void handleKeyboardInput(float deltaT, unsigned int shader)
{
	glfwPollEvents();
	float playerSpeed = 1.2*deltaT;

	/* Escape to Quit Game */
	if (isKeyDown(GLFW_KEY_ESCAPE)) {
		glDeleteProgram(shader);
		glfwTerminate();
		exit(0);
	}

	/* Shooting */
	isShooting = isKeyDown(GLFW_KEY_SPACE);

	/* Player Movement */
	playerRotationRate = 0.0;
	if (isKeyDown(GLFW_KEY_LEFT)) {
		playerRotationRate += -5.0;
	}
	if (isKeyDown(GLFW_KEY_RIGHT)) {
		playerRotationRate += 5.0;
	}
	if (isKeyDown(GLFW_KEY_LEFT_SHIFT)) {
		playerRotationRate *= 0.5;
	}
	int wPressed = isKeyDown(GLFW_KEY_W);
	int aPressed = isKeyDown(GLFW_KEY_A);
	int sPressed = isKeyDown(GLFW_KEY_S);
	int dPressed = isKeyDown(GLFW_KEY_D);

	// Detect if moving on X and Y axis at the same time
	float speedMultiplier = 1.0;
	playerVelocityX = 0.0;
	playerVelocityY = 0.0;
	if (wPressed != sPressed && aPressed != dPressed) {
		speedMultiplier = sqrt(2.0)/2.0;
	}
	if (wPressed) {
		playerVelocityY += speedMultiplier*playerSpeed*cos(playerAngle);
		playerVelocityX += speedMultiplier*playerSpeed*sin(playerAngle);
	}
	if (aPressed) {
		playerVelocityY += speedMultiplier*playerSpeed*sin(playerAngle);
		playerVelocityX -= speedMultiplier*playerSpeed*cos(playerAngle);
	}
	if (sPressed) {
		playerVelocityY -= speedMultiplier*playerSpeed*cos(playerAngle);
		playerVelocityX -= speedMultiplier*playerSpeed*sin(playerAngle);
	}
	if (dPressed) {
		playerVelocityY -= speedMultiplier*playerSpeed*sin(playerAngle);
		playerVelocityX += speedMultiplier*playerSpeed*cos(playerAngle);
	}
	playerX += playerVelocityX;
	playerY += playerVelocityY;
}

int updateRotationMatrix(float angle, float *rotationMatrix)
{
	rotationMatrix[0] = cos(angle);
	rotationMatrix[1] = -sin(angle);
	rotationMatrix[4] = sin(angle);
	rotationMatrix[5] = cos(angle);
	return 0;
}

int updateRotationMatrices(float locations[], float *rotationMatrices, int numElements)
{
	for (int i = 0; i < numElements; i++) {
		float angle = locations[i*4 + 2];
		rotationMatrices[i*16] = cos(angle);
		rotationMatrices[i*16 + 1] = -sin(angle);
		rotationMatrices[i*16 + 4] = sin(angle);
		rotationMatrices[i*16 + 5] = cos(angle);
	}
	return 0;
}

int createCircle(float *circleVertices, unsigned int *circleIndices, float type, int numSides)
{
	for (int i = 0; i < numSides; i++) {
		double angle = (float)i/numSides*2*PI;
		circleVertices[i*3] = cos(angle);
		circleVertices[i*3 + 1] = sin(angle);
		circleVertices[i*3 + 2] = type;
		circleIndices[i] = i;
	}
	return 0;
}

int main(void)
{

	// Initialize glfw
	if (!glfwInit()) {
		return -1;
	}

	// Create a fullscreen window and its OpenGL context
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
	int screenWidth = videoMode->width;
	int screenHeight = videoMode->height;
	float aspectRatio = (float)screenWidth/(float)screenHeight;
	printf("Screen Resolution: %dx%d\n", screenWidth, screenHeight);
	printf("Aspect Ratio: %f\n", aspectRatio);
	window = glfwCreateWindow(screenWidth, screenHeight, WINDOW_NAME, monitor, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	// Make the window's context current
	glfwMakeContextCurrent(window);

	// Turn on vsync
	glfwSwapInterval(1);

	// Initialize glew
	glewInit();

	/* Player Data */
	float playerVert[] = {
		-0.04,	-0.04, 0.0,
		0.04,	-0.04, 0.0,
		0.0,	0.08, 0.0,
	};

	unsigned int playerInd[] = {
		0, 1, 2,
	};

	float playerRotationMatrix[] = {
		cos(playerAngle), -sin(playerAngle), 0.0, 0.0,
		sin(playerAngle), cos(playerAngle), 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0,
	};


	/* Enemy Data */
	float enemyVert[] = {
		-0.05,	-0.2, 0.1,// Middle section
		-0.1,	-0.15, 0.1, // (0 - 6)
		-0.1,	0.3, 0.1,
		0.0,	0.5, 0.1,
		0.1,	0.3, 0.1,
		0.1,	-0.15, 0.1,
		0.05,	-0.2, 0.1,

		-0.1,	-0.1, 0.1,// Left connector
		-0.2,	-0.1, 0.1,// (7 - 10)
		-0.1,	0.1, 0.1,
		-0.2,	0.1, 0.1,

		0.1,	-0.1, 0.1,// Right connector
		0.2,	-0.1, 0.1,// (11 - 14)
		0.1,	0.1, 0.1,
		0.2,	0.1, 0.1,

		-0.2,	-0.15, 0.1, // Left section
		-0.2,	0.15, 0.1, // (15 - 19)
		-0.25,	0.2, 0.1,
		-0.3,	0.15, 0.1,
		-0.3,	-0.15, 0.1,

		0.2,	-0.15, 0.1, // Right section
		0.2,	0.15, 0.1, // (20 - 24)
		0.25,	0.2, 0.1,
		0.3,	0.15, 0.1,
		0.3,	-0.15, 0.1,
	};
	for (int i = 0; i < sizeof(enemyVert)/3/sizeof(float); i++) {
		enemyVert[3*i] *= 0.15;
		enemyVert[3*i + 1] *= 0.15;
	}

	unsigned int enemyInd[] = {
		0, 1, // Middle section
		1, 2,
		2, 3,
		3, 4,
		4, 5,
		5, 6,
		6, 0,

		7, 8, // Connectors
		9, 10,
		11, 12,
		13, 14,

		15, 16, // Left section
		16, 17,
		17, 18,
		18, 19,
		19, 15,

		20, 21, // Right section
		21, 22,
		22, 23,
		23, 24,
		24, 20,
	};
	float enemyHealth[NUM_ENEMIES];
	for (int i = 0; i < NUM_ENEMIES; i++) {
		enemyHealth[i] = 1.0;
	}

	float enemyLocations[4*NUM_ENEMIES] = {
		-4.0, 0.0, 1.0, 0.0,
		0.0, -4.0, 2.0, 0.0,
		4.0, 0.0, 3.0, 0.0,
		1.0, 3.0, 4.0, 0.0,
		0.0, 4.0, 5.0, 0.0,
		-1.0, 3.0, 6.0, 0.0,
	};
	float enemyRotationMatrices[16*NUM_ENEMIES] = {0.0};
	for (int i = 0; i < NUM_ENEMIES; i++) {
		for (int j = 0; j < 4; j++) {
			enemyRotationMatrices[i*16 + j*5] = 1.0;
		}
	}

	/* Wormhole Data */
	float wormholeVert[8*3];
	unsigned int wormholeInd[8];
	createCircle(wormholeVert, wormholeInd, 0.2, 8);

	for (int i = 0; i < sizeof(wormholeVert)/3/sizeof(float); i++) {
		wormholeVert[3*i] *= 0.2;
		wormholeVert[3*i + 1] *= 0.2;
	}

	double wormholeAngle = 0.0;
	float wormholeRotationMatrix[] = {
		cos(wormholeAngle), -sin(wormholeAngle), 0.0, 0.0,
		sin(wormholeAngle), cos(wormholeAngle), 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0,
	};
	float wormholeLocations[2*NUM_WORMHOLES] = {
		-4.0, 0.2,
		4.0, -0.2,
		0.0, 4.3,
	};


	/* Boundary Data */
	float boundaryVert[BOUNDARY_SIDES*3];
	unsigned int boundaryInd[BOUNDARY_SIDES];
	createCircle(boundaryVert, boundaryInd, 0.3, BOUNDARY_SIDES);

	for (int i = 0; i < BOUNDARY_SIDES; i++) {
		boundaryVert[3*i] *= BOUNDARY_RADIUS;
		boundaryVert[3*i + 1] *= BOUNDARY_RADIUS;
	}

	/* Player Bullet Data */

	float playerBulletVert[] = {
		-0.03, 0.0, 0.4,
		-0.03, 0.02, 0.4,
		0.0, 0.0, 0.4,
		0.0, 0.02, 0.4,
		0.03, 0.0, 0.4,
		0.03, 0.02, 0.4,
	};

	unsigned int playerBulletInd[] = {
		0, 1,
		2, 3,
		4, 5,
	};

	float playerBulletLocations[4*NUM_PLAYER_BULLETS];
	for (int i = 0; i < NUM_PLAYER_BULLETS; i++) {
		playerBulletLocations[i*4] = 0.0;
		playerBulletLocations[i*4 + 1] = 0.0;
		playerBulletLocations[i*4 + 2] = 1.0;
		playerBulletLocations[i*4 + 3] = -1.0;
	}
	float playerBulletRotationMatrices[16*NUM_PLAYER_BULLETS] = {0.0};
	for (int i = 0; i < NUM_PLAYER_BULLETS; i++) {
		for (int j = 0; j < 4; j++) {
			playerBulletRotationMatrices[i*16 + j*5] = 1.0;
		}
	}
	float playerBulletVelocities[2*NUM_PLAYER_BULLETS] = {0.0};

	// Setup Buffers
	void *objectDrawData[] = {
		&playerVert, &playerInd,
		&enemyVert, &enemyInd,
		&wormholeVert, &wormholeInd,
		&boundaryVert, &boundaryInd,
		&playerBulletVert, &playerBulletInd,
	};
	unsigned int objectSizeData[] = {
		sizeof(playerVert), sizeof(playerInd),
		sizeof(enemyVert), sizeof(enemyInd),
		sizeof(wormholeVert), sizeof(wormholeInd),
		sizeof(boundaryVert), sizeof(boundaryInd),
		sizeof(playerBulletVert), sizeof(playerBulletInd),
	};

	const unsigned int numObjects = sizeof(objectSizeData)/sizeof(unsigned int)/2;
	if (sizeof(objectDrawData)/sizeof(void*) != sizeof(objectSizeData)/sizeof(unsigned int)) {
		printf("Error: objectDrawData and objectSizeData must be same size\n");
		exit(-1);
	}
	unsigned int vertOffsets[numObjects];
	unsigned int indOffsets[numObjects];
	vertOffsets[0] = 0;
	indOffsets[0] = 0;
	for (int i = 1; i < numObjects; i++) {
		vertOffsets[i] = vertOffsets[i-1] + objectSizeData[i*2-2];
		indOffsets[i] = indOffsets[i-1] + objectSizeData[i*2-1];
	}

	for (int object = 1; object < numObjects; object++) {
		unsigned int* indPointer = objectDrawData[object*2 + 1];
		for (int i = 0; i < objectSizeData[object*2 + 1]/sizeof(unsigned int); i++) {
			indPointer[i] += vertOffsets[object]/sizeof(float)/3;
		}
	}

	// Generate and Bind Vertex Buffer
	unsigned int vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// GL_STATIC_DRAW means vertices will be written once and read many times
	unsigned int VBOsize = 0;
	for (int i = 0; i < numObjects; i++) {
		VBOsize += objectSizeData[i*2];
	}
	glBufferData(GL_ARRAY_BUFFER, VBOsize, 0, GL_STATIC_DRAW);
	for (int i = 0; i < numObjects; i++) {
		glBufferSubData(GL_ARRAY_BUFFER, vertOffsets[i], objectSizeData[i*2], objectDrawData[i*2]);
	}

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*3, 0);

	// Generate and Bind Index Buffer
	unsigned int ibo; // Index Buffer Object
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	// GL_STATIC_DRAW
	unsigned int IBOsize = 0;
	for (int i = 0; i < numObjects; i++) {
		IBOsize += objectSizeData[i*2 + 1];
	}
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, IBOsize, 0, GL_STATIC_DRAW);
	for (int i = 0; i < numObjects; i++) {
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indOffsets[i], objectSizeData[i*2 + 1], objectDrawData[i*2 + 1]);
	}

	// Read shader code from files
	unsigned int shader = createShaderFromFiles("vertex.shader", "fragment.shader");
	glUseProgram(shader);

	// Set Color
	double absColorChangeRate = 1.0;
	double colorChangeRate = absColorChangeRate;
	double redShade = 0.0;
	int colorLocation = glGetUniformLocation(shader, "u_Color");
	glUniform4f(colorLocation, redShade, 0.0, 1.0, 1.0);

	// Shader Variable Locations
	int playerRotationLocation = glGetUniformLocation(shader, "playerRotationMatrix");
	int enemyLocation = glGetUniformLocation(shader, "enemyLocations");
	int playerLocation = glGetUniformLocation(shader, "playerLocation");
	int enemyRotationMatrixLocation = glGetUniformLocation(shader, "enemyRotationMatrices");
	int wormholeMatrixLocation = glGetUniformLocation(shader, "wormholeRotationMatrix");
	int aspectRatioLocation = glGetUniformLocation(shader, "aspectRatio");
	int wormholeLocation = glGetUniformLocation(shader, "wormholeLocations");
	int playerBulletLocation = glGetUniformLocation(shader, "playerBulletLocations");
	int playerBulletRotationMatrixLocation = glGetUniformLocation(shader, "playerBulletRotationMatrices");

	// Set Shader Variables
	glUniformMatrix4fv(playerRotationLocation, 1, GL_FALSE, playerRotationMatrix);
	glUniform4fv(enemyLocation, NUM_ENEMIES, enemyLocations);
	glUniform2f(playerLocation, playerX, playerY);
	glUniformMatrix4fv(enemyRotationMatrixLocation, NUM_ENEMIES, GL_FALSE, enemyRotationMatrices);
	glUniformMatrix4fv(wormholeMatrixLocation, 1, GL_FALSE, wormholeRotationMatrix);
	glUniform1f(aspectRatioLocation, aspectRatio);
	glUniform2fv(wormholeLocation, NUM_WORMHOLES, wormholeLocations);
	glUniform4fv(playerBulletLocation, NUM_PLAYER_BULLETS, playerBulletLocations);
	glUniformMatrix4fv(playerBulletRotationMatrixLocation, NUM_PLAYER_BULLETS, GL_FALSE, playerBulletRotationMatrices);

	double lastTime = glfwGetTime();
	double maxFPS = 0;
	double minFPS = 1000000;
	double avgFPS = 0;
	int frameCount = 0;
	float timeSinceLastBullet = 0.0;

	// Enable Anti-Aliasing
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(1.3);
	glDepthMask(GL_FALSE);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	// Main Loop
	while (!glfwWindowShouldClose(window)) {

		// Render
		Render();

		// Display FPS
		double curTime = glfwGetTime();
		double deltaT = curTime - lastTime;
		double fps = 1.0/deltaT;
		maxFPS = (fps > maxFPS) ? fps : maxFPS;
		minFPS = (fps < minFPS) ? fps : minFPS;
		avgFPS = (avgFPS*frameCount + fps)/(frameCount+1);
		frameCount++;
//		printf("FPS: %.0f, MIN: %f, MAX: %F\n", avgFPS, minFPS, maxFPS);
		if (frameCount >= 10) {
			frameCount = 0;
			avgFPS = 0;
		}
		lastTime = curTime;

		// Handle Keyboard Input
		handleKeyboardInput(deltaT, shader);

		// Color fade
		redShade += colorChangeRate*deltaT;
		if (redShade >= 1.0) {
			redShade = 1.0;
			colorChangeRate = -absColorChangeRate;
		} else if (redShade <= 0.0) {
			redShade = 0.0;
			colorChangeRate = absColorChangeRate;
		}
		glUniform4f(colorLocation, redShade, 0.0, 1.0, 1.0);

		/* Player Rotation */
		playerAngle += playerRotationRate*deltaT;
		if (playerAngle >= 2*PI) {
			playerAngle -= 2*PI;
		};
		updateRotationMatrix(playerAngle, playerRotationMatrix);

		/* Enemy Movement and Rotation */
		for (int i = 0; i < NUM_ENEMIES; i++) {
			if (enemyLocations[i*4 + 3] == -1.0) continue;
			float enemySpeed = 0.5;
			// Update Angle
			float enemyAngle = enemyLocations[i*4 + 2];
			float deltaX = playerX - enemyLocations[i*4];
			float deltaY = playerY - enemyLocations[i*4 + 1];
			enemyAngle = atan2(deltaX, deltaY);
			enemyLocations[i*4 + 2] = enemyAngle;

			// Update Position
			int enemyOnScreen = abs(playerX - enemyLocations[i*4]) <= aspectRatio && abs(playerY - enemyLocations[i*4 + 1]) <= 1.0;
			if (enemyOnScreen) {
				enemyLocations[i*4] += sin(enemyAngle)*enemySpeed*deltaT; // X
				enemyLocations[i*4 + 1] += cos(enemyAngle)*enemySpeed*deltaT; // Y
			}

			// Collision with Player
			float disFromPlayer = sqrt(deltaX*deltaX + deltaY*deltaY);
			if (disFromPlayer < PLAYER_HITBOX_RAD + ENEMY_HITBOX_RAD) {
				enemyLocations[i*4 + 3] = -1.0;
				playerHealth -= 0.5;
			}

		}
		updateRotationMatrices(enemyLocations, enemyRotationMatrices, NUM_ENEMIES);

		/* Player Bullet Movement */

		// Check if each bullet is out of bounds
		for (int i = 0; i < NUM_PLAYER_BULLETS; i++) {
			float bulletX = playerBulletLocations[i*4];
			float bulletY = playerBulletLocations[i*4 + 1];
			float deltaX = bulletX - playerX;
			float deltaY = bulletY - playerY;
			int bulletOutOfBounds = deltaX < -aspectRatio*1.0 || deltaX > aspectRatio*1.0 || deltaY < -1.0 || deltaY > 1.0;
			if (bulletOutOfBounds) {
				playerBulletLocations[i*4 + 3] = -1.0;
			}
		}

		// Add new bullet
		if (isShooting) {
			timeSinceLastBullet += deltaT;
		}
		if (timeSinceLastBullet >= 1.0/BULLETS_PER_SECOND) {
			timeSinceLastBullet -= 1.0/BULLETS_PER_SECOND;
			for (int i = 0; i < NUM_PLAYER_BULLETS; i++) {
				if (playerBulletLocations[i*4 + 3] == 0.0) continue;
				playerBulletLocations[i*4] = playerX;
				playerBulletLocations[i*4 + 1] = playerY;
				playerBulletLocations[i*4 + 2] = playerAngle;
				playerBulletLocations[i*4 + 3] = 0.0;
				playerBulletVelocities[i*4] = 4.0*sin(playerAngle)*deltaT;
				playerBulletVelocities[i*4 + 1] = 4.0*cos(playerAngle)*deltaT;
				updateRotationMatrix(playerAngle, playerBulletRotationMatrices + i*16);
				break;
			}
		}

		// Move Bullets
		for (int i = 0; i < NUM_PLAYER_BULLETS; i++) {
			playerBulletLocations[i*4] += playerBulletVelocities[i*4];
			playerBulletLocations[i*4 + 1] += playerBulletVelocities[i*4 + 1];
		}

		/* Wormhole Rotation */
		wormholeAngle += 20.0*deltaT;
		updateRotationMatrix(wormholeAngle, wormholeRotationMatrix);

		/* Set Shader Variables */
		glUniform2f(playerLocation, playerX, playerY);
		glUniformMatrix4fv(playerRotationLocation, 1, GL_FALSE, playerRotationMatrix);
		glUniform4fv(enemyLocation, NUM_ENEMIES, enemyLocations);
		glUniformMatrix4fv(enemyRotationMatrixLocation, NUM_ENEMIES, GL_FALSE, enemyRotationMatrices);
		glUniformMatrix4fv(wormholeMatrixLocation, 1, GL_FALSE, wormholeRotationMatrix);
		glUniform4fv(playerBulletLocation, NUM_PLAYER_BULLETS, playerBulletLocations);
		glUniformMatrix4fv(playerBulletRotationMatrixLocation, NUM_PLAYER_BULLETS, GL_FALSE, playerBulletRotationMatrices);

		/* Collision Detection */

		// Out of Bounds Detection
		if ((playerX*playerX + playerY*playerY) >= BOUNDARY_RADIUS*BOUNDARY_RADIUS) {
			printf("Out of Bounds\n");
			exit(-1);
		}

		// Enemy and Player Bullet
		for (int enemy = 0; enemy < NUM_ENEMIES; enemy++) {
			if (enemyLocations[enemy*4 + 3] == -1.0) continue;
			float enemyX = enemyLocations[enemy*4];
			float enemyY = enemyLocations[enemy*4 + 1];
			for (int bullet = 0; bullet < NUM_PLAYER_BULLETS; bullet++) {
				if (playerBulletLocations[bullet*4 +3] == -1.0) continue;
				float bulletX = playerBulletLocations[bullet*4];
				float bulletY = playerBulletLocations[bullet*4 + 1];
				float deltaX = enemyX - bulletX;
				float deltaY = enemyY - bulletY;
				int isCollide = sqrt(deltaX*deltaX + deltaY*deltaY) <= ENEMY_HITBOX_RAD + PLAYER_BULLET_HITBOX_RAD;
				if (isCollide) {
					enemyHealth[enemy] -= 0.1;
				}
			}
			if (enemyHealth[enemy] <= 0.0) {
				enemyLocations[enemy*4 + 3] = -1.0;
			}
		}

		/* Win/Lose Game Detection */
		if (playerHealth <= 0.0) {
			printf("You Died\n");
			exit(-1);
		}
		int youWin = 1;
		for (int enemy = 0; enemy <= NUM_ENEMIES; enemy++) {
			if (enemyHealth[enemy] > 0.0) youWin = 0;
		}
		if (youWin) {
			printf("You Win!\n");
			exit(0);
		}
	}

	glDeleteProgram(shader);

	glfwTerminate();
	return 0;
}
