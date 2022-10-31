#include "udf.h"
#include "edem_coupling.h"
#include "quaternion_macros.h"

DEFINE_DPM_DRAG(drag_ganser, Re, p)
{
  /* Non-spherical drag model of Ganser */
  /* See books.google.co.uk/books?id=Z4a1BAAAQBAJ&q=ganser p297 */

  real Cd;
  cell_t c;
  Thread *ct;
  real relative_vel[ND_3], rotated_relative_vel[ND_3], vel_mag;
  quaternion q;
  double cross_section_area;
  double sphericity; /* Surface area of sphere with same volume / actual surface area */
  double dn, K1, K2;
  double ReK1K2;

  c = P_CELL(p);
  ct = P_CELL_THREAD(p);

  NV_D(relative_vel,=,C_U(c,ct),C_V(c,ct),C_W(c,ct));
  NV_V(relative_vel,-=,P_VEL(p));

  QN_Q(q,=,P_DEM_ORIENT(p));
  QN_CONJ(q,q); /* Rotation is inverse to particle orientation */

  Rotate_Vector_Q(relative_vel, q, rotated_relative_vel);

  vel_mag = NV_MAG(relative_vel);

  cross_section_area = getDPMCrossSection(p, rotated_relative_vel);
  sphericity = getDPMSphericity(p);

  dn = 2.0*sqrt(cross_section_area/M_PI);
  K1 = 1.0/(dn/(3.0*P_DIAM(p)) + 2.0/(3.0*sqrt(sphericity)));
  K2 = pow(10, 1.8148*pow(-log10(sphericity),0.5743));
  ReK1K2 = Re*K1*K2;

  Cd = K2*(24.0*(1.0+0.1118*pow(ReK1K2,0.6567))/ReK1K2 + 0.4305/(1.0+3305.0/ReK1K2));

  return 18.0*Cd*Re/24.0;
}

DEFINE_DPM_DRAG(drag_template, Re, p)
{
  real Cd;
  cell_t c;
  Thread *ct;
  real relative_vel[ND_3], rotated_relative_vel[ND_3], vel_mag;
  quaternion q;
  double cross_section_area, surface_area;
  double sphericity; /* Surface area of sphere with same volume / actual surface area */

  Cd = 1.0;
  c = P_CELL(p);
  ct = P_CELL_THREAD(p);

  NV_D(relative_vel,=,C_U(c,ct),C_V(c,ct),C_W(c,ct));
  NV_V(relative_vel,-=,P_VEL(p));

  QN_Q(q,=,P_DEM_ORIENT(p));
  QN_CONJ(q,q); /* Rotation is inverse to particle orientation */

  Rotate_Vector_Q(relative_vel, q, rotated_relative_vel);

  vel_mag = NV_MAG(relative_vel);

  cross_section_area = getDPMCrossSection(p, rotated_relative_vel);
  surface_area = getDPMSurfaceArea(p);
  sphericity = getDPMSphericity(p);


  return 18.0*Cd*Re/24.0;
}
