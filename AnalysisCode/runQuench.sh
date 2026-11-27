#!/bin/bash
# Full chain with the quenching bias (see BIAS.md), for the windows 100-200 and 150-250 GeV:
#   1. driveBinsQuench.C     -> reference (true pT in window) / biased (quenched pT in window) trees + configs
#   2. unbias_weights        -> unbiasing weights
#   3. plot_unbias_weights   -> validation plots
# Every output carries "_quench_<set>" in its name (set = $UNBIAS_QUENCH_PARAMS, default out,
# see driveBinsQuench.C). The input needs a high pTHat range, e.g.
#   UNBIAS_PTHAT_MIN=60 UNBIAS_PTHAT_MAX=600 ../PYTHIA/generate.sh 4000000 16 ../Output/MCOutput_4M_hipt
#
# Needs the same environment as the power-law chain (see SETUP.md).
set -e

# Run from this script's directory (AnalysisCode/), whatever the caller's cwd.
cd "$(dirname "$0")"
# Generated files live in ../Output/ (git-ignored): ROOT files in files/, plots in plots/.
FILES=../Output/files
PLOTS=../Output/plots
mkdir -p "$FILES" "$PLOTS"
# Sample tag (required, e.g. export UNBIAS_TAG=10M): every name ends in _quench_<set>_<tag>.
if [ -z "$UNBIAS_TAG" ]; then
  echo "$(basename "$0"): UNBIAS_TAG is not set. Name the input sample, e.g. export UNBIAS_TAG=10M" >&2
  exit 1
fi
T=quench_${UNBIAS_QUENCH_PARAMS:-out}_$UNBIAS_TAG   # quenching parameter set (see driveBinsQuench.C) + sample tag
# Fit length (see unbias_weights.cc): UNBIAS_MAX_ITER = number of iterations (default 20)
# or "auto": run until every moment matches its target to UNBIAS_CONVERGE_ACCURACY
# (relative, default 5e-3), or the loss stops improving, or UNBIAS_MAX_ITER_CAP iterations (default 5000).
FIT_ARGS="--max-iter ${UNBIAS_MAX_ITER:-20} --converge-accuracy ${UNBIAS_CONVERGE_ACCURACY:-5e-3} --max-iter-cap ${UNBIAS_MAX_ITER_CAP:-5000}"
# Plotting backend (UNBIAS_PLOTTER, see SETUP.md): root (default), matplotlib or both.
# ROOT plots go to $PLOTS/root, matplotlib plots (plot_mpl.py) to $PLOTS/mpl.
# plot_unbias_weights always runs: it fills the histograms that plot_mpl.py reads.
PLOTTER=${UNBIAS_PLOTTER:-root}
PY=${UNBIAS_PYTHON:-python3}
ROOT_DIR=$PLOTS/root
case "$PLOTTER" in
  root)       ROOT_FORMATS=pdf;  MPL_DIR= ;;
  matplotlib) ROOT_FORMATS=none; MPL_DIR=$PLOTS/mpl ;;
  both)       ROOT_FORMATS=pdf;  MPL_DIR=$PLOTS/mpl ;;
  *) echo "UNBIAS_PLOTTER=$PLOTTER not understood (root, matplotlib or both)" >&2; exit 1 ;;
esac
mkdir -p "$ROOT_DIR"

INPUT_DIR=${1:-$UNBIAS_INPUT_DIR}
if [ -z "$INPUT_DIR" ]; then
  echo "runQuench.sh: no input directory. Set UNBIAS_INPUT_DIR or pass it as the first argument." >&2
  exit 1
fi

# 0. build (no-op if up to date; binaries carry FastJet's lib dir as an rpath)
make

# 1. biased/reference trees + configs
root -l -b -q "driveBinsQuench.C(\"$INPUT_DIR\")"

# 2. unbiasing weights. No pT cut (--pt-min 0): the trees are already selected, and the
#    biased jets have a true pT above the window.
for B in "100 200" "150 250"; do
  set -- $B; LO=$1; HI=$2
  ./unbias_weights \
    --input $FILES/startBasis_${LO}_${HI}_${T}.root --tree tBiased \
    --target-input $FILES/startBasis_${LO}_${HI}_${T}.root --target-tree tRef \
    --config $FILES/unbiasing_config_${LO}_${HI}_${T}.env \
    --pt-min 0 --pt-max 1e9 --subjet-source fastjet \
    --out $FILES/unbias_weights_${LO}_${HI}_${T}.root --mode run ${FIT_ARGS}
done

# 3. validation plots (file names get _<LO>_<HI>_quench_<set>). The pT histograms reach
#    HI + 200 GeV, where the biased jets' true pT lies (and the jets of the substructure plots).
SUBSTRUCTURE_BINNING="--mass-bins 30 --rg-bins 13 --kt-bins 20 --broadening-bins 25 --tau-bins 25"
for B in "100 200" "150 250"; do
  set -- $B; LO=$1; HI=$2
  ./plot_unbias_weights \
    --input $FILES/unbias_weights_${LO}_${HI}_${T}.root \
    --target-input $FILES/startBasis_${LO}_${HI}_${T}.root --target-tree tRef \
    --config $FILES/unbiasing_config_${LO}_${HI}_${T}.env \
    --pt-min $LO --pt-max $HI --pt-hist-min $((LO - 10)) --pt-hist-max $((HI + 200)) --nbins 92 --tag $T \
    --jet-R 0.4 --sd-zcut 0.1 --sd-beta 1.0 --nsub-beta 1.0 \
    ${SUBSTRUCTURE_BINNING} \
    --plot-dir $ROOT_DIR --formats $ROOT_FORMATS \
    --out $FILES/unbias_weights_plots_${LO}_${HI}_${T}.root
  if [ -n "$MPL_DIR" ]; then
    $PY plot_mpl.py unbias \
        --hists $FILES/unbias_weights_plots_${LO}_${HI}_${T}.root \
        --weights $FILES/unbias_weights_${LO}_${HI}_${T}.root \
        --pt-min $LO --pt-max $HI --tag $T --target-tree tRef --plot-dir $MPL_DIR
  fi
done
