#ifndef EDEM_COUPLING_H
# define EDEM_COUPLING_H 1

#include "particle_prototype.h"

#define EDEM_COUPLING_NUM_PARTICLES_INIT -1

#ifndef ND_3
# define ND_3 3
#endif


typedef struct edem_coupling_struct
{
  int coupled;
  double time;
  int num_particles;

  int num_particle_prototypes;
  ParticlePrototype *particle_prototypes;

  Injection **injections;
  char **injection_names;
  int *injection_ids;

  int drag_law_option;
  cxboolean use_Fluent_drag;
  double scale_up_factor;

  int convective_heat_option;
  int radiative_heat_option;
  double emissivity;
  cxboolean use_Fluent_heat_transfer;

  cxboolean heat_registered;
  int temperature_property_index;
  int heat_flux_property_index;

} EDEM_Coupling;

extern EDEM_Coupling edem_coupling;

double getDPMCrossSection(Tracked_Particle *p, real *dir);
double getDPMSurfaceArea(Tracked_Particle *p);
double getDPMSphericity(Tracked_Particle *p);

enum
  {
    DEM_FORCE_X,
    DEM_FORCE_Y,
    DEM_FORCE_Z,
    DEM_TORQUE_X,
    DEM_TORQUE_Y,
    DEM_TORQUE_Z,
    DEM_SCALE,     /* Length scale relative to prototype */
    DEM_ORIENT_R,  /* Orientation quaternion */
    DEM_ORIENT_I,
    DEM_ORIENT_J,
    DEM_ORIENT_K,
    DEM_HEAT_FLUX,
    DEM_N_P_REALS_NO_HEAT_FLUX=DEM_HEAT_FLUX,
    DEM_N_P_REALS
  };

#define P_DEM_FORCE(P)((P)->user+DEM_FORCE_X)
#define P_DEM_FORCE_X(P)((P)->user[DEM_FORCE_X])
#define P_DEM_FORCE_Y(P)((P)->user[DEM_FORCE_Y])
#define P_DEM_FORCE_Z(P)((P)->user[DEM_FORCE_Z])

#define P_DEM_TORQUE(P)((P)->user+DEM_TORQUE_X)
#define P_DEM_TORQUE_X(P)((P)->user[DEM_TORQUE_X])
#define P_DEM_TORQUE_Y(P)((P)->user[DEM_TORQUE_Y])
#define P_DEM_TORQUE_Z(P)((P)->user[DEM_TORQUE_Z])

#define P_DEM_SCALE(P)((P)->user[DEM_SCALE])

#define P_DEM_ORIENT(P)((P)->user+DEM_ORIENT_R)
#define P_DEM_ORIENT_R(P)((P)->user[DEM_ORIENT_R])
#define P_DEM_ORIENT_I(P)((P)->user[DEM_ORIENT_I])
#define P_DEM_ORIENT_J(P)((P)->user[DEM_ORIENT_J])
#define P_DEM_ORIENT_K(P)((P)->user[DEM_ORIENT_K])

#define P_DEM_HEAT_FLUX(P)((P)->user[DEM_HEAT_FLUX])

#define P_DEM_PARTICLE_INDEX(P)((P)->stream_index)

#define GET_P_DEM_PARTICLE_INDEX(P)(((int)(P)->user[DEM_PART_INDEX_LO])+((int)(P)->user[DEM_PART_INDEX_HI])*MAX_INT_IN_REAL)
#define SET_P_DEM_PARTICLE_INDEX(P,I){(P)->user[DEM_PART_INDEX_LO] = (real)(I%MAX_INT_IN_REAL);(P)->user[DEM_PART_INDEX_HI] = (real)(I/MAX_INT_IN_REAL);}

#define DEM_DRAG_FLUENT_P(E)(E.use_Fluent_drag)

#define DEM_HEAT_COUPLED_P(E)(rf_energy && (E.use_Fluent_heat_transfer||E.convective_heat_option||E.radiative_heat_option))
#define DEM_HEAT_REGISTERED_P(E)(E.heat_registered)
#define DEM_HEAT_TRANSFER_FLUENT_P(E)(E.use_Fluent_heat_transfer)


#endif /* EDEM_COUPLING_H */
