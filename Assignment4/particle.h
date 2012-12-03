//
//  Particle.h
//  CSC562-Assignment4
//
//  Created by Chris Suich on 11/21/12.
//  Copyright (c) 2012 Chris Suich. All rights reserved.
//

#ifdef __APPLE__
#include <GLUT/GLUT.h>
#else
#include <gl/glut.h>
#endif
#include "common.h"

#define NUM_PARTICLES 100

float randomFloat();

struct Particle {
    vector3 position;
    vector3 velocity;
    vector3 color;
    int timeLeft;
};

class ParticleEngine {
private:
    Particle particles[NUM_PARTICLES];
    bool running = true;
public:
    ParticleEngine() {
        ParticleEngine(vector3());
    }

    ParticleEngine(vector3 position) {
        for (int i=0; i<NUM_PARTICLES; i++) {
            Particle p;
            p.position = position;
            p.position.x += 0.1f*(randomFloat()-0.5f);
            p.position.y += 0.1f*(randomFloat()-0.5f);
            p.velocity = vector3(0.1f*(randomFloat()-0.5f), 0.1f*(randomFloat()-0.5f), 0.1f*(randomFloat()-0.5f));
            p.velocity.Normalize();
            p.velocity = p.velocity * 0.1f;
            p.color = vector3(randomFloat(), randomFloat(), randomFloat());
            p.timeLeft = randomFloat()*20;
            particles[i] = p;
        }
    }

    bool isRunning() {
        return running;
    }

    void start() {
        running = true;
    }

    void stop() {
        running = false;
    }

    void update() {
        if (!isRunning())
            return;

        int count = 0;
        for (int i=0; i<NUM_PARTICLES; i++) {
            Particle *p = &particles[i];
            if (p->timeLeft <= 0)
                continue;
            p->timeLeft--;
            p->position += p->velocity;
            count++;
        }
        if (count==0)
            running = false;
    }

    void draw() {
        if (!isRunning())
            return;

        for (int i=0; i<NUM_PARTICLES; i++) {
            Particle p = particles[i];
            if (p.timeLeft <= 0)
                continue;
            // Draw the particle
            glColor3f(p.color.x, p.color.y, p.color.z);
            glPushMatrix();
            glTranslatef(p.position.x, p.position.y, p.position.z);
            gluDisk(gluNewQuadric(), 0, 0.1, 10, 10);
            glPopMatrix();
        }
    }
};

float randomFloat() {
    return (float)rand() / ((float)RAND_MAX + 1);
}