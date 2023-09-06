#include <string.h>

int addFacts(program, obs, choice) {
    return new_program
}

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

int updateChoice (choice, choice_size) {
    int i = 0;
    while (choice[i] == true) || (i < choice_size) {
        choice_condition[i] = false;
        i++;
    }
    if i < choice_size {
        choice_condition[i] = true;
    }
}

int buildADD(program, observations_index, observations_bool, observations_size, n_obs, choice_index, total_choice_vars, choice_size) {
    DdManager *manager;
    manager = Cudd_Init(0,0,CUDD_UNIQUE_SLOTS,CUDD_CACHE_SLOTS,0);
    DdNode *add, *tmp;

    Program *new_program;

    int i, j, n_models;

    bool choice[choice_size] = { false }; /* Boolean representation of a total choice*/

    /* TODO: Registrar variáveis segunda uma ordem calculada aqui também*/

    /* Iterate over obsersations*/
    for (i = 0; i < ipow(2, choice_size); i++) {
	    /* Iterate over total choices*/
	    for (j = 0; j < n_obs; j++) {
		    new_program = addFacts(program, observations_index, observation_bool, choice_index, choice);
		    num_models = count_models(new_program);
		    tmp = buildExpression(manager, observations_index[i], observations_bool[i], observations_size,
				    total_choice_var_index, choice, choice_size, num_models);
		    add = Cudd_addOr(manager, add, tmp); /*Perform OR Boolean operation*/

        }
        updateChoice(choice, choice_bool, choice_size, total_choice_vars, choice_var_size);
    }
}


int buildExpression(manager, obs_index, obs_bool, obs_size, total_choice_var_index, choice, choice_size, num_models) {
    /* Registers all observation/total choice variables and creates an ADD of the form:
    f(o1, ~o2, o3, ..., ~c1, c2, ~c3, ...) = num_models*o1*(~o2)*o3*...*(~c1)*c2*(~c3) */

    DdNode *f, *var, *tmp;

    f = Cudd_addConst(manager, num_models);
    Cudd_Ref(f);

    int i = 0;
    while (i < obs_size) || (obs_index[i] != 0) {
        var = Cudd_addIthVar(manager, obs_index[i]);
        Cudd_Ref(var);
        if obs_bool[i] {
            var = Cudd_Not(var);
        }
        tmp = Cudd_addApply(manager,Cudd_addTimes,var,f);
        Cudd_Ref(tmp);
        Cudd_RecursiveDeref(manager,f);
        Cudd_RecursiveDeref(manager,var);
        f = tmp;
        i++;
    }

    for (i = 0; i < choice_size; i++) {
        var = Cudd_addIthVar(manager, total_choice_var_index[j]);
        Cudd_Ref(var);
        if choice_bool[i] {
            var = Cudd_Not(var);
        }
        tmp = Cudd_addApply(manager,Cudd_addTimes,var,f);
        Cudd_Ref(tmp);
        Cudd_RecursiveDeref(manager,f);
        Cudd_RecursiveDeref(manager,var);
        f = tmp;
    }
    return f
}
