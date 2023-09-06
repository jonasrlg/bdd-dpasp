#include <stdio.h>
#include <cudd.h>
#include <cudd/util.h>
#include <cudd/st.h>
#include <stdbool.h>


/* Computer the power of 2 based on the number of nodes marginalized between
the parent and child node. If the child node is a terminal 1, we have a total
of <code> nvars - negative - 1 </code> nodes to be marginalized (we subtract
1 because we have to consider only the nodes between, not the extremeties).

If the node is a terminal 0, there are no models to be marginalized between
parent and child. Otherwise, we have a non-terminal chilld. Thus, we have
to marginalize its index (positive) and the index of the parent (negative).*/
int ipow(int base, int exp)
{
    int result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }
    return result;
}

int getIndex(
    DdManager *dd,
    int nvars,
    DdNode *node) 
{
    if ((node == Cudd_ReadOne(dd)) |  (node == Cudd_ReadZero(dd)))
        return nvars;
    return Cudd_NodeReadIndex(node);
}

int getPower(
    DdManager *dd, 
    DdNode *node, 
    int index_child,
    int index_parent) 
{
    return ipow(2, index_child - index_parent - 1);
}

int getPower_Cache(
    DdManager *dd, 
    DdNode *node,  
    int index_child, 
    int index_parent, 
    int obs_child, 
    int obs_parent,
    bool inside_parent) 
{
    /* Marginalization of all variables between child and parent. */
    int dif_index = index_child - index_parent - 1;
    /* Count all observed variables between (exclusive, withou couting the
     * ends) child and parent. */
    int dif_obs = obs_child - obs_parent - (int) inside_parent;
    return ipow(2, dif_index - dif_obs);
}

/* Traverses a sorted array of integers, trying to find the index
   where the variable <code> val </code> should be inserted. 
   Returns true if val was already inside the array; returns 
   false otherwise. 

   Since we use this code in a way that <code> begin </code> is 
   always updated with the value returned by <code> index </code>,
   we only pass each value of the array once. Thus, the amortized
   complexity of this function is O(1), better than binary search.
*/
bool isInside(
        DdManager *dd, 
        DdNode *node, 
        int *array, 
        int size, 
        int begin, 
        int val, 
        int *index) 
{
    if ((node == Cudd_ReadOne(dd)) | (node == Cudd_ReadZero(dd))) {
        *index = size;
        return false;
    }
    int i;
    bool inside = false;
    for (i = begin; i < size; i++) {
        /* We've found the variable inside the array. */
        if (array[i] == val) {
            inside = true;
            break;
        }
        /* The variable was not found inside the array, so we 
        return the index where it should be inserted. */
        else if (array[i] > val) {
            inside = false;
            break;
        }
    }
    *index = i;
    return inside;
}


/**
  @brief Count the number of models in a %BDD and store it in a hash 
  table.

  Computes model count by

  count(N) = count(N.Then) + count(N.Else)

  count(0) = 0

  count(1) = 2**(nvars-), being <code> nvars </code> the number 
  of variables registered on the %BDD manager, <code> dd </code>, 
  and <code>  </code> the depth of the path from the root node
  of the %BDD to the terminal node 1.

  @sideeffect None

*/

int SatCount_Aux(DdManager *dd, DdNode *node, st_table *countable, int nvars, int index, bool debug);

bool
SatCount(
    DdManager *dd,
    DdNode *node,
    st_table *countable,
    int nvars,
    int *count,
    bool debug
    )
{   
    int index = getIndex(dd, nvars, node);

    *count = SatCount_Aux(dd, node, countable, nvars, index, debug);

    /* If we enter this if, our hash table <code> countable </code> has
    exceded the memory limit.*/
    if (*count == -1)
        return false;
    
    /* When the root is a terminal 1, we marginalize all variables
    inside the BDD, rendering <math> 2^nvars </math> models. If it
    is a terminal 0, we clearly have 0 satisfiable models.
    
    This time, we only marginalize the latent variables before the  
    the root, rendering <math> 2^index </math> models. 
    
    We pass the argument index_parent as -1, since the root does
    not have a parent.*/ 
    *count = *count * getPower(dd, node, index, -1);
    return true;
}

int
SatCount_Aux(
  DdManager *dd,
  DdNode *node,
  st_table *countable,
  int nvars,
  int index,
  bool debug
  )
{
    DdNode *N, *T, *E;
    int count, countT, countE, indexT, indexE, powT, powE;
    int *dummy;

    if (node == Cudd_ReadOne(dd))
	    return 1;
	else if (node == Cudd_ReadZero(dd))
	    return 0;

    /* Return the entry in the table if found. */
    if (st_lookup(countable, node, (void **) &dummy)) {
        count = *dummy;
	    return count;
    }

    N = Cudd_Regular(node);

    T = Cudd_T(N);
    E = Cudd_E(N);
    T = Cudd_NotCond(T, Cudd_IsComplement(node));
    E = Cudd_NotCond(E, Cudd_IsComplement(node));
    
    indexT = getIndex(dd, nvars, T);
    indexE = getIndex(dd, nvars, E);

    /* Recur on the children. */
    countT = SatCount_Aux(dd, T, countable, nvars, indexT, debug);
    if (countT == -1) return -1;
    countE = SatCount_Aux(dd, E, countable, nvars, indexE, debug);
    if (countE == -1) return -1;
    
    /* If the child is a terminal node 1, the number of variables
    between the terminal 1 and the parent node is equal to
    <code> indexChild - indexParent -1 </code>. Thus, we have to
    marginalize this calculation, to consider all possible models,
    by computing <code> 2^(nvars - indexParent - 1) </code>. 
    
    If the child is a terminal 0, countChild will be zero and the
    computation is trivial. Otherwise, the child is not a terminal
    node. Hence, the number of variables marginalized between the
    child and parent will render 2^(indexChild - indexParent -1). */
    powT = getPower(dd, T, indexT, index);
    powE = getPower(dd, E, indexE, index);

    count = countT*powT + countE*powE;

    /* store */
    dummy = ALLOC(int, 1);
    if (dummy == NULL) {
        if (debug) printf("Memory could not be allocated\n");
        return -1;
    }
    *dummy = count;
    if (debug) printf("Node = %d / dummy = %d\n", getIndex(dd, nvars, N), *dummy);
    if (st_insert(countable, node, dummy) == ST_OUT_OF_MEM) printf("st table insert failed\n");
    return count;

} /* end of SatCount_Aux */

int SatCount_Cache_Aux(DdManager *dd, DdNode *node, st_table *countable, int nvars, int index,
                       int n_obs, int obs_pos, bool inside, int obs_index[], int assignments[]);

/**
  @brief Count the number of models in a %BDD using a hash table
  initialized by SatCount.

  @sideeffect None

*/

bool
SatCount_Cache(
    DdManager *dd,
    DdNode *node,
    st_table *countable,
    int nvars,
    int n_obs,
    int *obs_index,
    int *assignments,
    int *count
    )
{   
    int index  = getIndex(dd, nvars, node);
    /* The boolean <code> inside </code> determines if the root node
    is an observation node*/
    int obs_pos;
    bool inside = isInside(dd, node, obs_index, n_obs, 0, index, &obs_pos);

    *count = SatCount_Cache_Aux(dd, node, countable, nvars, index, n_obs, obs_pos, inside, obs_index, assignments);
    
    /* If we enter this if, our hash table <code> countable </code> has
    exceded the memory limit.*/
    if (*count == -1)
        return false;
    
    /* When the root is a terminal 1, we marginalize all variables
    inside the BDD, rendering <math> 2^nvars </math> models. If it
    is a terminal 0, we clearly have 0 satisfiable models.
    
    This time, we only marginalize the latent non-observed variables
    before the the root, rendering <math> 2^{index - obs_pos} </math> 
    models. */ 
    *count = *count * getPower_Cache(dd, node, index, -1, obs_pos, 0, false);
    return true;
}

int
SatCount_Cache_Aux(
  DdManager *dd,
  DdNode *node,
  st_table *countable,
  int nvars,
  int index,
  int n_obs,
  int obs_pos,
  bool inside,
  int obs_index[],
  int assignments[]
  )
{
    DdNode *N, *T, *E;
    int count, countT, countE; 
    int indexT, indexE; 
    int obs_posT, obs_posE;
    int powT, powE;
    int *dummy;
    bool insideT, insideE;

    if (node == Cudd_ReadOne(dd))
	    return 1;
	else if (node == Cudd_ReadZero(dd))
	    return 0;

    /* We've traversed all observed variables (implicitly or explicitly),
    so we use the hashe function <code> countable </code> to lookup the
    number of satistiable models at the current node. */
    if (obs_pos > n_obs) {
        if (st_lookup(countable, node, (void **) &dummy)) {
            count = *dummy;
            return count;
        }
        else
            printf("st table lookup failed\n");
    }

    N = Cudd_Regular(node);

    /* If the current node index is not an observation variable. */
    if (!inside) {
        T = Cudd_T(N);
        E = Cudd_E(N);
        T = Cudd_NotCond(T, Cudd_IsComplement(node));
        E = Cudd_NotCond(E, Cudd_IsComplement(node));

        indexT = getIndex(dd, nvars, T);
        indexE = getIndex(dd, nvars, E);

        insideT = isInside(dd, T, obs_index, n_obs, obs_pos, indexT, &obs_posT);
        insideE = isInside(dd, E, obs_index, n_obs, obs_pos, indexE, &obs_posE);

        /* Recur on both children. */        
        countT = SatCount_Cache_Aux(dd, T, countable, nvars, indexT, n_obs, obs_posT, insideT, obs_index, assignments);
        if (countT == -1) return -1;
        countE = SatCount_Cache_Aux(dd, E, countable, nvars, indexE, n_obs, obs_posE, insideE, obs_index, assignments);
        if (countE == -1) return -1;

        /* Marginalization of all non-observed variables between the current
        node (that has index smaller than obs_index[obs_pos]) and its 
        children (that have index smaller or equal to index[obs_pos_Child]), 
        by computing the power of 2 that will multiply each count, countT 
        and countE.
        
        If the current node is terminal, it's index is considered to be
        equal to <code> nvars </code>. Otherwise, we use the current index. */
        powT = getPower_Cache(dd, T, indexT, index, obs_posT, obs_pos, inside);
        powE = getPower_Cache(dd, E, indexE, index, obs_posE, obs_pos, inside);

        count = countT*powT + countE*powE;

        return count;
    }

    /* If the current node index is a positive observation variable. */
    else if (assignments[obs_pos] == 1) {
        T = Cudd_T(N);
        T = Cudd_NotCond(T, Cudd_IsComplement(node));

        indexT = getIndex(dd, nvars, T);
        insideT = isInside(dd, T, obs_index, n_obs, obs_pos, indexT, &obs_posT);

        /* Recur on the Then child. */        
        countT = SatCount_Cache_Aux(dd, T, countable, nvars, indexT, n_obs, obs_posT, insideT, obs_index, assignments);
        if (countT == -1) return -1;
    
        /* This time, we subtract 1 from the <code> negative </code> argument,
        since <code> index = obs_index[obs_pos] </code>, because we want to 
        count the number of element inside obs_index[obs_pos+1 .. obs_posE-1]
        (the number of observations after the current node and before Then).*/
        powT = getPower_Cache(dd, T, indexT, index, obs_posT, obs_pos, inside);

        count = countT*powT;

        return count;
    }

    /* If the current node index is a negative observation variable. */
    else if (assignments[obs_pos] == 0) {
        E = Cudd_E(N);
        E = Cudd_NotCond(E, Cudd_IsComplement(node));

        indexE = getIndex(dd, nvars, E);
        insideE = isInside(dd, E, obs_index, n_obs, obs_pos, indexE, &obs_posE);

        /* Recur on the Else child. */        
        countE = SatCount_Cache_Aux(dd, E, countable, nvars, indexE, n_obs, obs_posE, insideE, obs_index, assignments);
        if (countE == -1) return -1;

        powE = getPower_Cache(dd, E, indexE, index, obs_posE, obs_pos, inside);

        count = countE*powE;

        return count;
    }

    return -1;

} /* end of SatCount_Cache_Aux */

DdNode *
buildExpression(
    DdManager *dd, 
    int nvars,
    int assigments[]) 
{
    DdNode *f, *var, *tmp;

    f = Cudd_ReadOne(dd);
    Cudd_Ref(f);

    for (int i = 0; i < nvars; i++) {
        var = Cudd_bddIthVar(dd, i);
        Cudd_Ref(var);
        if (!assigments[i]) {
            var = Cudd_Not(var);
        }
        tmp = Cudd_bddAnd(dd, var, f);
        Cudd_Ref(tmp);
        Cudd_RecursiveDeref(dd, f);
        f = tmp;
    }
    return f;
}