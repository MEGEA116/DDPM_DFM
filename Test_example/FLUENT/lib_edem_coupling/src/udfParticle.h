// Parallel EDEM CFD Coupling for ANSYS FLUENT - Version 2.2
// Copyright 2016 ANSYS, Inc.     
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	 http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either expressed or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Check for the latest news at
//
// http://www.dem-solutions.com/forum


#ifndef UDFPARTICLE_H
# define UDFPARTICLE_H 1

// Fluent types used in AdaptorInterface
//   but which can't be redefined in a UDF

#ifndef _FLTG_MEM_H
typedef int cell_t;
#endif

/* Constants */
#ifndef NULL_CELL
# define NULL_CELL (-1)
#endif

#ifndef NULL_THREAD
# define NULL_THREAD ((void*)0)
#endif

#ifndef ND_3
# define ND_3 3
#endif 

/* Macros for property data types */
#define CM_PROP_DATA_TYPE_DOUBLE 0

/* macros for property unit types */
#define CM_PROP_UNIT_TYPE_OTHER                0
#define CM_PROP_UNIT_TYPE_NONE                 1
#define CM_PROP_UNIT_TYPE_ACCELERATION         2
#define CM_PROP_UNIT_TYPE_ANGLE                3
#define CM_PROP_UNIT_TYPE_ANGULAR_ACELERATION  4
#define CM_PROP_UNIT_TYPE_ANGULAR_VELOCITY     5
#define CM_PROP_UNIT_TYPE_DENSITY              6
#define CM_PROP_UNIT_TYPE_ENERGY               7
#define CM_PROP_UNIT_TYPE_WORK_FUNCTION        8
#define CM_PROP_UNIT_TYPE_FORCE                9
#define CM_PROP_UNIT_TYPE_CHARGE              10
#define CM_PROP_UNIT_TYPE_LENGTH              11
#define CM_PROP_UNIT_TYPE_MASS                12
#define CM_PROP_UNIT_TYPE_MOI                 13
#define CM_PROP_UNIT_TYPE_SHEAR_MOD           14
#define CM_PROP_UNIT_TYPE_TIME                15
#define CM_PROP_UNIT_TYPE_TORQUE              16
#define CM_PROP_UNIT_TYPE_VELOCITY            17
#define CM_PROP_UNIT_TYPE_VOLUME              18
#define CM_PROP_UNIT_TYPE_FREQUENCY           19
#define CM_PROP_UNIT_TYPE_TEMPERATURE         20
#define CM_PROP_UNIT_TYPE_HEAT_FLUX           21


/* Types */
typedef double tDimensionValue[ND_3];


typedef struct SamplePointVectors_struct
{
    char*    sParticleType;
    double** pSampleVectors;
} SamplePointVectors;

#define MAX_DE_TYPENAME_SZ 64  /* Matches MAX_TYPENAME_SZ set elsewhere as 64 */


typedef struct ParticleSphere_struct
{
  int nSphereID;
  char sSphereName[MAX_DE_TYPENAME_SZ];

  double nRadius;
  int bUsesContactRadius;
  double nContactRadius;

  tDimensionValue vPosition;
} ParticleSphere;


typedef struct ParticlePrototype_struct
{
  int nPrototypeID;
  char sPrototypeName[MAX_DE_TYPENAME_SZ];

  double nMass;
  double nVolume;

  tDimensionValue vMomentOfInertia;

  int nSurfaces;
  ParticleSphere *pSphereData;

  double nSurfaceArea;
  double nSphericity; /* Surface area of equivalent sphere / actual surface area */

  int nCrossSectionDirections;
  tDimensionValue *pCrossSectionDirections;
  double *pCrossSectionAreas;

} ParticlePrototype;


typedef struct DiscreteElement_struct
{
  int elemID;
  int typeIndex; /* The particle type index */
  char sType[MAX_DE_TYPENAME_SZ]; /* The particle type name */

  tDimensionValue vPos;         /* Particle position */
  tDimensionValue vVelocity;
  tDimensionValue vAngVelocity; /* Angular velocity */
  tDimensionValue vForce;   /* Force on particle  _including gravity_ but
                               excluding any particle/particle contact forces */
  tDimensionValue vTorque;
  double nVolume;
  double nScale;

  /* A list of vectors to points within the particle */
  double vOrientation[4];

  /* Cell and cell thread containing this DiscreteElement             */
  cell_t nContainingCell;   /* cell that particle is contained in     */
  void*  pContainingThread; /* Thread that containing cell is in      */
} DiscreteElement;

#endif /* UDFPARTICLE_H */

