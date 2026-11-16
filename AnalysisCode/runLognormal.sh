#!/bin/bash
# Full chain with the log-normal X bias (see BIAS.md), for both pT bins:
#   1. driveBinsLognormal.C  -> biased/reference trees + per-bin configs
#   2. unbias_weights        -> unbiasing weights
#   3. plot_unbias_weights   -> validation plots
# Every output carries "_lognormal" in its name, so it never overwrites the
# power-law outputs of driveBins.C / runUnbiasing.sh / runPlotUnbiasedWeights.sh.
#
# Needs the same environment as the power-law chain (see SETUP.md):
#   ROOT on PATH, FJI (or fastjet-config on PATH), and UNBIAS_INPUT_DIR
#   (or pass the input directory as the first argument).
set -e

# Run from this script's directory (AnalysisCode/), whatever the caller's cwd.
cd "$(dirname "$0")"
# Generated files live in ../Output/ (git-ignored): ROOT files in files/, plots in plots/.
FILES=../Output/files
PLOTS=../Output/plots
mkdir -p "$FILES" "$PLOTS"
T=lognormal

INPUT_DIR=${1:-$UNBIAS_INPUT_DIR}
if [ -z "$INPUT_DIR" ]; then
  echo "runLognormal.sh: no input directory. Set UNBIAS_INPUT_DIR or pass it as the first argument." >&2
  exit 1
fi

# 0. build (no-op if up to date; binaries carry FastJet's lib dir as an rpath)
make

# 1. biased/reference trees + configs
root -l -b -q "driveBinsLognormal.C(\"$INPUT_DIR\")"

# 2. unbiasing weights
for B in "100 140" "140 180"; do
  set -- $B; LO=$1; HI=$2
  ./unbias_weights \
    --input $FILES/startBasis_${LO}_${HI}_${T}.root --tree tBiased \
    --target-input $FILES/startBasis_${LO}_${HI}_${T}.root --target-tree tRef \
    --config $FILES/unbiasing_config_${LO}_${HI}_${T}.env \
    --pt-min $LO --pt-max $HI --subjet-source fastjet \
    --out $FILES/unbias_weights_${LO}_${HI}_${T}.root --mode run
done

# 3. validation plots (file names get _<LO>_<HI>_lognormal)
SUBSTRUCTURE_BINNING="--mass-bins 30 --rg-bins 13 --kt-bins 20 --broadening-bins 25 --tau-bins 25"
for B in "100 140" "140 180"; do
  set -- $B; LO=$1; HI=$2
  ./plot_unbias_weights \
    --input $FILES/unbias_weights_${LO}_${HI}_${T}.root \
    --target-input $FILES/startBasis_${LO}_${HI}_${T}.root --target-tree tRef \
    --config $FILES/unbiasing_config_${LO}_${HI}_${T}.env \
    --pt-min $LO --pt-max $HI --tag $T \
    --jet-R 0.4 --sd-zcut 0.1 --sd-beta 1.0 --nsub-beta 1.0 \
    ${SUBSTRUCTURE_BINNING} \
    --plot-dir $PLOTS \
    --out $FILES/unbias_weights_plots_${LO}_${HI}_${T}.root
done
