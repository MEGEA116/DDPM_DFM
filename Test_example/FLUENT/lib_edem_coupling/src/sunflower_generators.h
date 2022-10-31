#ifndef SUNFLOWER_GENERATORS_H
# define SUNFLOWER_GENERATORS_H 1

#define GOLDEN_RATIO 1.6180339887498949 /* (sqrt(5)+1)/2 */
#define GOLDEN_ANGLE 2.399963229728653 /* 137.5078 degs */
#define BAND_CENTER 0.5 /* Usually want to place point at centre of area as gives better looking distribution */

#ifndef M_TWO_PI
# define M_TWO_PI 6.28318530717958647692
#endif
#ifndef M_FOUR_PI
# define M_FOUR_PI 12.56637061435917295384
#endif

#define HEMISPHERE_SOLID_ANGLE M_TWO_PI
#define SPHERE_SOLID_ANGLE M_FOUR_PI

typedef enum
  {
    FLAT_SUNFLOWER = 0x00,
    SPHERICAL_SUNFLOWER = 0x01,
    ALIGNED_TO_AXIS = 0x02,
  }SunflowerType;

#include "udf.h"

typedef struct sunflower_struct
{
 int count;
/*
  Note _area values are area / 2.PI  for flat sunflower
  and solid angles / 2.PI for spherical sunflower. (= height)
*/
 double area;
 double area_inner;
 double area_outer;
 double area_delta;
 double angle;
 double nx[ND_3];
 double ny[ND_3];
 double nz[ND_3]; /* Normalised axis direction */
 SunflowerType type;
 double align_normal[ND_3]; /* Normal to alignment reflection plane */
}Sunflower;

/* Function declarations */

Sunflower *init_sunflower_generator(int num_points, double axis[ND_3], double inner_rad, double outer_rad, double start_angle, double pos);
int get_next_flat_sunflower_point(Sunflower *s, double point[ND_3]);

Sunflower *init_spherical_sunflower_generator(int num_points, double axis[ND_3], double inner_solid_angle, double outer_solid_angle, double start_angle, double pos, cxboolean align_to_axis);
int get_next_spherical_sunflower_point(Sunflower *s, double point[ND_3]);

int get_next_sunflower_point(Sunflower *s, double point[ND_3]);

void get_arbitrary_normal(double vec[ND_ND], double norm[ND_ND]);
void get_arbitrary_axes(double axis[ND_3], double nx[ND_3], double ny[ND_3], double nz[ND_3]);

void delete_sunflower(Sunflower *s);
#endif /* SUNFLOWER_GENERATORS_H */

