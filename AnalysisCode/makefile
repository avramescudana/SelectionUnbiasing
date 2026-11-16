CXX      := c++
CXXSTD   := -std=c++17 -O2

# Locating fastjet-config (first match wins):
#   1. make FASTJET_CONFIG=/path/to/fastjet-install/bin/fastjet-config
#   2. $FJI (the FastJet install prefix): $FJI/bin/fastjet-config
#   3. fastjet-config on PATH
FASTJET_CONFIG ?= $(if $(FJI),$(FJI)/bin/fastjet-config,$(shell which fastjet-config 2>/dev/null))
ifeq ($(strip $(FASTJET_CONFIG)),)
$(error fastjet-config not found: set FJI=/path/to/fastjet-install, put fastjet-config on PATH, or pass FASTJET_CONFIG=...)
endif

ROOT_FLAGS    := $(shell root-config --cflags --libs)
# The rpath bakes FastJet's lib dir into the binary, so no LD_LIBRARY_PATH /
# DYLD_LIBRARY_PATH is needed at run time and a different FastJet version
# (e.g. one shipped inside a conda env) cannot be picked up by mistake.
# NOTE: FASTJET_FLAGS must come BEFORE ROOT_FLAGS on the compile lines below, so that
# our FastJet headers, -L and rpath win over a (different) FastJet inside the ROOT/conda env.
# Some fastjet-config builds already emit this rpath in --libs; add it only if missing
# (avoids a "duplicate -rpath" linker warning).
FASTJET_LIBDIR := $(shell $(FASTJET_CONFIG) --prefix)/lib
FASTJET_FLAGS  := $(shell $(FASTJET_CONFIG) --cxxflags --libs)
ifeq ($(findstring -rpath,$(FASTJET_FLAGS)),)
FASTJET_FLAGS  += -Wl,-rpath,$(FASTJET_LIBDIR)
endif

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
	$(CXX) $(CXXSTD) unbias_weights.cc $(FASTJET_FLAGS) $(ROOT_FLAGS) -o unbias_weights

# plot_unbias_weights.cc now also requires FastJet (jet_observables.h's
# Soft Drop groomer uses fastjet::ClusterSequence), unlike before.
plot_unbias_weights: plot_unbias_weights.cc basis_functions.h jet_observables.h
	$(CXX) $(CXXSTD) plot_unbias_weights.cc $(FASTJET_FLAGS) $(ROOT_FLAGS) $(NSUB_FLAGS) -o plot_unbias_weights

clean:
	rm -f unbias_weights plot_unbias_weights