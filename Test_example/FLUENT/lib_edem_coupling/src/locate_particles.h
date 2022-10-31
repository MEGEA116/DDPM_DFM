#ifndef LOCATE_PARTICLES_H
# define LOCATE_PARTICLES_H 1

int Locate_Unsteady_Particle_List(Particle **pl, cxboolean urge_particles_into_domain);
void Host_to_Node_Injection_Particles(Injection *I);
void Node_to_Host_Injection_Particles(Injection *I);

#endif /* LOCATE_PARTICLES_H */
