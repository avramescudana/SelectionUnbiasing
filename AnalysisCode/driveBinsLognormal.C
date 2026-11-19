// driveBinsLognormal.C -- same as driveBins.C, but with the log-normal X bias
// (startBasis biasType = 1, see BIAS.md). All outputs carry "_lognormal" in
// their names, so they never overwrite the power-law outputs.
//
// Usage (run from the AnalysisCode/ directory -- all paths are relative to it):
//   root -l -b -q 'driveBinsLognormal.C("/path/to/MCOutput/")'
// or set the input directory once via the environment:
//   export UNBIAS_INPUT_DIR=/path/to/MCOutput/
//   root -l -b -q driveBinsLognormal.C
//
// Produces, per bin (in ../Output/files/, plots in ../Output/plots/):
//   startBasis_<bin>_lognormal.root           -- biased/reference trees
//   unbiasing_config_<bin>_lognormal.env      -- BinCenter/PtLow/PtHigh/
//                                                SubjetRadii/EECTermsFile
//
// Log-normal parameters are set per bin: the peak of the keep probability,
// at exp(mu - sigma^2), sits about 30% into each pT window (112 GeV for
// 100-140, 152 GeV for 140-180), so the bias acts inside both bins.
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
  const double lnNorm = 1.0;           // strongest bias for this shape
  const double lnMu_1 = 4.733,  lnSigma_1 = 0.12;   // bin 100-140 (peak at 112 GeV)
  const double lnMu_2 = 5.0383, lnSigma_2 = 0.12;   // bin 140-180 (peak at 152 GeV)

  // Generated files go under ../Output/ (next to AnalysisCode/, git-ignored).
  const TString filesDir = "../Output/files";
  const TString plotsDir = "../Output/plots";
  gSystem->mkdir(filesDir, kTRUE);
  gSystem->mkdir(plotsDir, kTRUE);

  // Sample tag (required), e.g. UNBIAS_TAG=10M -> startBasis_100_140_lognormal_10M.root,
  // so outputs made from different input samples never overwrite each other.
  const char* envTag = gSystem->Getenv("UNBIAS_TAG");
  if (!(envTag && envTag[0])) {
    std::cerr << "driveBinsLognormal: UNBIAS_TAG is not set. Name the input sample, e.g.\n"
              << "  export UNBIAS_TAG=10M" << std::endl;
    return;
  }
  const TString tag = TString("_") + envTag;

  // ---- Bin 1: 100-140 GeV ----
  startBasis(inputDir, filesDir + "/startBasis_100_140_lognormal" + tag + ".root",
             100, 140, p, d, rngSeed, binCenter,
             filesDir + "/unbiasing_config_100_140_lognormal" + tag + ".env",
             subjetRadii, eecTermsFile, unbiasRootPlotDir(plotsDir),
             biasType, lnMu_1, lnSigma_1, lnNorm);
  unbiasMplStartBasisPlot(filesDir + "/startBasis_100_140_lognormal" + tag + ".root", plotsDir);

  // ---- Bin 2: 140-180 GeV ----
  startBasis(inputDir, filesDir + "/startBasis_140_180_lognormal" + tag + ".root",
             140, 180, p, d, rngSeed, binCenter,
             filesDir + "/unbiasing_config_140_180_lognormal" + tag + ".env",
             subjetRadii, eecTermsFile, unbiasRootPlotDir(plotsDir),
             biasType, lnMu_2, lnSigma_2, lnNorm);
  unbiasMplStartBasisPlot(filesDir + "/startBasis_140_180_lognormal" + tag + ".root", plotsDir);
}
