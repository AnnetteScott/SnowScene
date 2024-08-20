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
#define MAX_PARTICLES 1500
#define M_PI 3.14159f
#define JUMP_TIME 48

int width = 1000;
int height = 1000;

int snowCount = 0;
int timeJumping = 0;
bool snowFall = false;
bool showDiagnostic = true;
bool jumping = false;

typedef struct {
	float x, y;
} Point;

typedef struct {
	int r, g, b;
} Colour;

typedef struct {
	float x, y, speed, size, transparency;
} Snow;

typedef struct {
	float cx, cy, r;
	int segments;
	Colour inner, outer;
} Snowman;

Colour WHITE = { 255, 255, 255 };
Colour GREY = { 130, 151, 173 };
Colour BLACK = { 0, 0, 0 };
Colour ORANGE = { 245, 127, 42 };


Point groundVertices[4];
Snow snowParticles[MAX_PARTICLES];
Snowman snowman[6];

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
#define KEY_JUMP			32 // D key.
#define KEY_D				100 // D key.
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
void setColour(int r, int g, int b, float a);
void drawBackground(void);
void drawCircle(float cx, float cy, float r, int numSegments, Colour inner, Colour outer);
void drawSnow(void);
void drawSnowman(void);
void displayDebug(void);

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
	glutInitWindowSize(width, height);
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
	drawSnowman();
	
	//Draw snow if snow is allowed to fall
	if (snowCount != 0) {
		drawSnow();
	}

	if (showDiagnostic) {
		displayDebug();
	}

	glutSwapBuffers();
}

/*
	Called when the OpenGL window has been resized.
*/
void reshape(int newWidth, int newHeight)
{
	glViewport(0, 0, newWidth, newHeight);

	// Switch to projection matrix mode
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Keep the projection fixed to the same orthogonal range (0 to 1)
	gluOrtho2D(0.0f, 1.0f, 0.0f, 1.0f);

	// Switch back to model view matrix mode
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
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
		case KEY_JUMP:
			if (!jumping) {
				jumping = true;
			}
			break;
		case KEY_D:
			showDiagnostic = !showDiagnostic;
			break;
		case KEY_Q:
			exit(0);
			break;
		case KEY_EXIT:
			exit(0);
			break;
		//default:
		//	OutputDebugStringW(key);
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
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gluOrtho2D(0.0f, 1.0f, 0.0f, 1.0f);

	srand(time(NULL));  

	groundVertices[0].x = 0.0f;
	groundVertices[0].y = 0.200f;
	
	groundVertices[1].x = rand() % 100 / 1000.0f + 0.050f;
	groundVertices[1].y = rand() % 100 / 1000.0f + 0.250f;

	groundVertices[2].x = 1.0f - rand() % 100 / 1000.0f - 0.050f;
	groundVertices[2].y = groundVertices[1].y;

	groundVertices[3].x = 1.0f;
	groundVertices[3].y = 0.200f;

	Snowman bottom = { 0.500f, 0.300f, 0.100f, 100, WHITE, GREY };
	Snowman mid = { 0.500f, 0.420f, 0.080f, 100, WHITE, GREY };
	Snowman top = { 0.500f, 0.520f, 0.060f, 100, WHITE, GREY };
	Snowman lEye = { 0.480f, 0.550f, 0.010f, 50, BLACK, BLACK };
	Snowman rEye = { 0.520f, 0.550f, 0.010f, 50, BLACK, BLACK };
	Snowman nose = { 0.500f, 0.520f, 0.012f, 7, ORANGE, ORANGE };

	snowman[0] = bottom;
	snowman[1] = mid;
	snowman[2] = top;
	snowman[3] = lEye;
	snowman[4] = rEye;
	snowman[5] = nose;
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
	//Add Snow
	if (snowCount < MAX_PARTICLES && snowFall) {
		createSnow(snowCount);
		snowCount++;
	}
	
	for (int i = 0; i < snowCount; i++) {
		snowParticles[i].y -= snowParticles[i].speed;
		snowParticles[i].x += (rand() % 4 - 1) / 10000.0f;
		if (snowParticles[i].y < 0.002 && snowFall) {
			createSnow(i);
		}
	}

	if (snowCount != 0 && !snowFall) {
		for (int i = 0; i < snowCount; i++) {
			if (snowParticles[i].y < 0.002) {
				for (int j = i; j < snowCount - 1; j++) {
					snowParticles[j] = snowParticles[j + 1];
				}
				snowCount--;
			}
		}
	}

	if (jumping && timeJumping < JUMP_TIME) {
		timeJumping++;

		int adjust = timeJumping > JUMP_TIME / 2 ? -1 : 1;
		float maxHeight = 0.008f;
		float normalizedTime = (float)timeJumping / JUMP_TIME;
		
		//Parabolic jumping height
		float height = maxHeight * (1 - pow(2 * normalizedTime - 1, 2));

		for (int i = 0; i < 6; i++) {
			snowman[i].cy += height * adjust;
		}
	}
	else if (timeJumping >= JUMP_TIME) {
		timeJumping = 0;
		jumping = false;
	}
}

void createSnow(int i) {
	snowParticles[i].x = rand() % 1000 / 1000.0f + 0.02f;
	snowParticles[i].y = 1.0f;
	snowParticles[i].size = rand() % 5 / 1.0f + 2;
	snowParticles[i].speed = snowParticles[i].size / 10000.0f + 0.0002f;
	snowParticles[i].transparency = rand() % 10 / 10.0f + 0.1f;
}

void static setColour(int r, int g, int b, float a) {
	glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, a);
}

void drawBackground(void) {
	//Draw the sky
	glBegin(GL_POLYGON);

	setColour(118, 186, 251, 1.0f);
	glVertex2f(0.0f, 0.0f);
	glVertex2f(1.0f, 0.0f);

	setColour(6, 130, 195, 1.0f);
	glVertex2f(1.0f, 1.0f);
	glVertex2f(0.0f, 1.0f);

	glEnd();


	//Draw the ground
	glBegin(GL_POLYGON);
	setColour(255, 250, 253, 1.0f);
	glVertex2f(1.0, 0.0);
	glVertex2f(0.0, 0.0);

	setColour(167, 191, 219, 1.0f);
	glVertex2f(groundVertices[0].x, groundVertices[0].y);
	glVertex2f(groundVertices[1].x, groundVertices[1].y);
	glVertex2f(groundVertices[2].x, groundVertices[2].y);
	glVertex2f(groundVertices[3].x, groundVertices[3].y);

	glEnd();
}

void drawCircle(float cx, float cy, float r, int numSegments, Colour inner, Colour outer) {
	float angleIncrement = 2.0f * M_PI / (float)numSegments;

	glBegin(GL_TRIANGLE_FAN);
	setColour(inner.r, inner.g, inner.b, 1.0f);
	glVertex2f(cx, cy);

	setColour(outer.r, outer.g, outer.b, 1.0f);
	for (int i = 0; i <= numSegments; i++) {
		float angle = i * angleIncrement;
		float x = cx + r * cosf(angle);
		float y = cy + r * sinf(angle);
		glVertex2f(x, y);
	}
	glEnd();
}

void drawSnow(void) {
	for (int i = 0; i < snowCount; i++) {
		setColour(255, 255, 255, snowParticles[i].transparency);
		glPointSize(snowParticles[i].size);
		glBegin(GL_POINTS);
		glVertex2f(snowParticles[i].x, snowParticles[i].y);
		glEnd();
	}
}

void drawSnowman(void) {
	drawCircle(snowman[0].cx, snowman[0].cy, snowman[0].r, snowman[0].segments, snowman[0].inner, snowman[0].outer);
	drawCircle(snowman[1].cx, snowman[1].cy, snowman[1].r, snowman[1].segments, snowman[1].inner, snowman[1].outer);
	drawCircle(snowman[2].cx, snowman[2].cy, snowman[2].r, snowman[2].segments, snowman[2].inner, snowman[2].outer);
	drawCircle(snowman[3].cx, snowman[3].cy, snowman[3].r, snowman[3].segments, snowman[3].inner, snowman[3].outer);
	drawCircle(snowman[4].cx, snowman[4].cy, snowman[4].r, snowman[4].segments, snowman[4].inner, snowman[4].outer);
	drawCircle(snowman[5].cx, snowman[5].cy, snowman[5].r, snowman[5].segments, snowman[5].inner, snowman[5].outer);
}

void displayDebug(void) {
	char infoString[200];
	sprintf_s(infoString, sizeof(infoString), "Diagnostics:\n particles: %d of %d\nScene controls:\n s: toggle snow\n q: quit\n d: toggle diagnostic\n space: jump", snowCount, MAX_PARTICLES);

	setColour(0, 0, 0, 1.0f);
	glRasterPos2f(0.02f, 0.95f);
	glutBitmapString(GLUT_BITMAP_HELVETICA_12, infoString);
}

/******************************************************************************/