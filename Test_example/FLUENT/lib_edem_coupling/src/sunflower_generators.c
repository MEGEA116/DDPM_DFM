#include "sunflower_generators.h"

Sunflower *init_sunflower_generator(int num_points, double axis[ND_3], double inner_rad, double outer_rad, double start_angle, double pos)
{
  Sunflower *sunflower;

  sunflower = (Sunflower*)malloc(sizeof(Sunflower));

  sunflower->type = FLAT_SUNFLOWER;
  sunflower->count = 0;

  sunflower->area_inner = inner_rad * inner_rad;
  sunflower->area_outer = outer_rad * outer_rad;
  sunflower->area_delta = (sunflower->area_outer - sunflower->area_inner)/num_points;

  sunflower->area = sunflower->area_inner + sunflower->area_delta * pos;
  sunflower->angle = start_angle;

  get_arbitrary_axes(axis, sunflower->nx, sunflower->ny, sunflower->nz);

  return sunflower;
}

int get_next_flat_sunflower_point(Sunflower *sunflower, double point[ND_ND])
{
  double radius, rSinA, rCosA;

  radius = sqrt(sunflower->area);

  rSinA = radius * sin(sunflower->angle);
  rCosA = radius * cos(sunflower->angle);

  NV_VS_VS(point,=,sunflower->nx,*,rCosA,+,sunflower->ny,*,rSinA);

  sunflower->area  += sunflower->area_delta;
  sunflower->angle += GOLDEN_ANGLE;
  sunflower->count++;

  return sunflower->count;
}

void _calc_spherical_sunflower_point(Sunflower *sunflower, double point[ND_ND])
{
  double height, radius, rSinA, rCosA;

  height = 1.0 - sunflower->area;
  height = MIN(height, 1.0); /* Limit height to project sqrt */
  height = MAX(-1.0, height);
  radius = sqrt(1.0 - height * height);

  rSinA = radius * sin(sunflower->angle);
  rCosA = radius * cos(sunflower->angle);

  NV_VS(point,=,sunflower->nz,*,height);
  NV_VS_VS(point,+=,sunflower->nx,*,rCosA,+,sunflower->ny,*,rSinA);
}

Sunflower *init_spherical_sunflower_generator(int num_points, double axis[ND_ND], double inner_solid_angle, double outer_solid_angle, double start_angle, double pos, cxboolean align_to_axis)
{
  Sunflower *sunflower;

  sunflower = (Sunflower*)malloc(sizeof(Sunflower));

  sunflower->type = SPHERICAL_SUNFLOWER;
  sunflower->count = 0;

  sunflower->area_inner = inner_solid_angle/M_TWO_PI;
  sunflower->area_outer = outer_solid_angle/M_TWO_PI;

  if(num_points > 0) 
    {
      /* For known fixed number with points in bands */
      sunflower->area_delta = (sunflower->area_outer - sunflower->area_inner)/(double)num_points;
      sunflower->area = sunflower->area_inner + sunflower->area_delta * pos;
    }
  else
    {
      /* 
         For endless points. If sunflower->area_inner = 0.0 & pos = 0.0, 
         first point will be aligned (See test below)
      */
      sunflower->area_delta = (sunflower->area_outer - sunflower->area_inner)/((2.0 + GOLDEN_RATIO)/(1.0 + GOLDEN_RATIO));
      sunflower->area = sunflower->area_inner + (sunflower->area_outer - sunflower->area_inner) * pos;
    }


  sunflower->angle = start_angle;

  get_arbitrary_axes(axis, sunflower->nx, sunflower->ny, sunflower->nz);


  if (align_to_axis)
    {
      double first_point[ND_ND];
      double len;

      _calc_spherical_sunflower_point(sunflower, first_point);
      NV_VV(sunflower->align_normal,=,first_point,-,sunflower->nz);
      len = NV_MAG(sunflower->align_normal);

      if(len > 0.0)
        {
          NV_S(sunflower->align_normal,/=,len);

          sunflower->type |= ALIGNED_TO_AXIS;
        }
      /* Else first_point already aligned */
    }

  return sunflower;
}

int get_next_spherical_sunflower_point(Sunflower *sunflower, double point[ND_ND])
{
  _calc_spherical_sunflower_point(sunflower, point);

  if(sunflower->type & ALIGNED_TO_AXIS)
    {
      double dist;

      dist = 2.0 * NV_DOT(sunflower->align_normal, point);

      NV_VS(point,-=,sunflower->align_normal,*,dist); /* Maintain same reflection as first point */
    }

  sunflower->area  += sunflower->area_delta;
  while (sunflower->area > sunflower->area_outer)
    {
      sunflower->area -= sunflower->area_outer - sunflower->area_inner;
    }

  sunflower->angle += GOLDEN_ANGLE;
  while(sunflower->angle > M_TWO_PI)
    sunflower->angle -= M_TWO_PI;

  sunflower->count++;

  return sunflower->count;
}

int get_next_sunflower_point(Sunflower *sunflower, double point[ND_ND])
{
  if(sunflower->type & SPHERICAL_SUNFLOWER)
    return get_next_spherical_sunflower_point(sunflower, point);

  return get_next_flat_sunflower_point(sunflower, point);
}

void get_arbitrary_normal(double vec[ND_ND], double norm[ND_ND])
{
  double length;

#if RP_2D
  length = sqrt(vec[0]*vec[0] + vec[1]*vec[1]);
  norm[0] = vec[1]/length;
  norm[1] = -vec[0]/length;
#else

  double smallest_coord;
  int smallest_index, i, i_up, i_dn;

  smallest_index = 0;
  smallest_coord = fabs(vec[0]);

  for (i=1;i<ND_3;i++)
    if (fabs(vec[i]) < smallest_coord)
      {
        smallest_index = i;
        smallest_coord = fabs(vec[i]);
      }

  i_up = (smallest_index+1)%ND_3;
  i_dn = (smallest_index+2)%ND_3;

  length = sqrt(vec[i_up]*vec[i_up] + vec[i_dn]*vec[i_dn]);
  norm[smallest_index] = 0.0;
  norm[i_up] = vec[i_dn]/length;
  norm[i_dn] = -vec[i_up]/length;

#endif
}

void get_arbitrary_axes(double axis[ND_3], double nx[ND_3], double ny[ND_3], double nz[ND_3])
{
  double axis_len;

  get_arbitrary_normal(axis, nx);

  axis_len = NV_MAG(axis);
  NV_VS(nz,=,axis,/,axis_len);
  NV_CROSS(ny,nz,nx);
}
