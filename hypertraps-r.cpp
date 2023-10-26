#include <Rcpp.h>
using namespace Rcpp;
#define _USE_CODE_FOR_R 1
#include "hypertraps.c"

List PosteriorAnalysis(List L,
		       Nullable<CharacterVector> featurenames_arg,
		       int use_regularised);
List OutputStatesR(double *ntrans, int LEN, int model);
List HyperTraPS(NumericMatrix matrix_arg,
		Nullable<NumericMatrix> initialstates_arg,
		Nullable<NumericVector> starttimes_arg,
		Nullable<NumericVector> endtimes_arg,
		NumericVector length_index_arg,
		NumericVector kernel_index_arg,
		NumericVector losses_arg,
		NumericVector apm_type_arg,
		NumericVector sa_arg,
		NumericVector sgd_arg,
		NumericVector sgd_scale_arg,
		NumericVector seed_arg,
		NumericVector outputinput_arg,
		NumericVector regularise_arg,
		NumericVector model_arg,
		NumericVector PLI_arg,
		NumericVector walkers_arg,
		NumericVector full_analysis_arg,
		Nullable<CharacterVector> featurenames_arg);

List OutputStatesR(double *ntrans, int LEN, int model)
{
  int i, j, k, a;
  int statedec;
  int src, dest;
  int state[LEN];
  double rate, totrate;
  int *active, *newactive;
  double *probs;
  int nactive, newnactive;
  int level;
  int found;
  
  //  fp = fopen(beststatesstr, "w");
  //fprintf(fp, "State Probability\n");
  NumericVector state_v, prob_v;
  NumericVector from_v, to_v, edgeprob_v, flux_v;

  probs = (double*)malloc(sizeof(double)*mypow2(LEN));
  active = (int*)malloc(sizeof(int)*mypow2(LEN));
  newactive = (int*)malloc(sizeof(int)*mypow2(LEN));
  for(i = 0; i < mypow2(LEN); i++)
    probs[i] = 0;
  level = 0;
  
  probs[0] = 1;
  
  active[0] = 0;
  nactive = 1;
  
  while(nactive > 0)
    {
      newnactive = 0;
      /*      printf("%i active\n", nactive);
	      for(a = 0; a < nactive; a++)
	      printf("%i ", active[a]);
	      printf("\n\n"); */
	    
      for(a = 0; a < nactive; a++)
	{
	  src = active[a];
	  statedec = src;
	  for(j = LEN-1; j >= 0; j--)
	    {
	      if(statedec >= mypow2(j))
		{
		  state[LEN-1-j] = 1;
		  statedec -= mypow2(j);
		}
	      else
		state[LEN-1-j] = 0;
	    }

	  totrate = 0;
	  for(j = 0; j < LEN; j++)
	    {
	      /* ntrans must be the transition matrix. ntrans[i+i*LEN] is the bare rate for i. then ntrans[j*LEN+i] is the modifier for i from j*/
	      if(state[j] == 0)
		{
		  rate = RetrieveEdge(state, j, ntrans, LEN, model);
		  totrate += rate;
		}
	    }

	  for(j = 0; j < LEN; j++)
	    {
	      /* ntrans must be the transition matrix. ntrans[i+i*LEN] is the bare rate for i. then ntrans[j*LEN+i] is the modifier for i from j*/
	      if(state[j] == 0)
		{
		  dest = src+mypow2(LEN-1-j);
		  rate = RetrieveEdge(state, j, ntrans, LEN, model);
		  probs[dest] += probs[src] * rate/totrate;
		  from_v.push_back(src);
		  to_v.push_back(dest);
		  edgeprob_v.push_back(rate/totrate);
		  flux_v.push_back(probs[src]*rate/totrate);
		  //		  printf("%i: %i (from %i, %e): %e\n", level, dest, src, probs[src], probs[dest]);
		
		  found = 0;
		  for(k = 0; k < newnactive; k++)
		    {
		      if(newactive[k] == dest) { found = 1; break; }
		    }
		  if(found == 0)
		    newactive[newnactive++] = dest;
		}
	    }
	}
      for(a = 0; a < newnactive; a++)
	active[a] = newactive[a];
      nactive = newnactive;
      level++;
    }
  
  for(dest = 0; dest < mypow2(LEN); dest++)
    {
      state_v.push_back(dest);
      prob_v.push_back(probs[dest]);
    }
  //    fprintf(fp, "%i %e\n", dest, probs[dest]);

  List L = List::create(Named("State") = state_v,
			Named("Probability") = prob_v);
  DataFrame Ldf(L);

  List Lflux = List::create(Named("From") = from_v,
			    Named("To") = to_v,
			    Named("Probability") = edgeprob_v,
			    Named("Flux") = flux_v);
  DataFrame Lfluxdf(Lflux);

  List Lout = List::create(Named("states") = Ldf, Named("trans") = Lfluxdf);
  
  //  fclose(fp);
  free(active);
  free(newactive);
  free(probs);

  return Lout;
}

List RegulariseR(int *matrix, int len, int ntarg, double *ntrans, int *parents, double *tau1s, double *tau2s, int model, int PLI)
{
  int i, j;
  int NVAL;
  double lik, nlik;
  double oldval;
  int biggestindex;
  double biggest;
  int pcount;
  double AIC, BIC, bestBIC;
  double *best;
  
  NVAL = nparams(model, len);
  best = (double*)malloc(sizeof(double)*NVAL);
  
  lik = GetLikelihoodCoalescentChange(matrix, len, ntarg, ntrans, parents, tau1s, tau2s, model, PLI);

  AIC = 2*NVAL-2*lik;
  BIC = log(ntarg)*NVAL-2*lik;
  bestBIC = BIC;
  for(i = 0; i < NVAL; i++)
    best[i] = ntrans[i];

  NumericVector NVAL_v, lik_v, AIC_v, BIC_v;
  
  // sprintf(fstr, "%s-regularising.csv", labelstr);
  //fp = fopen(fstr, "w");
  //fprintf(fp, "nparam,log.lik,AIC,BIC\n");
  //fprintf(fp, "%i,%e,%e,%e\n", NVAL, lik, AIC, BIC);

  NVAL_v.push_back(NVAL);
  lik_v.push_back(lik);
  AIC_v.push_back(AIC);
  BIC_v.push_back(BIC);
  
  printf("Regularising...\npruning ");
  // remove parameters stepwise
  for(j = 0; j < NVAL; j++)
    {
      printf("%i of %i\n", j+1, NVAL); 
      // find parameter that retains highest likelihood when removed
      biggest = 0;
      for(i = 0; i < NVAL; i++)
	{
	  oldval = ntrans[i];
	  ntrans[i] = 0;
	  nlik = GetLikelihoodCoalescentChange(matrix, len, ntarg, ntrans, parents, tau1s, tau2s, model, PLI);
	  ntrans[i] = oldval;
	  if((biggest == 0 || nlik > biggest) && ntrans[i] != 0)
	    {
	      biggest = nlik;
	      biggestindex = i;
	    }
	}
      // set this param to zero and count new param set
      ntrans[biggestindex] = 0;
      pcount = 0;
      for(i = 0; i < NVAL; i++)
	{
	  if(ntrans[i] != 0) pcount++;
	}
      // output
      AIC = 2*pcount-2*biggest;
      BIC = log(ntarg)*pcount-2*biggest;

      NVAL_v.push_back(pcount);
      lik_v.push_back(biggest);
      AIC_v.push_back(AIC);
      BIC_v.push_back(BIC);
      //fprintf(fp, "%i,%e,%e,%e\n", pcount, biggest, AIC, BIC);
      if(BIC < bestBIC)
	{
	  bestBIC = BIC;
	  for(i = 0; i < NVAL; i++)
	    best[i] = ntrans[i];
	}
    }

  List Ldyn = List::create(Named("params") = NVAL_v,
			   Named("lik") = lik_v,
			   Named("AIC") = AIC_v,
			   Named("BIC") = BIC_v);

  DataFrame Ldyndf(Ldyn);
  
  NumericVector best_v(NVAL);
  
  //sprintf(fstr, "%s-regularised.txt", labelstr);
  //fp = fopen(fstr, "w");
  // for(i = 0; i < NVAL; i++)
  //  fprintf(fp, "%e ", best[i]);
  //fprintf(fp, "\n");
  //fclose(fp);
  for(i = 0; i < NVAL; i++)
    best_v[i] = best[i];

  List Lout = List::create(Named("best") = best_v,
			   Named("lik.1") = GetLikelihoodCoalescentChange(matrix, len, ntarg, best, parents, tau1s, tau2s, model, PLI),
			   Named("lik.2") = GetLikelihoodCoalescentChange(matrix, len, ntarg, best, parents, tau1s, tau2s, model, PLI),
			   Named("reg.process") = Ldyndf);
			   
  //  sprintf(fstr, "%s-regularised-lik.txt", labelstr);
  //fp = fopen(fstr, "w"); fprintf(fp, "Step,LogLikelihood1,LogLikelihood2\n"); 
  // fprintf(fp, "0,%e,%e\n", 
    // fclose(fp);

    //  sprintf(fstr, "%s-regularised-trans.txt", labelstr);
    //OutputTransitions(fstr, best, len, model);
    //sprintf(fstr, "%s-regularised-states.txt", labelstr);
    //OutputStates(fstr, best, len, model);

    free(best);
  return Lout;
  
}


//' Runs HyperTraPS-related inference on a dataset of observations
//'
//' @param matrix_arg A matrix of observations. Should contain 0s, 1s, and optional 2s for missing data. Should be $n \times L$, containing $n$ cross-sectional observations of length $L$.
//' @param initialstates_arg An optional matrix of initial states. If we are using longitudinal observations, each row in this matrix gives the "before" state to the corresponding "after" state in the observations matrix. Omitting this matrix is equivalent to consider every observation to have a root "before" state. If specified, should be $n \times L$, containing $n$ cross-sectional observations of length $L$, to match the observations matrix.
//' @param start_times_arg An optional vector of $n$ times describing the beginning of the observation time window for each observation. If empty, equivalent to this time window beginning at time 0. If specified, should be of length $n$.
//' @param end_times_arg An optional vector of $n$ times describing the end of the observation time window for each observation. If empty, equivalent to this time window ending at time infinity. If specified, should be of length $n$.
//' @param length_index_arg Length of MCMC chain
//' @param kernel_index_arg Kernel index
//' @param losses_arg Whether to consider accumulation of gains (0) or losses (1)
//' @param apm_type_arg APM
//' @param sgd_scale_arg SGD
//' @param seed_arg Random number seed
//' @param outputinput_arg Option to output the input data
//' @param regularise_arg Regularise
//' @param model_arg Model structure
//' @param PLI_arg Phenotype landscape inference
//' @return A named list of objects from the inference process, containing parameter samples from the inference process, the maximum likelihood parameterisation, likelihood samples, and the sampling times.
// [[Rcpp::export]]
List HyperTraPS(NumericMatrix matrix_arg, //NumericVector len_arg, NumericVector ntarg_arg,
		Nullable<NumericMatrix> initialstates_arg = R_NilValue,
		Nullable<NumericVector> starttimes_arg = R_NilValue,
		Nullable<NumericVector> endtimes_arg = R_NilValue,
		NumericVector length_index_arg = 3,
		NumericVector kernel_index_arg = 5,
		NumericVector losses_arg = 0,
		NumericVector apm_type_arg = 0,
		NumericVector sa_arg = 0,
		NumericVector sgd_arg = 0,
		NumericVector sgd_scale_arg = 0.01,
		NumericVector seed_arg = 1,
		NumericVector outputinput_arg = 0,
		NumericVector regularise_arg = 0,
		NumericVector model_arg = 2,
		NumericVector PLI_arg = 0,
		NumericVector walkers_arg = 200,
		NumericVector full_analysis_arg = 1,
		Nullable<CharacterVector> featurenames_arg = R_NilValue)
{
  int parents[_MAXN];
  int *matrix;
  int len, ntarg;
  double *trans, *ntrans, *besttrans, *gradients;
  int t;
  int i, j;
  double lik, nlik;
  int maxt;
  int seed;
  double DELTA, MU;
  int NVAL;
  int expt;
  double acc, rej, lacc, lrej;
  double *tmpmat;
  double r;
  double tau1s[_MAXN], tau2s[_MAXN];
  int nancount = 0;
  int spectrumtype;
  double bestlik = 0;
  int lengthindex, kernelindex;
  int SAMPLE;
  int losses;
  int apm_seed, old_apm_seed;
  int apm_type;
  double testval;
  char obsfile[1000], timefile[1000], endtimefile[1000], paramfile[1000];
  int searchmethod;
  int filelabel;
  char labelstr[1000];
  int crosssectional;
  time_t start_t, end_t;
  double diff_t;
  struct timeval t_stop, t_start;
  int outputinput;
  double sgdscale;
  int model;
  int regularise;
  int outputtransitions;
  int readparams;
  int PLI;

  // default values
  spectrumtype = 0;
  lengthindex = length_index_arg[0];
  kernelindex = kernel_index_arg[0];
  losses = losses_arg[0];
  apm_type = apm_type_arg[0];
  sgdscale = sgd_scale_arg[0];
  filelabel = 0;
  seed = seed_arg[0];
  searchmethod = 0;
  BANK = walkers_arg[0];
  
  if(sgd_arg[0] == 1) searchmethod = 1;
  if(sa_arg[0] == 1) searchmethod = 2;
  
  outputinput = outputinput_arg[0];
  regularise = regularise_arg[0];
  model = model_arg[0];
  readparams = 0;
  PLI = PLI_arg[0];
  outputtransitions = 1;
  strcpy(obsfile, "rcpp");
  strcpy(paramfile, "");
  strcpy(timefile, "");
  strcpy(endtimefile, "");

  // basic input parsing
  len = matrix_arg.ncol();
  ntarg = matrix_arg.nrow()*2;
  // construct internal observation matrix
  matrix = (int*)malloc(sizeof(int)*len*ntarg);
 
  // check to see if we're doing crosssectional analysis, and if not, if we've got appropriate initial state info
  crosssectional = 1;
  if(initialstates_arg.isUsable()) {
    NumericMatrix initialstates(initialstates_arg);
    crosssectional = 0;
    if(initialstates.ncol() != len || initialstates.nrow() != ntarg/2)
      {
	Rprintf("If specifying initial states, we need one initial state for each observation.");
	myexit(0);
      }
    for(i = 0; i < ntarg/2; i++)
      {
	for(j = 0; j < len; j++)
	  matrix[i*(2*len)+j] = initialstates(i, j);
	for(j = 0; j < len; j++)
	  matrix[i*(2*len)+len+j] = matrix_arg(i,j);
      }
 
  }
  else {
    for(i = 0; i < ntarg/2; i++)
      {
	for(j = 0; j < len; j++)
	  matrix[i*(2*len)+j] = 0;
	for(j = 0; j < len; j++)
	  matrix[i*(2*len)+len+j] = matrix_arg(i,j);
      }
  }

  // populate timing vectors
  if(starttimes_arg.isUsable()) {
    NumericVector starttimes(starttimes_arg);
    if(starttimes.length() != ntarg/2) {
      Rprintf("If specifying start timings, we need one timing entry for each observation.");
      myexit(0);
    }
    for(i = 0; i < ntarg/2; i++)
      tau1s[i] = starttimes[i];
    spectrumtype = 1;
  }
  if(endtimes_arg.isUsable()) {
    NumericVector endtimes(endtimes_arg);
    if(endtimes.length() != ntarg/2) {
      Rprintf("If specifying end timings, we need one timing entry for each observation.");
      myexit(0);
    }
    for(i = 0; i < ntarg/2; i++)
      tau2s[i] = endtimes[i];

    spectrumtype = 1;
  }

  if(spectrumtype == 1)
    {
      if(!starttimes_arg.isNotNull()) {
	Rprintf("End timings, but not start timings, specified. Assuming t = 0 starts.\n");
	for(i = 0; i < ntarg/2; i++)
	  tau1s[i] = 0;
      }
      if(!endtimes_arg.isNotNull()) {
	Rprintf("Start timings, but not end timings, specified. Assuming t = inf ends.\n");
	for(i = 0; i < ntarg/2; i++)
	  tau2s[i] = INFINITY;
      }
      for(i = 0; i < ntarg/2; i++)
	{
	  if(tau2s[i] < tau1s[i])
	    {
	      Rprintf("End time %i is less than start time!\n", i);
	      myexit(0);
	    }
	}
    }
  else
    {
      for(i = 0; i < ntarg/2; i++)
	{
	  tau1s[i] = 0;
	  tau2s[i] = INFINITY;
	}
    }
  
  Rprintf("\nHyperTraPS(-CT)\nSep 2023\n\nUnpublished code -- please do not circulate!\nPublished version available at:\n    https://github.com/StochasticBiology/HyperTraPS\nwith stripped-down version at:\n    https://github.com/StochasticBiology/hypertraps-simple\n\n");

  if(PLI == 1) {
    Rprintf("Running Phenotype Landscape Inference with:\n[observations-file]: %s\n[start-timings-file]: %s\n[end-timings-file]: %s\n[random number seed]: %i\n[length index]: %i\n[kernel index]: %i\n[walkers]: %i\n[losses (1) or gains (0)]: %i\n[APM]: %i\n[model]: %i\n\n", obsfile, timefile, endtimefile, seed, lengthindex, kernelindex, BANK, losses, apm_type, model);
  } else if(spectrumtype == 1) {
    Rprintf("Running HyperTraPS-CT with:\n[observations-file]: %s\n[start-timings-file]: %s\n[end-timings-file]: %s\n[random number seed]: %i\n[length index]: %i\n[kernel index]: %i\n[walkers]: %i\n[losses (1) or gains (0)]: %i\n[APM]: %i\n[model]: %i\n\n", obsfile, timefile, endtimefile, seed, lengthindex, kernelindex, BANK, losses, apm_type, model);
  } else {
    Rprintf("Running HyperTraPS with:\n[observations-file]: %s\n[random number seed]: %i\n[length index]: %i\n[kernel index]: %i\n[walkers]: %i\n[losses (1) or gains (0)]: %i\n[APM]: %i\n[model]: %i\n\n", obsfile, seed, lengthindex, kernelindex, BANK, losses, apm_type, model);
  }
  switch(searchmethod) {
  case 0: Rprintf("Using MH MCMC\n"); break;
  case 1: Rprintf("Using SGD\n"); break;
  case 2: Rprintf("Using SA\n"); break;
  } 
  
  // initialise and allocate
  maxt = pow(10, lengthindex);
  if(maxt <= 10000) SAMPLE = 100; else SAMPLE = 1000;

  if(_EVERYITERATION)
    SAMPLE = 1;

  srand48(seed);

  // choose parameterisation based on command line
  expt = kernelindex;
  switch(expt)
    {
    case 0: DELTA = 0; break;
    case 1: DELTA = 0.005; MU = 0.1; break;
    case 2: DELTA = 0.05; MU = 1.; break;
    case 3: DELTA = 0.05; MU = 1.; break;
    case 4: DELTA = 0.1; MU = 1.; break;
    case 5: DELTA = 0.25; MU = 1.; break;
    case 6: DELTA = 0.5; MU = 1.; break;
    default: DELTA = 0.75; MU = 1.; break;
    }

  NVAL = nparams(model, len);
  
  if(outputinput)
    {
      Rprintf("Observed transitions:\n");
      for(i = 0; i < ntarg/2; i++)
	{
	  Rprintf("%i: ", i);
	  for(j = 0; j < len; j++) Rprintf("%i", matrix[2*len*i+j]);
	  Rprintf(" -> ");
	  for(j = 0; j < len; j++) Rprintf("%i", matrix[2*len*i+len+j]);
	  if(spectrumtype != 0)
	    Rprintf("(window %.3e-%.3e)", tau1s[i], tau2s[i]);
	  Rprintf("\n");
	}
      if(losses == 1) Rprintf("(where 1 is absence)\n\n");
      if(losses == 0) Rprintf("(where 1 is presence)\n\n");
    }
    
  if(spectrumtype == 0)
    {
      Rprintf("Number of features is %i, I found %i observation pairs\n", len, ntarg/2);
    }
  else
    {
      Rprintf("Number of features is %i, I found %i observation pairs and %i timing pairs\n", len, ntarg/2, ntarg/2);
      if(len > 30)
	{
	  Rprintf("*** CAUTION: continuous time calculations sometimes fail to converge for large (>30) feature sets. This can lead to NaNs appearing, which will stop the simulation. Consider running without continuous time option.\n");
	}
    }
  Rprintf("\n");

  if(len > 15 && outputtransitions == 1)
    {
      Rprintf("*** More than 15 features, meaning we'd need a lot of space to output transition and state information. I'm switching off this output.\n");
      outputtransitions = 0;
    }
  
  // allocate memory and initialise output file
  trans = (double*)malloc(sizeof(double)*NVAL); 
  ntrans = (double*)malloc(sizeof(double)*NVAL);
  besttrans = (double*)malloc(sizeof(double)*NVAL);
  gradients = (double*)malloc(sizeof(double)*NVAL);
  tmpmat = (double*)malloc(sizeof(double)*NVAL);

  if(filelabel == 0)
    {
      sprintf(labelstr, "%s-%i-%i-%i-%i-%i-%i-%i", obsfile, spectrumtype, searchmethod, seed, lengthindex, kernelindex, BANK, apm_type);
    }
  // prepare output files
  /*  sprintf(shotstr, "%s-posterior.txt", labelstr);
      fp = fopen(shotstr, "w"); fclose(fp);
      sprintf(bestshotstr, "%s-best.txt", labelstr);
      fp = fopen(bestshotstr, "w"); fclose(fp);
      sprintf(likstr, "%s-lik.txt", labelstr);
      fp = fopen(likstr, "w"); fprintf(fp, "Step,LogLikelihood1,LogLikelihood2\n"); fclose(fp);
  
      sprintf(besttransstr, "%s-trans.txt", labelstr);
      sprintf(beststatesstr, "%s-states.txt", labelstr);*/
  
  // initialise with an agnostic transition matrix
  if(readparams == 0)
    {
      Rprintf("Starting with simple initial param guess\n");
      InitialMatrix(trans, len, model, 0);
    }
  else
    {
      Rprintf("Starting with supplied parameterisation\n");
      ReadMatrix(trans, len, model, paramfile);
    }

  // compute initial likelihood given this matrix
  time(&start_t);
  gettimeofday(&t_start, NULL);
  lik = GetLikelihoodCoalescentChange(matrix, len, ntarg, trans, parents, tau1s, tau2s, model, PLI);
  time(&end_t);
  gettimeofday(&t_stop, NULL);
  diff_t = (t_stop.tv_sec - t_start.tv_sec) + (t_stop.tv_usec-t_start.tv_usec)/1.e6;
  //  diff_t = difftime(end_t, start_t);
  Rprintf("One likelihood estimation took %e seconds.\nInitial likelihood is %e\n", diff_t, lik);
  lik = GetLikelihoodCoalescentChange(matrix, len, ntarg, trans, parents, tau1s, tau2s, model, PLI);
  Rprintf("Second guess is %e\n", lik);
  // MCMC or simulated annealing
  if(searchmethod == 0 || searchmethod == 2)
    {
      Rprintf("This code (%i steps) will probably take around %.3f seconds (%.3f hours) to complete.\n\n", maxt, diff_t*maxt, diff_t*maxt/3600.);
    }
  if(isinf(lik))
    {
      Rprintf("Start parameterisation gave a nonsensical likelihood. I'm going to try random alternatives.\n");
      if(PLI) {
	Rprintf("With PLI, this often means we're not using enough random walkers to hit every datapoint on the hypercube. If this takes a while to find a suitable start parameterisation, consider re-running with more random walkers.\n");
      }
      do{
	InitialMatrix(trans, len, model, 0);
	lik = GetLikelihoodCoalescentChange(matrix, len, ntarg, trans, parents, tau1s, tau2s, model, PLI);
      }while(isinf(lik));
      Rprintf("OK, starting with initial likelihood %e\n", lik);
    }
  
  // initialise counters for acceptance ratio
  acc = rej = 0;
  lacc = lrej = 0;

  if(apm_type == 1)
    apm_seed = seed;

  int NSAMPLES = (maxt-maxt/5)/SAMPLE-1;
  
  NumericVector lik1_output, lik2_output, L_output, model_output, nparam_output, t_output;
  NumericVector best_output(NVAL);
  NumericMatrix posterior_output(NSAMPLES, NVAL);
  int sampleref = 0;

  List dynamics_output;
  
  // run the chain
  for(t = 0; t < maxt; t++)
    {
      // if we've got a new best likelihood, store it
      if(lik > bestlik || t == 0)
	{
	  for(i = 0; i < NVAL; i++)
	    best_output[i] = besttrans[i] = trans[i];
	  bestlik = lik;
	  
	  if(outputtransitions)
	    { 
	      dynamics_output = OutputStatesR(besttrans, len, model);
	    }
	}

      // output some info periodically
      if(t % SAMPLE == 0)
	Rprintf("%i - ", t);

      if(t > maxt/5 && t % SAMPLE == 0)
	{
	  // if we're burnt in, periodically sample the current parameterisation to an output file
	  // most appropriate for Bayesian MCMC but useful for all
	  for(i = 0; i < NVAL; i++)
	    posterior_output(sampleref, i) = trans[i];

	  sampleref++;
	  
	  nlik = GetLikelihoodCoalescentChange(matrix, len, ntarg, trans, parents, tau1s, tau2s, model, PLI);
	  lik1_output.push_back(nlik);
	  nlik = GetLikelihoodCoalescentChange(matrix, len, ntarg, trans, parents, tau1s, tau2s, model, PLI);
	  lik2_output.push_back(nlik);
	  L_output.push_back(len);
	  model_output.push_back(model);
	  nparam_output.push_back(NVAL);
	  t_output.push_back(t);
	}

      // MCMC or simulated annealing
      if(searchmethod == 0 || searchmethod == 2)
	{

	  if(apm_type == 0 || t%2 == 0)
	    {
	      // apply a perturbation to the existing parameterisation
	      // non-uniform priors can be employed here if desired 
		for(i = 0; i < NVAL; i++)
		  {
		    ntrans[i] = trans[i];
		    r = RND;
		    if(r < MU)
		      {
			ntrans[i] += gsl_ran_gaussian(DELTA);
		      }
		    if(ntrans[i] < -10) ntrans[i] = -10;
		    if(ntrans[i] > 10) ntrans[i] = 10;
		  }
	      if(APM_VERBOSE)
		{
		  Rprintf("step 0 (change theta): apm_seed %i, ntrans[0] %f\n", apm_seed, ntrans[0]);
		}
	    }
	  else
	    {
	      // change the random number seed and keep the parameterisation the same
	      old_apm_seed = apm_seed;
	      apm_seed = seed+t;
	      for(i = 0; i < NVAL; i++)
		ntrans[i] = trans[i];
	      if(APM_VERBOSE)
		{
		  Rprintf("step 1 (change u): apm_seed %i, ntrans[0] %f\n", apm_seed, ntrans[0]);
		}
	    }
      
	  // compute likelihood for the new parameterisation
	  if(apm_type == 1)
	    {
	      srand48(apm_seed);
	      if(APM_VERBOSE)
		{
		  Rprintf("r seeded with %i, first call is %f\n", apm_seed, RND);
		}
	    }
	  nlik = GetLikelihoodCoalescentChange(matrix, len, ntarg, ntrans, parents, tau1s, tau2s, model, PLI);

	  if(APM_VERBOSE)
	    {
	      Rprintf("likelihood %f\n", nlik);
	    }
      
	  // keep track of NaNs in calculations
	  if(isnan(nlik))
	    {
	      nancount++;
	    }

	  testval = RND;
	  if(searchmethod == 2)
	    {
	      testval = 0.1*sqrt(sqrt(t));
	    }

	  // compare likelihood to previous
	  if(nlik >= lik || -(lik-nlik) > log(testval))
	    {
	      // accept this new parameterisation
	      lik = nlik;
	  
	      if(apm_type == 0 || t%2 == 0)
		{
		  acc++; lacc++;
		  for(i = 0; i < NVAL; i++)
		    trans[i] = ntrans[i];
		}
	      if(APM_VERBOSE)
		{
		  Rprintf("accepted: apm_seed %i trans[0] %f\n\n", apm_seed, trans[0]);
		}
	    }
	  else 
	    {
	      // reject the change
	      if(apm_type == 1 && t%2 == 1)
		{
		  apm_seed = old_apm_seed;
		}
	      else
		{
		  rej++; lrej++;
		}
	      if(APM_VERBOSE)
		{
		  Rprintf("rejected: apm_seed %i trans[0] %f\n\n", apm_seed, trans[0]);
		}
	    }
	}
      // gradient descent
      if(searchmethod == 1)
	{
	  time(&start_t);
	  gettimeofday(&t_start, NULL);
	  GetGradients(matrix, len, ntarg, trans, parents, tau1s, tau2s, gradients, sgdscale, model, PLI);
	  time(&end_t);
	  gettimeofday(&t_stop, NULL);
	  diff_t = (t_stop.tv_sec - t_start.tv_sec) + (t_stop.tv_usec-t_start.tv_usec)/1.e6;
	  if(t == 0)
	    Rprintf("Using SGD: one gradient calculation took %e seconds\n\n", diff_t);
  
	  for(i = 0; i < NVAL; i++)
	    {
	      trans[i] = trans[i]+gradients[i]*sgdscale;
	      if(trans[i] < -10) trans[i] = -10;
	      if(trans[i] > 10) trans[i] = 10;
	    }
	  
	  nlik = GetLikelihoodCoalescentChange(matrix, len, ntarg, trans, parents, tau1s, tau2s, model, PLI);
	  Rprintf("Iteration %i likelihood %f previous-likelihood %f\n", t, nlik, lik);
	  lik = nlik;
	}
      //      if(t % SAMPLE == 0) printf("NaN count %i of %i\n", nancount, t);

      // output information periodically
      if(t % TMODULE == 0 && searchmethod != 1)
	{
	  Rprintf("Iteration %i likelihood %f total-acceptance %f recent-acceptance %f trial-likelihood %f\n", t, lik, acc/(acc+rej), lacc/(lacc+lrej), nlik);
	  lacc = lrej = 0;
	}
    }

  List Lts = List::create(Named("Step") = t_output,
			  Named("L") = L_output,
  			  Named("model") = model_output,
			  Named("nparam") = nparam_output,
			  Named("LogLikelihood1") = lik1_output,
			  Named("LogLikelihood2") = lik2_output);
  DataFrame Ltsdf(Lts);

  List L = List::create(Named("label") = labelstr ,
			Named("L") = len,
			Named("model") = model,
			Named("best") = best_output,
			Named("posterior.samples") = posterior_output,
			Named("lik.traces") = Ltsdf);

  if(outputtransitions) 
    L["dynamics"] = dynamics_output;

  if(regularise)
    {
      List regL = RegulariseR(matrix, len, ntarg, besttrans, parents, tau1s, tau2s, model, PLI);
      L["regularisation"] = regL;
    }

  if(full_analysis_arg[0] == 0)
    return L;
  else
    return PosteriorAnalysis(L, featurenames_arg, regularise);
}

//' Extracts information from HyperTraPS-related posterior samples
//'
//' @param L List output from HyperTraPS, containing posterior samples
//' @return Named list containing summary data for feature acquisition ordering ("bubbles"), time histograms, sampled accumulation routes, and timings of these sampled routes.
// [[Rcpp::export]]
List PosteriorAnalysis(List L,
		       Nullable<CharacterVector> featurenames_arg = R_NilValue,
		       int use_regularised = 0)
{
  int *matrix;
  int len, ntarg;
  double *trans, *ntrans;
  int t;
  int i, j;
  int *rec, *order;
  double *drec, *sortdrec, *mean;
  int allruns;
  int seed = 0;
  double tmp;
  int change;
  char names[200*FLEN];
  int count;
  double *meanstore, *fmeanstore;
  double *ctrec, ctnorm;
  double *times, *betas;
  int *route;
  int tlen;
  int verbose;
  double BINSCALE;
  char postfile[1000];
  int filelabel;
  char labelstr[1000];
  int NVAL;
  int model;
  int burnin, sampleperiod;
  char labelfile[1000];
  
  // default values
  BINSCALE = 10;
  verbose = 0;
  filelabel = 0;
  seed = 0;
  model = L["model"];
  burnin = 0;
  sampleperiod = 0;
  strcpy(postfile, "rcpp");
  strcpy(labelfile, "");
  
  Rprintf("\nHyperTraPS(-CT) posterior analysis\n\n");

  // deal with command-line arguments
  /*  for(i = 1; i < argc; i+=2)
      {
      if(strcmp(argv[i], "--posterior\0") == 0) strcpy(postfile, argv[i+1]);
      else if(strcmp(argv[i], "--label\0") == 0) { filelabel = 1; strcpy(labelstr, argv[i+1]); }
      else if(strcmp(argv[i], "--featurenames\0") == 0) { strcpy(labelfile, argv[i+1]); }
      else if(strcmp(argv[i], "--seed\0") == 0) seed = atoi(argv[i+1]);
      else if(strcmp(argv[i], "--model\0") == 0) model = atoi(argv[i+1]);
      else if(strcmp(argv[i], "--sims\0") == 0) NSAMP = atoi(argv[i+1]);
      else if(strcmp(argv[i], "--trajs\0") == 0) NTRAJ = atoi(argv[i+1]);
      else if(strcmp(argv[i], "--burnin\0") == 0) burnin = atoi(argv[i+1]);
      else if(strcmp(argv[i], "--period\0") == 0) sampleperiod = atoi(argv[i+1]);      
      else if(strcmp(argv[i], "--binscale\0") == 0) BINSCALE = atof(argv[i+1]);
      else if(strcmp(argv[i], "--verbose\0") == 0) { verbose = 1; i--; }
      else if(strcmp(argv[i], "--help\0") == 0) helpandquit(0);
      }

      if(strcmp(postfile, "") == 0)
      {
      Rprintf("*** I need at least a file of posterior samples! ***\n\n");
      helpandquit(0);
      }
      if(model == 0)
      {
      Rprintf("*** Posterior analysis isn't meaningful for a zero-parameter model ***\n\n");
      return 0;
      }*/

  Rprintf("Verbose flag is %i\n", verbose);
  Rprintf("Bin scale is %f\n", BINSCALE);


  NumericMatrix posterior;
  if(use_regularised == 0)
    {
      posterior = as<NumericMatrix>(L["posterior.samples"]);
      Rprintf("Using posterior samples with %i x %i entries\n", posterior.nrow(), posterior.ncol());
    }
  else
    {
      List tmpL = L["regularisation"];
      NumericVector tmpV = tmpL["best"];
      
      NumericMatrix tmpM(1,tmpV.size());
      for(i = 0; i < tmpV.size(); i++)
	tmpM(0,i) = tmpV[i];
      posterior = as<NumericMatrix>(tmpM);
      Rprintf("Using best regularised params with %i x %i entries\n", posterior.nrow(), posterior.ncol());
    }
  
  tlen = posterior.ncol();
  
  // figure out if posterior file is presented in L*L format; get L if so
  len = 0;
  for(i = 1; i < 200; i++)
    {
      if(tlen == nparams(model, i))
	{
	  len = i;
	  break;
	}
    }
  if(len == 0)
    {
      Rprintf("Given model type %i, couldn't determine number of features from %s, which seems to have %i params per sample\n", model, postfile, tlen);
      return 0;
    }

  Rprintf("Based on %s with %i params per model and model %i, there are %i features\n", postfile, tlen, model, len);

  NumericVector passedL = as<NumericVector>(L["L"]); 
  if(len != passedL[0]) {
    Rprintf("But this doesn't match the L=%i in my argument!\n", passedL[0]);
    myexit(0);
  }

  if(featurenames_arg.isUsable()) {
    CharacterVector featurenames(featurenames_arg);
    for(i = 0; i < len; i++)
      {
	sprintf(&names[i*FLEN], "%s", (char*)featurenames[i]);
      }

  } else {
    Label(names, len, NULL);
  }

  // initialise and allocate a lot of different arrays to compute and store statistics
  srand48(seed);
  allruns  =0;
  ntarg = 0;
      
  NVAL = nparams(model, len);
  
  matrix = (int*)malloc(sizeof(int)*10000);
  ctrec = (double*)malloc(sizeof(double)*MAXCT*len);
  times = (double*)malloc(sizeof(double)*len);
  betas = (double*)malloc(sizeof(double)*len);
  route = (int*)malloc(sizeof(int)*len);

  trans = (double*)malloc(sizeof(double)*NVAL); /* transition matrix */
  ntrans = (double*)malloc(sizeof(double)*NVAL);
  rec = (int*)malloc(sizeof(int)*len*len); /* stores step ordering, modified by getlikelihood */
  mean = (double*)malloc(sizeof(double)*len);
  meanstore = (double*)malloc(sizeof(double)*len);
  fmeanstore = (double*)malloc(sizeof(double)*len);
  order = (int*)malloc(sizeof(int)*len);
  drec = (double*)malloc(sizeof(double)*len*len);
  sortdrec = (double*)malloc(sizeof(double)*len*len);

  // initialise
  for(i = 0; i < MAXCT*len; i++)
    ctrec[i] = 0;
  ctnorm = 0;

  for(i = 0; i < len*len; i++)
    rec[i] = 0;

  for(i = 0; i < len; i++)
    fmeanstore[i] = 0;

  if(filelabel == 0)
    sprintf(labelstr, "%s", postfile);

  Rprintf("Output label is %s\n", labelstr);
  
  // set up file outputs
  if(verbose)
    {
      /*      sprintf(fstr, "%s-routes.txt", labelstr);
	      fp1 = fopen(fstr, "w");
	      sprintf(fstr, "%s-betas.txt", labelstr);
	      fp2 = fopen(fstr, "w");
	      sprintf(fstr, "%s-times.txt", labelstr);
	      fp3 = fopen(fstr, "w");*/
    }
  
  int NSAMPLES = ((posterior.nrow() - burnin)/(sampleperiod+1))*(NSAMP);
  NumericMatrix route_out(NSAMPLES, len);
  NumericMatrix betas_out(NSAMPLES, len);
  NumericMatrix times_out(NSAMPLES, len);
  int sampleindex = 0;
      
  for(count = 0; count < posterior.nrow(); count++)
    {
      // read in single posterior sample
      for(i = 0; i < NVAL; i++)
	ntrans[i] = posterior(count,i);
	  
      // this if statement controls which samples get processed
      // if we want to include burn-in or subsampling, can put it here
      if(count >= burnin && count % (sampleperiod+1) == 0)
	{
	  // loop through iterations
	  for(j = 0; j < NSAMP; j++)
	    {
	      for(i = 0; i < len; i++)
		meanstore[i] = 0;
	      // simulate behaviour on this posterior and add statistics to counts and histograms
	      GetRoutes(matrix, len, ntarg, ntrans, rec, meanstore, ctrec, times, betas, route, BINSCALE, model);
	      for(i = 0; i < len; i++)
		fmeanstore[i] += meanstore[i];
	      ctnorm += NTRAJ;
	      allruns++;

	      for(i = 0; i < len; i++)
		{
		  route_out(sampleindex, i) = route[i];
		  betas_out(sampleindex, i) = betas[i];
		  times_out(sampleindex, i) = times[i];
		}
	      sampleindex++;
	      /*		  if(verbose)
				  {
				  for(i = 0; i < len; i++)
				  fprintf(fp1, "%i ", route[i]);
				  for(i = 0; i < len; i++)
				  fprintf(fp2, "%.15f ", betas[i]);
				  for(i = 0; i < len; i++)
				  fprintf(fp3, "%.3e ", times[i]);
				  fprintf(fp1, "\n");
				  fprintf(fp2, "\n");
				  fprintf(fp3, "\n");
				  }*/
	    }
	}
    }
  /*  if(verbose)
      {
      fclose(fp1);
      fclose(fp2);
      fclose(fp3);
      } */


  Rprintf("allruns is %i\n", allruns);

  // output various summaries
  for(i = 0; i < len; i++)
    Rprintf("%i %f\n", i, fmeanstore[i]/allruns);

  // compute mean orderings
  // rec[t*len+i] is prob of obtaining i at time t

  for(i = 0; i < len*len; i++)
    drec[i] = (double)rec[i]/(allruns*NTRAJ);

  for(i = 0; i < len; i++)
    {
      mean[i] = 0;
      order[i] = i;
      for(t = 0; t < len; t++)
	mean[i] += t*drec[t*len+i];
    }

  // simple bubble sort orders features by mean acquisition order
  do{
    change = 0;
    for(i = 0; i < len-1; i++)
      {
	if(mean[i] > mean[i+1])
	  {
	    tmp = mean[i]; mean[i] = mean[i+1]; mean[i+1] = tmp;
	    tmp = order[i]; order[i] = order[i+1]; order[i+1] = tmp;
	    change = 1;
	  }
      }
  }while(change == 1);
  seed--;

  // output the set of summary statistics
  // rec[t*len+i] is prob of obtaining i at time t

  // this produces the heatmap of acquisition probability by feature and order
  // outputs both the original feature ordering and the above mean-sorted references

  NumericVector t_col(len*len), i_col(len*len), order_col(len*len), prob_col(len*len);
  CharacterVector name_col(len*len);
  
  for(t = 0; t < len; t++)
    {
      for(i = 0; i < len; i++)
	{
	  t_col(t*len+i) = t;
	  i_col(t*len+i) = i;
	  order_col(t*len+i) = order[i];
	  prob_col(t*len+i) = drec[t*len+order[i]];
	  name_col(t*len+i) = &names[FLEN*order[i]];
	}
    }

  List BubbleL = List::create(Named("Time") = t_col,
			      Named("ReorderedIndex") = i_col,
			      Named("OriginalIndex") = order_col,
			      Named("Name") = name_col,
			      Named("Probability") = prob_col);

  /*
    sprintf(str, "%s-bubbles.csv", labelstr);
    fp = fopen(str, "w");
    fprintf(fp, "Time,ReorderedIndex,OriginalIndex,Name,Probability\n");
    for(t = 0; t < len; t++)
    {
    for(i = 0; i < len; i++)
    fprintf(fp, "%i,%i,%i,%s,%.15f\n", t, i, order[i], &names[FLEN*order[i]], drec[t*len+order[i]]);
    fprintf(fp, "\n");
    }*/


  // this stores the time histograms associated with acquisition times for each feature
  // remember here that we've scaled by BINSCALE to store in an integer-referenced array (see GetRoutes())

  NumericVector i_col_ct(len*MAXCT), t_col_ct(len*MAXCT), prob_col_ct(len*MAXCT);
  for(i = 0; i < len; i++)
    {
      for(j = 0; j < MAXCT; j++)
	{
	  i_col_ct(i*MAXCT+j) = i;
	  t_col_ct(i*MAXCT+j) = j/BINSCALE;
	  prob_col_ct(i*MAXCT+j) = ctrec[MAXCT*i+j]/ctnorm;
	}
    }

  List THistL = List::create(Named("OriginalIndex") = i_col_ct,
			     Named("Time") = t_col_ct,
			     Named("Probability") = prob_col_ct);
			      

  /*  sprintf(str, "%s-timehists.csv", labelstr);
      fp = fopen(str, "w");
      fprintf(fp, "OriginalIndex,Time,Probability\n");
      for(i = 0; i < len; i++)
      {
      tmp = 0;
      for(j = 0; j < MAXCT; j++)
      {
      fprintf(fp, "%i,%f,%.6f\n", i, j/BINSCALE, ctrec[MAXCT*i+j]/ctnorm);
      tmp += ctrec[MAXCT*i+j]*j;
      }
      Rprintf("%i %.4f\n", i, tmp/ctnorm);
      fprintf(fp, "\n");
      }*/

  DataFrame Bubbledf(BubbleL);
  DataFrame THistdf(THistL);

  List OutputL = L;

  OutputL["bubbles"] = Bubbledf;
  OutputL["timehists"] = THistdf;
  OutputL["routes"] = route_out;
  OutputL["betas"] = betas_out;
  OutputL["times"] = times_out;

  return OutputL;
}

