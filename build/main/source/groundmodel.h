#ifndef GROUNDMODEL_H__
#define GROUNDMODEL_H__

#include "OgrePrerequisites.h"
#include "OgreColourValue.h"

typedef struct ground_model_t
{
	float va; //adhesion velocity
	float ms; //static friction coefficient
	float mc; //sliding friction coefficient
	float t2; //hydrodynamic friction (s/m)
	float vs; //stribeck velocity (m/s)
	float alpha; //steady-steady
	float strength; //ground strength

	float fluid_density;	// Density of liquid
	float flow_consistency_index;// general drag coefficient

	// if flow_behavior_index<1 then liquid is Pseudoplastic (ketchup, whipped cream, paint)
	// if =1 then liquid is Newtonian fluid
	// if >1 then liquid is Dilatant fluid (less common)
	float flow_behavior_index;

	// how deep the solid ground is
	float solid_ground_level;

	// Upwards/Downwards drag anisotropy
	float drag_anisotropy;

	int fx_type;
	Ogre::ColourValue fx_colour;
	char name[255];
	char basename[255];
	char particle_name[255];

	int fx_particle_amount; // amount of particles

	float fx_particle_min_velo; // minimum velocity to display sparks
	float fx_particle_max_velo; // maximum velocity to display sparks
	float fx_particle_fade; // fade coefficient
	float fx_particle_timedelta; // delta for particle animation
	float fx_particle_velo_factor; // velocity factor
	float fx_particle_ttl;

} ground_model_t;

#endif //GROUNDMODEL_H__
