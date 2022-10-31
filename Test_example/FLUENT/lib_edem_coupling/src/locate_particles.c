/*********************************************************
 *
 * UDF to locate a particle list
 *
 *********************************************************************/
#include "udf.h"
#include "dpm_parallel.h"


/* print out some verbosity messages */
void print_particle_mass_and_count(char *mess_str, Particle **pl)
{
  int n_par = count_not_removed_p_list(*pl);
  real p_mass = compute_mass_p_list(*pl);
  
#if RP_NODE
  Message("Locate-Node %d: %d particles %g mass%s\n", myid, n_par, p_mass, mess_str);
  n_par = PRF_GISUM1(n_par);
  p_mass = PRF_GRSUM1(p_mass);
#endif
  Message0("Locate-Total   : %d particles %g mass%s\n", n_par, p_mass, mess_str);
  CX_Flush();
}


/* Define the location routine 
 * **pl is the particle list to be located
 * **pl is also holding the particles after relocation
 */

int Locate_Unsteady_Particle_List(Particle **pl, cxboolean urge_particles_into_domain)
{
  int nlost=0;
  int verbosity = RP_Get_Integer("dpm/parallel/print-verbosity");

  if (!dpm_par.injections_defined)
    return 0;

  if (I_DO_DPM)
    {
      Particle *lost=NULL;
	
      if ( verbosity > 2)
        print_particle_mass_and_count(" ", pl);

      /* First locate particles in partition and collect lost particles */
      Locate_Particle_List_Local(pl, pl, &lost, FALSE, 0);
	
      if ( verbosity > 2)
        print_particle_mass_and_count(" found locally", pl);

#if RP_NODE
      if (compute_node_count > 1)
        {
          Particle *pp = lost;
	    
          if (verbosity > 2)
            print_particle_mass_and_count(" to be relocated netwide", &pp);

          lost = NULL;
          nlost = Locate_Particle_List_Netwide(&pp, pl, &lost, urge_particles_into_domain, 0, FALSE);  
        }
      else
        nlost = count_p_list(lost);

#else /* RP_NODE */

      nlost = count_p_list(lost);

      /* Now let's take care of particles, lost outside of the domain */

      if (nlost > 0 && urge_particles_into_domain)
        {
          Domain *domain;
          Particle *pp = lost;

          domain = Get_Domain(ROOT_DOMAIN_ID);

          lost = NULL;
          if (verbosity > 2)
            print_particle_mass_and_count(" not to be disallowed", &pp);

          Locate_Particle_List_in_Domain_Local(&pp, pl, &lost, domain);

          if (verbosity > 2)
            print_particle_mass_and_count(" not disallowed", &lost);

          nlost = free_particle_list(lost);
        }
#endif /* RP_NODE */

    }

  nlost = PRF_GISUM1(nlost);

  if ( nlost && dpm_par.verbosity >= 1)
    Message0("Lost %d particles during relocation\n",nlost);

  if ( verbosity > 2)
    print_particle_mass_and_count(" relocated", pl);

  return nlost;
}


void Host_to_Node_Injection_Particles(Injection *I)
{
  char *buff = NULL;
#if PARALLEL
  int buff_size;
#endif
#if RP_HOST
  cxboolean migrating = FALSE;
#endif

#if RP_HOST
  buff_size = Pack_Injection(I, &buff, dpm_par.homogeneous_net, migrating);
#endif
 
  host_to_node_int_1(buff_size);

#if RP_NODE
  alloc_particle_buffer(&(buff), buff_size, dpm_par.homogeneous_net);
#endif

#if PARALLEL
  if ( dpm_par.homogeneous_net )
    host_to_node_string((char *)buff, buff_size);
  else
    host_to_node_real((real *)buff, buff_size);
#endif

#if RP_NODE

  /* Release the particles of this injections if there are any */

  if (NNULLP(I->p))
    {
      free_particle_list(I->p);
      I->p = NULL; /* need to set I->p to NULL explicityl */
    }

  I->n_particles = 0;
  I->p_tail = NULL;

  /* Now unpack the new particles */

  Unpack_Injection(I, buff, buff_size, dpm_par.homogeneous_net, NULL);
#endif

#if !RP_HOST
  /* Locate only the particles local to the node or in serial, the other particles are kept on the other nodes */
  {
    Particle *lost = NULL;
    
    Locate_Particle_List_Local(&I->p, &I->p, &lost, FALSE, 0);
    reset_injection_nparticles_and_tail(I);

    free_particle_list(lost);
  }
#endif

  FREE_TEMP(buff);

}


void Node_to_Host_Injection_Particles(Injection *I)
{
#if RP_NODE
  /* Send buffers packed injection buffers to host via node-0 */
  cxboolean migrating = FALSE;
  char *sndbuff = NULL;
  int sndbuff_size;

  /* Alloc and pack send buffer on each node */

  sndbuff_size = Pack_Injection(I, &sndbuff, dpm_par.homogeneous_net, migrating);
  
  if ( dpm_par.homogeneous_net )
    PRF_all_to_one_csend_char(node_host, (char *)sndbuff, sndbuff_size);
  else
    PRF_all_to_one_csend_real(node_host, (real *)sndbuff, sndbuff_size);

  CX_Free(sndbuff);

  free_particle_list(I->p);
  I->p = NULL;
  I->n_particles = 0;
  I->p_tail = NULL;
#endif

#if RP_HOST
  /* Receive buffer length and buffer from each node via node-0 */
  int n;

  /* Release the particles of the injection if there are any on the host */
  if (NNULLP(I->p))
    {
      free_particle_list(I->p);
      I->p = NULL; /* need to set I->p to NULL explicitly */
    }
  I->n_particles = 0;
  I->p_tail = NULL;


  compute_node_loop(n)
  {
    int len;
    PRF_CRECV_INT(node_zero, &len, 1, n);

    if (len > 0)
      {
        char *rcvbuff = NULL;

        alloc_particle_buffer(&(rcvbuff), len, dpm_par.homogeneous_net);

        if (dpm_par.homogeneous_net)
          {
            PRF_CRECV_CHAR(node_zero, (char *)rcvbuff, len, n);
          }
        else
          {
            PRF_CRECV_REAL(node_zero, (real *)rcvbuff, len, n);
          }

        /* unpack buffers */
        Unpack_Injection(I, rcvbuff, len, dpm_par.homogeneous_net, NULL);

        CX_Free(rcvbuff);
      }
  }
#endif

}

