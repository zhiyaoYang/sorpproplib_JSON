///////////////////
// workingPair.c //
///////////////////
#include <stdio.h>
#include <stdlib.h>
#include "adsorption.h"
#include "cJSON.h"
#include "json_interface.h"
#include "refrigerant.h"
#include "workingPair.h"

#include "structDefinitions.c"


///////////////////////////
// Definition of structs //
///////////////////////////
/*
 * WorkingPair:
 * ------------
 *
 * Contains strings defining working pair and calculations approaches,
 * coefficients for calculation approaches, and structures that contain function
 * pointers for the isotherm and the refrigerant functions of the working pair.
 *
 * Attributes:
 * -----------
 * 	const char *wp_as:
 *		Name of sorbent.
 * 	const char *wp_st:
 *		Name of sub-type of sorbent.
 * 	const char *wp_rf:
 *		Name of refrigerant.
 * 	const char *wp_iso:
 *		Name of isotherm.
 * 	const char *rf_psat:
 *		Name of calculation approach for vapour pressure.
 * 	const char *rf_rhol:
 *		Name of calculation approach for liquid density.
 * 	const char *rf_ac:
 *		Name of calculation approach for activity coefficients.
 *
 * 	double *iso_par:
 *		Array containing coefficients of isotherm.
 * 	double *psat_par:
 *		Array containing coefficients of vapour pressure equation.
 * 	double *rhol_par:
 *		Array containing coefficients of saturated liquid density equation.
 * 	double *ac_par:
 *		Array containing coefficients of activity coefficent equation.
 *
 *	Adsorption *adsorption:
 *		Struct containing function pointers for isotherm functions.
 *	Refrigerant *refrigerant:
 *		Struct containing function pointers for refrigerant functions.
 *
 * History:
 * --------
 *	01/06/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */



/////////////////////////////
// Definition of functions //
/////////////////////////////
/*
 * newWorkingPair:
 * ---------------
 *
 * Initialisation function to create a new WorkingPair-struct. Therefore,
 * function checks first if enough memory available to allocate WorkingPair-
 * struct and then reads coefficients of functions from database and sets
 * pointers within the struct depending on working pair given as input.
 * If memory allocation fails, functions returns NULL and prints a warning.
 *
 * Parameters:
 * -----------
 *	const char *path_db:
 *		Path to database.
 * 	const char *wp_as:
 *		Name of sorbent.
 * 	const char *wp_st:
 *		Name of sub-type of sorbent.
 * 	const char *wp_rf:
 *		Name of refrigerant.
 * 	const char *wp_iso:
 *		Name of isotherm.
 * 	const char *rf_psat:
 *		Name of calculation approach for vapour pressure.
 * 	const char *rf_rhol:
 *		Name of calculation approach for liquid density.
 * 	const char *rf_ac:
 *		Name of calculation approach for activity coefficients.
 *
 * Returns:
 * --------
 *	struct *WorkingPair:
 *		Returns malloc-ed WorkingPair-struct containg all information requird
 *		to calculate the sorption equilibrium of the working working.
 *
 * History:
 * --------
 *	01/06/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API WorkingPair *newWorkingPair(const char *path_db, const char *wp_as, 
	const char *wp_st, const char *wp_rf, const char *wp_iso, 
	const char *rf_psat, const char *rf_rhol, const char *rf_ac) {
    // Try to allocate memory for vector structure
	//
    WorkingPair *retWorkingPair = (WorkingPair *) malloc(sizeof(WorkingPair));
	
    if (retWorkingPair == NULL) {
		// Not enough memory available for vector structure
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nI cannot allocate memory for WorkingPair-struct.");
        return NULL;
	}

    // Set general information of working pair
	//
	retWorkingPair->wp_as = wp_as;
	retWorkingPair->wp_st = wp_st;
	retWorkingPair->wp_rf = wp_rf;
	retWorkingPair->wp_iso = wp_iso;
	retWorkingPair->rf_psat = rf_psat;
	retWorkingPair->rf_rhol = rf_rhol;
	retWorkingPair->rf_ac = rf_ac;
	
	// Connect to JSON database and parse database's content to JSON-structs
	//
	char *json_content = json_read_file(path_db);	
    if (json_content == NULL) {
		// If connection to database fails, free memory
		//
		printf("\n\n###########\n# Error #\n###########");
		printf("\nCannot read JSON database! See warnings printed!");
		free(retWorkingPair);
        return NULL;
	}
	
	cJSON *json_file = json_parse_file(json_content);
    if (json_file == NULL) {
		// If content of database cannot be parsed to JSON-struct, free memory
		//
		printf("\n\n###########\n# Error #\n###########");
		printf("\nCannot work with JSON-structs! See warnings printed!");
		free(json_content);
		free(retWorkingPair);
        return NULL;
	}

	// First, read coefficients for isotherm
	//
	cJSON *json_isotherm = json_search_equation(wp_as, wp_st, wp_rf, json_file);
    if (json_isotherm == NULL) {
		// Cannot select isotherm, free memory
		//
		printf("\n\n###########\n# Error #\n###########");
		printf("\nCannot get coefficients of isotherm! See warnings!");
		free(json_file);
		free(json_content);
		free(retWorkingPair);
        return NULL;
	}
		
	retWorkingPair->iso_par = json_search_parameters(wp_iso, json_isotherm);
    if (retWorkingPair->iso_par == NULL) {
		// Cannot select coefficients of isotherm, free memory
		//
		printf("\n\n###########\n# Error #\n###########");
		printf("\nCannot save coefficients of isotherm! See warnings!");
		free(json_file);
		free(json_content);
		free(retWorkingPair);
        return NULL;
	}
	
	// Second, read coefficients for refrigerant
	// If equations are not implemented, throw warning but do not fail
	//
	cJSON *json_refrigerant = json_search_equation("dum_sorb", "dum_subtype",
		wp_rf, json_file);
    if (json_refrigerant == NULL) {
		// Cannot select refrigerant, free memory
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nCannot select refrigerant \"%s\" for calculating fluid properties.",
			wp_rf);
		retWorkingPair->psat_par = NULL;
		retWorkingPair->rhol_par = NULL;
		retWorkingPair->ac_par = NULL;
		
	} else {
		// Select coefficients for vapour pressure, saturated liquid density
		// and activity coefficients: If function and thus coefficients are not
		// implemented, pointer is set to NULL
		//
		retWorkingPair->psat_par = json_search_parameters(rf_psat,
			json_refrigerant);
		retWorkingPair->rhol_par = json_search_parameters(rf_rhol,
			json_refrigerant);
		retWorkingPair->ac_par = json_search_parameters(rf_ac,
			json_refrigerant);
	}
	
	// Free memory
	//
	free(json_file);
	free(json_content);
	
	// Check which equations of state existing for initialization of
	// "Refrigerant"-struct
	//
	const char *rf_psat_int = (retWorkingPair->psat_par == NULL) ?
		"NoVapourPressure" : rf_psat;
	const char *rf_rhol_int= (retWorkingPair->rhol_par == NULL) ?
		"NoSaturatedLiquidDensity" : rf_rhol;
	
			
	// Initialise structs containing functions
	//
	retWorkingPair->adsorption = newAdsorption(wp_iso);
    if (retWorkingPair->adsorption == NULL) {
		// Cannot create "Isotherm"-struct, free memory
		//
		printf("\n\n###########\n# Error #\n###########");
		printf("\nCannot create Adsorption-struct within WorkingPair-struct!");
		printf("\nTherefore, cannot execute isotherm functions!");
		free(retWorkingPair);
        return NULL;
	}
	
	retWorkingPair->refrigerant = newRefrigerant(rf_psat_int, rf_rhol_int);
    if (retWorkingPair->refrigerant == NULL) {
		// Cannot create "Refrigerant"-struct, free memory
		//
		printf("\n\n###########\n# Error #\n###########");
		printf("\nCannot create Refrigerant-struct within WorkingPair-struct!");
		printf("\nTherefore, cannot execute refrigerant functions!");
		free(retWorkingPair);
        return NULL;
	}
		
	// Return structure
	//
	return retWorkingPair;
}


/*
 * delWorkingPair:
 * ---------------
 *
 * Destructor function to free allocated memory of a WorkingPair-struct.
 *
 * Parameters:
 * -----------
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * History:
 * --------
 *	01/06/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API void delWorkingPair(void *workingPair) {
	// Typecast void pointer given as input to pointer of Refrigerant-struct
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
    // Because of initialization function, workingPair is NULL or fully built
	//
    if (retWorkingPair != NULL) {
		// Free allocated memory of structs containing function pointers
		//
		delRefrigerant(retWorkingPair->refrigerant);
		delAdsorption(retWorkingPair->adsorption);
		
		// Free allocated memory of coefficients of functions if memeory is
		// allocated
		//
		if (retWorkingPair->ac_par != NULL) {
			free(retWorkingPair->ac_par);
		}		
		if (retWorkingPair->rhol_par != NULL) {
			free(retWorkingPair->rhol_par);
		}		
		if (retWorkingPair->psat_par != NULL) {
			free(retWorkingPair->psat_par);
		}
		
		free(retWorkingPair->iso_par);		
		
		// Free allocated memory of WorkingPair-struct
		//
		free(retWorkingPair);
    }
}


/*
 * iso_w_pT:
 * ---------
 *
 * Calculates equilibrium loading w in kg/kg depending on equilibrium pressure
 * p in Pa and equilibrium temperature T in K.
 *
 * Parameters:
 * -----------
 * 	double p_Pa:
 *		Equilibrium pressure in Pa.
 *	double T_K:
 *		Equilibrium temperature in K.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Equilibrium loading in kg/kg.
 *
 * History:
 * --------
 *	01/06/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_w_pT(double p_Pa, double T_K, void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_w_pT\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->w_pT==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_w_pT\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->w_pT(p_Pa, T_K,
			retWorkingPair->iso_par, retWorkingPair->psat_par, 
			retWorkingPair->rhol_par, retWorkingPair->adsorption, 
			retWorkingPair->refrigerant);
		
	}
}


/*
 * iso_p_wT:
 * ---------
 *
 * Calculates equilibrium pressure p in Pa depending on equilibrium loading w
 * in kg/kg and equilibrium temperature T in K.
 *
 * Parameters:
 * -----------
 *	double w_kgkg:
 *		Equilibrium loading in kg/kg.
 *	double T_K:
 *		Equilibrium temperature in K.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 * 	double:
 *		Equilibrium pressure in Pa.
 *
 * History:
 * --------
 *	01/06/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_p_wT(double w_kgkg, double T_K, void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_p_wT\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->p_wT==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_p_wT\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->p_wT(w_kgkg, T_K,
			retWorkingPair->iso_par, retWorkingPair->psat_par,
			retWorkingPair->rhol_par, retWorkingPair->adsorption, 
			retWorkingPair->refrigerant);
		
	}
}


/*
 * iso_T_pw:
 * ---------
 *
 * Calculates equilibrium temperature in K depending on equilibrium pressure p
 * in Pa and equilibrium loading w in kg/kg.
 *
 * Parameters:
 * -----------
 *	double p_Pa:
 *		Equilibrium pressure in Pa.
 *	double w_kgkg:
 *		Equilibrium loading in kg/kg.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 * 	double:
 *		Equilibrium temperature in K.
 *
 * Remarks:
 * --------
 *	Function uses equation of states implemented for calculation of vapour
 *	pressure, saturated liquid density, derivative of vapour pressure wrt.
 *	temperature, and derivative of saturated liquid density wrt. temperature
 *	when isothem of working pair is based on volumetric approach (e.g.
 *	Dubinin theory). 
 *
 * History:
 * --------
 *	01/06/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_T_pw(double p_Pa, double w_kgkg, void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_T_pw\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->T_pw==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_T_pw\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->T_pw(p_Pa, w_kgkg,
			retWorkingPair->iso_par, retWorkingPair->psat_par,
			retWorkingPair->rhol_par, retWorkingPair->adsorption,
			retWorkingPair->refrigerant);
		
	}
}


/*
 * iso_dw_dp_pT:
 * -------------
 *
 * Calculates derivative of equilibrium loading w with respect to pressure 
 * p in kg/kg/Pa depending on equilibrium pressure p in Pa and equilibrium 
 * temperature T in K.
 *
 * Parameters:
 * -----------
 * 	double p_Pa:
 *		Equilibrium pressure in Pa.
 *	double T_K:
 *		Equilibrium temperature in K.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Derivative of equilibrium loading wrt. pressure in kg/kg/Pa.
 *
 * History:
 * --------
 *	01/06/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_dw_dp_pT(double p_Pa, double T_K, void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_dw_dp_pT\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->dw_dp_pT==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_dw_dp_pT\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->dw_dp_pT(p_Pa, T_K,
			retWorkingPair->iso_par, retWorkingPair->psat_par,
			retWorkingPair->rhol_par, retWorkingPair->adsorption,
			retWorkingPair->refrigerant);
		
	}
}


/*
 * iso_dw_dT_pT:
 * -------------
 *
 * Calculates derivative of equilibrium loading w with respect to temperature 
 * T in kg/kg/K depending on equilibrium pressure p in Pa and equilibrium 
 * temperature T in K.
 *
 * Parameters:
 * -----------
 * 	double p_Pa:
 *		Equilibrium pressure in Pa.
 *	double T_K:
 *		Equilibrium temperature in K.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Derivative of equilibrium loading wrt. temperature in kg/kg/K.
 *
 * History:
 * --------
 *	01/06/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_dw_dT_pT(double p_Pa, double T_K, void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_dw_dT_pT\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->dw_dT_pT==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_dw_dT_pT\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->dw_dT_pT(p_Pa, T_K,
			retWorkingPair->iso_par, retWorkingPair->psat_par,
			retWorkingPair->rhol_par, retWorkingPair->adsorption,
			retWorkingPair->refrigerant);
		
	}
}


/*
 * iso_dp_dw_wT:
 * -------------
 *
 * Calculates derivative of equilibrium pressure p with respect to loading 
 * w in kgPa/kg depending on equilibrium loading w in kg/kg and equilibrium 
 * temperature T in K.
 *
 * Parameters:
 * -----------
 *	double w_kgkg:
 *		Equilibrium loading in kg/kg.
 *	double T_K:
 *		Equilibrium temperature in K.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Derivative of equilibrium pressure wrt. loading in Pakg/kg.
 *
 * History:
 * --------
 *	01/13/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_dp_dw_wT(double w_kgkg, double T_K, void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_dp_dw_wT\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->dp_dw_wT==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_dp_dw_wT\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->dp_dw_wT(w_kgkg, T_K,
			retWorkingPair->iso_par, retWorkingPair->psat_par,
			retWorkingPair->rhol_par, retWorkingPair->adsorption,
			retWorkingPair->refrigerant);
		
	}
}


/*
 * iso_dp_dT_wT:
 * -------------
 *
 * Calculates derivative of equilibrium pressure p with respect to temperature 
 * T in kg/kg/K depending on equilibrium loading w in kg/kg and equilibrium 
 * temperature T in K.
 *
 * Parameters:
 * -----------
 *	double w_kgkg:
 *		Equilibrium loading in kg/kg.
 *	double T_K:
 *		Equilibrium temperature in K.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Derivative of equilibrium pressure wrt. temperature in Pa/K.
 *
 * History:
 * --------
 *	01/13/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_dp_dT_wT(double w_kgkg, double T_K, void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_dp_dT_wT\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->dp_dw_wT==NULL) {
		printf("\n\n###########\n# dp_dT_wT #\n###########");
		printf("\nChosen isotherm function \"iso_dp_dT_wT\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->dp_dT_wT(w_kgkg, T_K,
			retWorkingPair->iso_par, retWorkingPair->psat_par,
			retWorkingPair->rhol_par, retWorkingPair->adsorption,
			retWorkingPair->refrigerant);
		
	}
}


/*
 * iso_piStar_pyxgTM:
 * ------------------
 *
 * Calculates reduced spreading pressure piStar in kg/mol depending on 
 * equilibrium pressure p in Pa, molar fraction of refrigerant in vapour phase 
 * in mol/mol, molar fraction of refrigerant in adsorbed phase in mol/mol, 
 * activity coefficient of refrigerant in adsorbed phase, equilibrium 
 * temperature T in K and molar mass of refrigerant M in kg/mol. The reduced 
 * spreading pressure is defined as follows:
 *
 * 	piStar = A * pi / (R * T * m_sorbent) = 1 / M *
 *		Integral_0^p0{w(p,T) / p * dp}
 *
 *	where p0 = p_total*y / (gamma*x)
 *
 * Parameters:
 * -----------
 *	double p_total_Pa:
 *		Total pressure of vapour phase in Pa.
 *	double y_molmol:
 *		Molar fraction of refrigerant in vapour phase in mol/mol.
 *	double x_molmol:
 *		Molar fraction of refrigerant in adsorbed phase in mol/mol.
 *	double gamma:
 *		Activity coefficent of refrigerant in adsorbed phase.
 *	double T_K:
 *		Equilibrium temperature in K.
 *	double M_kgmol:
 *		Molar mass of refrigerant M in kg/mol.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Reduced spreading pressure in kg/mol.
 *
 */
DLL_API double iso_piStar_pyxgTM(double p_total_Pa, double y_molmol,
	double x_molmol, double gamma, double T_K, double M_kgmol, 
	void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_piStar_pyxgTM\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->piStar_pyxgTM==NULL) {
		printf("\n\n###########\n# dp_dT_wT #\n###########");
		printf("\nChosen isotherm function \"iso_piStar_pyxgTM\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->piStar_pyxgTM(p_total_Pa, y_molmol,
			x_molmol, gamma, T_K, M_kgmol, retWorkingPair->iso_par,
			retWorkingPair->psat_par, retWorkingPair->rhol_par,
			retWorkingPair->adsorption, retWorkingPair->refrigerant);
		
	}
}


/*
 * iso_W_ARho:
 * -----------
 *
 * Calculates equilibrium volumetric loading W in m³/kg depending on equilibrium
 * adsorption potential A in J/mol and saturated liquid density of adsorpt rho_l
 * in kg/m3.
 *
 * Parameters:
 * -----------
 * 	double A_Jmol:
 *		Equilibrium adsorption potential A in J/mol.
 * 	double rho_l_kgm3:
 *		Saturated liquid density of adsorpt rho_l in kg/m3.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Equilibrium volumetric loading W in m³/kg.
 *
 * Remarks:
 * --------
 *	Saturated liquid density of adsorpt is requird to calculate volumetric
 *	loading from characteristic curve in m3/kg when coefficients would directly
 *	return loading w in kg/kg. Therefore, function checks unit of coefficents
 *	by checking isotherm coefficients.
 *
 * History:
 * --------
 *	01/07/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_W_ARho(double A_Jmol, double rho_l_kgm3, void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_W_ARho\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->vol_W_ARho==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_W_ARho\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->vol_W_ARho(A_Jmol, rho_l_kgm3,
			retWorkingPair->iso_par);
		
	}
}


/*
 * iso_A_WRho:
 * -----------
 *
 * Calculates equilibrium adsorption potential A in J/mol depending on 
 * equilibrium volumetric loading W in m³/kg and saturated liquid density of
 * adsorpt rho_l in kg/m3.
 *
 * Parameters:
 * -----------
 * 	double W_m3kg:
 *		Equilibrium volumetric loading W in m³/kg.
 * 	double rho_l_kgm3:
 *		Saturated liquid density of adsorpt rho_l in kg/m3.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Equilibrium adsorption potential A in J/mol.
 *
 * Remarks:
 * --------
 *	Saturated liquid density of adsorpt is requird to calculate volumetric
 *	loading from characteristic curve in m3/kg when coefficients would directly
 *	return loading w in kg/kg.
 *
 * History:
 * --------
 *	01/07/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_A_WRho(double W_m3kg, double rho_l_kgm3, void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_A_WRho\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->vol_A_WRho==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_A_WRho\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->vol_A_WRho(W_m3kg, rho_l_kgm3,
			retWorkingPair->iso_par);
		
	}
}


/*
 * iso_w_pTpsatRho:
 * ----------------
 *
 * Calculates equilibrium loading w in kg/kg depending on equilibrium pressure
 * p in Pa, equilibrium temperature T in K, saturation pressure p_Sat in Pa and 
 * saturated liquid density of adsorpt rho in kg/m³.
 *
 * Parameters:
 * -----------
 * 	double p_Pa:
 *		Equilibrium pressure in Pa.
 *	double T_K:
 *		Equilibrium temperature in K.
 * 	double p_sat_Pa:
 *		Saturation pressure in Pa.
 *	double rho_l_kgm3:
 *		Saturated liquid density of adsorpt in kg/m³.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Equilibrium loading in kg/kg.
 *
 * History:
 * --------
 *	01/07/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_w_pTpsatRho(double p_Pa, double T_K, double p_sat_Pa,
	double rho_kgm3, void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_w_pTpsatRho\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->vol_w_pTpsatRho==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_w_pTpsatRho\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->vol_w_pTpsatRho(p_Pa, T_K, p_sat_Pa,
			rho_kgm3, retWorkingPair->iso_par);
		
	}
}


/*
 * iso_p_wTpsatRho:
 * ----------------
 *
 * Calculates equilibrium pressure p in Pa depending on equilibrium loading w in
 * kg/kg, equilibrium temperature T in K, saturation pressure p_Sat in Pa and 
 * density of adsorpt rho in kg/m³.
 *
 * Parameters:
 * -----------
 * 	double w_kgkg:
 *		Equilibrium loading in kg/kg.
 *	double T_K:
 *		Equilibrium temperature in K.
 * 	double p_sat_Pa:
 *		Saturation pressure in Pa.
 *	double rho_kgm3:
 *		Density of adsorpt in kg/m³.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Equilibrium pressure in Pa.
 *
 * History:
 * --------
 *	01/07/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_p_wTpsatRho(double w_kgkg, double T_K, double p_sat_Pa,
	double rho_kgm3, void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_p_wTpsatRho\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->vol_p_wTpsatRho==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_p_wTpsatRho\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->vol_p_wTpsatRho(w_kgkg, T_K,
			p_sat_Pa, rho_kgm3,retWorkingPair->iso_par);
		
	}
}


/*
 * iso_T_pwpsatRho:
 * ----------------
 *
 * Calculates equilibrium temperature T in K depending on equilibrium pressure
 * in Pa, equilibrium loading w in kg/kg, saturation pressure p_Sat in Pa and 
 * density of adsorpt rho in kg/m³.
 *
 * Parameters:
 * -----------
 *	double p_Pa:
 *		Equilibrium pressure in Pa.
 * 	double w_kgkg:
 *		Equilibrium loading in kg/kg.
 * 	double p_sat_Pa:
 *		Saturation pressure in Pa.
 *	double rho_kgm3:
 *		Density of adsorpt in kg/m³.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Equilibrium temperature in K.
 *
 * Remarks:
 * --------
 *	Uses internal euqation of states to calculate vapour pressure, saturated
 *	liquid density of adsorpt, derivative of vapour pressure wrt. temperature
 *	and derivative of saturated liquid density of adsorpt wrt. temperature. If
 *	equation of states are not implemented for refrigerant, function returns -1
 *  and throws warning. If user want to use his own equation of states, this
 *  function cannot be used and determination of root to calculate T must
 *  be implemented by user.
 *
 * History:
 * --------
 *	01/07/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_T_pwpsatRho(double p_Pa, double w_kgkg, double p_sat_Pa,
	double rho_kgm3, void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_T_pwpsatRho\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->vol_T_pwpsatRho==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_T_pwpsatRho\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {		
		// Avoid compiler warning C4100 by adding "do nothing" expression
		// "Do nothing" expression will be optimised out by compiler
		//
		(p_sat_Pa) = (p_sat_Pa);
		(rho_kgm3) = (rho_kgm3);
		
		return retWorkingPair->adsorption->vol_T_pwpsatRho(p_Pa, w_kgkg,
			retWorkingPair->refrigerant->psat_T,
			retWorkingPair->refrigerant->rho_l_T,
			retWorkingPair->refrigerant->dpsat_dT,
			retWorkingPair->refrigerant->drho_l_dT,
			retWorkingPair->iso_par, retWorkingPair->psat_par, 
			retWorkingPair->rhol_par);

	}
}
 

/*
 * iso_dW_dA_ARho:
 * ---------------
 *
 * Calculates derivative of equilibrium volumetric loading dW_dA in m³mol/kg/J
 * depending on equilibrium adsorption potential A in J/mol and saturated liquid
 * density of adsorpt rho_l in kg/m3.
 *
 * Parameters:
 * -----------
 * 	double A_Jmol:
 *		Equilibrium adsorption potential A in mol/J.
 * 	double rho_l_kgm3:
 *		Saturated liquid density of adsorpt rho_l in kg/m3.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Derivative of volumetric loading wrt. adsorption potential in
 *		m³mol/kg/J.
 *
 * Remarks:
 * --------
 *	Saturated liquid density of adsorpt is requird to calculate volumetric
 *	loading from characteristic curve in m3/kg when coefficients would directly
 *	return loading w in kg/kg.
 *
 * History:
 * --------
 *	01/07/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_dW_dA_ARho(double A_Jmol, double rho_l_kgm3, 
	void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_dW_dA_ARho\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->vol_dW_dA_ARho==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_dW_dA_ARho\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->vol_dW_dA_ARho(A_Jmol, rho_l_kgm3,
			retWorkingPair->iso_par);
		
	}
}


/*
 * iso_dA_dW_WRho:
 * ---------------
 *
 * Calculates derivative of equilibrium adsorption potential dA_dW in kgJ/mol/m³
 * depending on equilibrium volumetric loading W in m³/kg and saturated liquid
 * density of adsorpt rho_l in kg/m3.
 *
 * Parameters:
 * -----------
 * 	double W_m3kg:
 *		Equilibrium volumetric loading W in m³/kg.
 * 	double rho_l_kgm3:
 *		Saturated liquid density of adsorpt rho_l in kg/m3.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Derivative of adsorption potential wrt. volumetric loading in
 *		kgJ/mol/m³.
 *
 * Remarks:
 * --------
 *	Saturated liquid density of adsorpt is requird to calculate volumetric
 *	loading from characteristic curve in m3/kg when coefficients would directly
 *	return loading w in kg/kg.
 *
 * History:
 * --------
 *	01/07/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_dA_dW_WRho(double W_m3kg, double rho_l_kgm3, 
	void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_dA_dW_WRho\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->vol_dA_dW_WRho==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_dA_dW_WRho\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->vol_dA_dW_WRho(W_m3kg, rho_l_kgm3,
			retWorkingPair->iso_par);
		
	}
}


/*
 * iso_dw_dp_pTpsatRho:
 * --------------------
 *
 * Calculates derivative of equilibrium loading dw_dp with respect to pressure
 * in kg/kg/Pa depending on equilibrium pressure p in Pa, equilibrium 
 * temperature T in K, saturation pressure p_Sat in Pa and density of adsorpt 
 * rho in kg/m³.
 *
 * Parameters:
 * -----------
 * 	double p_Pa:
 *		Equilibrium pressure in Pa.
 *	double T_K:
 *		Equilibrium temperature in K.
 * 	double p_sat_Pa:
 *		Saturation pressure in Pa.
 *	double rho_kgm3:
 *		Density of adsorpt in kg/m³.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Derivative of equilibrium loading wrt. pressure in kg/kg/Pa.
 *
 * History:
 * --------
 *	01/07/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_dw_dp_pTpsatRho(double p_Pa, double T_K, double p_sat_Pa, 
	double rho_kgm3, void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_dw_dp_pTpsatRho\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->vol_dw_dp_pTpsatRho==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_dw_dp_pTpsatRho\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->vol_dw_dp_pTpsatRho(p_Pa, T_K,
			p_sat_Pa, rho_kgm3, retWorkingPair->iso_par);
		
	}
}


/*
 * iso_dw_dT_pTpsatRho:
 * --------------------
 *
 * Calculates derivative of equilibrium loading dw_dp with respect to
 * temperature in kg/kg/K depending on equilibrium pressure p in Pa, equilibrium 
 * temperature T in K, saturation pressure p_Sat in Pa, density of adsorpt 
 * rho in kg/m³, derivative of saturation pressure with respect to temperature
 * dp_sat_dT in Pa/K and derivative of density of adsorpt with respect to
 * temperature drho_dT in kg/m³/K.
 *
 * Parameters:
 * -----------
 * 	double p_Pa:
 *		Equilibrium pressure in Pa.
 *	double T_K:
 *		Equilibrium temperature in K.
 * 	double p_sat_Pa:
 *		Saturation pressure in Pa.
 *	double rho_kgm3:
 *		Density of adsorpt in kg/m³.
 * 	double dp_sat_dT_PaK:
 *		Derivative of saturation pressure wrt. temperature in Pa/K.
 *	double drho_dT_kgm3K:
 *		Derivative of density of adsorpt wrt. temperature in kg/m³/K.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Derivative of equilibrium loading wrt. temperature in kg/kg/K.
 *
 * History:
 * --------
 *	01/07/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_dw_dT_pTpsatRho(double p_Pa, double T_K, double p_sat_Pa, 
	double rho_kgm3, double dp_sat_dT_PaK, double drho_dT_kgm3K,
	void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_dw_dT_pTpsatRho\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->vol_dw_dT_pTpsatRho==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_dw_dT_pTpsatRho\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->vol_dw_dT_pTpsatRho(p_Pa, T_K,
			p_sat_Pa, rho_kgm3, dp_sat_dT_PaK, drho_dT_kgm3K,
			retWorkingPair->iso_par);
		
	}
}


/*
 * iso_dp_dw_wTpsatRho:
 * --------------------
 *
 * Calculates derivative of equilibrium pressure p with respect to loading 
 * w in kgPa/kg depending on equilibrium loading w in kg/kg, equilibrium 
 * temperature T in K, saturation pressure p_Sat in Pa and density of adsorpt 
 * rho in kg/m³.
 *
 * Parameters:
 * -----------
 *	double w_kgkg:
 *		Equilibrium loading in kg/kg.
 *	double T_K:
 *		Equilibrium temperature in K.
 * 	double p_sat_Pa:
 *		Saturation pressure in Pa.
 *	double rho_kgm3:
 *		Density of adsorpt in kg/m³.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Derivative of equilibrium pressure wrt. loading in Pakg/kg.
 *
 * History:
 * --------
 *	01/13/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_dp_dw_wTpsatRho(double w_kgkg, double T_K, double p_sat_Pa, 
	double rho_kgm3, void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_dp_dw_wTpsatRho\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->vol_dp_dw_wTpsatRho==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_dp_dw_wTpsatRho\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->vol_dp_dw_wTpsatRho(w_kgkg, T_K,
			p_sat_Pa, rho_kgm3, retWorkingPair->iso_par);
		
	}
}


/*
 * iso_dp_dT_wTpsatRho:
 * --------------------
 *
 * Calculates derivative of equilibrium pressure p with respect to temperature 
 * T in kg/kg/K depending on equilibrium loading w in kg/kg, equilibrium 
 * temperature T in K, saturation pressure p_Sat in Pa and density of adsorpt 
 * rho in kg/m³, derivative of saturation pressure with respect to temperature
 * dp_sat_dT in Pa/K and derivative of density of adsorpt with respect to
 * temperature drho_dT in kg/m³/K.
 *
 * Parameters:
 * -----------
 *	double w_kgkg:
 *		Equilibrium loading in kg/kg.
 *	double T_K:
 *		Equilibrium temperature in K.
 * 	double p_sat_Pa:
 *		Saturation pressure in Pa.
 *	double rho_kgm3:
 *		Density of adsorpt in kg/m³.
 * 	double dp_sat_dT_PaK:
 *		Derivative of saturation pressure wrt. temperature in Pa/K.
 *	double drho_dT_kgm3K:
 *		Derivative of density of adsorpt wrt. temperature in kg/m³/K.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Derivative of equilibrium pressure wrt. temperature in Pa/K.
 *
 * History:
 * --------
 *	01/13/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_dp_dT_wTpsatRho(double w_kgkg, double T_K, double p_sat_Pa, 
	double rho_kgm3, double dp_sat_dT_PaK, double drho_dT_kgm3K,
	void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_dp_dT_wTpsatRho\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->vol_dp_dT_wTpsatRho==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_dp_dT_wTpsatRho\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->vol_dp_dT_wTpsatRho(w_kgkg, T_K,
			p_sat_Pa, rho_kgm3, dp_sat_dT_PaK, drho_dT_kgm3K,
			retWorkingPair->iso_par);
		
	}
}


/*
 * iso_piStar_pyxgTpsatRhoM:
 * -------------------------
 *
 * Calculates reduced spreading pressure in kg/mol depending on equilibrium
 * pressure p in Pa, molar fraction of refrigerant in vapour phase in mol/mol,
 * molar fraction of refrigerant in adsorbed phase in mol/mol, activity
 * coefficient of refrigerant in adsorbed phase, equilibrium temperature T in K,
 * saturation pressure p_Sat in Pa and density of adsorpt rho in kg/m³ and molar
 * mass of refrigerant M in kg/mol. The reduced spreading pressure is defined as 
 * follows:
 *
 * 	piStar = A * pi / (R * T * m_sorbent) = 1 / M *
 *		Integral_0^p0{w(p,T) / p * dp}
 *
 *	where p0 = p_total*y / (gamma*x)
 *
 * Parameters:
 * -----------
 *	double p_total_Pa:
 *		Total pressure of vapour phase in Pa.
 *	double y_molmol:
 *		Molar fraction of refrigerant in vapour phase in mol/mol.
 *	double x_molmol:
 *		Molar fraction of refrigerant in adsorbed phase in mol/mol.
 *	double gamma:
 *		Activity coefficent of refrigerant in adsorbed phase.
 *	double T_K:
 *		Equilibrium temperature in K.
 * 	double p_sat_Pa:
 *		Saturation pressure in Pa.
 *	double rho_kgm3:
 *		Density of adsorpt in kg/m³.
 *	double M_kgmol:
 *		Molar mass of refrigerant M in kg/mol.
 * 	struct *WorkingPair:
 *		Pointer of WorkingPair-struct.
 *
 * Returns:
 * --------
 *	double:
 *		Reduced spreading pressure in kg/mol.
 *
 * History:
 * --------
 *	01/13/2020, by Mirko Engelpracht:
 *		First implementation.
 *
 */
DLL_API double iso_piStar_pyxgTpsatRhoM(double p_total_Pa, double y_molmol,
	double x_molmol, double gamma, double T_K, double p_sat_Pa, 
	double rho_kgm3, double M_kgmol, void *workingPair) {
	// Typecast void pointers given as inputs to correct structs
	//
	WorkingPair *retWorkingPair = (WorkingPair *) workingPair;
	
	if (retWorkingPair == NULL) {
		// WorkingPair-struct does not exists
		//
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nWorkingPair-struct does not exist.");
		printf("\nReturn -1 for function call \"iso_piStar_pyxgTpsatRhoM\".");
		return -1;
	}
	
	// Check if isotherm function exists for chosen isotherm type
	//
	if (retWorkingPair->adsorption->vol_piStar_pyxgTpsatRhoM==NULL) {
		printf("\n\n###########\n# Warning #\n###########");
		printf("\nChosen isotherm function \"iso_piStar_pyxgTpsatRhoM\" is not implemented.");
		printf("\nReturn -1 for function call.");
		return -1;
		
	} else {
		return retWorkingPair->adsorption->vol_piStar_pyxgTpsatRhoM(p_total_Pa, 
			y_molmol, x_molmol, gamma, T_K, p_sat_Pa, rho_kgm3, M_kgmol, 
			retWorkingPair->iso_par);
		
	}
}
	