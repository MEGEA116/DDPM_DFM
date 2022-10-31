#include "udf.h"
#include "edem_coupling.h"
#include "AdaptorInterface.h"
#include "dpm_parallel.h"
#include "locate_particles.h"
#include "particle_prototype.h"
#include "compute_particle_forces.h"
#include "compute_particle_heat_flux.h"
#include "seem2c.h"
#include "dpm_dist.h"
#include "quaternion_macros.h"

#define LOCAL_IP_ADDRESS "127.0.0.1"

EDEM_Coupling edem_coupling;

#define NULL_PROPERTY_INDEX -1

#define DEFAULT_PARTICLE_TEMP 300.0


/* EDEM expects an array of heat flux pairs : (Particle_to_Particle, Fluid_To_Particle) */
enum
  {
    PARTICLE_TO_PARTICLE = 0,
    FLUID_TO_PARTICLE,
    N_HEAT_FLUX_ELEMENTS
  };


#if !RP_NODE
static int get_injection_id_from_list(Injection *I, Injection *ilist)
{
  int ii=0;
  Injection *II;

  loop(II, ilist)
    {
      if (II==I)
        return ii;
      ii++;
    }

  if (I == Get_pdft_injection())
    return -1;

  return -2;
}
#endif

void init_edem_coupling_injections()
{
  edem_coupling.num_particles = EDEM_COUPLING_NUM_PARTICLES_INIT;

  if (edem_coupling.num_particle_prototypes && NNULLP(edem_coupling.injection_names))
    {
      int ip;
      Injection *I;

      for (ip=0;ip<edem_coupling.num_particle_prototypes;++ip)
        {
          if(NNULLP(edem_coupling.injection_names[ip]))
            {
              I = Pick_Injection(edem_coupling.injection_names[ip]);

              if(NNULLP(I))
                {
                  delete_injection(I);
                  CX_Free(edem_coupling.injection_names[ip]);
                }
            }
        }
    }

  CX_Free(edem_coupling.injections);
  edem_coupling.injections = NULL;
  CX_Free(edem_coupling.injection_names);
  edem_coupling.injection_names = NULL;
  CX_Free(edem_coupling.injection_ids);
  edem_coupling.injection_ids = NULL;
}


char *schemestrncpy(char *dest, char *src, size_t n)
{
  /* Create a scheme symbol like name from a string */
  int i, j;
  int letter_found, space_found;
  char *scan;

  i = 0;
  j = 0;

  letter_found = 0;
  space_found = 0;
  scan = src;

  while (i < n)
    {
      if (*scan == '\0')
        {
          dest[j] = *scan;
          return dest;
        }

      if ((*scan != ' ')&&(*scan != '\n')&&(*scan != '\t')) /* Not whitespace */
        {
          if(letter_found && space_found) /* Only add one '-' for consecutive whitespace blocks and not at start or end */
            {
              dest[j] = '-';
              j++;
            }

          letter_found = 1;
          space_found = 0;
          dest[j] = tolower(*scan);
          j++;
        }
      else
        {
          space_found = 1;
        }

      scan++;
    }

  /* No ending /0 found so set it */

  dest[n-1] = '\0';

  return dest;
}

char *get_particle_prototype_name(int i_pro)
{
  if((i_pro < 0) || (i_pro >= edem_coupling.num_particle_prototypes))
    return (char *)NULL;

  return (edem_coupling.particle_prototypes + i_pro)->sPrototypeName;
}

int check_edem_coupling_injections()
{
  int injections_found;
#if PARALLEL
  int injections_found_nodes;
#endif

  injections_found = 0;

  if (NNULLP(edem_coupling.injections)&&NNULLP(edem_coupling.injection_names)&&NNULLP(edem_coupling.injection_ids))
    {
      int i_pro;

      for(i_pro=0;i_pro<edem_coupling.num_particle_prototypes;i_pro++)
        {
          /* We need to pick the injections all the time since it might have been deleted in the meantime by accident */
          edem_coupling.injections[i_pro] = Pick_Injection(edem_coupling.injection_names[i_pro]);
 
          if(NNULLP(edem_coupling.injections[i_pro]))
            {
              ++injections_found;
#if !RP_NODE
              edem_coupling.injection_ids[i_pro] = get_injection_id_from_list(edem_coupling.injections[i_pro], Get_dpm_injections());
#endif
              free_injection_particles(edem_coupling.injections[i_pro]);
            }
        }
    }

#if PARALLEL
# if RP_NODE
  injections_found_nodes = PRF_GILOW1(injections_found);
# endif

  node_to_host_int_1(injections_found_nodes);

#if RP_HOST
  if ((injections_found_nodes != injections_found)||(injections_found != edem_coupling.num_particle_prototypes))
    injections_found = 0;
#endif
  host_to_node_int_1(injections_found);

  if(injections_found)
    host_to_node_int(edem_coupling.injection_ids, edem_coupling.num_particle_prototypes);
#endif

  return injections_found;
}

int create_edem_coupling_injections()
{
  int i_pro;
  int num_particle_types;
  int injections_found;
#if !RP_NODE
  char *edem_injection_name;
#endif

  num_particle_types = edem_coupling.num_particle_prototypes;

  if (num_particle_types <= 0)
    return 0;

  init_edem_coupling_injections(); /* Initialise edem coupling injection data before rebuilding it */

  /* Create injections */

  edem_coupling.injections = (Injection **)CX_Malloc(num_particle_types * sizeof(Injection *));
  edem_coupling.injection_ids = (int *)CX_Malloc(num_particle_types * sizeof(int));
  edem_coupling.injection_names = (char **)CX_Malloc(num_particle_types * sizeof(char *));


#if !RP_NODE
  edem_injection_name = RP_Get_String("edem/injection-name");
#endif

  injections_found = 0;

  for(i_pro=0;i_pro<num_particle_types;i_pro++)
    {
      edem_coupling.injection_names[i_pro] = (char *)CX_Malloc(DPM_NAME_LENGTH * sizeof(char));

#if !RP_NODE
      if(num_particle_types > 1)
        {
          char *particletypename;
          char schemeparticletypename[MAX_DE_TYPENAME_SZ];

          particletypename = get_particle_prototype_name(i_pro);

          if(NNULLP(particletypename))
            {
              schemestrncpy(schemeparticletypename, particletypename, MAX_DE_TYPENAME_SZ);
              snprintf(edem_coupling.injection_names[i_pro], DPM_NAME_LENGTH, "%s-%s", edem_injection_name, schemeparticletypename);
            }
          else
            snprintf(edem_coupling.injection_names[i_pro], DPM_NAME_LENGTH, "%s-%d", edem_injection_name, i_pro);
        }
      else  /* Base name used without ending "-0" to match old naming */
        snprintf(edem_coupling.injection_names[i_pro], DPM_NAME_LENGTH, "%s", edem_injection_name);
#endif

      host_to_node_string(edem_coupling.injection_names[i_pro], DPM_NAME_LENGTH);
    }

  return check_edem_coupling_injections();
}

void free_edem_coupling_particle_prototypes()
{
  if(NNULLP(edem_coupling.particle_prototypes))
    {
      int i_pro;
      ParticlePrototype *pp;

      for(i_pro=0;i_pro<edem_coupling.num_particle_prototypes;i_pro++)
        {   
          pp = edem_coupling.particle_prototypes + i_pro;

          free_ParticlePrototype(pp);
        }

      CX_Free(edem_coupling.particle_prototypes);

      edem_coupling.num_particle_prototypes = 0;
      edem_coupling.particle_prototypes = NULL;
    }
}

int create_edem_coupling_particle_prototypes(int num_particle_types)
{
  int i_pro;
  ParticlePrototype *pp;

#if !RP_NODE
  int n_cross_sections;
  int n_cross_section_samples;
  int n_surface_area_samples;
#endif

  /* Create Particle Prototypes from scratch */

  free_edem_coupling_particle_prototypes();

  edem_coupling.particle_prototypes = (ParticlePrototype*)CX_Malloc(num_particle_types*sizeof(ParticlePrototype));
  edem_coupling.num_particle_prototypes = num_particle_types;

#if !RP_NODE
  n_cross_sections = RP_Get_Integer("edem/n-cross-sections");
  n_cross_section_samples = RP_Get_Integer("edem/n-cross-section-samples");
  n_surface_area_samples = RP_Get_Integer("edem/n-surface-area-samples");
#endif

  for(i_pro=0;i_pro<num_particle_types;i_pro++)
    {   
      pp = edem_coupling.particle_prototypes + i_pro; 
      init_ParticlePrototype(pp); /* Initialise values to 0 & pointers to NULL to avoid seg fault in ADAPTOR_getParticlePrototype */

#if !RP_NODE
      ADAPTOR_getParticlePrototype(i_pro, pp);

      Message("%d particles of type %s (%d)\n", ADAPTOR_getNumParticles(i_pro), pp->sPrototypeName, i_pro);

      setParticlePrototypeSurfaceArea(pp, n_surface_area_samples);
      setParticlePrototypeSphericity(pp);

      setParticlePrototypeCrossSectionAreas(pp, n_cross_sections, n_cross_section_samples);
#endif

#if PARALLEL
      host_to_node_particle_prototype(pp);
#endif
    }

  return num_particle_types;
}


#if !RP_NODE
cxboolean register_heat_properties()
{
  /* Properties to register for the heat transfer: temperature and heat flux */

  int tempNumElements = 1;	 /* Define number of elements to the custom property */
  int tempDataType = 0;		 /* Define the data type as a double using EDEMs index system */
  int tempUnitType = 20;     /* Define the unit type as temperature using EDEMs index system */
  double initialTemp = DEFAULT_PARTICLE_TEMP;	 /* Define the starting temperature */

  /*
   * Heat flux is a special custom property. It has 2 elements:
   * The first element is used by EDEM particle contacts for heat flux.
   * The second element is used by the CFD package for fluid to particle
   * heat flux. Even though the first entry will not be used here we are
   * declaring the property here and therefore must declare both entries for
   * EDEM to work correctly
   */

  int heatFluxNumElements = 2;  /* Define number of elements to the custom property */
  int heatFluxDataType = 0;	    /* Define the data type as a double using EDEMs index system */
  int heatFluxUnitType = 21;    /* Define the unit type as heat flux using EDEMs index system */
  double initialHeatFlux = 0.0; /* Define the starting heat flux */


  cxboolean heat_registered;
  int temperature_property_index;
  int heat_flux_property_index;

  /* Register temperature AND heat flux */

  heat_registered = ADAPTOR_registerCustomProperty("Temperature", tempNumElements, tempDataType, tempUnitType, initialTemp, &temperature_property_index);

  if(heat_registered)
    heat_registered = ADAPTOR_registerCustomProperty("Heat Flux", heatFluxNumElements, heatFluxDataType, heatFluxUnitType, initialHeatFlux, &heat_flux_property_index);

  if(heat_registered)
    {
      edem_coupling.heat_registered = heat_registered;
      edem_coupling.use_Fluent_heat_transfer = TRUE;
      edem_coupling.temperature_property_index = temperature_property_index;
      edem_coupling.heat_flux_property_index = heat_flux_property_index;
    }

  return heat_registered;
}
#endif /* !RP_NODE */


int add_EDEM_particle_to_Injection(Injection *I, int inj_id, DiscreteElement *particle, int particle_index, double temperature, double scale_up_factor)
{
  Particle *p_new;

  p_new = new_particle(I, FALSE);

  p_new->part_id = get_next_part_id();

  P_INJECTION(p_new) = I;
  p_new->I_id = inj_id;

  /* Set all properties on particle that are available. 
     the particle ID cannot be easily set but the stream index can be set to i_part */

  P_DEM_PARTICLE_INDEX(p_new) = particle_index;
 
  NV_V(P_POS(p_new),=,particle->vPos);           /* Position */
  NV_V(P_VEL(p_new),=,particle->vVelocity);      /* Velocity */
/*  NV_V(P_OMEGA(p_new),=,particle->vAngVelocity);  Angular Velocity */

  /*P_DIAM(p_new) = cbrt(6.0*particle->nVolume/M_PI)/scale_up_factor;*/ /* Spherical particles assumed. Scaled down from DEM particle scaling*/

  P_DIAM(p_new) = pow(6.0*particle->nVolume/M_PI,0.3333)/scale_up_factor; /* Spherical particles assumed. Scaled down from DEM particle scaling*/

  P_T(p_new) = (real)temperature;

  p_new->n_steps = 0;

  update_particle_density(p_new); /* Sets P_MASS & P_RHO from the injection's material density and P_DIAM */

  /* Fill remaining information */

  P_N(p_new) = (real)CUB(scale_up_factor); /* This is scaled so total mass of scaled-up particles in EDEM is added to Fluent */
  p_new->next_time_step = solver_par.flow_time_step;


  P_TIME(p_new) = solver_par.flow_time;
  memcpy((char *) &(p_new->init_state),
	 (char *) &(p_new->state), sizeof(particle_state_t));
  
  P_FLOW_RATE(p_new) = P_N(p_new) * P_MASS(p_new) / solver_par.flow_time_step;
  p_new->time_of_birth = 0.0;
  
  P_ON_WALL(p_new) = FALSE;
  P_FILM_FACE(p_new) = NULL_FACE;
  P_FILM_THREAD_ID(p_new) = NULL_INDEX;

  Init_DPM_Scalars(p_new);

  if (dpm_par.n_user_reals > 0)
    Init_Unsteady_User(p_new);

  /* User real based values */

  P_DEM_SCALE(p_new) = particle->nScale/scale_up_factor;
  QN_Q(P_DEM_ORIENT(p_new),=,particle->vOrientation);

  if (dpm_par.n_cbk > 0)
    Init_DPM_cbk(p_new);

  /* Need to have this allocated for cas/dat file writing of particle information */
  if (NULLP(p_new->unsteady_coupled)) 
    Init_Unsteady_Coupled(p_new);

  alloc_pvars(p_new);

  append_particle_to_list(p_new, &(I->p), I);

  I->n_particles++;

  return p_new->part_id ;
}


int get_edem_coupling_particles(int num_particles)
{
  int num_located_particles;
  int i_pro;


#if !RP_NODE
  int i_part;

  /* Get the particle data for all particles. */

  Message("\nGetting particle data from EDEM...\n");

  ADAPTOR_getParticleData(); /* All particle data transferred into a hidden CFluentParticleData object */

  if(DEM_HEAT_COUPLED_P(edem_coupling) && !DEM_HEAT_REGISTERED_P(edem_coupling))
    {
      if (register_heat_properties())
        Message("  Heat flux properties registered with EDEM.\n");
      else
        Message("\n  WARNING: Unable to register heat flux properties.\n");
    }

  if(DEM_HEAT_COUPLED_P(edem_coupling) && DEM_HEAT_REGISTERED_P(edem_coupling))
    ADAPTOR_updateValuesForProperty(num_particles, edem_coupling.temperature_property_index);

  for(i_part=0;i_part<num_particles;i_part++)
    {
      DiscreteElement edem_particle;
      double particle_temperature;

      ADAPTOR_getParticle(i_part, &edem_particle);

      if(DEM_HEAT_COUPLED_P(edem_coupling) && DEM_HEAT_REGISTERED_P(edem_coupling))
        particle_temperature = ADAPTOR_getScalarProperty(edem_coupling.temperature_property_index, i_part);
      else
        particle_temperature = DEFAULT_PARTICLE_TEMP;

      add_EDEM_particle_to_Injection(edem_coupling.injections[edem_particle.typeIndex],
                                     edem_coupling.injection_ids[edem_particle.typeIndex],
                                     &edem_particle, i_part, particle_temperature,  edem_coupling.scale_up_factor);
    }

#endif /* !RP_NODE */

  num_located_particles = 0;

  for(i_pro=0;i_pro<edem_coupling.num_particle_prototypes;i_pro++)
    {
#if !RP_NODE
      reset_injection_nparticles_and_tail(edem_coupling.injections[i_pro]);
#endif /* !RP_NODE */

      Host_to_Node_Injection_Particles(edem_coupling.injections[i_pro]);

#if !RP_HOST
      num_located_particles += edem_coupling.injections[i_pro]->n_particles;
      Message("  Injection '%s' has %d particles",edem_coupling.injection_names[i_pro], edem_coupling.injections[i_pro]->n_particles);
# if RP_NODE
      Message(" on node %d\n", myid);
# else
      Message("\n");
# endif
#endif
    }

  num_located_particles = PRF_GISUM1(num_located_particles);

  return num_particles - num_located_particles; /* Number of particles that couldn't be located */
}

void init_edem_coupling()
{
  edem_coupling.coupled = 0;
  edem_coupling.time = 0.0;

  init_edem_coupling_injections();

  edem_coupling.drag_law_option = 0;
  edem_coupling.use_Fluent_drag = TRUE;
  edem_coupling.scale_up_factor = 1.0;

  edem_coupling.convective_heat_option = 0;
  edem_coupling.radiative_heat_option = 0;
  edem_coupling.emissivity = 1.0;
  edem_coupling.use_Fluent_heat_transfer = TRUE;

  edem_coupling.heat_registered = FALSE;
  edem_coupling.temperature_property_index = NULL_PROPERTY_INDEX;
  edem_coupling.heat_flux_property_index = NULL_PROPERTY_INDEX;
}

void update_edem_coupling_settings(cxboolean update_on_node)
{
#if !RP_NODE
  edem_coupling.convective_heat_option = RP_Get_Integer("edem/convective-heat-option");
  edem_coupling.radiative_heat_option = RP_Get_Integer("edem/radiative-heat-option");
  edem_coupling.scale_up_factor = RP_Get_Double("edem/scale-up-factor");
  edem_coupling.use_Fluent_drag = RP_Get_Boolean("edem/use-fluent-drag?");
  edem_coupling.use_Fluent_heat_transfer = RP_Get_Boolean("edem/use-fluent-heat-transfer?");
#endif

#if PARALLEL
  if(update_on_node)
    {
      host_to_node_int_2(edem_coupling.convective_heat_option, edem_coupling.radiative_heat_option);
      host_to_node_boolean_1(edem_coupling.use_Fluent_drag);
      host_to_node_boolean_1(edem_coupling.use_Fluent_heat_transfer);
      host_to_node_double_1(edem_coupling.scale_up_factor);
    }
#endif
}

int get_n_req_dpm_user_reals()
{
  update_edem_coupling_settings(FALSE); /* Don't update on Nodes as called from Scheme */

  if(DEM_HEAT_COUPLED_P(edem_coupling))
    return DEM_N_P_REALS;
  else
    return DEM_N_P_REALS_NO_HEAT_FLUX;
}

#if !RP_NODE /* Scheme calls */

static Pointer ledemisconnected()
{
  RETURNP(edem_coupling.coupled);
}

static Pointer ledemnreqdpmuserreals()
{
  RETURN_FIXNUM(get_n_req_dpm_user_reals());
}

static Pointer ledemupdatesettings()
{
  update_edem_coupling_settings(FALSE); /* Don't update on Nodes as called from Scheme */
  RETURNP(TRUE);
}

static Pointer ledemnparticletypes()
{
  if (edem_coupling.coupled)
    RETURN_FIXNUM(ADAPTOR_getNumParticleTypes());
  else
    RETURN_FIXNUM(0);
}

static Pointer ledemnparticletypeslookahead()
{
  if (edem_coupling.coupled)
    {
      int success;
      double orig_time;

      ADAPTOR_getEDEMTime(&orig_time, &success);

      if (success)
        /* Perform 1 step to get particle and other data initialised */
        ADAPTOR_performNumAnalysisSteps(1, &success);

      if (success)
        {
          int num_particle_types;

          num_particle_types = ADAPTOR_getNumParticleTypes();

          ADAPTOR_setEDEMTime(orig_time, &success);/* Reset Time to original value */

          RETURN_FIXNUM(num_particle_types);
        }

      RETURN_FIXNUM(0);
    }
  else
    RETURN_FIXNUM(0);
}


static Pointer ledemperformnumanalysissteps(Pointer index)
{
  if (edem_coupling.coupled)
    {
      int n_steps;
      int success;

      n_steps = INT_ARG(index, "%edem-perform-num-analysis-steps: wta[1](integer)");

      ADAPTOR_performNumAnalysisSteps(n_steps, &success);

      RETURN_FIXNUM(success);
    }
  else
    RETURN_FIXNUM(0);
}

static Pointer ledemparticletypename(Pointer index)
{
  int i_pro, n_pro;
  ParticlePrototype pp;

  i_pro = INT_ARG(index, "edem-particle-type-name: wta[1](integer)");
  n_pro = ADAPTOR_getParticlePrototypeData();

  if ((i_pro < 0)||(i_pro >= n_pro))
    RETURNP(FALSE);

  init_ParticlePrototype(&pp); /* Initialise values to 0 & pointers to NULL to avoid seg fault in ADAPTOR_getParticlePrototype */

  ADAPTOR_getParticlePrototype(i_pro, &pp);

  RETURN_STRING(pp.sPrototypeName);
}
#endif /* !RP_NODE Scheme calls */


DEFINE_EXECUTE_ON_LOADING(bind_edem_subrs, libname)
{
#if !RP_NODE /* Scheme calls */

  init_subr("%edem-is-connected?",tc_subr_0,(Subr)ledemisconnected);
  init_subr("%edem-update-settings",tc_subr_0,(Subr)ledemupdatesettings);
  init_subr("%edem-n-req-dpm-user-reals",tc_subr_0,(Subr)ledemnreqdpmuserreals);
  init_subr("%edem-n-particle-types",tc_subr_0,(Subr)ledemnparticletypes);
  init_subr("%edem-n-particle-types-look-ahead",tc_subr_0,(Subr)ledemnparticletypeslookahead);
  init_subr("%edem-perform-num-analysis-steps",tc_subr_1,(Subr)ledemperformnumanalysissteps);
  init_subr("%edem-particle-type-name",tc_subr_1,(Subr)ledemparticletypename);

#endif /* !RP_NODE Scheme calls */
}

DEFINE_ON_DEMAND(connect_edem_coupling)
{
  int coupling_success;
  int success;

  double time;

#if !RP_NODE
  char *edem_host_ip_address;
#endif /* !RP_NODE */

  init_edem_coupling();

  if(!rp_unsteady)
    {
      Message0("\n\nWARNING: Cannot connect to EDEM from the steady Fluent solver.\n");

      edem_coupling.coupled = 0;
      return;
    }

  Message0("\n\nInitialising EDEM-Fluent Coupling. Please wait...\n");

#if !RP_NODE

  if (RP_Get_Boolean("edem/remote-host?"))
    {
      edem_host_ip_address = RP_Get_String("edem/host-ip-address");

      if(strlen(edem_host_ip_address) == 0)
        edem_host_ip_address = LOCAL_IP_ADDRESS;
    }
  else
    edem_host_ip_address = LOCAL_IP_ADDRESS;

      
  ADAPTOR_init_connectEDEMCoupling_Address(&coupling_success, edem_host_ip_address);

  if(coupling_success)
    {
      Message("  Connected to EDEM Process ");
      if(strncmp(edem_host_ip_address, LOCAL_IP_ADDRESS, 5)) /* Does not start with "127.0" */
        Message("on host %s ",edem_host_ip_address);

      Message(".\n");
    }
  else
    {
      Message("\n  ERROR: Cannot connect to EDEM.\n");
      Message("           Check that EDEM Coupling Server has been started\n\n");

      ADAPTOR_disconnectEDEMCoupling(&success);
    }


#endif /* !RP_NODE */

  host_to_node_int_1(coupling_success);

  edem_coupling.coupled = coupling_success;

  if(!coupling_success) return;



#if !RP_NODE

  ADAPTOR_getEDEMTime(&time, &success);

#endif /* !RP_NODE */

  host_to_node_int_1(success);
  host_to_node_double_1(time);

  if(success)
    {
      Message0("  Current EDEM time is %e\n",time);

      edem_coupling.time = time;

      RP_Set_Float("edem/time", time);
    }
  else
    {
      Message0("\n\n  ERROR: Cannot get EDEM time.\n\n");

      return;
    }
}


DEFINE_ON_DEMAND(get_solution_from_edem)
{
  int num_particles;
  int num_particle_types;
  int num_lost_particles;


  Message0("\nGetting EDEM solution data...\n");

  if(!edem_coupling.coupled)
    {
      Message0("\n\n  WARNING: EDEM not connected to Fluent. Getting data failed\n\n");
      return;
    }


  DPM_Init_Oct_Tree_Search();

#if !RP_NODE

  num_particle_types = ADAPTOR_getParticlePrototypeData();

  num_particles = ADAPTOR_getTotalNumParticles();

#endif

#if PARALLEL
  host_to_node_int_2(num_particle_types, num_particles);
#endif

  if(num_particle_types <= 0)
    {
      Message0("\nNo particle types found \n");
      return;
    }


  if(num_particles > 0)
    {
      Message0("%d particle types.\n", num_particle_types);

      /* Edem has particles so check injections as they may have been deleted between calls. */
      if(edem_coupling.num_particle_prototypes != num_particle_types)
          create_edem_coupling_particle_prototypes(num_particle_types);

      if(check_edem_coupling_injections() != num_particle_types)
        {
          create_edem_coupling_injections();

          if(check_edem_coupling_injections() != num_particle_types) /* Creation of injections failed */
            {
              Message0("\n  WARNING: %d Injections for EDEM particles not set up.\n\n", num_particle_types);

              init_edem_coupling_injections();
              return;
            }
        }

      Message0("  Getting %d particles from EDEM solution.\n", num_particles);
      edem_coupling.num_particles = num_particles;
    }
  else
    {
      Message0("  No particles found in EDEM solution.\n");
      edem_coupling.num_particles = 0;

      return;
    }


  /* Get other EDEM Coupling settings */
  update_edem_coupling_settings(TRUE);

  num_lost_particles = get_edem_coupling_particles(num_particles);

  if(num_lost_particles)
    Message0("  %d Particles could not be located in Fluent.\n", num_lost_particles);

  Message0("  Done.\n");
}


void send_forces_torques_heat_fluxes_to_edem()
{
  int n_req_dpm_user_reals;
  int i_pro;

#if PARALLEL
  cxboolean node_injection_valid;
  int valid_injection_count;
  int node_injection_n_particles;
#endif

#if !RP_NODE
  double *forces, *torques, *heat_fluxes;
  double max_heat_flux, min_heat_flux, tot_heat_flux;
  double *offset_force, *offset_torque, *offset_heat_flux;
  Particle *p;
  int num_particles_check;
#endif /* !RP_NODE */

  Message0("\n  Sending forces to EDEM...\n");

  if(!edem_coupling.coupled)
    {
      Message0("\n\n  WARNING: EDEM not connected to Fluent. Getting data failed\n\n");
      return;
    }


  if(edem_coupling.num_particles <= 0)
    {
      Message0("    No EDEM particles found in Fluent.\n");
      return;
    }
 

  if(NULLP(edem_coupling.injections) || NULLP(edem_coupling.injection_names))
    {
      Message0("\n  WARNING: Injections for EDEM particles have not been set up.\n\n");
      return;
    }


  n_req_dpm_user_reals = get_n_req_dpm_user_reals();

  if(dpm_par.n_user_reals < n_req_dpm_user_reals)
    {
      Message0("\n  WARNING: Need to setup at least %d DPM User Scalars.\n\n", n_req_dpm_user_reals);

      return;
    }


#if PARALLEL
  valid_injection_count = 0;

  for(i_pro=0;i_pro<edem_coupling.num_particle_prototypes;++i_pro)
    {
#if RP_NODE
      node_injection_valid = PRF_GLOR1(NNULLP(edem_coupling.injections[i_pro]->p));
      node_injection_n_particles = PRF_GISUM1(edem_coupling.injections[i_pro]->n_particles);
#endif

      node_to_host_boolean_1(node_injection_valid);
      node_to_host_int_1(node_injection_n_particles);


      if (node_injection_n_particles > 0)
        {
          if (node_injection_valid)
            {
              /* Get the Fluent particles from the compute nodes to the host. */
              Node_to_Host_Injection_Particles(edem_coupling.injections[i_pro]); 
              valid_injection_count++;
            }
          else
            {
              Message0("\n  WARNING: EDEM particles on Injection '%s' are not allocated on the nodes.\n\n", edem_coupling.injection_names[i_pro]);
            }
        }
    }

  if (valid_injection_count == 0)
    {
      Message0("\n  WARNING: No EDEM particles on any Injections found on the nodes.\n\n");
      return;
    }
#endif


#if !RP_NODE
  forces = CX_Malloc(sizeof(double) * edem_coupling.num_particles * ND_ND);
  torques = CX_Malloc(sizeof(double) * edem_coupling.num_particles * ND_ND);

  /* Initialise forces torques as not all particles will be in the injection as some may be outside Fluent mesh */
  memset(forces, 0, sizeof(double) * edem_coupling.num_particles * ND_ND);
  memset(torques, 0, sizeof(double) * edem_coupling.num_particles * ND_ND);

  if(DEM_HEAT_COUPLED_P(edem_coupling))
    {
      heat_fluxes = CX_Malloc(sizeof(double) * edem_coupling.num_particles * N_HEAT_FLUX_ELEMENTS);

      /* Initialise heat fluxes as not all particles will be in the injection as some may be outside Fluent mesh */
      memset(heat_fluxes, 0, sizeof(double) * edem_coupling.num_particles * N_HEAT_FLUX_ELEMENTS);

    }
  else
    heat_fluxes = NULL;


  /* Set all forces and torques on particles in injection */

  max_heat_flux = -HUGE_VAL;
  min_heat_flux = HUGE_VAL;
  tot_heat_flux = 0.0;

  num_particles_check = 0;

  for(i_pro=0;i_pro<edem_coupling.num_particle_prototypes;++i_pro)
    {
      if ((edem_coupling.injections[i_pro]->n_particles > 0) && NNULLP(edem_coupling.injections[i_pro]->p))
        {
          loop (p, edem_coupling.injections[i_pro]->p)
            {
              ++num_particles_check;

              offset_force = forces + (ND_ND * P_DEM_PARTICLE_INDEX(p));
              NV_D(offset_force,=,P_DEM_FORCE_X(p),P_DEM_FORCE_Y(p),P_DEM_FORCE_Z(p));

              offset_torque = torques + (ND_ND * P_DEM_PARTICLE_INDEX(p));
              NV_D(offset_torque,=,P_DEM_TORQUE_X(p),P_DEM_TORQUE_Y(p),P_DEM_TORQUE_Z(p));

              if(DEM_HEAT_COUPLED_P(edem_coupling))
                {
                  offset_heat_flux = heat_fluxes + (N_HEAT_FLUX_ELEMENTS * P_DEM_PARTICLE_INDEX(p));
                  offset_heat_flux[FLUID_TO_PARTICLE] = P_DEM_HEAT_FLUX(p); /* Only fluid to particle term set */
                  max_heat_flux = MAX(max_heat_flux, P_DEM_HEAT_FLUX(p));
                  min_heat_flux = MIN(min_heat_flux, P_DEM_HEAT_FLUX(p));
                  tot_heat_flux += P_DEM_HEAT_FLUX(p);
                }
            }
        }
    }
 
  Message("  Setting forces and torques on %d (of %d) EDEM particles... \n", num_particles_check, edem_coupling.num_particles);

  ADAPTOR_setDragForceAndTorque(edem_coupling.num_particles, forces, torques);

  Message("  Done.\n");

  CX_Free(forces);
  CX_Free(torques);


  if(DEM_HEAT_COUPLED_P(edem_coupling) && DEM_HEAT_REGISTERED_P(edem_coupling))
    {
      Message("  Setting heat fluxes on %d (of %d) EDEM particles... ", num_particles_check, edem_coupling.num_particles);

      Message("\n    Total Particle Heat flux : %lfW\n", tot_heat_flux);
      Message("    Particle Heat fluxes in range [%lfW  %lfW]\n", min_heat_flux, max_heat_flux);

      ADAPTOR_setValuesForProperty(edem_coupling.num_particles, edem_coupling.heat_flux_property_index, heat_fluxes);

      Message("  Done.\n");

      CX_Free(heat_fluxes);
    }
#endif /* !RP_NODE */

  Message0("  Done.\n");
}



DEFINE_ON_DEMAND(update_edem_solution)
{
  int update_success;
  double time;


  Message0("\nUpdating EDEM solution ... ");

  if(!edem_coupling.coupled)
    {
      Message0("\n\n  WARNING: EDEM not connected to Fluent. Update failed\n\n");
      return;
    }

  send_forces_torques_heat_fluxes_to_edem();

#if PARALLEL
  Node_Idle_Wait_Host();
#endif

#if !RP_NODE

  time = CURRENT_TIME; /* Could be staggered to CURRENT_TIME + CURRENT_TIMESTEP/2 */

  if (edem_coupling.time < time)
    {
      ADAPTOR_performAnalysisToTime(time, &update_success);

      /* Set time to actual EDEM time not requested value as they may be slightly different */
      if(update_success == 1)
        ADAPTOR_getEDEMTime(&time, &update_success);
    }
  else
    {
      update_success = 2; /* Fluent in sync or behind EDEM so do nothing */
    }
#endif

#if PARALLEL
  Node_Idle_Wait_Host();
#endif

  host_to_node_int_1(update_success);
  host_to_node_double_1(time);

  if(update_success)
    {
      if(update_success == 1)
        {
          Message0("  EDEM solution updated to time %e.\n", time);
          edem_coupling.time = time;
          RP_Set_Float("edem/time", time);
        }

      if(update_success == 2)
        Message0("  EDEM solution already at or ahead of time %e.\n", time);
    }
  else
    {
      Message0("  EDEM solution NOT updated to time %e.\n", time);
    }

  Message0("  Done.\n");
}


DEFINE_ON_DEMAND(synchronize_fluent_to_edem_time)
{
  double time;
  int success;

  if(!edem_coupling.coupled)
    {
      Message0("\n\nWARNING: EDEM not connected to Fluent. Cannot Synchronise\n\n");
      return;
    }

#if !RP_NODE

  ADAPTOR_getEDEMTime(&time, &success);

#endif /* !RP_NODE */

  host_to_node_int_1(success);

  if(success)
    {
      host_to_node_double_1(time);

      RP_Set_Float("flow-time", time); /*  RP_Set_Float takes a double works on RP_NODE now */  
      RP_Set_Float("edem/time", time);

      Message0("\nFluent Synchronised to current EDEM time : %e\n",time);

      edem_coupling.time = time;
    }
  else
    {
      Message0("\n\nERROR: Cannot get EDEM time.\n\n");

      return;
    }
}


DEFINE_ON_DEMAND(disconnect_edem_coupling)
{
#if !RP_NODE
  int success;
#endif /* !RP_NODE */

  if(!edem_coupling.coupled)
    {
      Message0("\nFluent does not seem to be connected to an EDEM process.\n");

      return;
    }

  if(!rp_unsteady)
    {
      Message0("\n\nWARNING: Cannot be coupled to EDEM from steady Fluent solver.\n");

      edem_coupling.coupled = 0;

      return;
    }

#if !RP_NODE

  Message("\n\nDisconnecting EDEM-Fluent Coupling. Please wait...\n");

  ADAPTOR_stopSimulation(&success);

  ADAPTOR_disconnectEDEMCoupling(&success);

  if(success)
    {
      Message(" EDEM Process Disconnected Successfully.\n");
    }
  else
    {
      Message("\nERROR: Cannot Disconnect from EDEM.\n");
    }

#endif /* !RP_NODE */

  edem_coupling.coupled = 0; /* Set as not coupled regardless of success */
}

DEFINE_ADJUST(adjust_edem_solution, d)
{
  /* User might want to add extra setup operations here */
}


DEFINE_EXECUTE_AT_END(update_edem_at_end)
{
  /* User might want to add extra cleanup operations here */
}

DEFINE_EXECUTE_AT_EXIT(disconnect_edem_coupling_at_exit)
{
  disconnect_edem_coupling();
}


void activate_edem_injections(Domain *domain, cxboolean track_unsteady)
{
  int i_pro;
  Injection *I;

  for(i_pro=0;i_pro<edem_coupling.num_particle_prototypes;i_pro++)
    {
      I = Pick_Injection(edem_coupling.injection_names[i_pro]);

      if (NNULLP(I))
        I->active = TRUE;
    }
}


void deactivate_edem_injections(Domain *domain, cxboolean track_unsteady)
{
  int i_pro;
  Injection *I;

  for(i_pro=0;i_pro<edem_coupling.num_particle_prototypes;i_pro++)
    {
      I = Pick_Injection(edem_coupling.injection_names[i_pro]);

      if (NNULLP(I))
        I->active = FALSE;
    }
}

void track_edem_injections(Domain *domain, cxboolean track_unsteady)
{      
  cxboolean need_to_connect;

#if !RP_NODE
  need_to_connect = (!edem_coupling.coupled) && RP_Get_Boolean("edem/connected?");
#endif 

  host_to_node_boolean_1(need_to_connect);

  if(need_to_connect)
    connect_edem_coupling();
 

  if(!edem_coupling.coupled)
    {
      Message0("\n  Not connected to EDEM for particle tracking.\n");
      Message0("  Connect in EDEM Coupling panel.\n\n");
      return;
    }

  Message0("Tracking edem injections... ");

  update_edem_coupling_settings(TRUE);

  /* Get initial set of particles from EDEM only once after initialization */

  if (edem_coupling.num_particles == EDEM_COUPLING_NUM_PARTICLES_INIT)
    get_solution_from_edem();

  if (edem_coupling.num_particles > 0)
    {
      /* Compute forces and heat flux, but don't set source terms */
      Message0("\n Forces computed for EDEM particles WITHOUT sources\n");
      compute_forces_on_particles(edem_coupling, FALSE);

      if(DEM_HEAT_COUPLED_P(edem_coupling) && !DEM_HEAT_TRANSFER_FLUENT_P(edem_coupling))
        compute_heat_flux_to_particles(edem_coupling, FALSE);
    }

  /* Send particles to EDEM and track them there even if
     there are no particles so EDEM and Fluent kept in synch. */

  update_edem_solution();

  /* Get particles from EDEM and compute source terms for fluid */
  get_solution_from_edem();

  if (edem_coupling.num_particles > 0)
    {
      /* Need to reset distributions. This is deleting information from other injections */

      Update_Dist_Storage();
      Reset_Sampled_Distributions(c_par.cphase_interaction, FALSE, FALSE, DO_CELLS, NULL_CMD);

      Init_Node_Averages(domain, FALSE);  /* if node based averaging is used */

      Message0("\n Forces computed for EDEM particles WITH sources\n");

      /* Now compute the source terms */

      compute_forces_on_particles(edem_coupling, TRUE);

      if(DEM_HEAT_COUPLED_P(edem_coupling) && !DEM_HEAT_TRANSFER_FLUENT_P(edem_coupling))
        compute_heat_flux_to_particles(edem_coupling, TRUE);

      /* Now average distributions with information from EDEM particles */
      Average_Distributions(DO_CELLS, NULL_CMD);
      Cleanup_Node_Averages(FALSE);
    }

  deactivate_edem_injections(domain, track_unsteady);

  Message0("Done.\n");

  return;
}


ParticlePrototype *getDPMParticlePrototype(Tracked_Particle *p)
{
  if(NNULLP(edem_coupling.particle_prototypes)&&NNULLP(edem_coupling.injection_ids)&&NNULLP(p))
    {
      int i;

      for(i=0;i<edem_coupling.num_particle_prototypes;++i)
        if(edem_coupling.injection_ids[i] == p->pp->I_id)
          return edem_coupling.particle_prototypes+i;
    }

  return (ParticlePrototype *)NULL;
}

double getDPMCrossSection(Tracked_Particle *p, real *dir)
{
  ParticlePrototype *pp;

  pp = getDPMParticlePrototype(p);

  if(NNULLP(pp))
    {
      tDimensionValue direction;

      NV_V(direction,=(double),dir); /* direction is a pointer to double not real */

      return SQR(P_DEM_SCALE(p)) * getParticlePrototypeCrossSection(pp, direction);
    }

  return 0.0;
}

double getDPMSurfaceArea(Tracked_Particle *p)
{
  ParticlePrototype *pp;

  pp = getDPMParticlePrototype(p);

  if(NNULLP(pp))
    {
      return SQR(P_DEM_SCALE(p)) * getParticlePrototypeSurfaceArea(pp);
    }

  return 0.0;
}


double getDPMSphericity(Tracked_Particle *p)
{
  ParticlePrototype *pp;

  pp = getDPMParticlePrototype(p);

  if(NNULLP(pp))
    {
      return getParticlePrototypeSphericity(pp);
    }

  return 0.0;
}


/*
 * udf_util_data[] defining UDF utility function entries
 *
 * Note: The strings in each element should be typed exactly as shown
 */

#if sys_ntx86  || sys_win64
#  define DLLEXP __declspec(dllexport)
#else
#  define DLLEXP
#endif

DLLEXP UDF_Data udf_util_data[] = {
  {"fl_dpm_pre_solve_update",  (void (*)(void))deactivate_edem_injections, -1},
  {"fl_dpm_post_solve_update", (void (*)(void))track_edem_injections, -1},
};

DLLEXP int n_udf_util_data = sizeof(udf_util_data)/sizeof(UDF_Data);

