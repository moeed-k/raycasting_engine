#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>
#include <iostream>
#include <string.h>
using namespace std;

//map resolution
#define GRID_SIZE 4 // size of each square in the grid
#define RES_WIDTH GRID_SIZE*12
#define RES_HEIGHT GRID_SIZE*8

//screen resolution. width/height = 12/8 to correctly render map. Also, screen resolution should be in multiples of 384 x 256 to correctly scale map drawing.
#define SCREEN_WIDTH 768 //  = 384 x 2
#define SCREEN_HEIGHT 512 // = 256 x 2

//map size is fixed at 12 x 8 on current implementation!
#define MAP_WIDTH 12
#define MAP_HEIGHT 8

const float PI = 3.1415926535f;
string map;

//initial player coordinates (start right in the middle of the screen
float playerX = RES_WIDTH / 2.0f;
float playerY = RES_HEIGHT / 2.0f;

//delta X/Y i.e changes in X/Y coordinates
float dPlayerX = 0.0f;
float dPlayerY = 0.0f;

float turnRadius = 1.0f; //this determines how large the 'step' of the player is along the direction the player is looking at. r = 1 = unit circle
float turnIncrement = 0.0174533 * 2;//(2.0f * PI) / 72.0f; //chopping up a full turn of the player (360 degrees = 2PI) into small increments
float fov = PI / 3.0f; // pi/3 is a 60 degree field of view
float playerAngle = PI/2.0f;
float stepRadius = 1.0f;

//ray direction coordinates
float rayX = playerX;
float rayY = playerY;
float rayAngle = playerAngle;


float floorShading = 1; //turns floor shading on/off
int* rayXValues = new int[SCREEN_WIDTH];
int* rayYValues = new int[SCREEN_WIDTH];

//this function makes sure that the angle of the player/ray stays between 0 and 2PI
float adjustAngle(float angle) {
	if (angle >= 2 * PI) {
		return angle - 2 * PI;
	}
	else if (angle <= 0) {
		return  2 * PI + angle;
	}
	if (angle == PI || angle == 2 * PI || angle == 0 || angle == 3 * PI / 2) { //accounting for division by 0 when using tan()
		return angle + 0.0001;
	}
	return angle;

}

//this function converts screen coordinates into map coordinates. The x and y coordinates are converted to a single point system
int getMapPoint(int row, int col) {
	return (MAP_WIDTH * (row / GRID_SIZE)) + (col / GRID_SIZE);
}

float getLineLength(float x1, float y1, float x2, float y2) {
	return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

float deg2rad(float degree) {
	return degree * 0.0174533;
}

void userInput(unsigned char key,int x,int y)
{
	//Why is directionX being added and directionY being subtracted?  This is because the directionX is standard, going right resolves to +ve values and left to -ve values.
	//For directionY, imagine this. First imagine a unit circle. The arm at 90 degrees looks directly up, while the arm at 95 degrees faces slightly lower than that. At 90 degrees, you are facing directly up, and the value of Sin(90) is 1. Now you add 5 degrees and Sin(95) = 0.996 is a little less than 1. Now since the top left of the screen
	//is the origin at {0,0}, subtracting 1 from the playerY gives a lower value of Y than subtracting 0.996. The lower the value of Y, the higher the pixel is rendered on the screen.

	if (key == 'w') {
		if (map[getMapPoint(playerY - (dPlayerY*stepRadius*stepRadius), playerX + (dPlayerX*stepRadius))] != 35) {
			playerX += dPlayerX*stepRadius;
			playerY -= dPlayerY*stepRadius;
		}
	}

	if (key == 's') {
		if (map[getMapPoint(playerY + (dPlayerY*stepRadius), playerX - (dPlayerX*stepRadius))] != 35) {
			playerX -= dPlayerX*stepRadius;
			playerY += dPlayerY*stepRadius;
		}
	}

	if (key == 'd') {
		playerAngle -= turnIncrement;

		dPlayerX = (cos(playerAngle) * turnRadius); //recall that CosX = B/H, so H.CosX = B, where B is the magnitude of the X-axis. For a unit circle, CosX = B. The larger the value of H (which is the radius), the larger the displacement along the X-Axis.
		dPlayerY = (sin(playerAngle) * turnRadius);

		playerAngle = adjustAngle(playerAngle);
	}

	if (key == 'a') {
		playerAngle += turnIncrement;

		dPlayerX = (cos(playerAngle) * turnRadius);
		dPlayerY = (sin(playerAngle) * turnRadius);

		playerAngle = adjustAngle(playerAngle);
	}

	if (key == 'q') {
		float strafeAngle = adjustAngle(playerAngle + PI/2);

		float dPlayerXStrafe = (cos(strafeAngle) * turnRadius);
		float dPlayerYStrafe = (sin(strafeAngle) * turnRadius);

		if (map[getMapPoint(playerY - dPlayerYStrafe, playerX + dPlayerXStrafe)] != 35) {
			playerX += dPlayerXStrafe;
			playerY -= dPlayerYStrafe;
		}
	}

	if (key == 'e') {
		float strafeAngle = adjustAngle(playerAngle - PI / 2);

		float dPlayerXStrafe = (cos(strafeAngle) * turnRadius);
		float dPlayerYStrafe = (sin(strafeAngle) * turnRadius);

		if (map[getMapPoint(playerY - dPlayerYStrafe, playerX + dPlayerXStrafe)] != 35) {
			playerX += dPlayerXStrafe;
			playerY -= dPlayerYStrafe;
		}
	}

	if (key == 'p'){
		if(floorShading){
			floorShading = 0;
		}
		else{
			floorShading = 1;
		}
	}

	glutPostRedisplay();
}


void initMap()
{
	map += "############";
	map += "#..#.......#";
	map += "#........#.#";
	map += "#...#......#";
	map += "#...#..#...#";
	map += "#....##....#";
	map += "##......#..#";
	map += "############";

	//initial transformation values for player movement
	dPlayerX = (cos(playerAngle) * turnRadius);
	dPlayerY = (sin(playerAngle) * turnRadius);

	glClearColor(0,0,0,0);
	gluOrtho2D(0,SCREEN_WIDTH,SCREEN_HEIGHT,0);
 }


void drawWorld(){
			rayAngle = playerAngle + fov / 2.0f;
			rayAngle = adjustAngle(rayAngle);

	 	 	float horX, horY, verX, verY; //first coordinates of horizontal and vertical intersections
	 		float verticalDistance, horizontalDistance, selectedDistance;
	 		float incrementHorX, incrementHorY, incrementVerX, incrementVerY; //vertical and horizontal offsets
	 		int totalRays = SCREEN_WIDTH;
	 		float rayAngleIncrement = fov / totalRays;

	 		char select;

	 		for (int i = 0; i < totalRays; i++) {
	 			bool checkVertical = true;
	 			bool checkHorizontal = true;
	 			bool hitWall = false;

	 			//calculating first horizontal coordinates and the subsequent offsets
	 			if (rayAngle == 0 || rayAngle == PI || rayAngle == 2 * PI) {// if player is looking directly left or right there will never be a horizontal intersection
	 				checkHorizontal = false;
	 			}
	 			else if (rayAngle < PI && rayAngle > 0) { //player is looking up
	 				horY = (int)(playerY / GRID_SIZE) * GRID_SIZE;// -0.00001;
	 				horX = playerX + ((playerY - horY) / tan(rayAngle));

	 				incrementHorY = -1 * GRID_SIZE;
	 				incrementHorX = GRID_SIZE / tan(rayAngle);
	 			}
	 			else {//player is looking downwards
	 				horY = (int)(playerY / GRID_SIZE) * GRID_SIZE + GRID_SIZE;
	 				horX = playerX + ((playerY - horY) / tan(rayAngle));

	 				incrementHorY = GRID_SIZE;
	 				incrementHorX = -1 * GRID_SIZE / tan(rayAngle);
	 			}


	 			//calculating first vertical coordinates and the subsequent offsets
	 			if (rayAngle == PI / 2.0f || rayAngle == 3 * PI / 2.0f) { // if player is looking directly up or down there will never be a vertical intersection
	 				checkVertical = false;
	 			}
	 			else if (rayAngle > PI / 2.0f && rayAngle < 3.0f * PI / 2.0f) { //player is looking leftwards
	 				verX = int(playerX / GRID_SIZE) * GRID_SIZE;
	 				verY = playerY + (playerX - verX) * tan(rayAngle);

	 				incrementVerX = -1 * GRID_SIZE;
	 				incrementVerY = GRID_SIZE * tan(rayAngle);
	 			}
	 			else {//player is looking rightwards
	 				verX = int(playerX / GRID_SIZE) * GRID_SIZE + GRID_SIZE;
	 				verY = playerY + (playerX - verX) * tan(rayAngle);

	 				incrementVerX = GRID_SIZE;
	 				incrementVerY = -1 * GRID_SIZE * tan(rayAngle);
	 			}


	 			//checking horizontal intersections
	 			while (!hitWall && checkHorizontal) {
	 				int checkPointX, checkPointY;

	 				if (rayAngle < PI) {
	 					checkPointX = horX;
	 					checkPointY = horY - 1;
	 				}
	 				else {
	 					checkPointX = horX;
	 					checkPointY = horY;
	 				}

	 				if (horX <= 0 || horX >= RES_WIDTH || horY <= 0 || horY >= RES_HEIGHT) { //bound testing
	 					hitWall = true;
	 				}
	 				else if (map[getMapPoint(checkPointY, checkPointX)] == 35) {
	 					hitWall = true;
	 				}
	 				else {
	 					horX += incrementHorX;
	 					horY += incrementHorY;
	 				}
	 			}


	 			//checking vertical intersections
	 			hitWall = false;
	 			while (!hitWall && checkVertical) {
	 				int checkPointX, checkPointY;

	 				if (rayAngle > PI/2.0f && rayAngle < 3.0f*PI/2.0f) {
	 					checkPointX = verX - 1;
	 					checkPointY = verY;
	 				}
	 				else {
	 					checkPointX = verX;
	 					checkPointY = verY;
	 				}

	 				if (verX <= 0 || verX >= RES_WIDTH || verY <= 0 || verY >= RES_HEIGHT) { //bound testing
	 					hitWall = true;
	 				}
	 				else if (map[getMapPoint(checkPointY, checkPointX)] == 35) {
	 					hitWall = true;
	 				}
	 				else {
	 					verX += incrementVerX;
	 					verY += incrementVerY;
	 				}
	 			}

	 			//comparing lengths of the horizontal and vertical intersections and selecting the smallest one
	 			if (checkVertical) {
	 				verticalDistance = getLineLength(playerX, playerY, verX, verY);
	 			}
	 			else {
	 				verticalDistance = 9999;
	 			}

	 			if (checkHorizontal) {
	 				horizontalDistance = getLineLength(playerX, playerY, horX, horY);
	 			}
	 			else {
	 				horizontalDistance = 9999;
	 			}

	 			if (verticalDistance < horizontalDistance) {
	 				selectedDistance = verticalDistance;
	 				rayX = verX;
	 				rayY = verY;
	 				select = 'v';
	 			}
	 			else {
	 				selectedDistance = horizontalDistance;
	 				rayX = horX;
	 				rayY = horY;
	 				select = 'h';
	 			}

	 			//fishEye effect correction
	 			selectedDistance = selectedDistance * cos(playerAngle - rayAngle);

	 			rayAngle -= rayAngleIncrement;
	 			rayAngle = adjustAngle(rayAngle);

	 			rayXValues[i] = rayX;
	 			rayYValues[i] = rayY;

	 			//drawing3Dview
	 			int wall = (SCREEN_HEIGHT / selectedDistance);
	 			int ceiling = SCREEN_HEIGHT / 2 - wall;
	 			int floor = SCREEN_HEIGHT - ceiling;

	 			//ceiling
	 			glColor3f(0,0.8,0.8);
	 			glBegin(GL_LINES);
	 			glVertex2i(i,0);
	 			glVertex2i(i,ceiling);
	 			glEnd();

	 			//wall
	 			float wallShader;
	 			if (select == 'h'){
	 				wallShader = 0.2;
	 			}
	 			else{
	 				wallShader = 0.4;
	 			}

	 			glColor3f(wallShader*10/(selectedDistance*0.8),0,0);
	 			glBegin(GL_LINES);
	 			glVertex2i(i,ceiling);
	 			glVertex2i(i,floor);
	 			glEnd();

	 			//floor shading. Shades floor row by row by increasing lowest floor value from completely dark to progressively whiter
	 			int count = SCREEN_HEIGHT-floor;
	 			if (floorShading){
	 				for (int shader = 0; shader < count; shader++){
	 					glColor3f(0.1*shader/2,0.1*shader/2,0.1*shader/2);
	 					glBegin(GL_LINES);
	 					glVertex2i(i,floor);
	 					glVertex2i(i,floor + shader);
	 					glEnd();
	 					floor = floor+shader;
	 				}
	 			}
	 			else{
	 				glColor3f(0.1,0.1,0.1);
	 				glBegin(GL_LINES);
	 				glVertex2i(i,floor);
	 				glVertex2i(i,SCREEN_HEIGHT);
	 				glEnd();

	 			}
	 		}
}

void drawMap()
{
	//top right position of map on screen
	float xOffset = 3*SCREEN_WIDTH/4;
	float yOffset = 0;

	//drawing map base

	glColor3f(0.0, 0.0, 0.0);
	glBegin(GL_POLYGON);
	glVertex2i(0+xOffset,SCREEN_HEIGHT/4+yOffset);
	glVertex2i(0+xOffset, 0+yOffset);
	glVertex2i(SCREEN_WIDTH/4+xOffset, 0+yOffset);
	glVertex2i(SCREEN_WIDTH/4+xOffset, SCREEN_HEIGHT/4+yOffset);
	glEnd();

	//drawing map grid
	int boxWidth = (SCREEN_WIDTH/4)/MAP_WIDTH; //map width takes 1/4th screen area. This area is further divided into MAP_WIDTH (12) boxes.

	for (int col = 0; col < MAP_WIDTH; col++){
		for (int row = 1; row <= MAP_HEIGHT; row++){
			if (map[(row-1)*MAP_WIDTH + col] == 35){
				glColor3f(0.0, 0.0, 1.0);
				glBegin(GL_POLYGON);
				glVertex2i(col*boxWidth+1+xOffset,row*boxWidth-1+yOffset);//the +-1 offsets are there to draw the black outlines of grids in the map
				glVertex2i(col*boxWidth+1+xOffset, row*boxWidth-boxWidth+1+yOffset);
				glVertex2i(col*boxWidth+boxWidth-1+xOffset,row*boxWidth-boxWidth +1+yOffset);
				glVertex2i(col*boxWidth+boxWidth-1+xOffset,row*boxWidth-1+yOffset);
				glEnd();
			}
		}
	}

	int playerWidth = (SCREEN_WIDTH/(RES_WIDTH))/4; //gives scaling factor. Remember that if the grid size is 4, it means the map size is 12*4 x 8*4 = 48x32. Since this is being drawn on a screen res of 768x512 for example, this means the screen res has been scaled up 16 times because 48*16 x 32*16 = 768 x 512. So the scaling factor, to convert 1 pixel of the 48x32 map to 768x512 map, is 16. But since the map on the screen is only 1/4th the height/width of the screen, this 16 is downsized by 1/4th to become 4.
	//drawing player
	glColor3f(0.0, 1.0, 0.0);
	glBegin(GL_POLYGON);
	glVertex2i(playerX*playerWidth+xOffset,playerY*playerWidth+yOffset);
	glVertex2i(playerX*playerWidth+xOffset, playerY*playerWidth-playerWidth+yOffset);
	glVertex2i(playerX*playerWidth+playerWidth+xOffset,playerY*playerWidth-playerWidth+yOffset);
	glVertex2i(playerX*playerWidth+playerWidth+xOffset,playerY*playerWidth+yOffset);
	glEnd();

	//drawing rays
	glColor3f(0.0, 0.5, 0.0);
	for (int i = 0; i < SCREEN_WIDTH; i++){
		glBegin(GL_LINES);
		glVertex2i(playerX*playerWidth+xOffset + GRID_SIZE/2,playerY*playerWidth+yOffset -GRID_SIZE/2); //player/ray coordinates are 'pixels' that have to be scaled according to the resolution. if screen res is 768 x 512
		glVertex2i(rayXValues[i]*playerWidth+xOffset, rayYValues[i]*playerWidth+yOffset);
		glEnd();
	}
	glutPostRedisplay();

}
void display()
{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawWorld();
		drawMap();
		glutSwapBuffers();
}

int main(int argc, char* argv[])
{
 glutInit(&argc, argv);
 glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
 glutInitWindowSize(SCREEN_WIDTH,SCREEN_HEIGHT);
 glutCreateWindow("rayMKaster");
 initMap();

 glutDisplayFunc(display);
 glutKeyboardFunc(userInput);
 glutMainLoop();
}
