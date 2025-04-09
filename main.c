#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <alloca.h>

#define PI 3.14159265358979323846
#define WINDOW_NAME "Guardian of the Cosmos"

#define MAT_BUFFER_INDEX 2
#define VSYNC_ON 1

// Maximum number of each object type
// Update the vertex shader when any of these values are changed
#define NUM_ENEMIES 6
#define NUM_WORMHOLES 3
#define NUM_PLAYER_BULLETS 64
#define NUM_ENEMY_BULLETS 128
#define NUM_ASTEROIDS 256
#define NUM_TESTS 8
#define NUM_TESTS2 4

// How many sides in circles
#define BOUNDARY_SIDES 256
#define WORMHOLE_SIDES 8
#define ENEMY_BULLET_SIDES 8
#define ASTEROID_SIDES 8
#define TEST2_SIDES 16

#define BOUNDARY_RADIUS 5.0
#define PLAYER_SHOOT_RATE 10.0 // Bullets per second
#define ENEMY_SHOOT_RATE 2.0
#define PLAYER_HITBOX_RAD 0.04
#define ENEMY_HITBOX_RAD 0.055
#define PLAYER_BULLET_HITBOX_RAD 0.03
#define ENEMY_BULLET_RAD 0.005

//	glDrawElements(GL_LINE_LOOP, sizeof(playerInd)/sizeof(unsigned int), GL_UNSIGNED_INT, (void*)(long)(indOffsets[0]));
//	glDrawElementsInstanced(GL_LINES, sizeof(enemyInd)/sizeof(unsigned int), GL_UNSIGNED_INT, (void*)(long)(indOffsets[1]), NUM_ENEMIES);
//	glDrawElementsInstanced(GL_LINE_LOOP, sizeof(wormholeInd)/sizeof(unsigned int), GL_UNSIGNED_INT, (void*)(long)(indOffsets[2]), NUM_WORMHOLES);
//	glDrawElements(GL_LINE_LOOP, BOUNDARY_SIDES, GL_UNSIGNED_INT, (void*)(long)(indOffsets[3]));
//	glDrawElementsInstanced(GL_LINES, sizeof(playerBulletInd)/sizeof(unsigned int), GL_UNSIGNED_INT, (void*)(long)(indOffsets[4]), NUM_PLAYER_BULLETS);
//	glDrawElementsInstanced(GL_TRIANGLE_FAN, sizeof(enemyBulletInd)/sizeof(unsigned int), GL_UNSIGNED_INT, (void*)(long)(indOffsets[5]), NUM_ENEMY_BULLETS);
//	glDrawElementsInstanced(GL_LINE_LOOP, ASTEROID_SIDES, GL_UNSIGNED_INT, (void*)(long)(indOffsets[6]), NUM_ASTEROIDS);
//	glDrawElementsInstanced(GL_LINE_LOOP, sizeof(test2Ind)/sizeof(unsigned int), GL_UNSIGNED_INT, (void*)(long)(indOffsets[8]), NUM_TESTS2);


#define Render() do {\
	/* Set background to black */\
	glClear(GL_COLOR_BUFFER_BIT); \
\
	/* Render objects */\
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO); \
\
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0); \
	glDrawElementsInstanced(GL_LINES, sizeof(testInd)/sizeof(unsigned int), GL_UNSIGNED_INT, 0, NUM_TESTS); \
\
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)sizeof(testLocations)); \
	glDrawElementsInstanced(GL_LINE_LOOP, sizeof(test2Ind)/sizeof(unsigned int), GL_UNSIGNED_INT, (void*)sizeof(testInd), NUM_TESTS2); \
\
	/* Swap front and back buffers */ \
	glfwSwapBuffers(window); \
	} while(0)

GLFWwindow* window;
int* nullptr = NULL;
float aspectRatio = 1.0;

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

int spawnBullet(float *bulletLocations, float *bulletVelocities, float *bulletRotationMatrices, int numBullets, float x, float y, float angle, float deltaT, float velocity)
{
	for (int i = 0; i < numBullets; i++) {
		if (bulletLocations[i*4 + 3] == 0.0) continue;
		bulletLocations[i*4] = x;
		bulletLocations[i*4 + 1] = y;
		bulletLocations[i*4 + 2] = angle;
		bulletLocations[i*4 + 3] = 0.0;
		bulletVelocities[i*4] = velocity*sin(angle)*deltaT;
		bulletVelocities[i*4 + 1] = velocity*cos(angle)*deltaT;
		updateRotationMatrix(playerAngle, bulletRotationMatrices + i*16);
		break;
	}
	return 0;
}

int isOnScreen(float x, float y)
{
	float deltaX = x - playerX;
	float deltaY = y - playerY;
	return deltaX >= -aspectRatio*1.0 && deltaX <= aspectRatio*1.0 && deltaY >= -1.0 && deltaY <= 1.0;
}

int main(void)
{

	srand(time(NULL));

	// Initialize glfw
	if (!glfwInit()) {
		return -1;
	}

	// Create a fullscreen window and its OpenGL context
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
	int screenWidth = videoMode->width;
	int screenHeight = videoMode->height;
	aspectRatio = (float)screenWidth/(float)screenHeight;
	printf("Screen Resolution: %dx%d\n", screenWidth, screenHeight);
	printf("Aspect Ratio: %f\n", aspectRatio);
	window = glfwCreateWindow(screenWidth, screenHeight, WINDOW_NAME, monitor, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	// Make the window's context current
	glfwMakeContextCurrent(window);

	// Turn on/off vsync
	glfwSwapInterval(VSYNC_ON);

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

	float timeSinceLastEnemyBullet[NUM_ENEMIES] = {0.0};

	/* Wormhole Data */
	float wormholeVert[3*WORMHOLE_SIDES];
	unsigned int wormholeInd[WORMHOLE_SIDES];
	createCircle(wormholeVert, wormholeInd, 0.2, WORMHOLE_SIDES);

	for (int i = 0; i < WORMHOLE_SIDES; i++) {
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
	float boundaryVert[3*BOUNDARY_SIDES];
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

	/* Enemy Bullet Data */

	float enemyBulletVert[3*ENEMY_BULLET_SIDES];
	unsigned int enemyBulletInd[ENEMY_BULLET_SIDES];
	createCircle(enemyBulletVert, enemyBulletInd, 0.5, ENEMY_BULLET_SIDES);

	for (int i = 0; i < ENEMY_BULLET_SIDES; i++) {
		enemyBulletVert[3*i] *= ENEMY_BULLET_RAD;
		enemyBulletVert[3*i + 1] *= ENEMY_BULLET_RAD;
	}

	float enemyBulletLocations[4*NUM_ENEMY_BULLETS];
	for (int i = 0; i < NUM_ENEMY_BULLETS; i++) {
		enemyBulletLocations[i*4] = 0.0;
		enemyBulletLocations[i*4 + 1] = 0.0;
		enemyBulletLocations[i*4 + 2] = 1.0;
		enemyBulletLocations[i*4 + 3] = -1.0;
	}
	float enemyBulletRotationMatrices[16*NUM_ENEMY_BULLETS] = {0.0};
	for (int i = 0; i < NUM_ENEMY_BULLETS; i++) {
		for (int j = 0; j < 4; j++) {
			enemyBulletRotationMatrices[i*16 + j*5] = 1.0;
		}
	}
	float enemyBulletVelocities[2*NUM_ENEMY_BULLETS] = {0.0};

	/* Asteroid Data */

	float asteroidVert[] = {
		0.0, 0.02, 0.6,
		0.009, 0.009, 0.6,
		0.02, 0.0, 0.6,
		0.017, -0.017, 0.6,
		0.0, -0.01, 0.6,
		-0.017, -0.017, 0.6,
		-0.02, 0.0, 0.6,
		-0.014, 0.014, 0.6,
	};
	unsigned int asteroidInd[] = {
		0, 1, 2, 3, 4, 5, 6, 7,
	};

	float asteroidLocations[2*NUM_ASTEROIDS];
	for (int i = 0; i < NUM_ASTEROIDS; i++) {
		float asteroidDistance = ((float)rand())/RAND_MAX;
		asteroidDistance = sqrt(asteroidDistance)*BOUNDARY_RADIUS;
		float asteroidAngle = ((float)rand())/RAND_MAX*2*PI;
		asteroidLocations[i*2] = asteroidDistance*sin(asteroidAngle);
		asteroidLocations[i*2 + 1] = asteroidDistance*cos(asteroidAngle);
	}

	/* Test Object Data */

	float testVert[] = {
		0.1, 0.1, 0.7,
		0.1, -0.1, 0.7,
		-0.1, -0.1, 0.7,
		-0.1, 0.1, 0.7,
	};
	unsigned int testInd[] = {
		0, 1,
		1, 2,
		2, 3,
		3, 0,
		0, 2,
	};
	float testLocations[2*NUM_TESTS];
	for (int i = 0; i < NUM_TESTS; i++) {
		testLocations[i*2] = i*0.1 - 1.0;
		testLocations[i*2 + 1] = i*0.15 - 1.0;
	}

	/* Test Object 2 Data */
	float test2Vert[3*TEST2_SIDES];
	unsigned int test2Ind[TEST2_SIDES];
	createCircle(test2Vert, test2Ind, 0.7, TEST2_SIDES);

	for (int i = 0; i < TEST2_SIDES; i++) {
		test2Vert[3*i] *= 0.02;
		test2Vert[3*i + 1] *= 0.02;
	}
	float test2Locations[2*NUM_TESTS2];
	for (int i = 0; i < NUM_TESTS2; i++) {
		test2Locations[i*2] = 0.5;
		test2Locations[i*2 + 1] = i*0.1 - 0.8;
	}

	// Setup Buffers
	void *objectDrawData[] = {
		&playerVert, &playerInd,
		&enemyVert, &enemyInd,
		&wormholeVert, &wormholeInd,
		&boundaryVert, &boundaryInd,
		&playerBulletVert, &playerBulletInd,
		&enemyBulletVert, &enemyBulletInd,
		&asteroidVert, &asteroidInd,
	};
	unsigned int objectSizeData[] = {
		sizeof(playerVert), sizeof(playerInd),
		sizeof(enemyVert), sizeof(enemyInd),
		sizeof(wormholeVert), sizeof(wormholeInd),
		sizeof(boundaryVert), sizeof(boundaryInd),
		sizeof(playerBulletVert), sizeof(playerBulletInd),
		sizeof(enemyBulletVert), sizeof(enemyBulletInd),
		sizeof(asteroidVert), sizeof(asteroidInd),
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

	// Generate and Bind Rotation Matrix Buffer
	unsigned int matBuffer;
	glGenBuffers(1, &matBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, matBuffer);
	for (int i = 0; i < 4; i++) {
		GLuint index = MAT_BUFFER_INDEX + i;
		glEnableVertexAttribArray(index);
		glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, sizeof(float)*16, (void*)(4*sizeof(float)*i));
		glVertexAttribDivisor(index, 1);
	}
	glBufferData(GL_ARRAY_BUFFER, 16*sizeof(float)*NUM_PLAYER_BULLETS, playerBulletRotationMatrices, GL_DYNAMIC_DRAW);

	// New Buffers
	for (int i = 0; i < sizeof(test2Ind)/sizeof(unsigned int); i++) {
		test2Ind[i] += sizeof(testVert)/sizeof(float)/3;
	}

	unsigned int VBO2;
	glGenBuffers(1, &VBO2);
	glBindBuffer(GL_ARRAY_BUFFER, VBO2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(testVert)+sizeof(test2Vert), 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(testVert), &testVert[0]);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(testVert), sizeof(test2Vert), &test2Vert[0]);

	glEnableVertexAttribArray(0); // Positions
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*3, (void*)0);

	unsigned int IBO2;
	glGenBuffers(1, &IBO2);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO2);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(testInd)+sizeof(test2Ind), 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(testInd), &testInd[0]);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sizeof(testInd), sizeof(test2Ind), &test2Ind[0]);

	unsigned int instanceVBO;
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(testLocations)+sizeof(test2Locations), 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(testLocations), &testLocations[0]);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(testLocations), sizeof(test2Locations), &test2Locations[0]);

	glEnableVertexAttribArray(1); // Offsets
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
	glVertexAttribDivisor(1, 1);


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
	int enemyBulletLocation = glGetUniformLocation(shader, "enemyBulletLocations");
	int enemyBulletRotationMatrixLocation = glGetUniformLocation(shader, "enemyBulletRotationMatrices");
	int asteroidLocation = glGetUniformLocation(shader, "asteroidLocations");

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
	glUniform4fv(enemyBulletLocation, NUM_ENEMY_BULLETS, enemyBulletLocations);
	glUniformMatrix4fv(enemyBulletRotationMatrixLocation, NUM_ENEMY_BULLETS, GL_FALSE, enemyBulletRotationMatrices);
	glUniform2fv(asteroidLocation, NUM_ASTEROIDS, asteroidLocations);

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
		printf("FPS: %.0f, MIN: %f, MAX: %F\n", avgFPS, minFPS, maxFPS);
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

		/* Enemy Movement, Rotation and Shooting */
		for (int i = 0; i < NUM_ENEMIES; i++) {
			if (enemyLocations[i*4 + 3] == -1.0) continue;
			float enemySpeed = 0.5;
			// Update Angle
			float enemyAngle = enemyLocations[i*4 + 2];
			float deltaX = playerX - enemyLocations[i*4];
			float deltaY = playerY - enemyLocations[i*4 + 1];
			enemyAngle = atan2(deltaX, deltaY);
			enemyLocations[i*4 + 2] = enemyAngle;

			// Update position and shoot if on screen
			int enemyOnScreen = abs(playerX - enemyLocations[i*4]) <= aspectRatio && abs(playerY - enemyLocations[i*4 + 1]) <= 1.0;
			if (enemyOnScreen) {
				// Update Position
				enemyLocations[i*4] += sin(enemyAngle)*enemySpeed*deltaT; // X
				enemyLocations[i*4 + 1] += cos(enemyAngle)*enemySpeed*deltaT; // Y

				// Shoot
				timeSinceLastEnemyBullet[i] += deltaT;
				if (timeSinceLastBullet >= 1.0/ENEMY_SHOOT_RATE) {
					timeSinceLastBullet -= 1.0/ENEMY_SHOOT_RATE;
				}
			}
		}
		updateRotationMatrices(enemyLocations, enemyRotationMatrices, NUM_ENEMIES);

		/* Player Bullet Movement */

		// Check if each bullet is out of bounds
		for (int i = 0; i < NUM_PLAYER_BULLETS; i++) {
			float bulletX = playerBulletLocations[i*4];
			float bulletY = playerBulletLocations[i*4 + 1];
			if (!isOnScreen(bulletX, bulletY)) {
				playerBulletLocations[i*4 + 3] = -1.0;
			}
		}

		// Add new bullet
		if (isShooting) {
			timeSinceLastBullet += deltaT;
		}
		if (timeSinceLastBullet >= 1.0/PLAYER_SHOOT_RATE) {
			timeSinceLastBullet -= 1.0/PLAYER_SHOOT_RATE;
			spawnBullet(playerBulletLocations, playerBulletVelocities, playerBulletRotationMatrices, NUM_PLAYER_BULLETS, playerX, playerY, playerAngle, deltaT, 4.0);
		}

		// Move Bullets
		for (int i = 0; i < NUM_PLAYER_BULLETS; i++) {
			playerBulletLocations[i*4] += playerBulletVelocities[i*4];
			playerBulletLocations[i*4 + 1] += playerBulletVelocities[i*4 + 1];
		}

		/* Enemy Bullet Movement */

		// Check if each bullet is out of bounds
		for (int i = 0; i < NUM_ENEMY_BULLETS; i++) {
			float bulletX = enemyBulletLocations[i*4];
			float bulletY = enemyBulletLocations[i*4 + 1];
			if (!isOnScreen(bulletX, bulletY)) {
				enemyBulletLocations[i*4 + 3] = -1.0;
			}
		}

		// Add new bullet
		for (int i = 0; i < NUM_ENEMIES; i++) {
			if (enemyLocations[i*4 +3] == -1.0) continue;
			float enemyX = enemyLocations[i*4];
			float enemyY = enemyLocations[i*4 + 1];
			float enemyAngle = enemyLocations[i*4 + 2];
			if (isOnScreen(enemyX, enemyY)) {
				timeSinceLastEnemyBullet[i] += deltaT;
			}

			if (timeSinceLastEnemyBullet[i] >= 1.0/ENEMY_SHOOT_RATE) {
				timeSinceLastEnemyBullet[i] -= 1.0/ENEMY_SHOOT_RATE;
				spawnBullet(enemyBulletLocations, enemyBulletVelocities, enemyBulletRotationMatrices, NUM_ENEMY_BULLETS, enemyX, enemyY, enemyAngle, deltaT, 1.0);
			}
		}


		// Move Bullets
		for (int i = 0; i < NUM_ENEMY_BULLETS; i++) {
			enemyBulletLocations[i*4] += enemyBulletVelocities[i*4];
			enemyBulletLocations[i*4 + 1] += enemyBulletVelocities[i*4 + 1];
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
		glUniform4fv(enemyBulletLocation, NUM_PLAYER_BULLETS, enemyBulletLocations);
		glUniformMatrix4fv(enemyBulletRotationMatrixLocation, NUM_PLAYER_BULLETS, GL_FALSE, enemyBulletRotationMatrices);
		glUniform2fv(asteroidLocation, NUM_ASTEROIDS, asteroidLocations);

//		glBindBuffer(GL_ARRAY_BUFFER, matBuffer);
//		glBufferData(GL_ARRAY_BUFFER, 16*sizeof(float)*NUM_PLAYER_BULLETS, playerBulletRotationMatrices, GL_DYNAMIC_DRAW);


		/* Collision Detection */

		// Out of Bounds Detection
		if ((playerX*playerX + playerY*playerY) >= BOUNDARY_RADIUS*BOUNDARY_RADIUS) {
			printf("Out of Bounds\n");
			exit(-1);
		}

		// Enemy and Player / Player Bullet
		for (int enemy = 0; enemy < NUM_ENEMIES; enemy++) {
			if (enemyLocations[enemy*4 + 3] == -1.0) continue;
			float enemyX = enemyLocations[enemy*4];
			float enemyY = enemyLocations[enemy*4 + 1];
			float deltaX = enemyX - playerX;
			float deltaY = enemyY - playerY;

			// Collision with Player
			float disFromPlayer = sqrt(deltaX*deltaX + deltaY*deltaY);
			if (disFromPlayer <= PLAYER_HITBOX_RAD + ENEMY_HITBOX_RAD) {
				enemyLocations[enemy*4 + 3] = -1.0;
				enemyHealth[enemy] = 0.0;
				playerHealth -= 0.5;
			}

			for (int bullet = 0; bullet < NUM_PLAYER_BULLETS; bullet++) {
				if (playerBulletLocations[bullet*4 + 3] == -1.0) continue;
				float bulletX = playerBulletLocations[bullet*4];
				float bulletY = playerBulletLocations[bullet*4 + 1];
				float deltaX = enemyX - bulletX;
				float deltaY = enemyY - bulletY;
				int isCollide = sqrt(deltaX*deltaX + deltaY*deltaY) <= ENEMY_HITBOX_RAD + PLAYER_BULLET_HITBOX_RAD;
				if (isCollide) {
					enemyHealth[enemy] -= 0.1;
					playerBulletLocations[bullet*4 + 3] = -1.0;
				}
			}
			if (enemyHealth[enemy] <= 0.0) {
				enemyLocations[enemy*4 + 3] = -1.0;
			}
		}

		// Player and Enemy Bullet
		for (int bullet = 0; bullet < NUM_ENEMY_BULLETS; bullet++) {
			if (enemyBulletLocations[bullet*4 + 3] == -1.0) continue;
			float bulletX = enemyBulletLocations[bullet*4];
			float bulletY = enemyBulletLocations[bullet*4 + 1];
			float deltaX = playerX - bulletX;
			float deltaY = playerY - bulletY;
			int isCollide = sqrt(deltaX*deltaX + deltaY*deltaY) <= PLAYER_HITBOX_RAD + ENEMY_BULLET_RAD;
			if (isCollide) {
				playerHealth -= 0.25;
				enemyBulletLocations[bullet*4 + 3] = -1.0;
			}
		}

		/* Win/Lose Game Detection */
		if (playerHealth <= 0.0) {
			printf("You Died\n");
			exit(-1);
		}
		int youWin = 1;
		for (int enemy = 0; enemy <= NUM_ENEMIES; enemy++) {
			if (enemyHealth[enemy] > 0.0 && enemyLocations[enemy*4 + 3] == 0.0) youWin = 0;
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
