// Parallel EDEM CFD Coupling for ANSYS FLUENT - Version 2.0
// Copyright 2014 ANSYS, Inc.     
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


#ifndef ADAPTOR_INTERFACE_H
#define ADAPTOR_INTERFACE_H

#include "udfParticle.h"


#ifdef __cplusplus
#ifdef DEMLINUX
#define ADAP_INT extern "C"
#else
#define ADAP_INT extern "C" __declspec(dllexport)
#endif
#else
#define ADAP_INT
#endif


/** Use to initialise the coupling client */
/** THIS NEEDS TO BE UPDATED TO INCLUDE LICENSING */
/* Coupling done as one procedure 
ADAP_INT void ADAPTOR_initialiseEDEMCoupling(int *success);
ADAP_INT void ADAPTOR_connectEDEMCoupling(int *success);
*/

ADAP_INT void ADAPTOR_init_connectEDEMCoupling(int *success);
ADAP_INT void ADAPTOR_init_connectEDEMCoupling_Address(int *success, char ipAddress[15]);
ADAP_INT void ADAPTOR_disconnectEDEMCoupling(int *success);

/**
 * Exports a new mesh file to EDEM. EDEM will return a successful import
 * 
 * Sent parameters:
 * @param filename - filename of the meshfile 
 * @param gravity  - a 3 entry array of doubles representing a gravity vector
 * @param meshFilename - the name of the mesh file including its path
 *
 * Returned parameters:
 * @param success - success boolean returned when import is successful
 * Also obsolete

ADAP_INT void ADAPTOR_newMeshFile(const char * filename,
                                  const double * gravity,
                                  const char * meshFilename,
                                  int *success);
*/


/**
 * Initialises creator, ensuring CFD simulation is updated with the correct
 * details
 *
 * Returned parameters:
 * @param numParticles - The number of particles in the simulation at time 0
 * @param numParticleTypes - The number of particle types in the simulation
 * @param success - updates whether creator launched successfully
 */
ADAP_INT void ADAPTOR_showCreator(int* success);


/**
 * Sent to EDEM to show the simulator window
 * Returned parameters:
 * @param bSuccess - determines whether the request was successful
 */
ADAP_INT void ADAPTOR_showSimulator(int *success);


/**
 * Sent to EDEM to show the analyst window
 * Returned parameters:
 * @param bSuccess - determines whether the request was successful
 */
ADAP_INT void ADAPTOR_showAnalyst(int *bSuccess);


/**
 * Sent to EDEM to stop the simulation
 * Will kill an EDEM process if running in command line mode.
 * Returned parameters:
 * @param bSuccess - determines whether the request was successful
 */
ADAP_INT void ADAPTOR_stopSimulation(int *bSuccess);

/**
  * Request EDEM to perform some time-step calculations
  * dllPrepareAnalysisStep must have returned successfully before
  * this method is called
  * Sent parameters:
  * @param cfdTimeStep - The cfd simulation time step
  * Returned parameters:
  * @param bSuccess - determines whether the request was successful
  */
//ADAP_INT void ADAPTOR_performAnalysisStep(double cfdTimeStep, int* bSuccess);


/**
  * Request EDEM to perform some time-step calculations
  * dllPrepareAnalysisStep must have returned successfully before
  * this method is called
  * Sent parameters:
  * @param nEdemTimeStepsToSimulate - Number of EDEM time steps to run
  * Returned parameters:
  * @param bSuccess - determines whether the request was successful
  */
ADAP_INT void ADAPTOR_performNumAnalysisSteps(int nEdemTimeStepsToSimulate, int* bSuccess);

/**
  * Request EDEM to perform some time-step calculations
  * dllPrepareAnalysisStep must have returned successfully before
  * this method is called
  * Sent parameters:
  * @param cfdTime - The current cfd total simulation total time
  * Returned parameters:
  * @param bSuccess - determines whether the request was successful
  */
ADAP_INT void ADAPTOR_performAnalysisToTime(double cfdTime, int* bSuccess);


/**
 * Get the time from EDEM
 * Returned parameters:
 * @param nTime - Current elapsed simulation time in EDEM
 * @param bSuccess - determines whether the request was successful
 */
ADAP_INT void ADAPTOR_getEDEMTime(double *time, int *success);


/**
 * Set the time-step that EDEM is at. Simulation will commence from
 * this point when started
 * Sent parameters:
 * @param theTime - Time that EDEM is set to
 * Returned parameters:
 * @param numParticles - Number of particles in simulation
 * @param numParticleTypes - Number of particle types in simulation
 * @param bSuccess - determines whether the request was successful
 */
ADAP_INT void ADAPTOR_setEDEMTime(double theTime, int* success);

/**
 * Select to load a deck in EDEM
 * Sent parameters:
 * @param filename - The filname, including path, of the EDEM deck
 * Returned parameters:
 * @param numParticles - Number of particles in simulation
 * @param numParticleTypes - Number of particle types in simulation
 * @param bSuccess - determines whether the request was successful
 */
ADAP_INT void ADAPTOR_selectDeck(char* filename, int* success);


/**
 * Rotate a vector in EDEM using the Quaternion class
 * Sent parameters:
 * @param vec - The 3D vector double[3]
 * @param orientation - The orientation required
 * Returned parameters:
 * @param vec - vec is returned rotated to its new orientation
 * THIS SHOULD NOT BE OFFERED AS PART OF THE API BUT PORTED ACROSS AS A METHOD
 * SO INEFFICIENT
 */
ADAP_INT void ADAPTOR_rotateVector(double* vec, double* orientation);


/**
 * Get the data for all the particle prototypes
 * Fills a global array with data for later picking with 
 * ADAPTOR_getParticlePrototype(int index, ParticlePrototype * pp);
 * Returned parameter:
 * @return number of particle prototypes (AKA "types")
 */
ADAP_INT int ADAPTOR_getParticlePrototypeData();


/**
 * Get the data for a specific particle prototype
 * Sent parameters:
 * @param int type the particle type index
 * @param ParticlePrototype * pp the particle prototype sturcture to be filled
 * Returned parameter:
 * @return number of particle types (Prototypes)
 */
ADAP_INT int ADAPTOR_getParticlePrototype(int index, ParticlePrototype *pp);


/**
 * Clears the global array of particle prototype data
 */
ADAP_INT void ADAPTOR_clearParticlePrototypeData();


/**
 * Get the data for all the particles
 * Fills a global array with data for later picking with 
 * ADAPTOR_getParticle(int index, DiscreteElement* particle);
 * Returned parameter:
 * @return success
 */
ADAP_INT int ADAPTOR_getParticleData();

/**
 * Get the data for a particle based on the type and particle index
 * Index is NOT the same as the EDEM particle ID
 * Sent parameters:
 * @param int index the particle index
 * @param DiscreteElement* particle the particle structure to be filled
 * Returned parameters:
 * @return DiscreteElement* particle the pointer to the particle
 */
ADAP_INT void ADAPTOR_getParticle(int index, DiscreteElement* particle);


/**
 * Updates the particles current cell and thread id, for particle tracking purposes
 * @param index - The index of the particle
 * @param cell - The Fluent cell ID that the particle has
 * @param thread - The Fluent thread ID that the particle has
 */
ADAP_INT void ADAPTOR_updateCellAndThread(int index, cell_t cell, void* thread);


/**
 * Clears any particle data held in the data map
 */
ADAP_INT void ADAPTOR_clearParticleData();


/**
 * Get the number of particle types
 * @return The number of particle types
 */
ADAP_INT int ADAPTOR_getNumParticleTypes();


/**
 * Get the number of particle of a specified type
 * Sent parameters:
 * @param type - particle type index
 * @return The number of particles
 */
ADAP_INT int ADAPTOR_getNumParticles(int type);


/**
 * Get the total number of particles in the simulation
 * @return Returns the total number of particles
 */
ADAP_INT int ADAPTOR_getTotalNumParticles();

ADAP_INT void ADAPTOR_getNumParticlesPerType(int numParticleTypes, int* numParticlesPerType);


/**
 * Get the same points for particle occupation of a fluid cell
 * Sent parameters:
 * @param samples - number of samples to take
 * @param type - particle type index
 * Returned parameters
 * @param fluentSamplePoints - 3D vector of sample point coordinates
 * @return success? true/false
 */
ADAP_INT int ADAPTOR_getParticleTypeSamplePoints(tDimensionValue* fluentSamplePoints,
                                                 int samples,
                                                 int type);

/**
 * Set the drag force on a particle
 * Sent parameters:
 * @param numParticles - The number of particles in the simulation
 * @param force - A pointer to an array of 3D force vectors
 * @param torque - A pointer to an array of 3D torque vectors
 * @return success? true/false
 */
ADAP_INT int ADAPTOR_setDragForceAndTorque(int numParticles, double* force, double* torque);


ADAP_INT int ADAPTOR_registerCustomProperty(char* name,
						                    int numberElements,
						                    int dataType,
						                    int unitType,
						                    double initialValue,
						                    int *index);


ADAP_INT int ADAPTOR_updateValuesForProperty(int numParticles, int propertyIndex);


ADAP_INT int ADAPTOR_setValuesForProperty(int numParticles, int propertyIndex,
                                          double* data);


ADAP_INT double* ADAPTOR_getProperty(int customPropIndex, int particleIndex);


ADAP_INT double ADAPTOR_getScalarProperty(int customPropIndex, int particleIndex);

#endif
