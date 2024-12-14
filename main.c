#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <alloca.h>

#define PI 3.14159265358979323846
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800
#define WINDOW_NAME "Guardian of the Cosmos"

int* nullptr = NULL;

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

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		// menu maybe
	}
}

void handleKeyboardInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		playerRotationRate = -5.0;
	} else if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		playerRotationRate = 20.0;
	} else {
		playerRotationRate = 0.0;
	}
}

int main(void)
{
	GLFWwindow* window;

	// Initialize glfw and glew
	if (!glfwInit()) {
		return -1;
	}

	// Create a windowed mode window and its OpenGL context
	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_NAME, NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	// Make the window's context current
	glfwMakeContextCurrent(window);

	// Cut off vsync
	glfwSwapInterval(0);

	// Keyboard input
	glfwSetKeyCallback(window, keyboardCallback);

	if (glewInit() != GLEW_OK) {
		return -1;
	}

	float playerVert[] = {
		-0.04,	-0.04,
		0.04,	-0.04,
		0.0,	0.08,
	};

	unsigned int playerInd[] = {
		0, 1, 2,
	};

	float enemyVert[] = {
		-0.05,	-0.2, // Middle section
		-0.1,	-0.15, // (0 - 6)
		-0.1,	0.3,
		0.0,	0.5,
		0.1,	0.3,
		0.1,	-0.15,
		0.05,	-0.2,

		-0.1,	-0.1, // Left connector
		-0.2,	-0.1, // (7 - 10)
		-0.1,	0.1,
		-0.2,	0.1,

		0.1,	-0.1, // Right connector
		0.2,	-0.1, // (11 - 14)
		0.1,	0.1,
		0.2,	0.1,

		-0.2,	-0.15, // Left section
		-0.2,	0.15, // (15 - 19)
		-0.25,	0.2,
		-0.3,	0.15,
		-0.3,	-0.15,

		0.2,	-0.15, // Right section
		0.2,	0.15, // (20 - 24)
		0.25,	0.2,
		0.3,	0.15,
		0.3,	-0.15,
	};
	for (int i = 0; i < sizeof(enemyVert)/2/sizeof(float); i++) {
		enemyVert[2*i] *= 0.2;
		enemyVert[2*i + 1] *= 0.2;
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

	float wormholeVert[] = {
		1.000000, 0.000000,
		0.707107, 0.707107,
		-0.000000, 1.000000,
		-0.707107, 0.707107,
		-1.000000, -0.000000,
		-0.707107, -0.707107,
		0.000000, -1.000000,
		0.707107, -0.707107,
	};
	for (int i = 0; i < sizeof(wormholeVert)/2/sizeof(float); i++) {
		wormholeVert[2*i] *= 0.2;
		wormholeVert[2*i + 1] *= 0.2;
	}

	unsigned int wormholeInd[] = {
		0,1,2,3,4,5,6,7,
		0,
	};

	// Setup Buffers
	void *objectDrawData[] = {
		&playerVert, &playerInd,
		&enemyVert, &enemyInd,
		&wormholeVert, &wormholeInd,
	};
	unsigned int objectSizeData[] = {
		sizeof(playerVert), sizeof(playerInd),
		sizeof(enemyVert), sizeof(enemyInd),
		sizeof(wormholeVert), sizeof(wormholeInd),
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
			indPointer[i] += vertOffsets[object]/sizeof(float)/2;
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
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, 0);

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
	float enemyLocations[24] = {
		-0.5, -0.5, 0.0, 0.0,
		0.5, 0.0, 0.0, 0.0,
		0.5, 0.5, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		-0.5, 0.5, 0.0, 0.0,
		-0.5, 0.0, 0.0, 0.0,
	};
	int rotationLocation = glGetUniformLocation(shader, "rotationMatrix");
	glUniformMatrix4fv(rotationLocation, 1, GL_FALSE, rotationMatrix);
	int enemyLocation = glGetUniformLocation(shader, "enemyLocations");
	glUniform4fv(enemyLocation, 6, enemyLocations);

	double lastTime = glfwGetTime();
	double maxFPS = 0;
	double minFPS = 1000000;
	double avgFPS = 0;
	int frameCount = 0;

	// Main Loop
	while (!glfwWindowShouldClose(window)) {

		// Render
		glClear(GL_COLOR_BUFFER_BIT);

		glDrawElements(GL_LINE_LOOP, sizeof(playerInd)/sizeof(unsigned int), GL_UNSIGNED_INT, (void*)(long)(indOffsets[0]));
		glDrawElementsInstanced(GL_LINES, sizeof(enemyInd)/sizeof(unsigned int), GL_UNSIGNED_INT, (void*)(long)(indOffsets[1]), 6);
		glDrawElements(GL_LINE_STRIP, sizeof(wormholeInd)/sizeof(unsigned int), GL_UNSIGNED_INT, (void*)(long)(indOffsets[2]));

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
		printf("FPS: %.0f, MIN: %f, MAX: %F\n", avgFPS, minFPS, maxFPS);
		if (frameCount >= 10) {
			frameCount = 0;
			avgFPS = 0;
		}
		lastTime = curTime;

		// Poll for and process events
		glfwPollEvents();
		handleKeyboardInput(window);

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

		// Square Rotation
		playerAngle += playerRotationRate*deltaT;
		if (playerAngle >= 2*PI) {
			playerAngle -= 2*PI;
		};
		rotationMatrix[0] = cos(playerAngle);
		rotationMatrix[1] = -sin(playerAngle);
		rotationMatrix[4] = sin(playerAngle);
		rotationMatrix[5] = cos(playerAngle);
		glUniformMatrix4fv(rotationLocation, 1, GL_FALSE, rotationMatrix);
	}

	glDeleteProgram(shader);

	glfwTerminate();
	return 0;
}
