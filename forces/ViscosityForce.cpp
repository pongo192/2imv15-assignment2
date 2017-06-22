//
// Created by Ruud Andriessen on 11/06/2017.
//

#include "ViscosityForce.h"
#include "../Kernels.h"
#include "../System.h"

ViscosityForce::ViscosityForce(vector<Particle *> particles) {
    this->setTarget(particles);
}

void ViscosityForce::setTarget(std::vector<Particle *> particles) {
    this->particles = particles;
}

void ViscosityForce::apply(System *s) {
    float u = 100.f;
    // Evaluate viscosity force for every particle
    for (Particle *pi : particles) {
        Vector3f viscosityForce = Vector3f(0, 0, 0);
        for (Particle *pj : s->grid.query(pi->position)) {
            viscosityForce = pj->mass * (pj->velocity - pi->velocity) / pj->density
                             * Viscosity::ddW(pi->position - pj->position);
        }
        pi->force += u * viscosityForce;
    }
}

void ViscosityForce::draw() {
    // TODO
}
