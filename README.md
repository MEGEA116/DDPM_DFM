# DDPM_DFM
These are user defined functions for drag force calculation between fluid and particles in CFD-DEM coupling using ANSYS FLUENT and EDEM.

The drag force model "BL" uses the drag force coefficient proposed by Brown and Lawler (2003) and the drag force model "DAL" uses the original form described in Di Felice (1994).
All the models have already been compiled for serial and parallel computation.

HOW TO TEST the CODE?
First, the coupling procedure can be found in D.E.M. Solutions (2015). Parallel EDEM-CFD coupling for Ansys Fluent®–User Guide. DEM Solutions Ltd., Edinburgh, UK.
Second, you should import the user defined functions in FLUENT by clicking the button "compiled UDFs" before coupling and select the drag force model "BL".

We have provided you a test example of single particle settling in fluid.
