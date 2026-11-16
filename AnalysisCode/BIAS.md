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

**Functional form:** the normalised log-normal pdf

$$\bar D(p_T)=\frac{1}{\sqrt{2\pi}\,\sigma\,p_T}\exp\!\left[-\frac{(\ln p_T-\mu)^2}{2\sigma^2}\right]$$

is divided by its value at the mode $p_T^*=e^{\mu-\sigma^2}$, so the maximum is 1, and multiplied by $c$:

$$w_\mathrm{bias}(p_T)=c\,\frac{\bar D(p_T)}{\bar D(p_T^*)}
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

It builds (`make`), runs `driveBinsLognormal.C`, then `unbias_weights` and `plot_unbias_weights` (with `--tag lognormal`) for both pT bins.
Every output carries `_lognormal` in its name, so it never overwrites the power-law outputs: ROOT and config files in `Output/files/`, plots in `Output/plots/`.

## `driveBinsLognormal.C`

Driver macro: calls `startBasis()` once per pT bin with `biasType=1`, writing `startBasis_<bin>_lognormal.root` and `unbiasing_config_<bin>_lognormal.env` to `Output/files/`. Input directory as for `driveBins.C`: argument or `$UNBIAS_INPUT_DIR`.

`lnMu` and `lnSigma` are set per bin. The curve peaks at $e^{\mu-\sigma^2}$: 112 GeV for 100–140 (`mu=4.733`) and 152 GeV for 140–180 (`mu=5.0383`), both with `sigma=0.12`, so the bias acts inside each bin. With one shared pair of values, the 140–180 bin would see almost no bias.

## Plot

`plot_pt_bias.ipynb` (needs `uproot`, `numpy`, `matplotlib`) plots the unbiased and biased pT spectra and their ratio against the expected curve: first for the power-law output of `driveBins.C`, then (section "Log-normal bias") for the 100–140 output of `driveBinsLognormal.C`.
Its `LN_MU`, `LN_SIGMA`, `LN_NORM` are typed by hand and must match the bin-1 values above. Plots are saved to `Output/plots/`.
