#ifndef COMPUTE_PARTICLE_HEAT_FLUX_H
# define COMPUTE_PARTICLE_HEAT_FLUX_H 1

#define STEFAN_BOLTZMANN_CONSTANT 5.670373e-8

enum
  {
    RANZ_MARSHALL,
    GUNN,
    LI_MASON
  };

void compute_heat_flux_to_particles(EDEM_Coupling edem_coupling, cxboolean fluid_source_terms);

#endif /* COMPUTE_PARTICLE_HEAT_FLUX_H */
