
#ifndef FLUIDS_SURFACEFORCE_H
#define FLUIDS_SURFACEFORCE_H


#include "Force.h"

class SurfaceForce : public Force {
public:
    SurfaceForce(std::vector<Particle*> particles);
    void setTarget(std::vector<Particle*> particles) override;
    void apply(System* s) override;
    void draw() override;
};


#endif //FLUIDS_PRESSUREFORCE_H
