#include <stdio.h>
#include <stdbool.h>
/*
#include <cudd.h>
#include <cudd/util.h>
#include <cudd/st.h>
*/

#ifndef _COUNT_BDD   /* Include guard */
#define _COUNT_BDD

int ipow(int base, int exp);

int getPower_Cache(DdManager *dd, DdNode *node, int index_child, int index_parent, int obs_child, int obs_parent, bool inside_parent);

bool isInside(DdManager *dd, DdNode *node, int *array, int size, int begin, int val, int *index);

bool SatCount(DdManager *dd, DdNode *node, st_table *countable, int nvars, int *count, bool debug);

int SatCount_Aux(DdManager *dd, DdNode *node, st_table *countable, int nvars, int index, bool debug);

bool SatCount_Cache(DdManager *dd, DdNode *node, st_table *countable, int nvars, int n_obs, int *obs_index, int *assignments, int *count);

int SatCount_Cache_Aux(DdManager *dd, DdNode *node, st_table *countable, int nvars, int index, int n_obs, int obs_pos, bool inside, int obs_index[], int assignments[]);

DdNode * buildExpression(DdManager *dd, int nvars, int assigments[]);

#endif
