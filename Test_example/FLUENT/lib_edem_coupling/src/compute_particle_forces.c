#include "udf.h"
#include "edem_coupling.h"
#include "compute_particle_heat_flux.h"
#include "version.h"

#define P_PROJECTED_AREA(P)(0.25*M_PI*SQR(P_DIAM(P)))

#if !RP_HOST
static void
compute_next_velocity(real dt, real a[3], real b, real V0[3], real Vn[3])
{
  real beta_fact = 1./(1. + b*dt);
  int i;

  for (i=0; i<3; ++i)
    Vn[i] = (V0[i] + dt *a[i]) * beta_fact;
}
#endif

void compute_particle_forces_using_Fluent(Tracked_Particle *tp, Injection *I, cxboolean fluid_source_terms)
{
#if !RP_HOST
  Particle *pp;

  loop (pp, I->p)
    {
      real N3V_VEC(acc);         /* explicit part of particle acceleration */
      real N3V_VEC(dvdt) = {0.}; /* velocity derivative, not used */
      real beta;                 /* drag factor */
      real vmf_inv;              /* virtual mass factor */
      real next_vel[6];          /* first 3 components contain new velocity, last 3 components carry old velocity */
      cxboolean update_randoms = FALSE; /* do not update particle random numbers for Brownian motion inside ParticleAcceleration */
      cxboolean cphase_interaction = TRUE; /* fill relevant fluid source terms inside HeatMassUpdate. */
      particle_state_t p_old;    /* needed to restore unintended changes in HeatMassUpdate */


      /* Fill tracked particle with information in pp and interpolate cphase_state */

#if (RampantReleaseMajor <= 17)
      init_tracked_particle(tp, pp, dpm_par.unsteady_tracking, FALSE, FALSE);
#endif

#if (RampantReleaseMajor == 18)
# if (RampantReleaseMinor >= 1)
      init_tracked_particle(tp, pp, dpm_par.unsteady_tracking, FALSE, TRUE);
# else
      init_tracked_particle(tp, pp, dpm_par.unsteady_tracking, FALSE, FALSE, TRUE);
# endif
#endif

#if (RampantReleaseMajor >= 19)
      init_tracked_particle(tp, pp, dpm_par.unsteady_tracking, FALSE, TRUE);
#endif


      P_DT(tp) = pp->next_time_step;
      P_TIME(tp) += P_DT(tp);
      
      if (DEM_DRAG_FLUENT_P(edem_coupling))
        {
          real p_dem_mass;

          /* Now let's do a virtual step into the particle's direction for the given flow time step */
          ParticleAcceleration(tp, acc, dvdt, &beta, &vmf_inv, update_randoms);
          compute_next_velocity(P_DT(tp), acc,  beta, P_VEL(tp), next_vel);

          /* acc is carrying explicit part of particle acceleration, 
           * let us add implicit part and subtract gravity included in body forces
           * to provide fluid related acceleration to EDEM as a force */
          N3V_VS(acc,-=,P_VEL(pp),*,beta);
          /*           N3V_V (acc, -=, tp->source.bf_acc); */
          N3V_V (acc,-=,solver_par.G);

          p_dem_mass = P_N(pp) * P_MASS(pp);

          /* Now let's compute the particle force from the acceleration */
          ND_VS(P_DEM_FORCE_X(pp),P_DEM_FORCE_Y(pp),P_DEM_FORCE_Z(pp),=,acc,*,p_dem_mass);

          /* Update velocity of tracked particle to be considered in AddSources */
          N3V_V(P_VEL(tp),=,next_vel);
        }

      /* need to call this always since some DPM source terms are treated inside */
      /* save state before calling HeatMassUpdate */
      memcpy((char *) &(p_old), (char *) &(tp->state), sizeof(particle_state_t));
      HeatMassUpdate(tp, cphase_interaction);

      /* Consider Fluent heat transfer here since we have tp already set */
      if (DEM_HEAT_COUPLED_P(edem_coupling) && DEM_HEAT_TRANSFER_FLUENT_P(edem_coupling))
        {
          /* HeatMassUpdate provides the new temperature and we can balance between old temperature P_T0 and new temperature P_T. */

          P_DEM_HEAT_FLUX(pp) = P_N(pp) * (P_MASS(tp) * tp->Cp * (P_T(tp) - P_T0(tp))/solver_par.flow_time_step);
          if (edem_coupling.radiative_heat_option && dpm_par.radiation_p)
            P_DEM_HEAT_FLUX(pp) -= P_N(pp) * (tp->source.emiss * STEFAN_BOLTZMANN_CONSTANT);
        }
      else
        {
          /* let us restore the old state of the particle */
          memcpy((char *) &(tp->state), (char *) &(p_old), sizeof(particle_state_t));
        }

      /* Now we can deposit all source terms in Fluent's standard DPM way */
      if (fluid_source_terms)
        AddSources(tp);

      /* Ignore torque for the time being */ 
      ND_S(P_DEM_TORQUE_X(pp),P_DEM_TORQUE_Y(pp),P_DEM_TORQUE_Z(pp),=,0.0);
    }
#endif /* !RP_HOST */
}

void compute_particle_forces_step_by_step(Tracked_Particle *tp, Injection *I, cxboolean fluid_source_terms)
{
#if !RP_HOST
  Particle *pp;
  real buoyancyForce[ND_ND];
  real dragForce[ND_ND];
  real relVel[ND_ND], relVelMag;
  Thread *ct, *vt;
  cell_t c;
  real factor;
  real Re, Cd;

  loop (pp, I->p)
    {
      c = P_CELL(pp);
      vt = ct = P_CELL_THREAD(pp);
      /* for euler/euler && DDPM we need the thread holding the velocity */
      if (mp_mfluid)
        vt = DPM_THREAD(ct, NULL);

#if (RampantReleaseMajor <= 17)
      init_tracked_particle(tp, pp, dpm_par.unsteady_tracking, FALSE, FALSE);
#endif

#if (RampantReleaseMajor == 18)
# if (RampantReleaseMinor >= 1)
      init_tracked_particle(tp, pp, dpm_par.unsteady_tracking, FALSE, TRUE);
# else
      init_tracked_particle(tp, pp, dpm_par.unsteady_tracking, FALSE, FALSE, TRUE);
# endif
#endif

#if (RampantReleaseMajor >= 19)
      init_tracked_particle(tp, pp, dpm_par.unsteady_tracking, FALSE, TRUE);
#endif


      /* Buoyancy force */
      if (M_gravity_p)
        {
          factor = - P_N(pp) * C_R(c,vt) * DPM_VOLUME(P_DIAM(pp)); /* Volume has been stored via diameter assuming spherical particles */
          NV_VS(buoyancyForce,=,M_gravity,*,factor);
        }
      else
        NV_S(buoyancyForce,=,0.0);

      ND_V(P_DEM_FORCE_X(pp),P_DEM_FORCE_Y(pp),P_DEM_FORCE_Z(pp),=,buoyancyForce);

      /* Drag Force */

      NV_D(relVel,=,C_U(c,vt),C_V(c,vt),C_W(c,vt));
      NV_V(relVel,-=,P_VEL(pp));

      relVelMag = NV_MAG(relVel);

      Re = (C_R(c,vt) * relVelMag * P_DIAM(pp)) / C_MU_L(c,vt);

      if(Re <= 0.55)
        Cd = 24.0 / Re;
      else if(Re <= 987.0)
        Cd = 24.0 * (1.0 + 0.15 * pow(Re,0.687)) / Re;
      else
        Cd = 0.44;

      factor = P_N(pp) * (0.5 * Cd * C_R(c,vt) * relVelMag * P_PROJECTED_AREA(pp));

      NV_VS(dragForce,=,relVel,*,factor);

      ND_V(P_DEM_FORCE_X(pp),P_DEM_FORCE_Y(pp),P_DEM_FORCE_Z(pp),+=,dragForce);

      /* if (fluid_source_terms) */
      /* not yet implemented */

      /* Ignoring torque for the time being */
      /* Scaling by P_N may need to be SQR(P_N) */
      ND_S(P_DEM_TORQUE_X(pp),P_DEM_TORQUE_Y(pp),P_DEM_TORQUE_Z(pp),=,0.0);
    }
#endif /* !RP_HOST */
}

void compute_forces_on_particles(EDEM_Coupling edem_coupling, cxboolean fluid_source_terms)
{
#if !RP_HOST
  int i_pro;
  Injection *Ip;
#endif

  if (edem_coupling.num_particle_prototypes <= 0)
    {  
      Message0("\nWARNING: Particle data has not been read from EDEM yet.\n\n");
      return;
    }

  if(NULLP(edem_coupling.injections) || NULLP(edem_coupling.injection_names))
    {  
      Message0("\nWARNING: Injections for EDEM particles have not been set up.\n\n");
      return;
    }

#if !RP_HOST

  for(i_pro=0;i_pro<edem_coupling.num_particle_prototypes;i_pro++)
    {
      Ip = edem_coupling.injections[i_pro];

      if(NNULLP(Ip->p))
        {
          Tracked_Particle tp_init = {0};
          Tracked_Particle *tp = &tp_init;

          alloc_tracked_particle_memory(tp);
          alloc_tp_pvars(tp, Ip);

          if (DEM_DRAG_FLUENT_P(edem_coupling) || DEM_HEAT_TRANSFER_FLUENT_P(edem_coupling))
            compute_particle_forces_using_Fluent(tp, Ip, fluid_source_terms);

          if (!DEM_DRAG_FLUENT_P(edem_coupling))
            compute_particle_forces_step_by_step(tp, Ip, fluid_source_terms);

          free_tp_pvars(tp);
          free_tracked_particle_memory(tp);
        }
    }

#endif /* !RP_HOST */
}
