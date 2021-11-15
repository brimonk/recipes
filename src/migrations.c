// Brian Chrzanowski
// 2021-11-05 17:08:16
//
// NOTE	(Brian): Migrations in EF Core (what I'm used to) are the biggest pain in the ass in the
// world. So, if this little migration "framework" isn't very easy to use in some case, then I must
// have failed in my design in some way.
//
// Each migration is listed in order from Version 0x0001, to Version 0xFFFFF...

#include "common.h"

#include "objects.h"
#include "store.h"

typedef struct migration_t {
	int version;
	int (*func)(void);
} migration_t;

migration_t migrations[];

// migration_0x0001 : migrate entity ids to be one based
int migration_0001(void)
{
	s64 i, len;

	// NOTE (Brian): iterate on every table, and update the index to be 1 based, and not 0 based.
	// At this point in the codebase, the code has already been changed to perform 1 based indexing,
	// but the data is still 0 indexed.

	for (i = 0, len = store_getlen(RT_USER); i < len; i++) {
		user_t *user;
		user = store_getobj(RT_USER, i + 1);

		if (user != NULL) {
			user->base.id++;
			user->username++;
			user->email++;
			user->password++;
			user->salt++;
		}
	}

	for (i = 0, len = store_getlen(RT_RECIPE); i < len; i++) {
		recipe_t *recipe;
		recipe = store_getobj(RT_RECIPE, i + 1);

		if (recipe != NULL) {
			recipe->base.id++;

			recipe->user_id++;
			recipe->name_id++;
			recipe->prep_time_id++;
			recipe->cook_time_id++;
			recipe->servings_id++;

			// NOTE (Brian): before now, we didn't really have a way to denote an empty note
			recipe->note_id++;
		}
	}

	for (i = 0, len = store_getlen(RT_INGREDIENT); i < len; i++) {
		ingredient_t *ingredient;
		ingredient = store_getobj(RT_INGREDIENT, i + 1);

		if (ingredient != NULL && ingredient->base.flags & OBJECT_FLAG_USED) {
			ingredient->base.id++;
			ingredient->recipe_id++;
			ingredient->string_id++;
		}
	}

	for (i = 0, len = store_getlen(RT_STEP); i < len; i++) {
		step_t *step;
		step = store_getobj(RT_STEP, i + 1);

		if (step != NULL && step->base.flags & OBJECT_FLAG_USED) {
			step->base.id++;
			step->recipe_id++;
			step->string_id++;
		}
	}

	for (i = 0, len = store_getlen(RT_TAG); i < len; i++) {
		tag_t *tag;
		tag = store_getobj(RT_TAG, i + 1);

		if (tag != NULL && tag->base.flags & OBJECT_FLAG_USED) {
			tag->base.id++;
			tag->recipe_id++;
			tag->string_id++;
		}
	}

	for (i = 0, len = store_getlen(RT_STRING128); i < len; i++) {
		string_128_t *string;
		string = store_getobj(RT_STRING128, i + 1);

		if (string != NULL && string->base.flags & OBJECT_FLAG_USED) {
			string->base.id++;
		}
	}

	for (i = 0, len = store_getlen(RT_STRING256); i < len; i++) {
		string_256_t *string;
		string = store_getobj(RT_STRING256, i + 1);

		if (string != NULL && string->base.flags & OBJECT_FLAG_USED) {
			string->base.id++;
		}
	}

	return 0;
}

// migrations : the actual table of migrations, in order from earliest to latest
migration_t migrations[] = {
	{ 0x0001, migration_0001 }
};

// migrations_checktab : returns true if there's an error with the migrations table
int migrations_checktab(void)
{
	int i;

	for (i = 0; i < ARRSIZE(migrations); i++) {
		if (i + 1 != migrations[i].version)
			return 1;
	}

	return 0;
}

// migrations_exec : performs migrations one at a time
int migrations_exec(void)
{
	s64 i;
	int rc;

	// NOTE (Brian): this will return < 0 if there was an error

	rc = migrations_checktab();
	if (rc) {
		ERR("there's an order error in the migration table!\n");
		return -1;
	}

	for (i = store_getversion() - 1; i < ARRSIZE(migrations); i++) {
		migrations[i].func();
		store_setversion(migrations[i].version + 1);
	}

	return 0;
}

