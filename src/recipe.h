#ifndef RECIPE_H
#define RECIPE_H

#include "common.h"
#include "objects.h"

// Recipe : this is the object the user will, effectively, send us
struct Recipe {
	recipe_id id;

	int cook_time;
	int prep_time;
	int servings;

	char *ingredients[128];
	char *steps[128];
	char *tags[64];

	char note[256];
};

#endif

