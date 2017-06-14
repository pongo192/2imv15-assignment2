//
// Created by Ruud Andriessen on 03/05/2017.
//

#include "System.h"
#include "solvers/Solver.h"
#include "solvers/ConstraintSolver.h"
#include "fields/PressureField.h"
#include "fields/ColorField.h"

#if defined(_WIN32) || defined(WIN32)

#include <GL/glut.h>

#else
#include <GLUT/glut.h>
#endif

System::System(Solver *solver) : solver(solver), time(0.0f), wallExists(false), dt(0.001) {
    densityField = new DensityField(this);
    pressureField = new PressureField(this);
    colorField = new ColorField(this);
}

System::~System() {
    delete densityField;
    delete pressureField;
    delete colorField;
}
/**
 * Adds a given particle to the system
 * @param p The particle to add
 */
void System::addParticle(Particle *p) {
    particles.push_back(p);
    for (Force* f : forces) {
        f->addAsTarget(p);
    }
}

/**
 * Adds a force to use in the system when advancing a time step
 * @param f The new force to use in the system
 */
void System::addForce(Force *f) {
    forces.push_back(f);
}

/**
 * Adds a constraint to use in the system when advancing a time step
 * @param c The new constraint to use in the system
 */
void System::addConstraint(Constraint *c) {
    constraints.push_back(c);
}

/**
 * Frees all system data
 */
void System::free() {
    particles.clear();
    forces.clear();
}

/**
 * Resets all the system to it's initial state
 */
void System::reset() {
    for (Particle *p : particles) {
        p->reset();
    }
}

/**
 * Draws the forces
 */
void System::draw(bool drawVelocity, bool drawForce, bool drawConstraint) {
    drawParticles(drawVelocity, drawForce);
    if (drawForce) {
        drawForces();
    }
    if (drawConstraint){
        drawConstraints();
    }
}

/**
 * Runs the active solver on the system to progress it's state by dt time
 * @param dt the amount of time to advance the system
 */
void System::step(bool adaptive) {
    if (adaptive) {
        VectorXf before = this->getState();
        solver->simulateStep(this, dt);
        VectorXf xa = this->getState();
        this->setState(before);

        solver->simulateStep(this, dt / 2);
        solver->simulateStep(this, dt / 2);
        VectorXf xb = this->getState();

        float err = (xa - xb).norm();
        if (err > 0)
            dt *= pow(0.001f / err, .5f);

        this->setState(before);
    }

    solver->simulateStep(this, dt);
}


unsigned long System::getDim() {
    return particles.size() * 3 * 2; // 3 dimensions, velocity and position
}

/**
 * Constructs a state given the current system
 * @return A copy of the current state of the system
 */
VectorXf System::getState() {
    VectorXf r(this->getDim());

    for (int i = 0; i < this->particles.size(); i++) {
        Particle *p = particles[i];
        r[i * 6 + 0] = p->position[0];
        r[i * 6 + 1] = p->position[1];
        r[i * 6 + 2] = p->position[2];
        r[i * 6 + 3] = p->velocity[0];
        r[i * 6 + 4] = p->velocity[1];
        r[i * 6 + 5] = p->velocity[2];
    }

    return r;
}

float System::getTime() {
    return time;
}

/**
 * Evaluates a derivative
 * @param dst The destination vector
 */
VectorXf System::derivEval() {
    clearForces();
    computeForces();
//    ConstraintSolver::solve(this, 100.0f, 10.0f);
    return computeDerivative();
}

void System::setState(VectorXf src) {
    this->setState(src, this->getTime());
}

void System::setState(VectorXf src, float t) {
    for (int i = 0; i < particles.size(); i++) {
        if (particles[i]->movable) {
            particles[i]->position[0] = src[i * 6 + 0];
            particles[i]->position[1] = src[i * 6 + 1];
            particles[i]->position[2] = src[i * 6 + 2];
            particles[i]->velocity[0] = src[i * 6 + 3];
            particles[i]->velocity[1] = src[i * 6 + 4];
            particles[i]->velocity[2] = src[i * 6 + 5];
        }
    }
    this->time = t;
}

/// Private ///

void System::computeForces() {
    // Compute all densities
    float restDensity = 0;
    for (Particle *p : particles) {
        p->density = densityField->eval(p);
        restDensity += p->density;
    }

    float k = .1f;
    restDensity /= particles.size();

    // Compute all pressures at each particle
    for (Particle* p : particles) {
        p->pressure = k * (p->density - restDensity);
    }

    // Apply all forces
    for (Force *f : forces) {
        f->apply(this);
    }
}

void System::clearForces() {
    for (Particle *p : particles) {
        p->force = Vector3f(0.0f, 0.0f, 0.0f);
    }
}

VectorXf System::computeDerivative() {
    VectorXf dst(this->getDim());
    for (int i = 0; i < particles.size(); i++) {
        Particle *p = particles[i];
        dst[i * 6 + 0] = p->velocity[0];            /* Velocity */
        dst[i * 6 + 1] = p->velocity[1];
        dst[i * 6 + 2] = p->velocity[2];
        dst[i * 6 + 3] = p->force[0] / p->density;  /* new acceleration is F/density */
        dst[i * 6 + 4] = p->force[1] / p->density;
        dst[i * 6 + 5] = p->force[2] / p->density;
    }
    return dst;
}

void System::drawParticles(bool drawVelocity, bool drawForce) {
    for (Particle *p : particles) {
        p->draw(drawVelocity, drawForce);
    }
}

void System::drawForces() {
    for (Force *f : forces) {
        f->draw();
    }
}

void System::drawConstraints() {
    for (Constraint *c : constraints) {
        c->draw();
    }
}

VectorXf System::checkCollisions(VectorXf newState) {
    //collision from x side
    for (int i = 0; i < particles.size(); i++) {
        if (newState[i * 6] < -0.2f) {
            newState[i * 6] = -0.2f;
            if(newState[i*6+3]<0){
                newState[i*6+3]=-newState[i*6+3];
            }
        }
    }
    for (int i = 0; i < particles.size(); i++) {
        if (newState[i * 6] > 0.2f) {
            newState[i * 6] = 0.2f;
            if(newState[i*6+3]>0){
                newState[i*6+3]=-newState[i*6+3];
            }
        }
    }
    //collision from z side
    for (int i = 0; i < particles.size(); i++) {
        if (newState[i * 6+2] < -0.2f) {
            newState[i * 6+2] = -0.2f;
            if(newState[i*6+5]<0){
                newState[i*6+5]=-newState[i*6+3];
            }
        }
    }
    for (int i = 0; i < particles.size(); i++) {
        if (newState[i * 6+2] > 0.2f) {
            newState[i * 6+2] = 0.2f;
            if(newState[i*6+5]>0){
                newState[i*6+5]=-newState[i*6+3];
            }
        }
    }
    //Check collision with y side
    for (int i = 0; i < particles.size(); i++) {
        if(newState[i * 6 + 1]<-2.0f){
            newState[i * 6 + 1]=-2.0f;
            if(newState[i*6+4]<0){
                newState[i*6+4]=-newState[i*6+4];
            }
        }
    }
    return newState;
}
