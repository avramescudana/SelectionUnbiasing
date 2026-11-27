# Where and how the pT bias is applied

**Where:** `startBasis()` in `AnalysisCode/startBasis.C`, in the event loop (search for `biasWeight`).

**How:** every jet with $p_{T,\min} < p_T < p_{T,\max}$ enters both as an X jet and as a Y jet, and the unbiased reference sample `tRef` is X ∪ Y.
The biased sample `tBiased` is built in two steps.

1. **Downsample X.** Each X jet is kept with probability

   $$w_\mathrm{bias}(p_T)=\frac{p_{T,\min}^{p}-d\,p_{T,\max}^{p}+(d-1)\,p_T^{p}}{p_{T,\min}^{p}-p_{T,\max}^{p}},$$

   so $w_\mathrm{bias}(p_{T,\min})=d$ and $w_\mathrm{bias}(p_{T,\max})=1$. Low-pT jets are suppressed and the default parameters are $p=8$, $d=0.1$.
   A jet is kept if a uniform random number $u\in[0,1)$ satisfies $u<w_\mathrm{bias}(p_T)$.

2. **Oversample Y to Y′.** $N_{Y'}=N_X+N_Y-N_{X,\mathrm{kept}}$ Y jets are drawn uniformly with replacement,
   so the biased sample has the same size as the reference. `tBiased` = X (kept) ∪ Y′.

Expected shape: with $k=\langle w_\mathrm{bias}\rangle_X$, the ratio biased/unbiased is $\big(w_\mathrm{bias}(p_T)+2-k\big)/2$.

Plot it with `plot_pt_bias.ipynb`.

# Alternative: log-normal bias

**Where:** `lognormalBiasWeight(pt, mu, sigma, norm)` in `AnalysisCode/startBasis.C` (above `startBasis()`), called from the same `biasWeight` line in the event loop.

**New `startBasis()` arguments** (appended last, after `plotDir`, so existing calls such as `driveBins.C` are unaffected):

| argument | default | meaning |
|---|---|---|
| `biasType` | `0` | `0` = original power-law bias (`p`, `d`); `1` = log-normal |
| `lnMu` | `4.733` | $\mu$, mean of $\ln(p_T/\mathrm{GeV})$ |
| `lnSigma` | `0.12` | $\sigma$, width of $\ln(p_T/\mathrm{GeV})$ |
| `lnNorm` | `1.0` | amplitude $c\in(0,1]$ of the keep probability |

With `biasType=1`, `p` and `d` are ignored. Y′ oversampling is unchanged.

**How the reference and biased samples are built** (`fillMode`, the last argument of `startBasis()`, for both bias shapes, set with `UNBIAS_FILL`).
"Kept" means kept with probability $w_\mathrm{bias}(p_T)$; $k$ is the fraction kept.

- **Partly biased** (`fillMode` 0, default, tag `<TAG>`): every jet is both an X and a Y jet. Reference = X ∪ Y; biased = X (kept) ∪ Y′, with Y′ drawn from Y up to the reference size. Only a fraction $k/2$ of the biased sample carries the bias. Biased/unbiased $=(w_\mathrm{bias}+2-k)/2$, exact weights $\propto 2/(w_\mathrm{bias}+2-k)$.
- **Fully biased** (`fillMode` 1, tag `<TAG>_downsampled`): reference = all jets; biased = only the kept jets, none repeated, so it is about $k$ times smaller. Biased/unbiased $=w_\mathrm{bias}/k$, exact weights $\propto 1/w_\mathrm{bias}$.

The exact weights depend only on $p_T$. The `tWeights` tree (class weights of the notes' Eq. 5.1) is written only for partly biased, and is not the exact weight for a $p_T$-dependent bias.
The fit does not need equal sample sizes (the targets are averages and the weights have mean 1). For the plots, `plot_unbias_weights` and the startBasis plots scale the biased and reweighted histograms to the reference's integral (a factor 1 when partly biased).
Give each mode its own `UNBIAS_TAG`, or its outputs overwrite the other's.

Earlier tests, kept on disk only: `<TAG>_resampled` (fill-up by resampling the kept X jets), `<TAG>_notes` (X and Y disjoint halves) and `<TAG>_split` (reference and biased sample from independent halves). They agreed with partly biased (`_notes`) and fully biased (`_resampled`, `_split`) within errors.

**Functional form:** the normalised log-normal pdf

$$\bar D(p_T)=\frac{1}{\sqrt{2\pi}\,\sigma\,p_T}\exp\!\left[-\frac{(\ln p_T-\mu)^2}{2\sigma^2}\right]$$

is divided by its value at its peak (mode) $p_T^\mathrm{peak}=e^{\mu-\sigma^2}$, the pT where a jet is most likely to be kept (not the mean jet pT), so the maximum is 1, and multiplied by $c$:

$$w_\mathrm{bias}(p_T)=c\,\frac{\bar D(p_T)}{\bar D(p_T^\mathrm{peak})}
=c\,\exp\!\left[-\frac{(\ln p_T-\mu)^2}{2\sigma^2}-\ln p_T+\mu-\frac{\sigma^2}{2}\right].$$

**Normalisation $c$:** the shape of the kept X jets does not depend on $c$, but the strength does. The expected ratio is $(w_\mathrm{bias}+2-k)/2$ with $k=\langle w_\mathrm{bias}\rangle_X$, so its deviation from 1 scales with $c$. $c=1$ is the strongest bias for a given shape; smaller $c$ weakens it. $w_\mathrm{bias}$ is capped at 1.

**Example** (from `AnalysisCode/`, one bin by hand):

```bash
root -l -b -q 'startBasis.C("/path/to/MCOutput/", "../Output/files/out.root", 100, 140, 8.0, 0.1, 12345, -1, "../Output/files/cfg.env", "0.10,0.20", "eec_terms_example.csv", "../Output/plots", 1, 4.733, 0.12, 1.0)'
```

## Running the full log-normal chain

```bash
./runLognormal.sh          # uses $UNBIAS_INPUT_DIR, or: ./runLognormal.sh /path/to/MCOutput/
```

It builds (`make`), runs `driveBinsLognormal.C`, then `unbias_weights` and `plot_unbias_weights` (with `--tag lognormal_<set>_<TAG>`) for both pT bins.
Every output carries `_lognormal_<set>` in its name, so it never overwrites the power-law outputs or the other parameter set: ROOT and config files in `Output/files/`, plots in `Output/plots/`.

## `driveBinsLognormal.C`

Driver macro: calls `startBasis()` once per pT bin with `biasType=1`, writing `startBasis_<bin>_lognormal_<set>_<TAG>.root` and `unbiasing_config_<bin>_lognormal_<set>_<TAG>.env` to `Output/files/`. Input directory as for `driveBins.C`: argument or `$UNBIAS_INPUT_DIR`.

The parameters come from a named set, chosen with `UNBIAS_LN_PARAMS` (default `params1`). `lnMu` is set per bin, so the bias acts inside each bin; with one shared value, the 140–180 bin would see almost no bias. The curve peaks at $e^{\mu-\sigma^2}$:

| set | 100–140 | 140–180 | `sigma` | `norm` | effect on the mean jet pT |
|---|---|---|---|---|---|
| `params1` | peak 112 GeV (`mu=4.733`) | peak 152 GeV (`mu=5.0383`) | 0.12 | 1 | peak below the unbiased mean, the biased mean goes down |
| `params2` | peak 128 GeV (`mu=4.85563`) | peak 168 GeV (`mu=5.12756`) | 0.06 | 1 | narrower, peak above the unbiased mean, the biased mean goes up |

Single values can still be overridden on top of the set with `UNBIAS_LN_SIGMA`, `UNBIAS_LN_NORM`, `UNBIAS_LN_PEAK1/2`. Example, `params2` fully biased:

```bash
UNBIAS_LN_PARAMS=params2 UNBIAS_FILL=1 UNBIAS_TAG=1M_downsampled ./runLognormal.sh /path/to/MCOutput/
```

## Plot

`plot_pt_bias.ipynb` (needs `uproot`, `numpy`, `matplotlib`) plots the unbiased and biased pT spectra and their ratio against the expected curve: first for the power-law output of `driveBins.C`, then (section "Log-normal bias") for the 100–140 output of `driveBinsLognormal.C`.
Its `LN_PARAMS` picks the parameter set; the values in its `LN_SETS` are typed by hand and must match the table above. Plots are saved to `Output/plots/`.

# Jet quenching log-normal bias

**Where:** `startBasis()` with `biasType = 2` (block "Quenching" in the event loop), run with `runQuench.sh` (driver `driveBinsQuench.C`).

**How:** every jet of the whole spectrum ($|\eta|<2.4$) loses an energy $\epsilon$, with $\bar\epsilon=\epsilon/\omega_c$ drawn from the log-normal of [Arleo, JHEP 11 (2002) 044](https://arxiv.org/abs/hep-ph/0210104), Eq. (3.7):

$$\bar D(\bar\epsilon)=\frac{1}{\sqrt{2\pi}\,\sigma\,\bar\epsilon}\exp\!\left[-\frac{(\ln\bar\epsilon-\mu)^2}{2\sigma^2}\right],\qquad \langle\epsilon\rangle=\omega_c\,e^{\mu+\sigma^2/2}.$$

Then a pT window is cut (100–200 and 150–250 GeV):

- **Reference** `tRef`: jets with the true pT in the window.
- **Biased** `tBiased`: jets with the quenched $p_T-\epsilon$ in the window, stored with their true pT and unchanged substructure (`pt_shifted` = quenched pT). They come from higher pT, so the biased mean pT goes up.

The two samples overlap and differ in size; `fillMode` is ignored. The same $\epsilon$ distribution is used for quark and gluon jets.

**Parameters** (Table 1 of the paper), set with `UNBIAS_QUENCH_PARAMS`:

| set | $\mu$ | $\sigma$ | $\langle\epsilon\rangle$ |
|---|---|---|---|
| `out` (default, outgoing quark) | −1.5 | 0.73 | $0.3\,\omega_c$ |
| `in` (incoming quark) | −2.55 | 0.57 | $0.1\,\omega_c$ |

and (`UNBIAS_QUENCH_OMEGAC`, in GeV)

$$\omega_c=\tfrac{1}{2}\hat q L^2=62.5~\mathrm{GeV},\qquad \hat q=5~\mathrm{GeV/fm^2},\quad L=5~\mathrm{fm}.$$

**Running:** the input needs a high pTHat range (`UNBIAS_PTHAT_MIN/MAX` in `generate.sh`, default 50/200). `runQuench.sh` fits with no pT cut and plots the pT histograms up to the window's upper edge + 200 GeV, so the biased jets above the window are included.

```bash
UNBIAS_PTHAT_MIN=60 UNBIAS_PTHAT_MAX=600 ../PYTHIA/generate.sh 4000000 16 ../Output/MCOutput_4M_hipt
UNBIAS_TAG=4M_hipt UNBIAS_QUENCH_PARAMS=out ./runQuench.sh ../Output/MCOutput_4M_hipt/
```

The mean-pT comparison is in `compare_bias.ipynb`, section "Mean jet pT: quenching bias".
