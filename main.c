#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <alloca.h>

#define PI 3.14159265358979323846
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define WINDOW_NAME "Guardian of the Cosmos"

#define NUM_ENEMIES 6 // also change in vertex shader
#define NUM_WORMHOLES 3 // also change in vertex shader
#define BOUNDARY_SIDES 256
#define BOUNDARY_RADIUS 5.0

int* nullptr = NULL;

float playerX = 0.0;
float playerY = 0.0;
double playerRotationRate = 0.0;


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

void handleKeyboardInput(GLFWwindow* window, float playerSpeed, float playerAngle)
{
	/* Escape to Quit Game */
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		exit(0);
	}

	/* Player Movement */

	playerRotationRate = 0.0;
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		playerRotationRate = -5.0;
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		playerRotationRate = 5.0;
	}
	int wPressed = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
	int aPressed = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
	int sPressed = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
	int dPressed = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

	// Detect if moving on X and Y axis at the same time
	float speedMultiplier = 1.0;
	if (wPressed != sPressed && aPressed != dPressed) {
		speedMultiplier = sqrt(2.0)/2.0;
	}
	if (wPressed) {
		playerY += speedMultiplier*playerSpeed*cos(playerAngle);
		playerX += speedMultiplier*playerSpeed*sin(playerAngle);
	}
	if (aPressed) {
		playerY += speedMultiplier*playerSpeed*sin(playerAngle);
		playerX -= speedMultiplier*playerSpeed*cos(playerAngle);
	}
	if (sPressed) {
		playerY -= speedMultiplier*playerSpeed*cos(playerAngle);
		playerX -= speedMultiplier*playerSpeed*sin(playerAngle);
	}
	if (dPressed) {
		playerY -= speedMultiplier*playerSpeed*sin(playerAngle);
		playerX += speedMultiplier*playerSpeed*cos(playerAngle);
	}
}

int updateEnemyMatrices(float enemyLocations[], float *enemyRotationMatrices)
{
	for (int i = 0; i < NUM_ENEMIES; i++) {
		float enemyAngle = enemyLocations[i*4 + 2];
		float sinAngle = sin(enemyAngle);
		float cosAngle = cos(enemyAngle);
		enemyRotationMatrices[i*16] = cosAngle;
		enemyRotationMatrices[i*16 + 1] = -sinAngle;
		enemyRotationMatrices[i*16 + 4] = sinAngle;
		enemyRotationMatrices[i*16 + 5] = cosAngle;
	}
	return 0;
}

int createCircle(float *circleVertices, unsigned int *circleIndices, float type, int numSides) {
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
	GLFWwindow* window;

	// Initialize glfw and glew
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

	// Keyboard input

	glewInit();

	float playerVert[] = {
		-0.04,	-0.04, 0.0,
		0.04,	-0.04, 0.0,
		0.0,	0.08, 0.0,
	};

	unsigned int playerInd[] = {
		0, 1, 2,
	};

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
		enemyVert[3*i] *= 0.2;
		enemyVert[3*i + 1] *= 0.2;
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

	/* Wormhole */
	float wormholeVert[8*3];
	unsigned int wormholeInd[8];
	createCircle(wormholeVert, wormholeInd, 0.2, 8);

	for (int i = 0; i < sizeof(wormholeVert)/3/sizeof(float); i++) {
		wormholeVert[3*i] *= 0.2;
		wormholeVert[3*i + 1] *= 0.2;
	}

	/* Boundary */
	float boundaryVert[BOUNDARY_SIDES*3];
	unsigned int boundaryInd[BOUNDARY_SIDES];
	createCircle(boundaryVert, boundaryInd, 0.3, BOUNDARY_SIDES);

	for (int i = 0; i < BOUNDARY_SIDES; i++) {
		boundaryVert[3*i] *= BOUNDARY_RADIUS;
		boundaryVert[3*i + 1] *= BOUNDARY_RADIUS;
	}

	// Setup Buffers
	void *objectDrawData[] = {
		&playerVert, &playerInd,
		&enemyVert, &enemyInd,
		&wormholeVert, &wormholeInd,
		&boundaryVert, &boundaryInd,
	};
	unsigned int objectSizeData[] = {
		sizeof(playerVert), sizeof(playerInd),
		sizeof(enemyVert), sizeof(enemyInd),
		sizeof(wormholeVert), sizeof(wormholeInd),
		sizeof(boundaryVert), sizeof(boundaryInd),
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

	// Square Color
	double absColorChangeRate = 1.0;
	double colorChangeRate = absColorChangeRate;
	double squareRed = 0.0;
	int colorLocation = glGetUniformLocation(shader, "u_Color");
	glUniform4f(colorLocation, squareRed, 0.0, 1.0, 1.0);

	// Square Rotation
	double playerAngle = 0.0;
	float rotationMatrix[] = {
		cos(playerAngle), -sin(playerAngle), 0.0, 0.0,
		sin(playerAngle), cos(playerAngle), 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0,
	};

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
	float enemyLocations[4*NUM_ENEMIES] = {
		-4.0, 0.0, 1.0, 0.0,
		0.0, -4.0, 2.0, 0.0,
		4.0, 0.0, 3.0, 0.0,
		1.0, 3.0, 4.0, 0.0,
		0.0, 4.0, 5.0, 0.0,
		-1.0, 3.0, 6.0, 0.0,
	};
	float enemyRotationMatrices[16*NUM_ENEMIES] = {};
	for (int i = 0; i < NUM_ENEMIES; i++) {
		for (int j = 0; j < 4; j++) {
			enemyRotationMatrices[i*16 + j*5] = 1.0;
		}
	}
	updateEnemyMatrices(enemyLocations, enemyRotationMatrices);
	int rotationLocation = glGetUniformLocation(shader, "rotationMatrix");
	glUniformMatrix4fv(rotationLocation, 1, GL_FALSE, rotationMatrix);
	int enemyLocation = glGetUniformLocation(shader, "enemyLocations");
	glUniform4fv(enemyLocation, NUM_ENEMIES, enemyLocations);

	int playerLocation = glGetUniformLocation(shader, "playerLocation");
	glUniform2f(playerLocation, playerX, playerY);

	int matrixLocation = glGetUniformLocation(shader, "matrices");
	glUniformMatrix4fv(matrixLocation, NUM_ENEMIES, GL_FALSE, enemyRotationMatrices);

	int wormholeMatrixLocation = glGetUniformLocation(shader, "wormholeRotationMatrix");
	glUniformMatrix4fv(wormholeMatrixLocation, 1, GL_FALSE, wormholeRotationMatrix);

	int aspectRatioLocation = glGetUniformLocation(shader, "aspectRatio");
	glUniform1f(aspectRatioLocation, aspectRatio);

	int wormholeLocation = glGetUniformLocation(shader, "wormholeLocations");
	glUniform2fv(wormholeLocation, NUM_WORMHOLES, wormholeLocations);

	double lastTime = glfwGetTime();
	double maxFPS = 0;
	double minFPS = 1000000;
	double avgFPS = 0;
	int frameCount = 0;

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
		glClear(GL_COLOR_BUFFER_BIT);

		glDrawElements(GL_LINE_LOOP, sizeof(playerInd)/sizeof(unsigned int), GL_UNSIGNED_INT, (void*)(long)(indOffsets[0]));
		glDrawElementsInstanced(GL_LINES, sizeof(enemyInd)/sizeof(unsigned int), GL_UNSIGNED_INT, (void*)(long)(indOffsets[1]), NUM_ENEMIES);
		glDrawElementsInstanced(GL_LINE_LOOP, sizeof(wormholeInd)/sizeof(unsigned int), GL_UNSIGNED_INT, (void*)(long)(indOffsets[2]), NUM_WORMHOLES);
		glDrawElements(GL_LINE_LOOP, BOUNDARY_SIDES, GL_UNSIGNED_INT, (void*)(long)(indOffsets[3]));

		// Swap front and back buffers
		glfwSwapBuffers(window);

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

		// Poll for and process events
		glfwPollEvents();
		float playerSpeed = 1.2*deltaT;
		handleKeyboardInput(window, playerSpeed, playerAngle);

		// Out of Bounds Detection
		if ((playerX*playerX + playerY*playerY) >= BOUNDARY_RADIUS*BOUNDARY_RADIUS) {
			printf("Out of Bounds\n");
			exit(-1);
		}

		// Calculate Movement based on deltaT

		// Square color fade
		squareRed += colorChangeRate*deltaT;
		if (squareRed >= 1.0) {
			squareRed = 1.0;
			colorChangeRate = -absColorChangeRate;
		} else if (squareRed <= 0.0) {
			squareRed = 0.0;
			colorChangeRate = absColorChangeRate;
		}
		glUniform4f(colorLocation, squareRed, 0.0, 1.0, 1.0);

		// Player Rotation
		playerAngle += playerRotationRate*deltaT;
		if (playerAngle >= 2*PI) {
			playerAngle -= 2*PI;
		};
		rotationMatrix[0] = cos(playerAngle);
		rotationMatrix[1] = -sin(playerAngle);
		rotationMatrix[4] = sin(playerAngle);
		rotationMatrix[5] = cos(playerAngle);

		// Enemy Rotation
		for (int i = 0; i < NUM_ENEMIES; i++) {
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

			// Update Rotation Matrix
			float sinAngle = sin(enemyAngle);
			float cosAngle = cos(enemyAngle);
			enemyRotationMatrices[i*16] = cosAngle;
			enemyRotationMatrices[i*16 + 1] = -sinAngle;
			enemyRotationMatrices[i*16 + 4] = sinAngle;
			enemyRotationMatrices[i*16 + 5] = cosAngle;
		}

		// Wormhole Rotation
		wormholeAngle += 20.0*deltaT;
		wormholeRotationMatrix[0] = cos(wormholeAngle);
		wormholeRotationMatrix[1] = -sin(wormholeAngle);
		wormholeRotationMatrix[4] = sin(wormholeAngle);
		wormholeRotationMatrix[5] = cos(wormholeAngle);

		// Update Matrices
		updateEnemyMatrices(enemyLocations, enemyRotationMatrices);
		glUniform2f(playerLocation, playerX, playerY);
		glUniformMatrix4fv(rotationLocation, 1, GL_FALSE, rotationMatrix);
		glUniform4fv(enemyLocation, NUM_ENEMIES, enemyLocations);
		glUniformMatrix4fv(matrixLocation, NUM_ENEMIES, GL_FALSE, enemyRotationMatrices);
		glUniformMatrix4fv(wormholeMatrixLocation, 1, GL_FALSE, wormholeRotationMatrix);

	}

	glDeleteProgram(shader);

	glfwTerminate();
	return 0;
}
