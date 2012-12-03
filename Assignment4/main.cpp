#include <stdlib.h>
#ifdef __APPLE__
#include <GLUT/GLUT.h>
#else
#include <gl/glut.h>
#endif
#include "common.h"
#include "Particle.h"
#include <math.h>
#include <vector>
#include <list>
#include <stdio.h>
#include <cstdio>
#include <ctime>
//using namespace Raytracer;
using namespace std;

int winW = 512;
int winH = 512;

#define APP_NAME "Assignment 4 - Tempest"

void glutDisplay();
void glutTimer(int arg);
void glutKeyboard(unsigned char key, int x, int y);
void glutSpecial(int key, int xx, int yy);
void glutResize(int width, int height);

void makeLevelGeometry(int level, vector<vector3> *level_geometry, vector<vector3> *ship_points);
void makeLevel1Geometry(vector<vector3> *level_geometry, vector<vector3> *ship_points);
void makeLevel2Geometry(vector<vector3> *level_geometry, vector<vector3> *ship_points);
void makeLevel3Geometry(vector<vector3> *level_geometry, vector<vector3> *ship_points);

void displayHUD();
void updateBullets();
void updateEnemy();
void updateParticles();
void detectDeath();
void moveEnemyToRandomLocation();

void drawLevel(vector<vector3> level_geometry);

class ParticleEngine;

#define TIMER 16
#define TRANSITION_STEPS 50
#define LEVEL2_SCORE 30
#define LEVEL3_SCORE 60

float UPPER_LEVEL_Z = -1;
float LOWER_LEVEL_Z = 10;

struct position_info {
	int location;
	vector3 position;
};

vector<vector3> level_geometry;
vector<vector3> ship_points;
vector<position_info> bullets;
int ship_location = 0;
int enemy_location = 0;
float enemy_depth = LOWER_LEVEL_Z;
bool in_game = 0;
int transitioning_level = 0;
bool alive = 0;
int lives = 3;
int score = 0;
int level = 1;
ParticleEngine pengine;

time_t bullet_frequency = 0.5;
time_t last_bullet_time = 0;

#ifdef __APPLE__
int main(int argc, char **argv) {
    glutInit(&argc, argv);
#else
void main() {
#endif
	glutInitWindowSize(winW, winH);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

	glutCreateWindow(APP_NAME);
	glutDisplayFunc(glutDisplay);
	glutTimerFunc(TIMER, glutTimer, 0);
	glutReshapeFunc(glutResize);
	glutKeyboardFunc(glutKeyboard);
	glutSpecialFunc(glutSpecial);

	glClearColor(0.0, 0.0, 0.0, 0.0);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	makeLevel1Geometry(&level_geometry, &ship_points);

    srand((int)time(NULL));

	glutMainLoop();

    return 0;
}

void glutTimer(int arg) {
	glutPostRedisplay();
	glutTimerFunc(TIMER, glutTimer, 0);
}

void glutResize(int width, int height) {
	winW = width;
	winH = height;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, winW, winH);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glFrustum(-1.1, 1.1, -1.1, 1.1, .9, 100);
	glMatrixMode(GL_MODELVIEW);

	gluLookAt(0, -0.5, -2, // eye position
			  0, 0, 1,  // look at vector
			  0, 1, 0); // up vector
}

void glutKeyboard(unsigned char key, int x, int y) {
	switch (key) {
	case ' ':
		if (!in_game) {
			in_game = 1;
			alive = 1;
		} else if (alive) {
			time_t current = time(NULL);
			if (current >= last_bullet_time + bullet_frequency) {
				last_bullet_time = current;
				position_info bullet;
				bullet.location = ship_location;
				bullet.position = ship_points[ship_location];
				bullets.push_back(bullet);
			}
		}
		break;
	}
}

void glutSpecial(int key, int xx, int yy) {
    int size = (int)ship_points.size();
	switch (key) {
	case GLUT_KEY_LEFT:
		ship_location = (ship_location + size + 1) % size;
		break;
	case GLUT_KEY_RIGHT:
		ship_location = (ship_location + size - 1) % size;
		break;
	case GLUT_KEY_UP:
		break;
	case GLUT_KEY_DOWN:
		break;
	}
}

void glutDisplay() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!in_game) {
		glColor3f(1, 1, 1);
		glPushMatrix();
		glTranslatef(5,0,5);
		glScalef(1/152.38, 1/152.38, 1/152.38);
		glRotatef(180, 0, 1, 0);
		const char *text = "Press <space> to begin";
		for (const char *c = text; *c; c++) {
			glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
		}
		glPopMatrix();
		glutSwapBuffers();
		return;
	}

    if (transitioning_level) {
        vector<vector3> geo1, geo2, geo3, trash;
        makeLevelGeometry(level, &geo1, &trash);
        makeLevelGeometry(level+1, &geo2, &trash);
        for (int i=0; i<min(geo1.size(), geo2.size()); i++) {
            vector3 pt1 = geo1[i];
            vector3 pt2 = geo2[i];
            vector3 pt3 = pt1+transitioning_level*(pt2-pt1)/TRANSITION_STEPS;
            geo3.push_back(pt3);
        }
        drawLevel(geo3);
		glutSwapBuffers();
        if (++transitioning_level == TRANSITION_STEPS) {
            transitioning_level = 0;
            level++;
            makeLevelGeometry(level, &level_geometry, &ship_points);
            moveEnemyToRandomLocation();
            ship_location = 0;
        }
        return;
    }

	displayHUD();
	updateBullets();
	updateEnemy();
    updateParticles();
	detectDeath();

    drawLevel(level_geometry);

	// Draw the ship
	glColor3f(0.0f, 1.0f, 0.0f);
	glBegin(GL_TRIANGLES);
	vector3 location = ship_points[ship_location];
	glVertex3f(location.x, location.y+0.05, UPPER_LEVEL_Z);
	glVertex3f(location.x-0.04, location.y-0.01, UPPER_LEVEL_Z);
	glVertex3f(location.x+0.04, location.y-0.01, UPPER_LEVEL_Z);
	glEnd();

	// Draw the enemy
	glColor3f(1.0f, 0.0f, 0.0f);
	glPushMatrix();
	location = ship_points[enemy_location];
	glTranslatef(location.x, location.y, enemy_depth);
	gluDisk(gluNewQuadric(), 0, 0.1, 10, 10);
	glPopMatrix();
	glutSwapBuffers();
}

void drawLevel(vector<vector3> level_geometry) {
    glColor3f(0.0f, 0.0f, 1.0f);

	// Draw the upper geometry
	glBegin(GL_LINE_LOOP);
	for (int i=0; i<level_geometry.size(); i++) {
		vector3 pt = level_geometry[i];
		glVertex3f(pt.x, pt.y, UPPER_LEVEL_Z);
	}
	glEnd();
	// Draw the lower geometry
	glBegin(GL_LINE_LOOP);
	for (int i=0; i<level_geometry.size(); i++) {
		vector3 pt = level_geometry[i];
		glVertex3f(pt.x, pt.y, LOWER_LEVEL_Z);
	}
	glEnd();
	// Draw the lines between the upper and lower geometry
	glBegin(GL_LINES);
	for (int i=0; i<level_geometry.size(); i++) {
		vector3 pt = level_geometry[i];
		glVertex3f(pt.x, pt.y, UPPER_LEVEL_Z);
		glVertex3f(pt.x, pt.y, LOWER_LEVEL_Z);
	}
	glEnd();
}

void moveEnemyToRandomLocation() {
	enemy_depth = LOWER_LEVEL_Z;
	enemy_location = rand() % ship_points.size();
}

void makeLevelGeometry(int level, vector<vector3> *level_geometry, vector<vector3> *ship_points) {
    switch (level) {
        case 1:
            makeLevel1Geometry(level_geometry, ship_points);
            break;
        case 2:
            makeLevel2Geometry(level_geometry, ship_points);
            break;
        case 3:
            makeLevel3Geometry(level_geometry, ship_points);
            break;
        default:
            break;
    }
}

void makeLevel1Geometry(vector<vector3> *level_geometry, vector<vector3> *ship_points) {
	// Help from:
	//http://board.flashkit.com/board/showthread.php?773919-trying-to-find-coordinates-for-points-around-a-circle
    level_geometry->clear();
    ship_points->clear();

	float CIRCLE_SECTIONS = 16;
	float alpha = (PI*2)/CIRCLE_SECTIONS;

	for (int i=0; i<CIRCLE_SECTIONS; i++) {
		float theta = alpha*i;
		level_geometry->push_back(vector3(cos(theta), sin(theta), 0.0f));
		theta += (PI*2)/(CIRCLE_SECTIONS*2);
		ship_points->push_back(vector3(cos(theta), sin(theta), 0.0f));
	}
}

void makeLevel2Geometry(vector<vector3> *level_geometry, vector<vector3> *ship_points) {
    level_geometry->clear();
    ship_points->clear();

    vector3 start = vector3(1, 1, 0);
    vector3 end = vector3(-1, 1, 0);
    vector3 diff = end-start;
    for (int i=0; i<4; i++) {
        vector3 point = start+i*(diff)/4.0f;
        level_geometry->push_back(point);
        ship_points->push_back(point+(diff)/8.0f);
    }
    start = end;
    end = vector3(-1, -1, 0);
    diff = end-start;
    for (int i=0; i<4; i++) {
        vector3 point = start+i*(diff)/4.0f;
        level_geometry->push_back(point);
        ship_points->push_back(point+(diff)/8.0f);
    }
    start = end;
    end = vector3(1, -1, 0);
    diff = end-start;
    for (int i=0; i<4; i++) {
        vector3 point = start+i*(diff)/4.0f;
        level_geometry->push_back(point);
        ship_points->push_back(point+(diff)/8.0f);
    }
    start = end;
    end = vector3(1, 1, 0);
    diff = end-start;
    for (int i=0; i<4; i++) {
        vector3 point = start+i*(diff)/4.0f;
        level_geometry->push_back(point);
        ship_points->push_back(point+(diff)/8.0f);
    }
}

void makeLevel3Geometry(vector<vector3> *level_geometry, vector<vector3> *ship_points) {
    level_geometry->clear();
    ship_points->clear();

    vector3 start = vector3(0, 1, 0);
    vector3 end = vector3(-1, -1, 0);
    vector3 diff = end-start;
    for (int i=0; i<4; i++) {
        vector3 point = start+i*(diff)/4.0f;
        level_geometry->push_back(point);
        ship_points->push_back(point+(diff)/8.0f);
    }
    start = end;
    end = vector3( 1, -1, 0);
    diff = end-start;
    for (int i=0; i<4; i++) {
        vector3 point = start+i*(diff)/4.0f;
        level_geometry->push_back(point);
        ship_points->push_back(point+(diff)/8.0f);
    }
    start = end;
    end = vector3(0, 1, 0);
    diff = end-start;
    for (int i=0; i<4; i++) {
        vector3 point = start+i*(diff)/4.0f;
        level_geometry->push_back(point);
        ship_points->push_back(point+(diff)/8.0f);
    }
}

void displayHUD() {
	glColor3f(0, 1, 0);
	glPushMatrix();
	glTranslatef(7,-5,5);
	glScalef(1/152.38, 1/152.38, 1/152.38);
	glRotatef(180, 0, 1, 0);
	char text[100];
#ifdef __APPLE__
    snprintf(text, 100, "Lives: %d", lives);
#else
	sprintf_s(text, 100, "Lives: %d", lives);
#endif
	for (char *c = text; *c; c++) {
		glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
	}
	glPopMatrix();
	glPushMatrix();
	glTranslatef(7,-6,5);
	glScalef(1/152.38, 1/152.38, 1/152.38);
	glRotatef(180, 0, 1, 0);
#ifdef __APPLE__
    snprintf(text, 100, "Score: %d", score);
#else
	sprintf_s(text, 100, "Score: %d", score);
#endif
	for (char *c = text; *c; c++) {
		glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
	}
	glPopMatrix();
}

void updateBullets() {
	vector<position_info>::iterator it;
	vector<position_info> new_bullets;

	for (it=bullets.begin(); it < bullets.end(); it++) {
		position_info bullet = *it;
		if (bullet.position.z >= LOWER_LEVEL_Z) {
			continue;
		}
		if (bullet.location == enemy_location && abs((float)enemy_depth - (float)bullet.position.z < 0.5)) {
			moveEnemyToRandomLocation();
			score += 10;
            if (score >= LEVEL2_SCORE && level == 1)
                transitioning_level = 1;
            else if (score >= LEVEL3_SCORE && level == 2)
                transitioning_level = 2;
            pengine = ParticleEngine(bullet.position);
            pengine.start();
			continue;
		}
#ifdef __APPLE__
		bullet.position.z += fabs(UPPER_LEVEL_Z - LOWER_LEVEL_Z)/20;
#else
		bullet.position.z += abs(UPPER_LEVEL_Z - LOWER_LEVEL_Z)/20;
#endif
		glColor3f(1, 0, 1);
		glPushMatrix();
		glTranslatef(bullet.position.x, bullet.position.y, bullet.position.z);
		gluDisk(gluNewQuadric(), 0, 0.1, 10, 10);
		glPopMatrix();
		new_bullets.push_back(bullet);
	}
	
	bullets = new_bullets;
}

void updateEnemy() {
	// If the enemy has reached the top, move it to the bottom of a random lane
	if (enemy_depth <= UPPER_LEVEL_Z) {
		moveEnemyToRandomLocation();
	} else {
#ifdef __APPLE__
		enemy_depth -= fabs(UPPER_LEVEL_Z - LOWER_LEVEL_Z)/125.0;
#else
        enemy_depth -= abs(UPPER_LEVEL_Z - LOWER_LEVEL_Z)/125.0;
#endif
	}
}

void updateParticles() {
    if (pengine.isRunning()) {
        pengine.update();
        pengine.draw();
    }
}

void detectDeath() {
	// If the ship and enemy are in the same column close to each other
#ifdef __APPLE__
    if (ship_location == enemy_location && fabs((float)enemy_depth - (float)UPPER_LEVEL_Z) < 0.5) {
#else
	if (ship_location == enemy_location && abs((float)enemy_depth - (float)UPPER_LEVEL_Z) < 0.5) {
#endif
		ship_location = 0;
		moveEnemyToRandomLocation();
		lives--;
		if (lives == 0) {
			in_game = 0;
			alive = 0;
			score = 0;
            lives = 3;
		}
	}
}