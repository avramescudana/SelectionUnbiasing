#!/bin/bash
# Validate the unbiasing with pT / EEC / basis-vector / jet-substructure
# comparison plots, at paper-figure quality (larger fonts, proper
# statistical error bars, configurable basis-panel selection), for both pT
# bins.
#
# Build (NOTE: plot_unbias_weights.cc now requires linking FastJet too --
# jet_observables.h's Soft Drop groomer uses fastjet::ClusterSequence --
# unlike before, ROOT flags alone are no longer enough):
#   c++ -std=c++17 -O2 plot_unbias_weights.cc \
#       $(root-config --cflags --libs) \
#       $(/data/ALEPH/MC/mcgen/fastjet-install/bin/fastjet-config --cxxflags --libs) \
#       -o plot_unbias_weights
# or just `make plot_unbias_weights` (see Makefile). To also enable
# the N-subjettiness (tau1/tau2/tau3/tau21/tau32) crosscheck, which needs
# fastjet-contrib's separate Nsubjettiness package, build with
# NSUB_FLAGS="-DJET_OBS_HAVE_NSUBJETTINESS -I<fjcontrib include> -L<fjcontrib lib> -lNsubjettiness"
# -- otherwise those five plots are simply skipped.
#
# NOTE: uses each bin's own shared "unbiasing_config_<bin>.env" TEnv file
# (written by startBasis.C / driveBins.C, consumed by
# unbias_weights.cc) so the EEC normalization in these diagnostic plots
# always matches what that bin's fit used.
#
# NOTE: every plot below is labeled with its pT bin (e.g. "100 < p_{T} < 140
# GeV", drawn directly on the canvas) and every saved file name carries the
# bin as a "_<ptmin>_<ptmax>" suffix (e.g. plot_unbias_pT_check_100_140.pdf),
# derived automatically from --pt-min/--pt-max -- no extra flag needed, so
# the label can never drift from the window actually plotted.
#
# NOTE: jet-substructure crosscheck plots (jet mass, R_g, k_T, tau_N,
# broadening -- see plot_unbias_weights.cc's "JET-SUBSTRUCTURE CROSSCHECKS"
# doc comment) use --jet-R (default 0.4, the README's stated PYTHIA jet
# radius), --sd-zcut/--sd-beta (Soft Drop grooming, default 0.1/1.0), and
# --nsub-beta (N-subjettiness, default 1.0). Pass them explicitly below to
# override.
#
# NOTE (2026-10-22): the jet-substructure crosscheck histograms use COARSER
# binning than the plot_unbias_weights.cc defaults (roughly half the bins),
# because the default binning left many bins with large statistical error
# bars. Same axis ranges as before; only the bin counts change:
#   --mass-bins       60 -> 30
#   --rg-bins         25 -> 13
#   --kt-bins         40 -> 20
#   --broadening-bins 50 -> 25
#   --tau-bins        50 -> 25   (only used if built with Nsubjettiness)
# Edit SUBSTRUCTURE_BINNING below to change them for both bins at once.
SUBSTRUCTURE_BINNING="--mass-bins 30 --rg-bins 13 --kt-bins 20 --broadening-bins 25 --tau-bins 25"

# ---- Bin 1: 100-140 GeV ----
./plot_unbias_weights \
    --input unbias_weights_100_140.root \
    --target-input startBasis_100_140.root \
    --target-tree tRef \
    --config unbiasing_config_100_140.env \
    --pt-min 100 --pt-max 140 \
    --jet-R 0.4 --sd-zcut 0.1 --sd-beta 1.0 --nsub-beta 1.0 \
    ${SUBSTRUCTURE_BINNING} \
    --out unbias_weights_plots_100_140.root

# ---- Bin 2: 140-180 GeV ----
./plot_unbias_weights \
    --input unbias_weights_140_180.root \
    --target-input startBasis_140_180.root \
    --target-tree tRef \
    --config unbiasing_config_140_180.env \
    --pt-min 140 --pt-max 180 \
    --jet-R 0.4 --sd-zcut 0.1 --sd-beta 1.0 --nsub-beta 1.0 \
    ${SUBSTRUCTURE_BINNING} \
    --out unbias_weights_plots_140_180.root

# Example paper-figure run instead (bigger text, PDF+PNG, a curated subset
# of basis panels, and explicit jet-substructure crosscheck parameters) --
# uncomment / adapt as needed, per bin:
#
# ./plot_unbias_weights \
#     --input unbias_weights_100_140.root \
#     --target-input startBasis_100_140.root \
#     --target-tree tRef \
#     --config unbiasing_config_100_140.env \
#     --pt-min 100 --pt-max 140 \
#     --label-size 0.050 --title-size 0.058 --legend-size 0.044 \
#     --line-width 3 --marker-size 1.3 \
#     --formats pdf,png \
#     --basis-filter "sjmom_R0.10" --basis-ncol 3 --basis-nbins 25 \
#     --jet-R 0.4 --sd-zcut 0.1 --sd-beta 1.0 --nsub-beta 1.0 \
#     ${SUBSTRUCTURE_BINNING}