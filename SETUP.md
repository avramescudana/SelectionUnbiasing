# Setup on a new machine

Everything below runs from `AnalysisCode/` unless stated otherwise.

## 1. Dependencies

ROOT 6, FastJet 3 (with `fastjet-config`), a C++17 compiler, PYTHIA 8 (only to generate test input),
and for the Python notebooks `numpy`, `matplotlib`, `uproot` and `ipykernel`.

Example with conda (macOS or Linux):

```bash
conda create -n unbias -c conda-forge --strict-channel-priority \
    root fastjet compilers python=3.12 matplotlib numpy uproot ipykernel pythia8
```

To add the notebook packages to an existing env: `conda install -n unbias -c conda-forge uproot ipykernel`.

The conda FastJet package may not ship `fastjet-config`. If so, build FastJet from source:

```bash
curl -LO https://fastjet.fr/repo/fastjet-3.5.0.tar.gz
tar xzf fastjet-3.5.0.tar.gz && cd fastjet-3.5.0
./configure --prefix=$HOME/fastjet-install --enable-cgal=no --disable-auto-ptr
make -j8 && make install
```

## 2. Environment (once per shell)

```bash
conda activate unbias                     # ROOT, compilers, PYTHIA (sets $CONDA_PREFIX, PYTHIA8DATA)
export FJI=$HOME/fastjet-install          # FastJet install prefix (contains bin/fastjet-config)
export UNBIAS_INPUT_DIR=/path/to/MCOutput/ # directory with the input .root files
cd AnalysisCode
```

If the env was created with `-p /some/path` (or moved), conda does not know it by name: activate it
by path instead, `conda activate /some/path/unbias`.

In a script or other non-interactive shell, `conda activate` may not work. Then put the env's `bin/`
first on `PATH` instead; that is enough for building and running the analysis (not for generating
PYTHIA test input, which needs the activated env):

```bash
export PATH=/path/to/conda-env/bin:$PATH
```

Do not set `LD_LIBRARY_PATH`/`DYLD_LIBRARY_PATH` for the build: `make` records FastJet's library
directory in the binaries (rpath), so a second FastJet (e.g. inside the conda env) is not picked up
by mistake. That mix-up causes a segfault on macOS.

## 3. Build

```bash
make        # builds unbias_weights and plot_unbias_weights
```

`make` finds `fastjet-config` via `$FJI`, then `PATH`; override with `make FASTJET_CONFIG=/path/to/fastjet-config`.

## 4. Run

```bash
root -l -b -q driveBins.C      # builds the toy biased/reference samples (uses $UNBIAS_INPUT_DIR)
./runUnbiasing.sh              # computes the weights
./runPlotUnbiasedWeights.sh    # validation plots
```

`driveBins.C` aborts with a message if the input directory is missing or contains no entries.
Every `.root` file in that directory is read, so keep unrelated ROOT files out of it.

All outputs go to `Output/` at the top of the repository (created automatically, git-ignored):
ROOT, config and CSV files in `Output/files/`, plots in `Output/plots/`.

## Test input (no production data)

`PYTHIA/ppjets_root.cc` generates a PYTHIA sample. Set `nEvent` and the output file name inside it first
(they are hard-coded). 50 000 events is enough for a smoke test; use ~500 000 for usable statistics.
Run from the repository root, with the conda env activated (for `$CONDA_PREFIX`):

```bash
c++ -std=c++17 -O2 PYTHIA/ppjets_root.cc -o ppjets \
    $($FJI/bin/fastjet-config --cxxflags --libs) -Wl,-rpath,$FJI/lib \
    $(root-config --cflags --libs) \
    -I$CONDA_PREFIX/include -L$CONDA_PREFIX/lib -lpythia8 -Wl,-rpath,$CONDA_PREFIX/lib
export PYTHIA8DATA=$CONDA_PREFIX/share/Pythia8/xmldoc
./ppjets -b
```

Then move the output `.root` file into its own directory and point `UNBIAS_INPUT_DIR` at it.

## Notebooks (Jupyter / VS Code)

The notebooks (e.g. `AnalysisCode/plot_pt_bias.ipynb`) run in the conda env. Register it once as a
Jupyter kernel:

```bash
conda activate unbias      # or: conda activate /path/to/unbias
python -m ipykernel install --user --name unbias --display-name "Python (unbias)"
```

In VS Code (with the Python and Jupyter extensions): open the notebook, click **Select Kernel**
(top right) → **Jupyter Kernel…** → **Python (unbias)**. In Jupyter Lab/Notebook, pick
**Python (unbias)** from the kernel menu. The notebooks read from `../Output/files/`, so run the
corresponding chain first (`driveBins.C` / `driveBinsLognormal.C`).
