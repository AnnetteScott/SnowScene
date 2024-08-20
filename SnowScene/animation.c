/******************************************************************************
 *
 * Animation v1.0 (23/02/2021)
 *
 * This template provides a basic FPS-limited render loop for an animated scene.
 *
 ******************************************************************************/

#include <Windows.h>
#include <freeglut.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>


 /******************************************************************************
  * Animation & Timing Setup
  ******************************************************************************/

  // Target frame rate (number of Frames Per Second).
#define TARGET_FPS 60
#define MAX_PARTICLES 1000
#define M_PI 3.14159

float width = 1000.0;
float height = 1000.0;

int snowCount = 0;
bool snowFall = false;

typedef struct {
	float x, y;
} Point;

typedef struct {
	int r, g, b;
} Colour;

typedef struct {
	int x, y, speed, size, transparency;
} Snow;

Colour WHITE = { 255, 255, 255 };
Colour GREY = { 130, 151, 173 };
Colour BLACK = { 0, 0, 0 };
Colour ORANGE = { 245, 127, 42 };


Point groundVertices[4];
Snow snowParticles[MAX_PARTICLES];

// Ideal time each frame should be displayed for (in milliseconds).
const unsigned int FRAME_TIME = 1000 / TARGET_FPS;

// Frame time in fractional seconds.
// Note: This is calculated to accurately reflect the truncated integer value of
// FRAME_TIME, which is used for timing, rather than the more accurate fractional
// value we'd get if we simply calculated "FRAME_TIME_SEC = 1.0f / TARGET_FPS".
const float FRAME_TIME_SEC = (1000 / TARGET_FPS) / 1000.0f;

// Time we started preparing the current frame (in milliseconds since GLUT was initialized).
unsigned int frameStartTime = 0;

/******************************************************************************
 * Keyboard Input Handling Setup
 ******************************************************************************/

 // Define all character keys used for input (add any new key definitions here).
 // Note: USE ONLY LOWERCASE CHARACTERS HERE. The keyboard handler provided converts all
 // characters typed by the user to lowercase, so the SHIFT key is ignored.

#define KEY_EXIT			27 // Escape key.
#define KEY_S				115 // S key.
#define KEY_Q				113 // q key.

/******************************************************************************
 * GLUT Callback Prototypes
 ******************************************************************************/

void display(void);
void reshape(int width, int h);
void keyPressed(unsigned char key, int x, int y);
void idle(void);

/******************************************************************************
 * Animation-Specific Function Prototypes (add your own here)
 ******************************************************************************/

void main(int argc, char **argv);
void init(void);
void think(void);
void createSnow(int i);
void setColour(int r, int g, int b);
void drawBackground(void);
void drawCircle(float cx, float cy, float r, int numSegments, Colour inner, Colour outer);
void drawSnow(void);

/******************************************************************************
 * Animation-Specific Setup (Add your own definitions, constants, and globals here)
 ******************************************************************************/

/******************************************************************************
 * Entry Point (don't put anything except the main function here)
 ******************************************************************************/

void main(int argc, char **argv)
{
	// Initialize the OpenGL window.
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize((int)width, (int)height);
	glutCreateWindow("Animation");

	// Set up the scene.
	init();

	// Disable key repeat (keyPressed or specialKeyPressed will only be called once when a key is first pressed).
	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);

	// Register GLUT callbacks.
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyPressed);
	glutIdleFunc(idle);

	// Record when we started rendering the very first frame (which should happen after we call glutMainLoop).
	frameStartTime = (unsigned int)glutGet(GLUT_ELAPSED_TIME);

	// Enter the main drawing loop (this will never return).
	glutMainLoop();
}

/******************************************************************************
 * GLUT Callbacks (don't add any other functions here)
 ******************************************************************************/

 /*
	 Called when GLUT wants us to (re)draw the current animation frame.

	 Note: This function must not do anything to update the state of our simulated
	 world. Animation (moving or rotating things, responding to keyboard input,
	 etc.) should only be performed within the think() function provided below.
 */

void display(void)
{	
	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT);

	drawBackground();
	
	//Draw the Snowman body
	drawCircle(500, 300, 100, 100, WHITE, GREY);
	drawCircle(500, 420, 80, 100, WHITE, GREY);
	drawCircle(500, 520, 60, 100, WHITE, GREY);
	
	//Draw the eyes and nose
	drawCircle(480, 550, 10, 50, BLACK, BLACK);
	drawCircle(520, 550, 10, 50, BLACK, BLACK);
	drawCircle(500, 520, 12, 7, ORANGE, ORANGE);

	//Draw snow if snow is allowed to fall
	if (snowCount != 0) {
		drawSnow();
	}

	glutSwapBuffers();
}

/*
	Called when the OpenGL window has been resized.
*/
void reshape(int w, int h)
{
}

/*
	Called each time a character key (e.g. a letter, number, or symbol) is pressed.
*/
void keyPressed(unsigned char key, int x, int y)
{
	switch (tolower(key)) {
		case KEY_S:
			snowFall = !snowFall;
			break;
		case KEY_Q:
			exit(0);
			break;
		case KEY_EXIT:
			exit(0);
			break;
	}

}

/*
	Called by GLUT when it's not rendering a frame.

	Note: We use this to handle animation and timing. You shouldn't need to modify
	this callback at all. Instead, place your animation logic (e.g. moving or rotating
	things) within the think() method provided with this template.
*/
void idle(void)
{
	// Wait until it's time to render the next frame.

	unsigned int frameTimeElapsed = (unsigned int)glutGet(GLUT_ELAPSED_TIME) - frameStartTime;
	if (frameTimeElapsed < FRAME_TIME)
	{
		// This frame took less time to render than the ideal FRAME_TIME: we'll suspend this thread for the remaining time,
		// so we're not taking up the CPU until we need to render another frame.
		unsigned int timeLeft = FRAME_TIME - frameTimeElapsed;
		Sleep(timeLeft);
	}

	// Begin processing the next frame.

	frameStartTime = glutGet(GLUT_ELAPSED_TIME); // Record when we started work on the new frame.

	think(); // Update our simulated world before the next call to display().

	glutPostRedisplay(); // Tell OpenGL there's a new frame ready to be drawn.
}

/******************************************************************************
 * Animation-Specific Functions (Add your own functions at the end of this section)
 ******************************************************************************/

/*
	Initialise OpenGL and set up our scene before we begin the render loop.
*/
void init(void)
{
	// set window mode to 2D orthographic and set the window size
	gluOrtho2D(0.0, width, 0.0, height);

	// Randomize vertices and color
	srand(time(NULL));  // Seed for random number generation

	groundVertices[0].x = 0.0;
	groundVertices[0].y = 200.0;	
	
	groundVertices[1].x = rand() % 100 / 1.0f + 50;
	groundVertices[1].y = rand() % 100 / 1.0f + 250;

	groundVertices[2].x = width - rand() % 100 / 1.0f - 50;
	groundVertices[2].y = groundVertices[1].y;

	groundVertices[3].x = 1000.0;
	groundVertices[3].y = 200.0;

}

/*
	Advance our animation by FRAME_TIME milliseconds.

	Note: Our template's GLUT idle() callback calls this once before each new
	frame is drawn, EXCEPT the very first frame drawn after our application
	starts. Any setup required before the first frame is drawn should be placed
	in init().
*/
void think(void)
{
	for (int i = 0; i < snowCount; i++) {
		snowParticles[i].y -= snowParticles[i].speed / 10;
		snowParticles[i].x += rand() % 4 - 1;
		if (snowParticles[i].y < 20) {
			if (snowFall) {
				createSnow(i);
			}
		}
	}

	if (snowCount != 0 && !snowFall) {
		for (int i = 0; i < snowCount; i++) {
			if (snowParticles[i].y < 20) {
				for (int j = i; i < snowCount - 1; i++) {
					snowParticles[i] = snowParticles[i + 1];
				}
				snowCount--;
			}
		}
	}

	//Add Snow
	if (snowCount < MAX_PARTICLES && snowFall) {
		for (int i = snowCount; i < snowCount + 1; i++) {
			createSnow(i);
		}
		snowCount++;
	}
}

void createSnow(int i) {
	snowParticles[i].x = rand() % 100 / 100.0f;
	snowParticles[i].y = rand() % 50 / 50.0f + height;
	snowParticles[i].size = rand() % 5 / 5.0f + 1;
	snowParticles[i].speed = rand() % 2 + 1;
	snowParticles[i].transparency = rand() % 10;
}

void static setColour(int r, int g, int b) {
	glColor3f(r / 255.0f, g / 255.0f, b / 255.0f);
}

void drawBackground(void) {
	//Draw the sky
	glBegin(GL_POLYGON);

	setColour(118, 186, 251);
	glVertex2f(0, 0);
	glVertex2f(width, 0);

	setColour(6, 130, 195);
	glVertex2f(width, height);
	glVertex2f(0, height);

	glEnd();


	//Draw the ground
	glBegin(GL_POLYGON);
	setColour(255, 250, 253);
	glVertex2f(width, 0.0);
	glVertex2f(0.0, 0.0);

	setColour(167, 191, 219);
	glVertex2f(groundVertices[0].x, groundVertices[0].y);
	glVertex2f(groundVertices[1].x, groundVertices[1].y);
	glVertex2f(groundVertices[2].x, groundVertices[2].y);
	glVertex2f(groundVertices[3].x, groundVertices[3].y);

	glEnd();
}

void drawCircle(float cx, float cy, float r, int numSegments, Colour inner, Colour outer) {
	float angleIncrement = 2.0f * M_PI / (float)numSegments;

	glBegin(GL_TRIANGLE_FAN);
	setColour(inner.r, inner.g, inner.b);
	glVertex2f(cx, cy);

	setColour(outer.r, outer.g, outer.b);
	for (int i = 0; i <= numSegments; i++) {
		float angle = i * angleIncrement;
		float x = cx + r * cosf(angle);
		float y = cy + r * sinf(angle);
		glVertex2f(x, y);
	}
	glEnd();
}

void drawSnow(void) {
	setColour(255, 255, 255);
	for (int i = 0; i < snowCount; i++) {
		glPointSize(snowParticles[i].size);
		glBegin(GL_POINTS);
		glVertex2i(snowParticles[i].x, snowParticles[i].y);
		glEnd();
	}
}

/******************************************************************************/