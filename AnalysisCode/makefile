CXX      := c++
CXXSTD   := -std=c++17 -O2

FASTJET_CONFIG := /data/ALEPH/MC/mcgen/fastjet-install/bin/fastjet-config

ROOT_FLAGS    := $(shell root-config --cflags --libs)
FASTJET_FLAGS := $(shell $(FASTJET_CONFIG) --cxxflags --libs)

# Optional fastjet-contrib Nsubjettiness support for
# plot_unbias_weights.cc's tau1/tau2/tau3/tau21/tau32 crosscheck plots.
# Empty by default -- without it, plot_unbias_weights builds and runs fine,
# it just skips those five plots (see jet_observables.h /
# plot_unbias_weights.cc's JET_OBS_HAVE_NSUBJETTINESS guard). To enable,
# override on the command line, e.g.:
#   make plot_unbias_weights \
#     NSUB_FLAGS="-DJET_OBS_HAVE_NSUBJETTINESS -I/path/to/fjcontrib-install/include -L/path/to/fjcontrib-install/lib -lNsubjettiness"
NSUB_FLAGS ?=

.PHONY: all clean

all: unbias_weights plot_unbias_weights

unbias_weights: unbias_weights.cc subjet_basis.h basis_functions.h
	$(CXX) $(CXXSTD) unbias_weights.cc $(ROOT_FLAGS) $(FASTJET_FLAGS) -o unbias_weights

# plot_unbias_weights.cc now also requires FastJet (jet_observables.h's
# Soft Drop groomer uses fastjet::ClusterSequence), unlike before.
plot_unbias_weights: plot_unbias_weights.cc basis_functions.h jet_observables.h
	$(CXX) $(CXXSTD) plot_unbias_weights.cc $(ROOT_FLAGS) $(FASTJET_FLAGS) $(NSUB_FLAGS) -o plot_unbias_weights

clean:
	rm -f unbias_weights plot_unbias_weights