#include <stdio.h>
#include <math.h>

#define PI 3.14159265358979323846
#define POINTS 8

int main()
{
	for (int i = 0; i < POINTS; i++) {
		float angle = ((float)i/POINTS)*2*PI;
		float x = cos(angle);
		float y = sin(angle);
		printf("%f, %f,\n", x, y);
	}
	return 0;
}
