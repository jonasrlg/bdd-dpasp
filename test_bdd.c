#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <cudd.h>
#include <cudd/util.h>
#include <cudd/st.h>
#include <stdbool.h>
#include "count_bdd.h"

/**
 * Print a dd summary
 * pr = 0 : prints nothing
 * pr = 1 : prints counts of nodes and minterms
 * pr = 2 : prints counts + disjoint sum of product
 * pr = 3 : prints counts + list of nodes
 * pr > 3 : prints counts + disjoint sum of product + list of nodes
 * @param the dd node
 */
void print_dd (DdManager *gbm, DdNode *dd, int n, int pr)
{
    printf("DdManager nodes: %ld | ", Cudd_ReadNodeCount(gbm)); /*Reports the number of live nodes in BDDs and ADDs*/
    printf("DdManager vars: %d | ", Cudd_ReadSize(gbm) ); /*Returns the number of BDD variables in existence*/
    printf("DdManager reorderings: %d | ", Cudd_ReadReorderings(gbm) ); /*Returns the number of times reordering has occurred*/
    printf("DdManager memory: %ld \n", Cudd_ReadMemoryInUse(gbm) ); /*Returns the memory in use by the manager measured in bytes*/
    Cudd_PrintDebug(gbm, dd, n, pr);  // Prints to the standard output a DD and its statistics: number of nodes, number of leaves, number of minterms.
}

/**
 * Writes a dot file representing the argument DDs
 * @param the node object
 */
void write_dd (DdManager *gbm, DdNode *dd, char* filename)
{
    FILE *outfile; // output file pointer for .dot file
    outfile = fopen(filename,"w");
    DdNode **ddnodearray = (DdNode**)malloc(sizeof(DdNode*)); // initialize the function array
    ddnodearray[0] = dd;
    Cudd_DumpDot(gbm, 1, ddnodearray, NULL, NULL, outfile); // dump the function to .dot file
    free(ddnodearray);
    fclose (outfile); // close the file */
}

/**
 * Writes a txt file representing the minterm count DDs
 * @param the node object
 */
void minterm_print (DdManager *gbm, DdNode *dd, int nvars, char* fp)
{
    FILE *outfile; // output file pointer for .dot file
    outfile = fopen(fp,"w");
    Cudd_ApaPrintMinterm(outfile, gbm, dd, nvars);
}

// This program creates a single BDD variable
int test0()
{   
    char filename[30], fp[30];
    DdManager *gbm; /* Global BDD manager. */
    gbm = Cudd_Init(0,0,CUDD_UNIQUE_SLOTS,CUDD_CACHE_SLOTS,0); /* Initialize a new BDD manager. */
    DdNode *bdd, *tmp, *aux, *x0, *x1, *x2, *x3, *x4, *x5, *x6;
    x0 = Cudd_bddIthVar(gbm, 0); /*Create a new BDD variable x0*/
    x1 = Cudd_bddIthVar(gbm, 1); /*Create a new BDD variable x1*/
    x2 = Cudd_bddIthVar(gbm, 2); /*Create a new BDD variable x2*/
    x3 = Cudd_bddIthVar(gbm, 3); /*Create a new BDD variable x3*/
    x4 = Cudd_bddIthVar(gbm, 4); /*Create a new BDD variable x3*/
    x5 = Cudd_bddIthVar(gbm, 5); /*Create a new BDD variable x3*/
    x6 = Cudd_bddIthVar(gbm, 6); /*Create a new BDD variable x3*/
    bdd = Cudd_bddAnd(gbm, x1, x3); /*Perform OR Boolean operation*/
    Cudd_Ref(bdd);          /*Update the reference count for the node just created.*/
    aux = Cudd_bddAnd(gbm, x1, Cudd_Not(x2)); /*Perform OR Boolean operation*/
    Cudd_Ref(aux);          /*Update the reference count for the node just created.*/
    tmp = Cudd_bddXor(gbm, bdd, aux); /*Perform OR Boolean operation*/
    Cudd_Ref(tmp);
    bdd = tmp;          /*Update the reference count for the node just created.*/
    aux = Cudd_bddAnd(gbm, x2, x4); /*Perform OR Boolean operation*/
    Cudd_Ref(aux);          /*Update the reference count for the node just created.*/
    tmp = Cudd_bddXor(gbm, bdd, aux); /*Perform OR Boolean operation*/
    Cudd_Ref(tmp);
    bdd = tmp;          /*Update the reference count for the node just created.*/
    aux = Cudd_bddXor(gbm, bdd, x5);
    Cudd_Ref(aux);
    bdd = aux;
    bdd = Cudd_BddToAdd(gbm, bdd); /*Convert BDD to ADD for display purpose*/
    print_dd (gbm, bdd, 2,4);   /*Print the dd to standard output*/
    sprintf(filename, "./test0.dot"); /*Write .dot filename to a string*/
    write_dd(gbm, bdd, filename);  /*Write the resulting cascade dd to a file*/

    int count, count_cache, count_cache2, nvars = 7;
    st_table *countable = st_init_table(st_ptrcmp,st_ptrhash);

    SatCount(gbm, bdd, countable, nvars, &count, true);

    printf("Contagem de mundos: %d\n", count);

    int obs_index[3] = {0, 2, 4};
    int assignemnt[3] = {1, 0, 1};

    SatCount_Cache(gbm, bdd, countable, nvars, 3, obs_index, assignemnt, &count_cache);

    printf("Contagem de mundos (0, ~2, 4): %d\n", count_cache);

    int obs_index2[1] = {4};
    int assignemnt2[1] = {1};

    SatCount_Cache(gbm, bdd, countable, nvars, 1, obs_index2, assignemnt2, &count_cache2);

    printf("Contagem de mundos (4): %d\n", count_cache2);
    
    Cudd_Quit(gbm);

    return 0; 
}

int test1()
{
    char filename[30];
    DdManager *dd;
    DdNode *bdd, *aux, *tmp, *add;

    dd = Cudd_Init(0,0,CUDD_UNIQUE_SLOTS,CUDD_CACHE_SLOTS,0);
    
    int n_models = 10; /* Size of the stable models. */
    int nvars = 5;
    int var_assigments[10][5] = {
                                    {0, 0, 0, 1, 1}, 
                                    {0, 0, 1, 0, 1},
                                    {0, 1, 0, 0, 1},
                                    {1, 0, 0, 0, 1},
                                    {0, 0, 1, 1, 0}, 
                                    {0, 1, 0, 1, 0},
                                    {1, 0, 0, 1, 0},
                                    {0, 1, 1, 0, 0},
                                    {1, 0, 1, 0, 0}, 
                                    {1, 1, 0, 0, 0}
    };
    
    bdd = Cudd_ReadLogicZero(dd);
    
    /* Iterate over all models*/
    for(int i = 0; i < n_models; i++) {
        tmp = buildExpression(dd, nvars, var_assigments[i]);
	    aux = Cudd_bddOr(dd, bdd, tmp); /*Perform OR Boolean operation*/
        Cudd_Ref(aux);
        Cudd_RecursiveDeref(dd, bdd);
        bdd = aux;
    }
   
    bdd = Cudd_BddToAdd(dd, bdd); /*Convert BDD to ADD for display purpose*/
    print_dd (dd, bdd, 2,4);   /*Print the dd to standard output*/
    sprintf(filename, "./test1.dot"); /*Write .dot filename to a string*/
    write_dd(dd, bdd, filename);  /*Write the resulting cascade dd to a file*/

    int count, count_cache;
    st_table *countable = st_init_table(st_ptrcmp,st_ptrhash);

    SatCount(dd, bdd, countable, nvars, &count, true);

    printf("Contagem de mundos: %d\n", count);

    int obs_index[2] = {0, 2};
    int assignemnt[2] = {0, 1};

    SatCount_Cache(dd, bdd, countable, nvars, 2, obs_index, assignemnt, &count_cache);

    printf("Contagem  (com cache) de mundos: %d\n", count_cache);

    
    Cudd_Quit(dd);

    return 0; 
}


int main(int argc, char *argv[])
{   
    printf("Test 0:\n");
    test0();
    printf("Test 1:\n");
    test1();
    return 0;
}
