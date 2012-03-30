
/**
 * Stata plugin for computing risk of data disclosure
 *
 * @author: Pascal heus (pascal.heus@gmail.com)
 *
 * adapted for R by Bernd Prantner and Alexander Kowarik
 * Developed with the financial and technical support of the
 * International Household Survey Network
 * http://www.surveynetwork.org
 *
 * Copyright 2006, 2007 Pascal Heus (pascal.heus@gmail.com),
 * Organisation For Economic Co-Operation And Development
 *
 * 	This program is free software; you can redistribute it and/or modify it under the terms of the
 * 	GNU Lesser General Public License as published by the Free Software Foundation; either version
 * 	2.1 of the License, or (at your option) any later version.
 *
 * 	This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * 	without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 	See the GNU Lesser General Public License for more details.
 *
 *  The full text of the license is available at http://www.gnu.org/copyleft/lesser.html
 */

/*
 * HOW IT WORKS:
 * This plugin takes the identification variables and optionaly the weight variable as data input.
 * The output is a level of risk stored in the specifed risk variable.
 *
 * For weighted data, the risk is computed based on the micro-argus methodology
 * For unweighted data, the risk is the inverse of the frequency
 *
 * Note that
 * 1) *** THE DATASET MUST BE SORTED BY THE KEY IDENTIFIERS PRIOR TO THE CALL ***
 * 2) The order of the variables provide as input parameters is crucial.
 *    the routine assume that the last two variables are respectively the
 *    weight and the output risk.
 * 3) All the variables must be numeric
 *
 */


/*=============*/
/* L-DIVERSITY */
/*=============*/
/**
 * Computes the l-diversity values for the current group
 */
void compute_group_ldiversity(int group_size, SVariable *pSensitive_Var, int NbVar = MAX_SENSITIVE_VAR)
{
	int i, j, k;
	SCategory *pCat;
	int count;
	double entropy_ratio;
	double entropy;

	for (i = 0; i < NbVar; i++)
	{
		SVariable &Var = pSensitive_Var[i];

		if (!Var.Require_Ldiversity)
			continue;

		count = 0;
		entropy = 0.0;
		pCat = Var.pFirstCategory;

		while (pCat != NULL)
		{
			if (pCat->group_freq > 0)
			{
				// distinct diversity count
				count++;

				// entropy l-diversity
				entropy_ratio = pCat->group_freq / (double)group_size;
				entropy += entropy_ratio * log(entropy_ratio);
			}

			pCat = pCat->pNext;
		}

		// include missing values (unless no other value exists)
		if (Var.Nb_Missing_Value_In_Group > 0 && count > 0)
		{
			count++;
			entropy_ratio = Var.Nb_Missing_Value_In_Group / (double)group_size;
			entropy += entropy_ratio * log(entropy_ratio);
		}

		// update variable
		Var.Group_Distinct_Ldiversity = count;
		Var.Group_Entropy_Ldiversity = exp(-entropy);

			//===TOO: recursive l-diversity
			//=== Put all unused Categories at the end of the list
		int NbUsedCategory = 0;
		//SCategory *pLastCategoryBak = Var.pLastCategory,
		//			*pPrevCategory = NULL;

		pCat = Var.pFirstCategory;

		while (pCat)
		{
			if (pCat->group_freq)
				++NbUsedCategory;

			pCat = pCat->pNext;
		}

			//=== Bubble sort the used categories by the frequency
		if (!NbUsedCategory)
			continue;

		SCategory **ppUsedCat = (SCategory **) malloc(sizeof(SCategory *) * NbUsedCategory);
		pCat = Var.pFirstCategory;

		for (j = 0; pCat; pCat = pCat->pNext)
		{
			if (pCat->group_freq)
				ppUsedCat[j++] = pCat;
		}

		for (j = 0; j < NbUsedCategory; ++j)
		{
			for (k = j + 1; k < NbUsedCategory; ++k)
			{
				if (ppUsedCat[j]->group_freq < ppUsedCat[k]->group_freq)
					Swap(ppUsedCat[k], ppUsedCat[j]);
			}
		}

			//=== recursive l-diversity
		ASSERT(NbUsedCategory >= 1);

		if (NbUsedCategory > 1)
		{
			float c = g_Config.Ldiversity_Recursivity_Constant;
			float Sum = 0.0f;

			for (j = 1; j < NbUsedCategory; ++j)
				Sum += ppUsedCat[j]->group_freq;

			Sum *= c;

			for (j = 1; j < NbUsedCategory && Sum > ppUsedCat[0]->group_freq; ++j)
				Sum -= c * ppUsedCat[j]->group_freq;

			Var.Group_Recursive_Ldiversity = j;
		}
		else
			Var.Group_Recursive_Ldiversity = 1;

		free(ppUsedCat);

		/*OS_Printf("; Var %d (%d): Ldiv-D: %d; Ldiv-E: %g; Ldiv-R: %d\n",
								i, NbUsedCategory,
								Var.Group_Distinct_Ldiversity,
								Var.Group_Entropy_Ldiversity,
								Var.Group_Recursive_Ldiversity);*/
	}
}

/**
 * Updates data in stata for l-diversity related output variables
 */
//void stata_update_ldiversity(long obs)
//{
//	int i;
//	Rcpp::NumericMatrix *Mat_Risk(Mat.rows(), Mat.cols()*3+2);
//
//	for (i = 0; i < MAX_SENSITIVE_VAR; i++)
//	{
//		SVariable &Var = g_Config.Sensitive_Var[i];
//
//		if (Var.Require_Ldiversity)
//		{
//			Mat(obs, i) = Var.Group_Distinct_Ldiversity;
//			Mat(obs, i+1) = Var.Group_Entropy_Ldiversity;
//			Mat(obs, i+2) = Var.Group_Recursive_Ldiversity;
//			// g_pDataset->SetValue(Var.Ldiversity_Distinct_Pos, obs, Var.Group_Distinct_Ldiversity);
//			// g_pDataset->SetValue(Var.Ldiversity_Entropy_Pos, obs, Var.Group_Entropy_Ldiversity);
//			// g_pDataset->SetValue(Var.Ldiversity_Recursive_Pos, obs, Var.Group_Recursive_Ldiversity);
//		}
//	}
//
//	if (g_Config.Nb_Sensitive_Var >= 2)
//	{
//		Mat(obs, Mat.cols()) = g_Config.Group_MultiEntropy_Ldiversity;
//		Mat(obs, Mat.cols()+1) = g_Config.Group_MultiRecursive_Ldiversity;
//		// g_pDataset->SetValue(g_Config.Ldiversity_MultiEntropy_Pos, obs, g_Config.Group_MultiEntropy_Ldiversity);
//		// g_pDataset->SetValue(g_Config.Ldiversity_MultiRecursive_Pos, obs, g_Config.Group_MultiRecursive_Ldiversity);
//	}
//}

/*===================*/
/* Multi L-DIVERSITY */
/*===================*/
/**
 * Computes the multi l-diversity values for the current group
 */
void Compute_Multi_LDiversity(int Obs, int GroupSize)
{
	int i, j, k, l;
	const int NbVar = g_Config.Nb_Sensitive_Var;
	double *pSet = new double[GroupSize * NbVar];
	int *pSetIndex = new int[GroupSize];
	BOOL First = TRUE;
	Rcpp::NumericMatrix Mat;

	//=== Load the SubSet
	ForLoop (i, GroupSize)
	{
		ForLoop (j, NbVar)
			pSet[i * NbVar + j] = Mat(Obs + i, g_Config.Sensitive_Var[j].position);
			//g_pDataset->GetValue(g_Config.Sensitive_Var[j].position, Obs + i, pSet + i * NbVar + j);

		pSetIndex[i] = i;
	}

	ForLoop (i, NbVar)
	{
		SVariable &VarI = g_Config.Sensitive_Var[i];

		if (!VarI.Require_Ldiversity)
			continue;

			//=== Bubble sort by all Vars except VarI
		ForLoop (j, GroupSize - 1)
		{
			for (k = j + 1; k < GroupSize; ++k)
			{
				int &IndexJ = pSetIndex[j],
					&IndexK = pSetIndex[k];

				BOOL DoSwap = FALSE;

				ForLoop (l, NbVar)
				{
					if (l == i)		// ignore current VarI
						continue;

					if (pSet[IndexK * NbVar + l] == pSet[IndexJ * NbVar + l])
						continue;

					if (pSet[IndexK * NbVar + l] > pSet[IndexJ * NbVar + l])
						DoSwap = TRUE;

					break;
				}

				if (DoSwap)
					Swap(IndexJ, IndexK);
			}
		}

			//=== Count How many SubGroups
		int NbSubGroup = 1;
		int Index1 = pSetIndex[0];

		for (j = 1; j < GroupSize; ++j)
		{
			int Index2 = pSetIndex[j];

			ForLoop (l, NbVar)
			{
				if (l == i)		// ignore current VarI
					continue;

				if (pSet[Index2 * NbVar + l] != pSet[Index1 * NbVar + l])
					break;
			}

			if (l < NbVar)		// Different ?
			{
				Index1 = Index2;
				++NbSubGroup;
			}
		}

//		printf("==> %d SubGroup\n", NbSubGroup);

			//=== Compute Entropy & Recursive for each SubGroup
		SVariable Var;

		init_var(&Var);
		reset_var_cat_group_freq(&Var);

		Var.Require_Ldiversity = VarI.Require_Ldiversity;

		float *pSubEntropy = new float[NbSubGroup];
		int *pSubRecursive = new int[NbSubGroup];

		int i1 = 0;
//		int NbSubGroupBak = NbSubGroup;
		NbSubGroup = 0;

		for (j = 1; j < GroupSize + 1; ++j)
		{
			int Index2 = pSetIndex[j];
			Index1 = pSetIndex[i1];

			/*OS_Printf("Var %d-%02d:", i, j);
			ForLoop (l, NbVar)
			{
				if (l != i)		// ignore current VarI
					OS_Printf(" %g", pSet[pSetIndex[j-1] * NbVar + l]);
			}
			OS_Printf("; Sensitive => %g\n", pSet[pSetIndex[j-1] * NbVar + i]);*/

			if (j < GroupSize)
			{
				ForLoop (l, NbVar)
				{
					if (l == i)		// ignore current VarI
						continue;

					if (pSet[Index2 * NbVar + l] != pSet[Index1 * NbVar + l])
						break;
				}
			}
			else
				l = 0;

			add_var_cat_value(&Var, pSet[pSetIndex[j-1] * NbVar + i]);

			if (l < NbVar)		// End of Group ?
			{
				int SubGroupSize = j - i1;
				compute_group_ldiversity(SubGroupSize, &Var, 1);

				pSubEntropy[NbSubGroup] = (float) Var.Group_Entropy_Ldiversity;
				pSubRecursive[NbSubGroup] = Var.Group_Recursive_Ldiversity;

				i1 = j;
				reset_var_cat_group_freq(&Var);
				++NbSubGroup;
				//OS_Printf("Done SubGroup %d : %g Entropy / %d Recurs\n", NbSubGroup, Var.Group_Entropy_Ldiversity,
				//																		Var.Group_Recursive_Ldiversity);
			}
		}

//		ASSERT(NbSubGroupBak == NbSubGroup);

		float MultiEntropy = pSubEntropy[0];
		int MultiRecursive = pSubRecursive[0];

		for (j = 1; j < NbSubGroup; ++j)
		{
			MultiEntropy = Min(pSubEntropy[j], MultiEntropy);
			MultiRecursive = Min(pSubRecursive[j], MultiRecursive);
		}

		//OS_Printf("===> Var%d : %g Entropy / %d Recurs\n", i, MultiEntropy, MultiRecursive);
		//OS_Printf("...\n");

		if (First)
		{
			g_Config.Group_MultiEntropy_Ldiversity = MultiEntropy;
			g_Config.Group_MultiRecursive_Ldiversity = MultiRecursive;
			First = FALSE;
		}
		else
		{
			g_Config.Group_MultiEntropy_Ldiversity = Min(MultiEntropy, g_Config.Group_MultiEntropy_Ldiversity);
			g_Config.Group_MultiRecursive_Ldiversity = Min(MultiRecursive, g_Config.Group_MultiRecursive_Ldiversity);
		}

		delete[] pSubEntropy;
		delete[] pSubRecursive;

		free_var(&Var);
	}

	delete[] pSet;
	delete[] pSetIndex;
}

/*======*/
/* RISK */
/*======*/
/**
 * Computes the risk based on frequency and weight
 */
double compute_risk(int freq, double weight)
{
	//int i;
	int tmp;
	double risk;
	double pk = freq / weight;
	double qk = 1 - pk;

	// compute risk (see micro-Argus 4.1 manual, p21+)
	switch (freq)
	{
	case 1:
		risk = -log(pk);
		risk *= (pk / qk);
		break;
	case 2:
		risk = (pk * log(pk)) + qk;
		risk *= (pk) / (qk * qk);
		break;
	case 3:
		risk = qk * ((3 * qk) - 2);
		risk -= 2 * pk * pk * log(pk);
		risk *= (pk / (2 * pow(qk, 3)));
		break;
	default:
		risk = 1;
		tmp = freq + 1;
		risk += qk / tmp;
		tmp *= freq + 2;
		risk += (2 * pow(qk, 2)) / tmp;
		tmp *= freq + 3;
		risk += (6 * pow(qk, 3)) / tmp;
		tmp *= freq + 4;
		risk += (24 * pow(qk, 4)) / tmp;
		tmp *= freq + 5;
		risk += (120 * pow(qk, 5)) / tmp;
		tmp *= freq + 6;
		risk += (720 * pow(qk, 6)) / tmp;
		tmp *= freq + 7;
		risk += (5040 * pow(qk, 7)) / tmp;
		risk *= pk / freq;
		break;
	}
	return risk;

}

/*===========*/
/* UTILITIES */
/*===========*/

/**
 * Displays a key (for debug purposes)
 */
//void display_key(double key[], int key_size)
//{
////	printf("KEY:");
//
//	int i;
//
//	for (i = 0; i < key_size; i++)
//	{
//		if (!SF_IsMissing(key[i]))
//			sprintf(g_TxtBuffer, " %.2f", key[i]);
//		else
//			sprintf(g_TxtBuffer, " .");
////		printf(g_TxtBuffer);
//	}
//
////	printf("\n");
//}

/**
 * Displays g_Config (for debug purposes)
 */
//void display_config(void)
//{
//	int i;
//
////	printf("CONFIG\n");
//	sprintf(g_TxtBuffer, "Weighted:%d #key_vars:%d #sens_vars:%d weight_var:%d group_var:%d risk_var:%d \n",
//			  g_Config.is_weighted, g_Config.Nb_QuasiId_Var, g_Config.Nb_Sensitive_Var, g_Config.weight_var_pos,
//			  g_Config.group_var_pos, g_Config.risk_var_pos);
////	printf(g_TxtBuffer);
//
//	for (i = 0; i < MAX_SENSITIVE_VAR; i++)
//	{
//		sprintf(g_TxtBuffer,
//				  "l-diversity%d: position:%d dist_pos:%d entr_pos:%d level:%d entropy:%d recursive:%d\n",
//				  i, g_Config.Sensitive_Var[i].position, g_Config.Sensitive_Var[i].Ldiversity_Distinct_Pos,
//				  g_Config.Sensitive_Var[i].Ldiversity_Entropy_Pos, g_Config.Sensitive_Var[i].Require_Ldiversity,
//				  g_Config.Sensitive_Var[i].ldiv_do_entropy, g_Config.Sensitive_Var[i].ldiv_do_recursivity);
////		printf(g_TxtBuffer);
//	}
//
////	printf("\n");
//}

/*======*/
/* MAIN */
/*======*/
// int argc, char *argv[]
RcppExport SEXP measure_risk(SEXP data, SEXP weighted_R, SEXP n_key_vars_R, SEXP l_recurs_c_R, SEXP ldiv_index_R, SEXP missing_value_R)
{
	int i;//, j, k;
	//	time_t start;
	//	time_t end;

	//	time(&start);

	/*========*/
	/* CONFIG */
	/*========*/

	Rcpp::NumericMatrix Mat(data);
	Rcpp::NumericMatrix Mat_Risk(Mat.rows(), Mat.cols()*3+2);
	// Result matrix for group_count and group_risk;
	Rcpp::NumericMatrix Res(Mat.rows(), 3);
	int NbRow = Mat.rows();
	int NbCol = Mat.cols();
	//GetDataSet();
	//int NbRow = g_pDataset->GetNbRow();
	//g_Config.is_weighted = 0;
	//g_Config.Nb_QuasiId_Var = 0;
	g_Config.Nb_QuasiId_Var = Rcpp::as<int>(n_key_vars_R);
	g_Config.Nb_Sensitive_Var = 0;
	g_Config.weight_var_pos = 0;
	g_Config.risk_var_pos = 0;
	g_Config.missing_value = Rcpp::as<double>(missing_value_R);

	for (i = 0; i < MAX_SENSITIVE_VAR; i++)
		init_var(&g_Config.Sensitive_Var[i]);

	// g_Config.Ldiversity_Recursivity_Constant = 1.0f;

	bool weighted = Rcpp::as<bool>(weighted_R);
	g_Config.is_weighted = (weighted) ? 1 : 0;

	g_Config.Ldiversity_Recursivity_Constant = Rcpp::as<float>(l_recurs_c_R);

	// FIXME: input of settings

	// for (i = 0; i < argc; i++)
	// {
	// if (!strncmp(argv[i], "weighted", 8))
	// g_Config.is_weighted = 1;

	// if (!strncmp(argv[i], "n_key_vars(", 11))
	// {
	// argv[i][strlen(argv[i]) - 1] = '\0';	// get rid of closing parenthesis
	// g_Config.Nb_QuasiId_Var = atoi(argv[i] + 11);
	// }

	// if (!strncmp(argv[i], "l_recurs_c(", 11))
	// {
	// g_Config.Ldiversity_Recursivity_Constant = (float) strtod(argv[i]+11, NULL);

	if (!g_Config.Ldiversity_Recursivity_Constant)
	{
		//				printf("Warning: Wrong value for l-recursity Contanst; Reset to 1\n");
		g_Config.Ldiversity_Recursivity_Constant = 1.0f;
	}
	// }
	// }

	// look for l-diversity arguments ldiv?
	Rcpp::NumericVector ldiv_index_RR(ldiv_index_R);
	int length_ldiv_index=ldiv_index_RR.length();
	if(ldiv_index_RR(0)>0){
		ForLoopD(ll, length_ldiv_index){
			int index_run = ldiv_index_RR(ll)-1;
			//      printf("SENSIVITY VAR %d \n",index_run);
			g_Config.Sensitive_Var[index_run].Require_Ldiversity = TRUE;
		}
	}
	//	int ldiv_index = Rcpp::as<int>(ldiv_index_R);
	//	int ldiv_lvl = Rcpp:: as<int>(ldiv_lvl_R);

	// for (i = 0; i < argc; i++)
	// {
	// if (strncmp(argv[i], "ldiv", 4) == 0)
	// {
	// // 0 based index in C
	// int ldiv_index = atoi(argv[i] + 4) - 1;

	//			if (ldiv_index >= 0)
	//			{
	//				// g_Config.Sensitive_Var[ldiv_index].Require_Ldiversity = TRUE;
	//				g_Config.Sensitive_Var[ldiv_index].Require_Ldiversity = ldiv_lvl;
	////				Printf("Need Ldiversity for Var %d\n", ldiv_index + 1);
	//			}

	// ldiv?_var
	//			if (strncmp(argv[i] + 6, "lvl(", 4) == 0)
	//				g_Config.Sensitive_Var[ldiv_index].Require_Ldiversity = atoi(argv[i] + 10);

	// ldiv?_lvl
	// ldiv?_ent (entropy)
	// ldiv?_recur (recursivity)
	// }
	// }

	//TODO: return an error if Nb_QuasiId_Var(n) is not specified and there are ldiv vars..?

	// count number of l-diversity variables
	// and initialize ldiv variable position
	for (i = 0; i < MAX_SENSITIVE_VAR; i++)
	{
		if (g_Config.Sensitive_Var[i].Require_Ldiversity)
			++g_Config.Nb_Sensitive_Var;
	}
	// setup g_Config
	if (g_Config.is_weighted)
	{
		// weighted
		//g_Config.Nb_QuasiId_Var = g_pDataset->GetNbVar() - 3;			
		//g_Config.Nb_QuasiId_Var = NbCol - 3; // all variables except weight, group and risk
		g_Config.Nb_QuasiId_Var = NbCol-1; // all variables except weight, group and risk
		g_Config.weight_var_pos = g_Config.Nb_QuasiId_Var + 0;
		g_Config.group_var_pos = g_Config.Nb_QuasiId_Var + 1;
		g_Config.risk_var_pos = g_Config.Nb_QuasiId_Var + 2;

		// if (g_pDataset->GetNbVar() < 4)
		if (NbCol < 2)
		{
			//			printf("Error: you should have at least 4 variables in the DataSet ( => 1 QuasiID + Weight + Group + Risk)\n");
			return Rcpp::wrap(-1);
		}
	}
	else
	{
		// unweighted
		g_Config.weight_var_pos = 0;
		g_Config.group_var_pos = g_Config.Nb_QuasiId_Var + g_Config.Nb_Sensitive_Var + 0;
		g_Config.risk_var_pos = g_Config.Nb_QuasiId_Var + g_Config.Nb_Sensitive_Var + 1;

		// init ldiv variable positions
		int var_pos = g_Config.Nb_QuasiId_Var + 0;	// the first ldiv input variable follows the identifiers
		int ldiv_pos = g_Config.risk_var_pos + 1;		// the first ldiv output variable follows the risk variable

		for (i = 0; i < MAX_SENSITIVE_VAR; i++)
		{
			if (g_Config.Sensitive_Var[i].Require_Ldiversity)
			{
				g_Config.Sensitive_Var[i].position = var_pos++;
				g_Config.Sensitive_Var[i].Ldiversity_Distinct_Pos = ldiv_pos++;
				g_Config.Sensitive_Var[i].Ldiversity_Entropy_Pos = ldiv_pos++;
				g_Config.Sensitive_Var[i].Ldiversity_Recursive_Pos = ldiv_pos++;
			}
		}

		if (g_Config.Nb_Sensitive_Var >= 2)
		{
			g_Config.Ldiversity_MultiEntropy_Pos = ldiv_pos++;
			g_Config.Ldiversity_MultiRecursive_Pos = ldiv_pos++;
		}

		// if (ldiv_pos > g_pDataset->GetNbVar())
		//		if (ldiv_pos-7> NbCol)
		//		{
		//			printf("Error: you should have at least %d variables in the DataSet (only %d provided ; need Distinct + Entropy + Recursive output Variables for each Variable requiring Ldiversity)\n", ldiv_pos, NbCol);
		//			return Rcpp::wrap(-2);
		//		}
	}
	//display_config();
	/*=========*/
	/* PROCESS */
	/*=========*/
	//if (g_pDataset->GetNbVar() < 2)
	if (NbCol < 2)
	{
		sprintf(g_TxtBuffer, "Invalid number of parameters %d\n", NbCol);
		//		printf(g_TxtBuffer);
	}
	else
	{
		//double group_key[g_Config.Nb_QuasiId_Var];	//< the current group identification key
		//double obs_key[g_Config.Nb_QuasiId_Var];		//< the current observation identification key

		double *group_key = new double[g_Config.Nb_QuasiId_Var];	//< the current group identification key
		double *obs_key = new double[g_Config.Nb_QuasiId_Var];

		long current_obs;
		long obs_count = 0;					// total number of observations
		double obs_weight;				//< weight of the current observation
		double obs_value;					//< generic value
		long group_count = 0;				//< total number of groups
		long group_missing;					//< used to detect missing values in group key
		int group_size;
		double group_weight;				//< the sum of the weights for this group
		double group_risk;

		double global_risk_ER = 0.0;	//< The expected number of re-identification
		double global_risk = 0.0;		//< The re-identification rate or global risk
		int i;//, j;
		// get first observation
		current_obs = 0;//SF_GetRowStart();
		do
		{

			// Init group
			group_count++;

			group_size = 0;
			group_weight = 0.0;
			group_missing = 0;

			// reset ldiv counts
			if (!g_Config.is_weighted && g_Config.Nb_Sensitive_Var > 0)
			{
				for (i = 0; i < MAX_SENSITIVE_VAR; i++)
				{
					if (g_Config.Sensitive_Var[i].Require_Ldiversity)
						reset_var_cat_group_freq(&g_Config.Sensitive_Var[i]);
				}
			}

			// read the group key
			for (i = 0; i < g_Config.Nb_QuasiId_Var; i++)
			{
				group_key[i] = Mat(current_obs, i);
				//g_pDataset->GetValue(i, current_obs, &group_key[i]);
				if (SF_IsMissing(group_key[i]))
					group_missing++;
			}
			if (group_missing == g_Config.Nb_QuasiId_Var)
			{
				//
				// CASE 1: ALL MISSING VALUES IN KEY (risk is zero)
				//
				// all the key variables are missing, the risk is zero
				sprintf(g_TxtBuffer, "WARNING: all variables are MISSING for group starting at %ld\n",
						current_obs);
				//				printf(g_TxtBuffer);
				group_risk = 0;

				// UPDATE STATA
				do
				{					
					Res(current_obs, 0) = group_count;
					Res(current_obs, 1) = group_risk;
					Res(current_obs, 2) = group_size;
					// g_pDataset->SetValue(g_Config.group_var_pos, current_obs, group_count);
					// g_pDataset->SetValue(g_Config.risk_var_pos, current_obs, group_risk);

					// l-diversity --> do not compute
					obs_count++;
					current_obs++;

					if (current_obs >= NbRow)
						break;

					// read next obs key
					for (i = 0; i < g_Config.Nb_QuasiId_Var; i++)
						obs_key[i] = Mat(current_obs, i);
					//g_pDataset->GetValue(i, current_obs, &obs_key[i]);
				}
				while (is_same_key_Risk(group_key, obs_key, g_Config.Nb_QuasiId_Var));
			}
			else if (group_missing > 0)
			{
				//
				// CASE 2: SOME MISSING VALUES IN KEY
				//
				sprintf(g_TxtBuffer, "WARNING: %ld variable(s) MISSING for Observation %ld", group_missing, current_obs);
				//				printf(g_TxtBuffer);
				//				display_key(group_key, g_Config.Nb_QuasiId_Var);

				// The weights and count of the observations with the same
				// non-missing key values need to be used for this group
				// This requires a full scan of the dataset
				int i, j;						// local definitions to prevent conflicts
				double value;

				//				for (i = SF_GetRowStart(); i <= SF_GetRowEnd(); i++)
				ForLoop (i, NbRow)
				{
					// if (g_pDataset->IsRowSelected(i)) - Always TRUE
					// {
					// compare partial keys
					for (j = 0; j < g_Config.Nb_QuasiId_Var; j++)
					{
						// if this variable is a missing component of the current key, ignore it
						if (SF_IsMissing(group_key[j]))
							continue;

						// read this variable value
						value = Mat(i, j);
						// g_pDataset->GetValue(j, i, &value);

						// if not equal to the current key, this is not a match
						if (value != group_key[j])
							break;
					}

					if (j == g_Config.Nb_QuasiId_Var)
					{
						// we didn't hit the break, this is a match
						group_size++;
						if (g_Config.is_weighted)
						{
							value = Mat(i, g_Config.weight_var_pos);
							//g_pDataset->GetValue(g_Config.weight_var_pos, i, &value);
							group_weight += value;
						}

						// if unweighted data and sensitive variables exist
						if (!g_Config.is_weighted && g_Config.Nb_QuasiId_Var > 0)
						{
							//add l-diversity category frequency
							for (i = 0; i < MAX_SENSITIVE_VAR; i++)
							{
								// for active variable only
								if (g_Config.Sensitive_Var[i].Require_Ldiversity)
								{
									// read value for this variable and add to category count
									obs_value = Mat(current_obs, g_Config.Sensitive_Var[i].position);
									//g_pDataset->GetValue(g_Config.Sensitive_Var[i].position, current_obs, &obs_value);
									add_var_cat_value(&g_Config.Sensitive_Var[i], obs_value);
								}
							}
						}
					}
					// }
				}

				// compute risk
				if (g_Config.is_weighted)
				{
					group_risk = compute_risk(group_size, group_weight);
				}
				else
				{
					group_risk = 1 / group_size;

					// compute group l-diversity
					if (g_Config.Nb_Sensitive_Var > 0)
					{
												compute_group_ldiversity(group_size, g_Config.Sensitive_Var);

					}
				}

				sprintf(g_TxtBuffer, "         --> frequency: %d weight: %.2f risk: %.7f\n", group_size,
						group_weight, group_risk);
				//				printf(g_TxtBuffer);

				// UPDATE STATA
				do
				{
					// add this observation contribution to the global risk
					global_risk_ER += group_size * group_risk;

					Res(current_obs, 0) = group_count;
					Res(current_obs, 1) = group_risk;
					Res(current_obs, 2) = group_size;
					// g_pDataset->SetValue(g_Config.group_var_pos, current_obs, group_count);
					// g_pDataset->SetValue(g_Config.risk_var_pos, current_obs, group_risk);

					// if unweighted data and sensitive variables exist
					if (!g_Config.is_weighted && g_Config.Nb_Sensitive_Var > 0)
					{
						int ii;
						for (ii = 0; ii < MAX_SENSITIVE_VAR; ii++)
						{
							if (g_Config.Sensitive_Var[ii].Require_Ldiversity)
							{
								Mat_Risk(i, (ii*3)) = g_Config.Sensitive_Var[ii].Group_Distinct_Ldiversity;
								Mat_Risk(i, (ii*3)+1) = g_Config.Sensitive_Var[ii].Group_Entropy_Ldiversity;
								Mat_Risk(i, (ii*3)+2) = g_Config.Sensitive_Var[ii].Group_Recursive_Ldiversity;
								// g_pDataset->SetValue(Var.Ldiversity_Distinct_Pos, obs, Var.Group_Distinct_Ldiversity);
								// g_pDataset->SetValue(Var.Ldiversity_Entropy_Pos, obs, Var.Group_Entropy_Ldiversity);
								// g_pDataset->SetValue(Var.Ldiversity_Recursive_Pos, obs, Var.Group_Recursive_Ldiversity);
							}
						}

						if (g_Config.Nb_Sensitive_Var >= 2)
						{
							Mat_Risk(i, Mat_Risk.cols()-2) = g_Config.Group_MultiEntropy_Ldiversity;
							Mat_Risk(i, Mat_Risk.cols()-1) = g_Config.Group_MultiRecursive_Ldiversity;
							// g_pDataset->SetValue(g_Config.Ldiversity_MultiEntropy_Pos, obs, g_Config.Group_MultiEntropy_Ldiversity);
							// g_pDataset->SetValue(g_Config.Ldiversity_MultiRecursive_Pos, obs, g_Config.Group_MultiRecursive_Ldiversity);
						}
						//						stata_update_ldiversity(i);
					}

					obs_count++;
					current_obs++;

					if (current_obs >= NbRow)
						break;

					// read next obs key
					for (i = 0; i < g_Config.Nb_QuasiId_Var; i++)
						obs_key[i] = Mat(current_obs, i);
					//g_pDataset->GetValue(i, current_obs, &obs_key[i]);

				}
				while (is_same_key_Risk(group_key, obs_key, g_Config.Nb_QuasiId_Var));
			}
			else
			{
				//
				// CASE 3: FULL KEY (NORMAL CASE)
				//
				// read all observations for this group
				do
				{
					obs_count++;
					if (g_Config.is_weighted)
					{
						// add to group weight
						obs_weight = Mat(current_obs, g_Config.weight_var_pos);
						//g_pDataset->GetValue(g_Config.weight_var_pos, current_obs, &obs_weight);
						group_weight += obs_weight;
					}
					else
					{
						if (g_Config.Nb_Sensitive_Var > 0)
						{
							// add l-diversity category frequency
							for (i = 0; i < MAX_SENSITIVE_VAR; i++)
							{
								if (g_Config.Sensitive_Var[i].Require_Ldiversity)
								{
									// read value for this variable and add to category count
									obs_value = Mat(current_obs, i);//g_Config.Sensitive_Var[i].position);
									//g_pDataset->GetValue(g_Config.Sensitive_Var[i].position, current_obs, &obs_value);
									add_var_cat_value(&g_Config.Sensitive_Var[i], obs_value);
								}
							}
						}
					}

					// add to group frequency
					group_size++;

					// next
					current_obs++;

					if (current_obs >= NbRow)
						break;

					// read next obs key
					for (i = 0; i < g_Config.Nb_QuasiId_Var; i++)
						obs_key[i] = Mat(current_obs, i);
					//g_pDataset->GetValue(i, current_obs, &obs_key[i]);

				}
				while (is_same_key_Risk(group_key, obs_key, g_Config.Nb_QuasiId_Var));
				// compute risk for this group
				if (g_Config.is_weighted)
				{
					group_risk = compute_risk(group_size, group_weight);
				}
				else
				{
					group_risk = 1.0 / group_size;

					// compute group l-diversity
					if (g_Config.Nb_Sensitive_Var > 0)
					{
												compute_group_ldiversity(group_size, g_Config.Sensitive_Var);


						if (g_Config.Nb_Sensitive_Var >= 2){
														Compute_Multi_LDiversity(current_obs - group_size, group_size);

						}
					}
				}
				// UPDATE STATA
				for (i = current_obs - group_size; i < current_obs; i++)
				{
					// Write value back to stata file for all observations in the group					
					Res(i, 0) = group_count;
					Res(i, 1) = group_risk;
					Res(i, 2) = group_size;

					// g_pDataset->SetValue(g_Config.group_var_pos, i, group_count);
					// g_pDataset->SetValue(g_Config.risk_var_pos, i, group_risk);

					// l-diversity
					if (!g_Config.is_weighted && g_Config.Nb_Sensitive_Var > 0){

						for (int ii = 0; ii < MAX_SENSITIVE_VAR; ii++)
						{
							if (g_Config.Sensitive_Var[ii].Require_Ldiversity)
							{
								Mat_Risk(i, (ii*3)) = g_Config.Sensitive_Var[ii].Group_Distinct_Ldiversity;
								Mat_Risk(i, (ii*3)+1) = g_Config.Sensitive_Var[ii].Group_Entropy_Ldiversity;
								Mat_Risk(i, (ii*3)+2) = g_Config.Sensitive_Var[ii].Group_Recursive_Ldiversity;
							}
						}
					}

					if (g_Config.Nb_Sensitive_Var >= 2)
					{
						Mat_Risk(i, Mat_Risk.cols()-2) = g_Config.Group_MultiEntropy_Ldiversity;
						Mat_Risk(i, Mat_Risk.cols()-1) = g_Config.Group_MultiRecursive_Ldiversity;
					}
					//						stata_update_ldiversity(i);
				}

				// add this group contribution to the global risk
				global_risk_ER += group_size * group_risk;
			}
			//=== debug display
			/*if (group_count <= 20)
			{
			   OS_Printf("Group %d; KEY: ", i);

				for(i = 0; i < g_Config.Nb_QuasiId_Var; ++i)
					OS_Printf(" %.1f", group_key[i]);

				OS_Printf("; FREQ:%d WGT:%.2f RISK:%.6f\n", group_size, group_weight, group_risk);
			} */
		}
		while (current_obs < NbRow);
		// compute global risk and store in stata scalars
		global_risk = global_risk_ER / obs_count;
		// SF_ScalarSave("global_risk_ER", global_risk_ER);
		// SF_ScalarSave("global_risk", global_risk);
		// SF_ScalarSave("global_risk_pct", global_risk * 100);
		//Rcpp::NumericVector global_risk_ER_rcpp(global_risk_ER);
		//Rcpp::NumericVector global_risk_rcpp(global_risk);
		//Rcpp::NumericVector global_risk_pct_rcpp(global_risk * 100);
		// clean memory
		for (i = 0; i < MAX_SENSITIVE_VAR; i++)
		{
			if (g_Config.Sensitive_Var[i].Require_Ldiversity)
				free_var(&g_Config.Sensitive_Var[i]);
		}
		// display statistics
		//		time(&end);
		//		printf("Processed %ld groups / %ld observations in %.1f second(s)\n",
		//											group_count, obs_count, difftime(end, start));

		delete[] group_key;
		delete[] obs_key;

		return Rcpp::List::create(
				Rcpp::Named( "global_risk_ER" ) = global_risk_ER,
				Rcpp::Named( "global_risk" ) = global_risk,
				Rcpp::Named( "global_risk_pct" ) = global_risk*100,
				Rcpp::Named( "Res") = Res,
				Rcpp::Named( "Mat_Risk") = Mat_Risk
		);
	}
	return 0;
}