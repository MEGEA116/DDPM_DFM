#include "udf.h"
#include "edem_coupling.h"
#include "compute_particle_heat_flux.h"

#define P_PROJECTED_AREA(P)(0.25*M_PI*SQR(P_DIAM(P)))


void compute_heat_flux_to_particles(EDEM_Coupling edem_coupling, cxboolean fluid_source_terms)
{
#if !RP_HOST
  Particle *p;
  real relVel[ND_ND], relVelMag;
  Thread *ct, *vt;
  cell_t c;
  real Re, Pr, Nu;
  int i_pro;
#endif /* !RP_HOST */


  if(NULLP(edem_coupling.injections) || NULLP(edem_coupling.injection_names))
    {  
      Message0("\nWARNING: Injections for EDEM particles have not been set up.\n\n");
      return;
    }

#if !RP_HOST 

  for(i_pro=0;i_pro<edem_coupling.num_particle_prototypes;i_pro++)
    {
      loop (p, edem_coupling.injections[i_pro]->p)
        {
          c = P_CELL(p);
          vt = ct = P_CELL_THREAD(p);

          /* For Euler/Euler && DDPM we need the primary phase thread holding phase specific variables like velocity, properties, etc */
          if (mp_mfluid)
            vt = DPM_THREAD(ct, NULL);

          /* Convective Heat Flux */
          if(edem_coupling.convective_heat_option)
            {
              NV_D(relVel,=,C_U(c,vt),C_V(c,vt),C_W(c,vt));
              NV_V(relVel,-=,P_VEL(p));

              relVelMag = NV_MAG(relVel);

              Re = (C_R(c,vt) * relVelMag * P_DIAM(p)) / C_MU_L(c,vt); /* Reynolds number */
              Pr = (C_MU_L(c,vt) * C_CP(c,vt)) / C_K_L(c,vt); /* Prandtl number */

              if(edem_coupling.convective_heat_option == RANZ_MARSHALL)
                {		
                  /*Nu = 2.0 + 0.6*sqrt(Re)*cbrt(Pr);	*/
                  Nu = 2.0 + 0.6*sqrt(Re)*pow(Pr,0.33333);	
                }
              else
                {
                  /* Local porosity dependent Nu */
                  real porosity = 1.0;
                  real LiMasonExponent = 1.0; /* TODO Should be added to edem_coupling structure */

                  if (mp_mfluid)
                    porosity = C_VOF(c, vt);

                  /* Li & Mason's correlation for Ranz and Marshall */
                  if(Re <= 200.0)
                    {			
                      /*Nu = 2.0 + 0.6*pow(porosity,LiMasonExponent)*sqrt(Re)*cbrt(Pr);	*/
                      Nu = 2.0 + 0.6*pow(porosity,LiMasonExponent)*sqrt(Re)*pow(Pr,0.33333);	
                    }
                  else if(Re <= 1500.0)
                    {
                      /*Nu = 2.0 + 0.5*pow(porosity,LiMasonExponent)*sqrt(Re)*cbrt(Pr) +
                        0.02*pow(porosity,LiMasonExponent)*pow(Re,0.8)*cbrt(Pr);*/
                      Nu = 2.0 + 0.5*pow(porosity,LiMasonExponent)*sqrt(Re)*pow(Pr,0.33333) +
                        0.02*pow(porosity,LiMasonExponent)*pow(Re,0.8)*pow(Pr,0.3333);
                    }
                  else /* Re > 1500.0 */
                    {			
                      Nu = 2.0 + 4.5e-5*pow(porosity,LiMasonExponent)*pow(Re,1.8);	
                    }
                }

              /* P_DEM_HEAT_FLUX(p) is positive if heat goes in to particle */
              P_DEM_HEAT_FLUX(p) = M_PI * Nu * C_K_L(c,vt) * P_DIAM(p) * (C_T(c,vt) - P_T(p));
            }
          else
            P_DEM_HEAT_FLUX(p) = 0.0;


          /* Radiative Heat Flux */
          if(edem_coupling.radiative_heat_option)
            {
              P_DEM_HEAT_FLUX(p) -= edem_coupling.emissivity * STEFAN_BOLTZMANN_CONSTANT * M_PI * P_DIAM(p) * P_DIAM(p) * PW4(P_T(p));
            }

          /*  if (fluid_source_terms) */
          /*      not yet implemented */
        }
    }


#endif /* !RP_HOST */
}
