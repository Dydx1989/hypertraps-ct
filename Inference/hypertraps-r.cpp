#include <Rcpp.h>
using namespace Rcpp;
#define _USE_CODE_FOR_R 1
#include "hypertraps-all.c"

// [[Rcpp::export]]
List HyperTraPS(NumericMatrix matrix_arg, //NumericVector len_arg, NumericVector ntarg_arg,
		Nullable<NumericMatrix> initialstates_arg = R_NilValue,
		Nullable<NumericVector> starttimes_arg = R_NilValue,
		Nullable<NumericVector> endtimes_arg = R_NilValue,
			 NumericVector length_index_arg = 3,
			 NumericVector kernel_index_arg = 5,
			 NumericVector losses_arg = 0,
			 NumericVector apm_type_arg = 0,
			 NumericVector sgd_scale_arg = 0.01,
			 NumericVector seed_arg = 1,
			 NumericVector crosssectional_arg = 0,
			 NumericVector outputinput_arg = 0,
			 NumericVector regularise_arg = 0,
			 NumericVector model_arg = 2,
			 NumericVector PLI_arg = 0)
{
  int parents[_MAXN];
  FILE *fp;
  int *matrix;
  int len, ntarg;
  double *trans, *ntrans, *gradients;
  int t;
  int i, j;
  double lik, nlik;
  int maxt;
  int seed;
  char shotstr[200], bestshotstr[200], besttransstr[200], beststatesstr[200];
  double DELTA, MU;
  int NVAL;
  int expt;
  double acc, rej, lacc, lrej;
  double *tmpmat;
  double r;
  double tau1s[_MAXN], tau2s[_MAXN];
  int ntau;
  int nancount = 0;
  int spectrumtype;
  double bestlik = 0;
  int lengthindex, kernelindex;
  int SAMPLE;
  int losses;
  int apm_seed, old_apm_seed;
  int apm_type;
  char likstr[100];
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
  crosssectional = crosssectional_arg[0];
  searchmethod = 0;
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
  ntarg = matrix_arg.nrow();
   // construct internal observation matrix
  matrix = (int*)malloc(sizeof(int)*2*len*ntarg);
 
  // check to see if we're doing crosssectional analysis, and if not, if we've got appropriate initial state info
  crosssectional = 1;
  if(initialstates_arg.isNotNull()) {
    NumericMatrix initialstates(initialstates_arg);
    crosssectional = 0;
    if(initialstates.ncol() != len || initialstates.nrow() != ntarg)
    {
      Rprintf("If specifying initial states, we need one initial state for each observation.");
      myexit(0);
    }
     for(i = 0; i < ntarg; i++)
    {
      for(j = 0; j < len; j++)
	    matrix[i*len+j] = initialstates(i, j);
      for(j = 0; j < len; j++)
	matrix[i*(2*len)+len+j] = matrix_arg(i,j);
    }
 
  }
  else {
  for(i = 0; i < ntarg; i++)
    {
      for(j = 0; j < len; j++)
	    matrix[i*(2*len)+j] = 0;
      for(j = 0; j < len; j++)
	matrix[i*(2*len)+len+j] = matrix_arg(i,j);
    }
  }

  // XXX TIMINGS TO POPULATE
  if(starttimes_arg.isNotNull()) {
    NumericVector starttimes(starttimes_arg);
    if(starttimes.length() != ntarg) {
      Rprintf("If specifying start timings, we need one timing entry for each observation.");
      myexit(0);
    }
    spectrumtype = 1;
  }
    if(endtimes_arg.isNotNull()) {
    NumericVector endtimes(starttimes_arg);
    if(endtimes.length() != ntarg) {
      Rprintf("If specifying end timings, we need one timing entry for each observation.");
      myexit(0);
    }
    spectrumtype = 1;
  }

    if(spectrumtype == 1)
      {
	if(!starttimes_arg.isNotNull()) {
	  Rprintf("End timings, but not start timings, specified. Assuming t = 0 starts.\n");
	}
	if(!endtimes_arg.isNotNull()) {
	  Rprintf("Start timings, but not end timings, specified. Assuming t = inf ends.\n");
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
	  Rprintf("\n");
	}
      if(losses == 1) Rprintf("(where 1 is absence)\n\n");
      if(losses == 0) Rprintf("(where 1 is presence)\n\n");
    }
  
  // grab timings from data file provided
  if(spectrumtype != 0)
    {
      ntau = 0;
      fp = fopen(timefile, "r");
      if(fp == NULL)
	{
	  Rprintf("Couldn't find start timings file %s\n", timefile);
	  return 0;
	}
      while(!feof(fp))
	{
	  fscanf(fp, "%lf", &(tau1s[ntau]));
	  if(!feof(fp)) { ntau++; }
	}
      fclose(fp);
      if(ntau != ntarg/2) 
	{
	  Rprintf("I found %i timings and %i observation pairs -- these numbers should be equal.\n", ntau, ntarg);
	  return 0;
	}

      fp = fopen(endtimefile, "r");
      if(fp == NULL)
	{
	  Rprintf("Couldn't find end timings file -- I'm assuming that start times *are* end times (i.e. each observation has a precisely specified single time)\n");
	  for(i = 0; i < ntau; i++) tau2s[i] = tau1s[i];
	}
      else
	{
	  ntau = 0;
	  while(!feof(fp))
	    {
	      fscanf(fp, "%lf", &(tau2s[ntau]));
	      if(tau2s[ntau] < tau1s[ntau])
		{
		  Rprintf("End time %f was less than start time %f!\n", tau2s[ntau], tau1s[ntau]);
		  myexit(0);
		}
	      if(!feof(fp)) { ntau++; }
	    }
	  fclose(fp);
	}
      if(ntau != ntarg/2) 
	{
	  Rprintf("I found %i timings and %i observation pairs -- these numbers should be equal.\n", ntau, ntarg);
	  return 0;
	}

    }
  else
    {
      ntau = ntarg/2;
      for(i = 0; i < ntau; i++)
	{
	  tau1s[i] = 0;
	  tau2s[i] = INFINITY;
	}
    }
  
  if(spectrumtype == 0)
    {
      Rprintf("Number of features is %i, I found %i observation pairs\n", len, ntarg/2);
    }
  else
    {
      Rprintf("Number of features is %i, I found %i observation pairs and %i timing pairs\n", len, ntarg/2, ntau);
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
  gradients = (double*)malloc(sizeof(double)*NVAL);
  tmpmat = (double*)malloc(sizeof(double)*NVAL);

  if(filelabel == 0)
    {
      sprintf(labelstr, "%s-%i-%i-%i-%i-%i-%i-%i", obsfile, spectrumtype, searchmethod, seed, lengthindex, kernelindex, BANK, apm_type);
    }
  // prepare output files
  sprintf(shotstr, "%s-posterior.txt", labelstr);
  fp = fopen(shotstr, "w"); fclose(fp);
  sprintf(bestshotstr, "%s-best.txt", labelstr);
  fp = fopen(bestshotstr, "w"); fclose(fp);
  sprintf(likstr, "%s-lik.txt", labelstr);
  fp = fopen(likstr, "w"); fprintf(fp, "Step,LogLikelihood1,LogLikelihood2\n"); fclose(fp);
  
  sprintf(besttransstr, "%s-trans.txt", labelstr);
  sprintf(beststatesstr, "%s-states.txt", labelstr);
  
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
  
  NumericVector lik1_output, lik2_output, t_output;
  NumericVector best_output(NVAL);
  NumericMatrix posterior_output(NSAMPLES, NVAL);
  int sampleref = 0;
  
  // run the chain
  for(t = 0; t < maxt; t++)
    {
      // if we've got a new best likelihood, store it
      if(lik > bestlik || t == 0)
	{
	  for(i = 0; i < NVAL; i++)
	    best_output[i] = trans[i];

	  /*	  if(outputtransitions)
	    {
	      OutputTransitions(besttransstr, trans, len, model);
	      OutputStates(beststatesstr, trans, len, model);
	      }*/
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
  if(regularise)
    {
      Regularise(matrix, len, ntarg, trans, parents, tau1s, tau2s, model, labelstr, PLI);
    }

  List L = List::create(Named("label") = labelstr ,
			Named("best") = best_output,
			Named("posterior.samples") = posterior_output,
			Named("sample.times") = t_output,
			Named("l.samples.1") = lik1_output,
			Named("l.samples.2") = lik2_output);
  
  //  return posterior_out;
  return L;
}

// [[Rcpp::export]]
List PosteriorAnalysis(List L)
{
  FILE *fp;
  int *matrix;
  int len, ntarg;
  double *trans, *ntrans;
  int t;
  int i, j;
  int *rec, *order;
  double *drec, *sortdrec, *mean;
  int allruns;
  int seed = 0;
  char str[200];
  double tmp;
  int change;
  char names[200*FLEN];
  int count;
  double *meanstore, *fmeanstore;
  double *ctrec, ctnorm;
  double *times, *betas;
  int *route;
  FILE *fp1, *fp2, *fp3;
  char fstr[200];
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
  model = 2;
  burnin = 0;
  sampleperiod = 0;
  sprintf(postfile, "rcpp");
  sprintf(labelfile, "");
  
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

  NumericMatrix posterior = as<NumericMatrix>(L["posterior.samples"]);
  
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


  // initialise and allocate a lot of different arrays to compute and store statistics
  srand48(seed);
  allruns  =0;
  ntarg = 0;
  Label(names, len, labelfile);
    
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
      sprintf(fstr, "%s-routes.txt", labelstr);
      fp1 = fopen(fstr, "w");
      sprintf(fstr, "%s-betas.txt", labelstr);
      fp2 = fopen(fstr, "w");
      sprintf(fstr, "%s-times.txt", labelstr);
      fp3 = fopen(fstr, "w");
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

  // these appended comments give gnuplot commands for axis labels if required: both in original and mean-sorted orderings
  // commented here, as we're shifting to csv format
  /*  fprintf(fp, "# set xtics (");
  for(i = 0; i < len; i++)
    fprintf(fp, "\"%s\" %i%c", &names[FLEN*order[i]], i, (i == len-1 ? ')' : ','));
  fprintf(fp, "\n");
  fprintf(fp, "# default-order set xtics (");
  for(i = 0; i < len; i++)
    fprintf(fp, "\"%s\" %i%c", &names[FLEN*i], i, (i == len-1 ? ')' : ','));
  fprintf(fp, "\n");

  fprintf(fp, "# (");
  for(i = 0; i < len; i++)
    fprintf(fp, "%i, ", order[i]);
  fprintf(fp, ")\n");

  fprintf(fp, "# ");
  for(i = 0; i < len; i++)
    fprintf(fp, "%s %.4f, ", &names[FLEN*order[i]], mean[i]);
  fprintf(fp, ")\n");

  fclose(fp);*/

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

   List OutputL = List::create(Named("Bubbles") = Bubbledf,
			       Named("THist") = THistdf,
			       Named("Routes") = route_out,
			       Named("Betas") = betas_out,
			       Named("Times") = times_out);

   return OutputL;
}

