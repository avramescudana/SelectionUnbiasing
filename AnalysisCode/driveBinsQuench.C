// driveBinsQuench.C -- quenching bias (startBasis biasType = 2, see BIAS.md):
// every jet of the whole spectrum loses an energy eps, with eps/omegaC drawn
// from the log-normal of Arleo, JHEP 11 (2002) 044 [hep-ph/0210104], Eq. (3.7).
// Reference = jets with the true pT in the window; biased = jets with the
// quenched pT in the window (stored with their true pT and substructure).
// All outputs carry "_quench_<set>" in their names.
//
// Usage (run from the AnalysisCode/ directory):
//   root -l -b -q 'driveBinsQuench.C("/path/to/MCOutput/")'
// or with UNBIAS_INPUT_DIR set. The input needs a pTHat range well above the
// windows (jets lose energy into them), e.g. generated with
//   UNBIAS_PTHAT_MIN=60 UNBIAS_PTHAT_MAX=600 PYTHIA/generate.sh ...
//
// Windows (wider than the 40 GeV bins of driveBins.C): 100-200 and 150-250 GeV.
//
// Named parameter sets (UNBIAS_QUENCH_PARAMS, default out), Table 1 of the paper:
//   out: mu = -1.5,  sigma = 0.73  (outgoing quark, <eps> = 0.3 omegaC)
//   in:  mu = -2.55, sigma = 0.57  (incoming quark, <eps> = 0.1 omegaC)
// omegaC = qhat L^2 / 2 (Eq. 2.9), UNBIAS_QUENCH_OMEGAC in GeV, default 62.5:
//   qhat = 5 GeV/fm^2 (the paper's hot-medium value) and L = 5 fm.
#include "startBasis.C"

void driveBinsQuench(const char* inputDirArg = "") {
  const char* envDir = gSystem->Getenv("UNBIAS_INPUT_DIR");
  const char* inputDir = (inputDirArg && inputDirArg[0]) ? inputDirArg
                         : (envDir && envDir[0])         ? envDir
                         : "";
  if (!inputDir[0]) {
    std::cerr << "driveBinsQuench: no input directory. Pass it as an argument or set UNBIAS_INPUT_DIR." << std::endl;
    return;
  }
  const double p = 8.0, d = 0.1;       // power-law parameters, ignored
  const unsigned rngSeed = 12345;
  const double binCenter = -1.0;       // midpoint of the pT window
  const char* subjetRadii = "0.10,0.20";
  const char* eecTermsFile = "eec_terms_example.csv";
  const int biasType = 2;              // quenching

  struct QParams { const char* name; double mu, sigma; };
  const QParams sets[] = {
    {"out", -1.5,  0.73},
    {"in",  -2.55, 0.57},
  };
  const char* envSet = gSystem->Getenv("UNBIAS_QUENCH_PARAMS");
  const TString setName = (envSet && envSet[0]) ? envSet : "out";
  const QParams* qs = nullptr;
  for (const QParams& s : sets)
    if (setName == s.name) qs = &s;
  if (!qs) {
    std::cerr << "driveBinsQuench: UNBIAS_QUENCH_PARAMS=" << setName << " is not a known set (out, in)" << std::endl;
    return;
  }
  const char* envW = gSystem->Getenv("UNBIAS_QUENCH_OMEGAC");
  const double omegaC = (envW && envW[0]) ? std::atof(envW) : 62.5;
  std::cout << "driveBinsQuench: " << setName << " mu=" << qs->mu << " sigma=" << qs->sigma
            << " omegaC=" << omegaC << " GeV, <eps>=" << omegaC * std::exp(qs->mu + 0.5 * qs->sigma * qs->sigma)
            << " GeV" << std::endl;

  const TString filesDir = "../Output/files";
  const TString plotsDir = "../Output/plots";
  gSystem->mkdir(filesDir, kTRUE);
  gSystem->mkdir(plotsDir, kTRUE);

  const char* envTag = gSystem->Getenv("UNBIAS_TAG");
  if (!(envTag && envTag[0])) {
    std::cerr << "driveBinsQuench: UNBIAS_TAG is not set. Name the input sample, e.g. export UNBIAS_TAG=4M_hipt" << std::endl;
    return;
  }
  const TString tag = TString("_quench_") + setName + "_" + envTag;

  const double windows[][2] = {{100, 200}, {150, 250}};
  for (const auto& w : windows) {
    const TString bin = Form("%g_%g", w[0], w[1]);
    const TString out = filesDir + "/startBasis_" + bin + tag + ".root";
    startBasis(inputDir, out, w[0], w[1], p, d, rngSeed, binCenter,
               filesDir + "/unbiasing_config_" + bin + tag + ".env",
               subjetRadii, eecTermsFile, unbiasRootPlotDir(plotsDir),
               biasType, qs->mu, qs->sigma, 1.0, 0, omegaC);
    unbiasMplStartBasisPlot(out, plotsDir);
  }
}
