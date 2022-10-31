#ifndef PARTICLE_PROTOTYPE_H
# define PARTICLE_PROTOTYPE_H 1

# include "udf.h"
# include "udfParticle.h"


void init_ParticlePrototype(ParticlePrototype *pp);
void free_ParticlePrototype(ParticlePrototype *pp);

double setParticlePrototypeSurfaceArea(ParticlePrototype *pp, int nPoints);
double getParticlePrototypeSurfaceArea(ParticlePrototype *pp);
double setParticlePrototypeSphericity(ParticlePrototype *pp);
double getParticlePrototypeSphericity(ParticlePrototype *pp);

void setParticlePrototypeCrossSectionAreas(ParticlePrototype *pp, int nDirs, int nLines);
double getParticlePrototypeCrossSection(ParticlePrototype *pp, tDimensionValue ldir);


# if PARALLEL
void host_to_node_particle_prototype(ParticlePrototype *pp);
# endif

#endif /* PARTICLE_PROTOTYPE_H */
