# HyperTraPS(-CT)

Hypercubic transition path sampling: Flexible inference of accumulation pathways, in discrete or continuous time, under different model structures, using combinations of longitudinal, cross-sectional, and phylogenetically-linked observations.

General content
=========

Requirements
------
For the command-line-only version, you'll just need the ability to compile and run C code.

For the R version without helper and plotting functions, you'll need R and the `Rcpp` library.

For those helper and plotting functions, you'll need R with these libraries for full functionality: `Rcpp`, `ggplot2`, `ggpubr`, `ggraph`, `ggwordcloud`, `igraph`, `stringr`, `stringdist`.

For the specific scientific case studies involving mitochondrial and tuberculosis evolution, the data wrangling also needs Python with `ETE3`.

Setting up and running
-----------

This repo contains code to run HyperTraPS and its variants either from the command line or via R. First, pull the repo down and set the working directory to its root. For command-line work, compile the `hypertraps.c` source code, with (for example):

`gcc -o3 hypertraps.c -lm -o hypertraps.ce`

This will produce the executable `hypertraps.ce` (you can of course name it whatever you like), which can then be run from the command line with various arguments below.

For R work, you can load just the inference code using `Rcpp` alone with

`library(Rcpp)`  
`sourceCpp("hypertraps-r.cpp")`

or, to attach some useful helper and plotting functions (which depend on more libraries), you can use

`source("hypertraps.R")`

The core function in R is then `HyperTraPS` which can then be run from the R console with various arguments below.

Input
------

HyperTraPS deals fundamentally with *transitions between states labelled by binary strings*. This allows us to work with cross-sectional, longitudinally, and phylogenetically-coupled samples.

The fundamental data element that goes into HyperTraPS is an observed transition from a "before" state to an "after" state. In the case of cross-sectional data, the "before" state is assumed to be the state of all zeroes (0000...), corresponding to a state which has not acquired any features. For longitudinal and/or phylogenetic observations, "before" and "after" states must be specified.

HyperTraPS requires at least a matrix describing "after" states -- this is a required argument. In R, it is passed as a matrix; at the command line, it is passed as a file.

If a matrix supplying "before" states is absent, the data are assumed to be cross-sectional. For example, the matrix

`0 0 1`  
`0 1 1`

would reflect cross-sectional observations of states 001 and 011, implicitly corresponding to transitions 000->001 and 000->011.

A matrix of "before" states may be specified as a matrix in R or a file at the command line, using `initialstates`. For example, including the initial states

`0 0 1`  
`0 0 1`

with the above observations would now reflect the transitions 001->001 (i.e. remaining in state 001) and 001->011.

For compatibility with older studies, a different input format is possible at the command-line. When a single file containing a matrix of observations is provided, it is by default assumed that this gives cross-sectional observations. But the `--transitionformat` flag can be used to interpret this matrix as paired "before" and "after" observations. Now, the matrix should contain the "before" states as *odd* rows (starting from row 1) and the "after" states as *even* rows.

For example,

`0 0 1`  
`0 1 1`

with the `--transitionformat` flag would be interpreted as a transition 001->011 (odd row -> even row). Without the `--transitionformat` flag this would be interpreted as two independent observations, corresponding (as in the R case) to transitions 000->001 and 000->011.

The digit 2 can be used to reflect uncertainty in a state. For example,

`0 2 1`

corresponds to an observation where the first feature is absent, the second feature may be present or absent, and the third feature is present.

For continuous-time inference, HyperTraPS works with a time window for each observed transition, specified via a start time and an end time. If the start time and end time are equal, the transition is specified as taking exactly that time. If start time = 0 and end time = Inf, the transition can take any amount of time, which is mathematically equivalent to the case without continuous time. In general, the start and end times specify an allowed set of durations for the given transition, allowing uncertain timings to be accounted for.

In R, these start and end times are vectors specified by `starttimes` and `endtimes`. At the command line, they are stored in files accessed by the `--starttimes` and `--endtimes` flags. In both cases, absent start times means that all start times are assumed to be zero; absent end times means that all end times are assumed to be Inf.

HyperTraPS also accepts descriptions of prior distributions on parameters. For the moment these are assumed to be uniform in log parameter space, and are specified by the min and max for each distribution. At the command line these values should be passed as a single file, given by `--priors`, with two columns and N rows, where the ith row gives the minimum and maximum value for parameter i. In R this should be provided as a matrix `priors` with two columns and N rows. Remember that N, the number of parameters, will depend on the model structure chosen and the number of features in the system. For example, the 3-feature system above and the default model structure (2, referring to L^2 parameters) would have N = 9.

Output
------

At the command line, HyperTraPS will output information about the progress of a run to the screen, and produce a set of files containing outputs from and descriptions of the inference process.

In R, HyperTraPS will output information about the progress of a run to the console, and return a named list containing outputs from and descriptions of the inference process.

The files from the command line version can be read into R (for plotting, analysis, etc) using `readHyperinf` from `hypertraps.R`. This reads a collection of output files and returns a named list. Similar, the named list structure in R can be written to files (for storage) using `writeHyperinf` from `hypertraps.R`. This takes a named list and produces the corresponding file set.

The output structures are

| Information | R | File output | Notes |
|-------------|---|-------------|-------|
| Number of features, model type, number of parameters, and likelihood traces over the run | *list*$lik.traces | *label*-lik.csv | |
| Best parameterisation found during run | *list*$best | *label*-best.txt | |
| (Posterior) samples of parameterisation | *list*$posterior.samples | *label*-posterior.txt | |
| Probabilities for individual states | *list*$dynamics$states | *label*-states.csv | Only produced for L < 16 |
| Probabilities for individual transitions | *list*$dynamics$trans | *label*-trans.csv | Only produced for L < 16 |
| Best parameterisation after regularisation | *list*$regularisation$best | *label*-regularised.txt | Optional |
| Number of parameters, likelihood, and information criteria during regularisation | *list*$regularisation$reg.process | *label*-regularising.csv | Optional |
| "Bubble" probabilities of trait *i* acquisition at ordinal time *j* | *list*$bubbles | *label*-bubbles.csv | |
| Histograms of times of trait *i* acquisition | *list*$timehists | *label*-timehists.csv | |
| Individual sampled routes of accumulation | *list*$routes | *label*-routes.txt | |
| Transition times for individual sampled routes of accumulation | *list*$times | *label*-times.txt | |
| Dwelling statistics for individual sampled routes of accumulation | *list*$betas | *label*-betas.txt | |

The named list in R can be passed to plotting and prediction functions for analysis -- see "Visualising and using output" below.

Demonstration
-----------
A good place to start is `hypertraps-demos.R`, where the basic form of R commands for HyperTraPS, most of the more interesting arguments that can be provided, and several scientific case studies are demonstrated. The expected behaviour for most of this demonstration is in `docs/hypertraps-demos.html`, compiled in R Markdown format with `hypertraps-demos.Rmd`.

Arguments to HyperTraPS
------

HyperTraPS needs at least a set of observations. In R this should take the form of a matrix; on the command line it should be stored in a file.

| Argument | R | Command-line | Default |
|----------|---|--------------|---------|
| Input data | obs=*matrix* | --obs *filename* | None (required) |
| Precursor states | initialstates=*matrix* | --initialstates *filename* | None; on command line can also be specified as odd-element rows in "Input data" |
| Transition format observations | (not available) | --transitionformat | (off) |
| Time window start | starttimes=*vector* | --times *filename* | 0 |
| Time window end | endtimes=*vector* | --endtimes *filename* | starttimes if present (i.e. precisely specified times); otherwise Inf |
| Prior mins and maxs | priors=*matrix* | --priors *filename* | -10 to 10 in log space for each parameter (i.e. very broad range over orders of magnitude)
| Model structure | model=*N* | --model *N* | 2 |
| Number of walkers | walkers=*N* | --walkers *N* | 200 |
| Inference chain length | length=*N* | --length *N* | 3 |
| Perturbation kernel | kernel=*N* | --kernel *N*| 5 |
| Random seed | seed=*N* | --seed *N* | 1 |
| Gains (0) or losses (1) | losses=*N* | --losses *N* | 0 |
| Use APM (0/1) | apm=*N* | --apm | 0 |
| Use SA (0/1) | sa=*N* | --sa | 0 |
| Use SGD (0/1) | sgd=*N* | --sgd | 0 |
| Use PLI (0/1) | pli=*N* | --pli | 0 |
| Regularise model (0/1) | regularise=*N* | --regularise | 0 |

So some example calls are (see the various demo scripts for more):

| Task | R | Command-line |
|------|---|--------------|
| Run HyperTraPS with default settings | HyperTraPS(*matrix*) | ./hypertraps.ce --obs *filename* |
| Run HyperTraPS-CT with default settings | HyperTraPS(*matrix*, starttimes=*vector*, endtimes=*vector*) | ./hypertraps.ce --obs *filename* --times *filename* --endtimes *filename* |
| Run HyperTraPS with all-edges model, then regularise | HyperTraPS(*matrix*, model=-1, regularise=1) | ./hypertraps.ce --obs *filename* --model -1 --regularise |

Visualising and using output
--------

The various outputs of HyperTraPS can be used in the R plotting functions below, which summarise (amongst other things) the numerical behaviour of the inference processes, the ordering and timings (where appropriate) of feature acquisitions, the structure of the learned hypercubic transition network, and any outputs from regularisation. All of these except `plotHypercube.prediction` take a fitted model -- the output of `HyperTraPS` -- as a required argument, and may take other options as the table describes. `plotHypercube.prediction` takes a required argument that is the output of prediction functions described below.

| Plot function | Description | Options and defaults |
|---------------|-------------|---------|
| `plotHypercube.lik.trace` | Trace of likelihood over inference run, calculated twice (to show consistency or lack thereof) | |
| `plotHypercube.bubbles` | "Bubble plot" of probability of acquiring trait *i* at ordinal step *j* | *transpose*=FALSE (horizontal and vertical axis), *reorder*=FALSE (order traits by mean acquisition ordering) |
| `plotHypercube.motifs` | Motif-style plot of probability of acquiring trait *i* at ordinal step *j* | |
| `plotHypercube.graph` | Transition graph with edge weights showing probability flux (from full output) | *thresh*=0.05 (minimum threshold of flux for drawing an edge) |
| `plotHypercube.sampledgraph` | Transition graph with edge weights showing probability flux (from sampled paths) | *thresh*=0.05 (minimum threshold of flux for drawing an edge), max=1000 (maximum number of sampled routes to consider) |
| `plotHypercube.sampledgraph2` | As above, with mean and s.d. of absolute timings for each step | *thresh*=0.05 (minimum threshold of flux for drawing an edge), *max*=1000 (maximum number of sampled routes to consider), *no.times*=FALSE (avoid annotating edges with time information), *use.arc*=TRUE (arc edge format -- looks messier but less prone to overlapping edge labels), *node.labels*=TRUE (binary labels for nodes), *edge.label.size*=2 (font size for edge labels) |
| `plotHypercube.timehists` | Histograms of absolute timings for each trait's acquisition | |
| `plotHypercube.regularisation` | Information criterion vs number of nonzero parameters during regularisation | |
| `plotHypercube.timeseries` | Time series of acquisitions across sampled routes | |
| `plotHypercube.prediction` | Visualise predictions of unobserved features or future behaviour, given a model fit | *prediction* (required, the output of `predictHiddenVals` or `predictNextStep` (see below) |
| `plotHypercube.summary` | Summary plot combining several of the above | *f.thresh*=0.05 (flux threshold for graph plot), *t.thresh*=20 (time threshold for time histograms), *continuous.time*=TRUE (plot continuous time summary information) |


All but the last are demonstrated here:
![image](https://github.com/StochasticBiology/hypertraps-ct/assets/50171196/153ed0d7-88ea-4dc2-a3bc-0c24b25923db)

In addition, we can ask HyperTraPS to make predictions about (a) any unobserved features for a given observation (for example, what value the ?s might take in 01??), and (b) what future evolutionary behaviour is likely given that we are currently in a certain state. These are achieved with functions `predictHiddenVals` and `predictNextStep` respectively. Both of these require a fitted hypercubic model (the output of `HyperTraPS`), which we'll call `fit`.

To predict unobserved values in a given observation, you can invoke

`predictHiddenVals(fit, state)`

where `fit` is the fitted model and `state` is a vector describing the incomplete observations with 0s, 1s, and 2s, the latter of which mark unobserved features. So 0122 has uncertain features at positions 3 and 4. The function will output two dataframes, one giving the probability of observing each specific state that could correspond to the incomplete observation, and one giving the aggregate probability of each uncertain feature being 1.

You can optionally provide an argument `level.weights` which provides probability weightings for different "levels" of the hypercube, that is, the different total number of 1s in the observation. This allows you to specify how likely it is that the true observation has acquired a certain number of features. By default this is uniform between the number of certain 1s and the maximum number of possible 1s.

To predict future evolutionary behaviour, you can invoke

`predictNextStep(fit, state)`

where `fit` is the fitted model and `state` is a given state. This will return a dataframe describing the possible future dynamics from `state` and their probabilities.

You can pass the output of both these prediction functions to `plotHypercube.prediction` for a visualisation of the corresponding prediction.

Specific content for introduction paper
=======

In the `Scripts/`, `Verify/`, `RawData/`, and `Process/` directories are various scripts, datafiles, and data-generating code to demonstrate HyperTraPS functionality and explore two scientific case studies (anti-microbial resistance in tuberculosis and reductive mitochondrial evolution). There's also data from previous studies using HyperTraPS for comparison and demonstration. The demonstration files in the root directory make use of these datafiles.

`Scripts/`
----
Scripts wrapping the curation, inference, and analysis process for different case. In each case, datasets are produced and set in their own directory. The analysis code is then called to process these data, and plotting code follows.
  * `infer-verify.sh` -- generates verification datasets and runs HyperTraPS
  * `infer-tests.sh` -- generates tests of various parameters and experiments and runs HyperTraPS
  * `prepare-all.sh` -- Bash script using the code below in `Process/` to set up TB and MRO datasets. TB, and MRO with NCBI phylogeny, are straightforwardly processed using `cook-data.sh`. MRO with TimeTree phylogeny is a bit more involved and has its own script `mro-timetree-parse.sh`.
  * `infer-tb.sh` -- runs HyperTraPS for TB data
  * `infer-mro.sh` -- runs HyperTraPS for (different versions of) MRO data
  * `infer-others.sh` -- runs HyperTraPS for various other scientific cases
    
`Verify/`
---------
Code to generate synthetic test datasets
  * `generate-easycube.c` -- C code to generate an easy L=3 case
  * `generate-hardcube.c` -- C code to generate a more difficult L=3 case
  * `generate-cross.c` -- C code to generate several L=5 cases supporting competing pathways
    
`RawData/`
----------
  * Phylogenies and feature lists for MRO and TB
  * Feature lists and name sets for previously published case studies: C4 photosynthesis evolution, ovarian cancer progression, severe malaria clinical progression, tool use evolution

`Process/`
----------
Code for (1), distilling transition data suitable for HyperTraPS, for raw MRO and TB data. 
  * `prune-tree.py` -- Python code to reduce a raw Newick tree to just those nodes contained a barcode datafile
  * `internal-labels.c` -- C code to introduce dummy internal node labels (for use in followup preparation)
  * `parse-new.py` -- Python code to infer internal node barcodes and produce transition datafiles ready for HyperTraPS (and summary graphic for checking)
  * `cook-data.sh` -- Bash script applying these steps to given data
  * `mro-timetree-parse.sh` -- Bash script taking care of some subtleties when combining MRO data with TimeTree phylogeny
