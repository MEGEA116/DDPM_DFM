#include "particle_prototype.h"
#include "sunflower_generators.h"

void init_ParticlePrototype(ParticlePrototype *pp)
{
  pp->nPrototypeID = 0;
  pp->sPrototypeName[0]='\0';  /* Particle Prototype name.  */

  pp->nSurfaces = 0;
  pp->nMass = 0.0;
  pp->nVolume = 0.0;

  NV_S(pp->vMomentOfInertia,=,0.0);

  pp->pSphereData = NULL;

  pp->nSurfaceArea = 0.0;
  pp->nSphericity = 0.0;

  pp->nCrossSectionDirections = 0;
  pp->pCrossSectionDirections = NULL;
  pp->pCrossSectionAreas = NULL;
}

void free_ParticlePrototype(ParticlePrototype *pp)
{
  if (NNULLP(pp->pSphereData))
    CX_Free(pp->pSphereData);
  pp->pSphereData = NULL;

  if (NNULLP(pp->pCrossSectionDirections))
    CX_Free(pp->pCrossSectionDirections);
  pp->pCrossSectionDirections = NULL;

  if (NNULLP(pp->pCrossSectionAreas))
    CX_Free(pp->pCrossSectionAreas);
  pp->pCrossSectionAreas = NULL;
}

int _testSphereContainsPoint(ParticleSphere *sp, tDimensionValue lpt)
{
  tDimensionValue diff;

  NV_VV(diff,=,lpt,-,sp->vPosition);

  if (NV_MAG2(diff) < SQR(sp->nRadius))
    return 1;

  return 0;
}

int _testSphereLineIntersect(ParticleSphere *sp, tDimensionValue lpt, tDimensionValue ldir)
{
  tDimensionValue diff;
  double dist;

  NV_VV(diff,=,lpt,-,sp->vPosition);

  dist = NV_DOT(ldir,diff);

  NV_VS(diff,-=,ldir,*,dist);

  if (NV_MAG2(diff) < SQR(sp->nRadius))
    return 1;

  return 0;
}

double setParticlePrototypeSurfaceArea(ParticlePrototype *pp, int nPoints)
{
  Sunflower *sunflower;
  double start_angle;
  double total_surface_area;
  tDimensionValue lpos;
  int i, iss, isp, nhit;
  int surface_area_count;
  double axis[ND_3] = {1.0,0.0,0.0};

  start_angle = (double)uniform_random()*M_TWO_PI;

  total_surface_area = 0.0;

  for (iss=0;iss<pp->nSurfaces;++iss)
    {
      /* Loop through on surface of each sphere */
      sunflower = init_spherical_sunflower_generator(nPoints, axis, 0.0, M_FOUR_PI, start_angle, BAND_CENTER, FALSE);

      surface_area_count = 0;

      for(i=0;i<nPoints;++i)
        {
          get_next_sunflower_point(sunflower, lpos);

          NV_S(lpos,*=,pp->pSphereData[iss].nRadius);
          NV_V(lpos,+=,pp->pSphereData[iss].vPosition);

          nhit = 0;

          /* Check if point inside any of the other Spheres. */
          for (isp=0;(isp<pp->nSurfaces)&&(!nhit);++isp)
            {
              if(isp!=iss)
                nhit = _testSphereContainsPoint(pp->pSphereData+isp, lpos);
            }

          if (!nhit)
            ++surface_area_count;
        }

      free(sunflower);

      total_surface_area += ((double)surface_area_count/(double)nPoints) * M_FOUR_PI * SQR(pp->pSphereData[iss].nRadius);
    }

  pp->nSurfaceArea = total_surface_area;

  return total_surface_area;
}

double getParticlePrototypeSurfaceArea(ParticlePrototype *pp)
{
  return pp->nSurfaceArea;
}

double setParticlePrototypeSphericity(ParticlePrototype *pp)
{
  double sphericity;

  /*sphericity = cbrt(36.0*M_PI*SQR(pp->nVolume))/pp->nSurfaceArea;*/
  sphericity = pow(36.0*M_PI*SQR(pp->nVolume),0.33333)/pp->nSurfaceArea;

  pp->nSphericity = sphericity;

  return sphericity;
}

double getParticlePrototypeSphericity(ParticlePrototype *pp)
{
  return pp->nSphericity;
}

double calcParticlePrototypeCrossSection(ParticlePrototype *pp, tDimensionValue ldir, int nLines)
{
  Sunflower *sunflower;
  double start_angle;
  double cross_section_fraction, total_cross_section;
  tDimensionValue lpos;
  int i, iss, isp, nhit;

  start_angle = (double)uniform_random()*M_TWO_PI;

  total_cross_section = 0.0;

  for (iss=0;iss<pp->nSurfaces;++iss)
    {
      /* Loop through lines originating on circle of each sphere */
      sunflower = init_sunflower_generator(nLines, ldir, 0.0, pp->pSphereData[iss].nRadius, start_angle, BAND_CENTER);

      cross_section_fraction = 0.0;

      for(i=0;i<nLines;++i)
        {
          get_next_sunflower_point(sunflower, lpos);
          NV_V(lpos,+=,pp->pSphereData[iss].vPosition);

          nhit = 0;

          /* Count how many of the other Spheres the line passes through. */
          for (isp=0;isp<pp->nSurfaces;++isp)
            {
              if(isp!=iss)
                nhit += _testSphereLineIntersect(pp->pSphereData+isp, lpos, ldir);
            }

          if (nhit)
            cross_section_fraction += 1.0/(1.0+(double)nhit); /* Scale to allow for number of overlapping circles */
          else
            cross_section_fraction += 1.0;
        }

      free(sunflower);

      total_cross_section += (cross_section_fraction/(double)nLines) * M_PI * SQR(pp->pSphereData[iss].nRadius);
    }

  return total_cross_section;
}



void setParticlePrototypeCrossSectionAreas(ParticlePrototype *pp, int nDirs, int nLines)
{
  Sunflower *sunflower;
  tDimensionValue axis = {1.0,0.0,0.0};
  int idir;
  tDimensionValue dir;
  double start_angle;

  pp->nCrossSectionDirections = nDirs;

  if (NNULLP(pp->pCrossSectionDirections))
    CX_Free(pp->pCrossSectionDirections);
  pp->pCrossSectionDirections = (tDimensionValue *)CX_Malloc(nDirs*sizeof(tDimensionValue));

  if (NNULLP(pp->pCrossSectionAreas))
    CX_Free(pp->pCrossSectionAreas);
  pp->pCrossSectionAreas = (double *)CX_Malloc(nDirs*sizeof(double));

  start_angle = (double)uniform_random()*M_TWO_PI;
  sunflower = init_spherical_sunflower_generator(nDirs, axis, 0.0, 4.0*M_PI, start_angle, BAND_CENTER, FALSE);

  for (idir=0;idir<nDirs;++idir)
    {
      get_next_spherical_sunflower_point(sunflower, dir);
      NV_V(pp->pCrossSectionDirections[idir],=,dir);
      pp->pCrossSectionAreas[idir] = calcParticlePrototypeCrossSection(pp, dir, nLines);
    }
}

double getParticlePrototypeCrossSection(ParticlePrototype *pp, tDimensionValue ldir)
{
  int idir, iclosest;
  double dotp, closest;

  if (!pp->nCrossSectionDirections || NULLP(pp->pCrossSectionDirections) || NULLP(pp->pCrossSectionAreas))
    return 0.0;

  closest = 0.0;
  iclosest = 0;

  for (idir=0;idir<pp->nCrossSectionDirections;++idir)
    {
      dotp = ABS(NV_DOT(pp->pCrossSectionDirections[idir],ldir));
      if (dotp > closest)
        {
          closest = dotp;
          iclosest = idir;
        }
    }

  return pp->pCrossSectionAreas[iclosest];
}


#if PARALLEL

void host_to_node_sphere(ParticleSphere *sp)
{
  host_to_node_string(sp->sSphereName, MAX_DE_TYPENAME_SZ);

  host_to_node_int_2(sp->nSphereID, sp->bUsesContactRadius);
  host_to_node_double_2(sp->nRadius, sp->nContactRadius);

  host_to_node_double(sp->vPosition, ND_3);
}


void host_to_node_particle_prototype(ParticlePrototype *pp)
{
  int isp;

  host_to_node_string(pp->sPrototypeName, MAX_DE_TYPENAME_SZ);

  host_to_node_int_3(pp->nPrototypeID, pp->nSurfaces, pp->nCrossSectionDirections);
  host_to_node_double_4(pp->nMass, pp->nVolume, pp->nSurfaceArea, pp->nSphericity);

  host_to_node_double(pp->vMomentOfInertia, ND_3);

#if RP_NODE
  /* Reset memory on Nodes to match sizes on Host */

  if (NNULLP(pp->pSphereData))
    CX_Free(pp->pSphereData);
  pp->pSphereData = (ParticleSphere*)CX_Malloc(pp->nSurfaces*sizeof(ParticleSphere));


  if (NNULLP(pp->pCrossSectionDirections))
    CX_Free(pp->pCrossSectionDirections);
  pp->pCrossSectionDirections = (tDimensionValue *)CX_Malloc(pp->nCrossSectionDirections*sizeof(tDimensionValue));

  if (NNULLP(pp->pCrossSectionAreas))
    CX_Free(pp->pCrossSectionAreas);
  pp->pCrossSectionAreas = (double *)CX_Malloc(pp->nCrossSectionDirections*sizeof(double));

#endif /* RP_NODE */

  for(isp=0;isp<pp->nSurfaces;isp++)
    {
      host_to_node_sphere(pp->pSphereData + isp);
    }

  for(isp=0;isp<pp->nCrossSectionDirections;isp++)
    {
      host_to_node_double(pp->pCrossSectionDirections[isp], ND_3);
    }

  host_to_node_double(pp->pCrossSectionAreas, pp->nCrossSectionDirections);
}
#endif /* PARALLEL */
