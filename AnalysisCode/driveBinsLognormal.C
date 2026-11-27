// driveBinsLognormal.C -- same as driveBins.C, but with the log-normal X bias
// (startBasis biasType = 1, see BIAS.md). All outputs carry "_lognormal_<set>" in
// their names (set = named parameter set, below), so they never overwrite the
// power-law outputs or each other.
//
// Usage (run from the AnalysisCode/ directory -- all paths are relative to it):
//   root -l -b -q 'driveBinsLognormal.C("/path/to/MCOutput/")'
// or set the input directory once via the environment:
//   export UNBIAS_INPUT_DIR=/path/to/MCOutput/
//   root -l -b -q driveBinsLognormal.C
//
// Produces, per bin (in ../Output/files/, plots in ../Output/plots/):
//   startBasis_<bin>_lognormal_<set>_<TAG>.root       -- biased/reference trees
//   unbiasing_config_<bin>_lognormal_<set>_<TAG>.env  -- BinCenter/PtLow/PtHigh/
//                                                        SubjetRadii/EECTermsFile
//
// Named log-normal parameter sets, chosen with UNBIAS_LN_PARAMS (default params1).
// The peak of the keep probability is at exp(mu - sigma^2), set per pT bin:
//   params1: peak 112 GeV (100-140) / 152 GeV (140-180), sigma = 0.12, norm = 1
//            (peak below the unbiased mean: the biased mean goes down)
//   params2: peak 128 GeV (100-140) / 168 GeV (140-180), sigma = 0.06, norm = 1
//            (narrower, peak above the unbiased mean: the biased mean goes up)
#include "startBasis.C"

void driveBinsLognormal(const char* inputDirArg = "") {
  // Input directory: macro argument, else $UNBIAS_INPUT_DIR, else abort.
  const char* envDir = gSystem->Getenv("UNBIAS_INPUT_DIR");
  const char* inputDir = (inputDirArg && inputDirArg[0]) ? inputDirArg
                         : (envDir && envDir[0])         ? envDir
                         : "";
  if (!inputDir[0]) {
    std::cerr << "driveBinsLognormal: no input directory. Pass it as an argument, e.g.\n"
              << "  root -l -b -q 'driveBinsLognormal.C(\"/path/to/MCOutput/\")'\n"
              << "or set UNBIAS_INPUT_DIR." << std::endl;
    return;
  }
  const double p = 8.0, d = 0.1;       // power-law parameters, ignored when biasType = 1
  const unsigned rngSeed = 12345;
  const double binCenter = -1.0;       // midpoint of the pT window
  const char* subjetRadii = "0.10,0.20";
  const char* eecTermsFile = "eec_terms_example.csv";
  const int biasType = 1;              // log-normal
  // Named parameter set (see the top of this file), from $UNBIAS_LN_PARAMS:
  //   name, mu for bin 1 (100-140), mu for bin 2 (140-180), sigma, norm
  struct LnParams { const char* name; double mu1, mu2, sigma, norm; };
  const LnParams lnSets[] = {
    {"params1", 4.733,   5.0383,  0.12, 1.0},   // peaks 112 / 152 GeV
    {"params2", 4.85563, 5.12756, 0.06, 1.0},   // peaks 128 / 168 GeV
  };
  const char* envSet = gSystem->Getenv("UNBIAS_LN_PARAMS");
  const TString setName = (envSet && envSet[0]) ? envSet : "params1";
  const LnParams* lnSet = nullptr;
  for (const LnParams& s : lnSets)
    if (setName == s.name) lnSet = &s;
  if (!lnSet) {
    std::cerr << "driveBinsLognormal: UNBIAS_LN_PARAMS=" << setName << " is not a known set (params1, params2)" << std::endl;
    return;
  }
  // Optional overrides of single values of the set (use a distinct UNBIAS_TAG
  // per variant, e.g. 1M_sharp, so the outputs do not overwrite each other):
  //   UNBIAS_LN_SIGMA            width sigma (both bins)
  //   UNBIAS_LN_NORM             amplitude of the keep probability (0-1]
  //   UNBIAS_LN_PEAK1/2          peak pT in GeV for bin 1/2 (sets mu = ln(peak) + sigma^2)
  // and how the reference and biased samples are built (startBasis fillMode, see BIAS.md):
  //   UNBIAS_FILL                0 = partly biased (default): X = Y, biased = kept X + Y',
  //                              1 = fully biased: biased = the kept jets only
  auto envNum = [](const char* name, double def) {
    const char* v = gSystem->Getenv(name);
    return (v && v[0]) ? std::atof(v) : def;
  };
  const double lnNorm    = envNum("UNBIAS_LN_NORM", lnSet->norm);   // 1 = strongest bias for this shape
  const int fillMode     = (int)envNum("UNBIAS_FILL", 0);
  const double lnSigma_1 = envNum("UNBIAS_LN_SIGMA", lnSet->sigma), lnSigma_2 = lnSigma_1;
  const double peak_1 = envNum("UNBIAS_LN_PEAK1", -1.0), peak_2 = envNum("UNBIAS_LN_PEAK2", -1.0);
  const double lnMu_1 = (peak_1 > 0) ? std::log(peak_1) + lnSigma_1 * lnSigma_1 : lnSet->mu1;   // bin 100-140
  const double lnMu_2 = (peak_2 > 0) ? std::log(peak_2) + lnSigma_2 * lnSigma_2 : lnSet->mu2;   // bin 140-180
  std::cout << "driveBinsLognormal: " << setName << " sigma=" << lnSigma_1 << " norm=" << lnNorm << " fill=" << fillMode
            << " mu1=" << lnMu_1 << " (peak " << std::exp(lnMu_1 - lnSigma_1 * lnSigma_1) << " GeV)"
            << " mu2=" << lnMu_2 << " (peak " << std::exp(lnMu_2 - lnSigma_2 * lnSigma_2) << " GeV)" << std::endl;

  // Generated files go under ../Output/ (next to AnalysisCode/, git-ignored).
  const TString filesDir = "../Output/files";
  const TString plotsDir = "../Output/plots";
  gSystem->mkdir(filesDir, kTRUE);
  gSystem->mkdir(plotsDir, kTRUE);

  // Sample tag (required), e.g. UNBIAS_TAG=10M -> startBasis_100_140_lognormal_params1_10M.root,
  // so outputs made from different input samples never overwrite each other.
  const char* envTag = gSystem->Getenv("UNBIAS_TAG");
  if (!(envTag && envTag[0])) {
    std::cerr << "driveBinsLognormal: UNBIAS_TAG is not set. Name the input sample, e.g.\n"
              << "  export UNBIAS_TAG=10M" << std::endl;
    return;
  }
  const TString tag = TString("_") + setName + "_" + envTag;   // "_<set>_<TAG>", after "_lognormal"

  // ---- Bin 1: 100-140 GeV ----
  startBasis(inputDir, filesDir + "/startBasis_100_140_lognormal" + tag + ".root",
             100, 140, p, d, rngSeed, binCenter,
             filesDir + "/unbiasing_config_100_140_lognormal" + tag + ".env",
             subjetRadii, eecTermsFile, unbiasRootPlotDir(plotsDir),
             biasType, lnMu_1, lnSigma_1, lnNorm, fillMode);
  unbiasMplStartBasisPlot(filesDir + "/startBasis_100_140_lognormal" + tag + ".root", plotsDir);

  // ---- Bin 2: 140-180 GeV ----
  startBasis(inputDir, filesDir + "/startBasis_140_180_lognormal" + tag + ".root",
             140, 180, p, d, rngSeed, binCenter,
             filesDir + "/unbiasing_config_140_180_lognormal" + tag + ".env",
             subjetRadii, eecTermsFile, unbiasRootPlotDir(plotsDir),
             biasType, lnMu_2, lnSigma_2, lnNorm, fillMode);
  unbiasMplStartBasisPlot(filesDir + "/startBasis_140_180_lognormal" + tag + ".root", plotsDir);
}
