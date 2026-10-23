// jet_observables.h
// =====================================================================
// Self-contained crosscheck-only jet-substructure observables, used by
// plot_unbias_weights.cc to test closure of the unbiasing weights on
// quantities that are NOT part of the fit basis (basis_functions.h).
// Nothing here feeds unbias_weights.cc's optimisation.
//
// All of these consume the same massless (pt,eta,phi) constituent
// convention and E-scheme recombination already used throughout this
// codebase (see subjet_basis.h).
//
// Provided:
//   (1) soft_drop(...)      -- groomed radius R_g and groomed-splitting
//                              k_T, via a plain fastjet::ClusterSequence
//                              (Cambridge/Aachen) + hand-written recursive
//                              Soft Drop declustering. Needs ONLY base
//                              FastJet (already a hard dependency via
//                              subjet_basis.h) -- no fastjet-contrib.
//   (2) nsubjettiness(...)  -- tau_1, tau_2, tau_3 with one-pass
//                              winner-take-all k_T axes, the NORMALIZED
//                              measure (dimensionless; beta exponent;
//                              R0 = the jet radius). This genuinely
//                              requires fastjet-contrib's Nsubjettiness
//                              package (separate install from base
//                              FastJet). Guarded by the
//                              JET_OBS_HAVE_NSUBJETTINESS macro: define
//                              it (and link -lNsubjettiness) to enable;
//                              otherwise this function is simply not
//                              compiled, and callers must guard their own
//                              use of it the same way (see
//                              plot_unbias_weights.cc).
//   (3) broadening(...)     -- the "width"/"girth"/"broadening" form of
//                              the generalised angularities, i.e.
//                              lambda_beta^kappa (Eq. 5.11 of the
//                              attached reference) with beta=1, kappa=1:
//                                lambda_1 = Sum_i z_i * (DeltaR_i,jet / R)
//                              using the STORED jet axis (eta,phi) --
//                              never a axis re-derived from constituents
//                              -- and a jet radius R supplied by the
//                              caller. This file makes no assumption
//                              about what R is; it must be passed in.
// =====================================================================
#pragma once

#include <TVector2.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include <fastjet/ClusterSequence.hh>
#include <fastjet/PseudoJet.hh>

#ifdef JET_OBS_HAVE_NSUBJETTINESS
#include "fastjet/contrib/Nsubjettiness.hh"
#endif

namespace jetobs {

// Massless 4-vector from (pt, eta, phi), same convention as
// subjet_basis.h's make_pseudo().
inline fastjet::PseudoJet make_pj(double pt, double eta, double phi) {
    const double px = pt * std::cos(phi);
    const double py = pt * std::sin(phi);
    const double pz = pt * std::sinh(eta);
    const double E  = pt * std::cosh(eta);   // massless constituent
    return fastjet::PseudoJet(px, py, pz, E);
}

// ---------------------------------------------------------------------
// Soft Drop: groomed radius R_g and groomed-splitting k_T.
//
//   Soft Drop condition:  z > zcut * (DeltaR12 / R0)^beta
//   where z = pt_softer / (pt_softer + pt_harder) at each declustering
//   step, walking down from the jet as a whole, recursing into the
//   harder branch at each step that fails the condition (the standard
//   SoftDrop/mMDT recursive procedure).
//
// R0 is the ORIGINAL jet radius (a physical input, supplied by the
// caller -- this file never guesses it); it enters only the angular
// exponent above. The radius used to RECLUSTER the jet's own
// constituents into a single Cambridge/Aachen merger tree in the first
// place is a separate, purely technical parameter with no physical
// meaning: it only needs to be large enough that all of the jet's
// constituents end up back in one combined object, which is already
// guaranteed by construction (they came from one jet). Standard Soft
// Drop implementations handle this by reclustering at a large,
// non-physical radius; we do the same below.
//
// If the Soft Drop condition is never satisfied (the jet grooms away to
// a single particle), or the jet has fewer than 2 constituents, returns
// groomed_away = true (Rg = kT = -1), so callers can distinguish that
// from a genuine Rg = 0.
// ---------------------------------------------------------------------
struct SoftDropResult {
    double Rg = -1.0;
    double kT = -1.0;
    bool   groomed_away = true;
};

inline SoftDropResult soft_drop(const std::vector<double> &cpt,
                                const std::vector<double> &ceta,
                                const std::vector<double> &cphi,
                                double R0, double zcut, double beta) {
    SoftDropResult res;
    std::vector<fastjet::PseudoJet> constituents;
    const size_t nc = cpt.size();
    constituents.reserve(nc);
    for (size_t i = 0; i < nc; ++i) {
        if (cpt[i] <= 0.0) continue;
        if (i >= ceta.size() || i >= cphi.size()) break;
        constituents.push_back(make_pj(cpt[i], ceta[i], cphi[i]));
    }
    if (constituents.size() < 2) return res;   // nothing to decluster

    // Purely technical reclustering radius -- see comment above. Large
    // enough to guarantee the jet's own constituents merge into one
    // object regardless of R0.
    const double kReclusterR = std::max(3.0, 4.0 * R0);
    const fastjet::JetDefinition jet_def(fastjet::cambridge_algorithm, kReclusterR);
    fastjet::ClusterSequence cs(constituents, jet_def);
    const std::vector<fastjet::PseudoJet> incl = cs.inclusive_jets();
    if (incl.empty()) return res;

    // Hardest inclusive jet (should be the only one at this radius).
    fastjet::PseudoJet cur = incl[0];
    for (const auto &j : incl) if (j.pt() > cur.pt()) cur = j;

    // ---- manual recursive Soft Drop declustering ----
    fastjet::PseudoJet j1, j2;
    while (cur.has_parents(j1, j2)) {
        if (j2.pt() > j1.pt()) std::swap(j1, j2);   // j1 = harder branch
        const double dR    = j1.delta_R(j2);
        const double ptsum = j1.pt() + j2.pt();
        const double z     = (ptsum > 0.0) ? (j2.pt() / ptsum) : 0.0;
        const double sd_cut = zcut * std::pow(dR / R0, beta);
        if (z > sd_cut) {
            res.Rg = dR;
            res.kT = j2.pt() * dR;   // kT of the groomed (softer) branch
            res.groomed_away = false;
            return res;
        }
        cur = j1;   // recurse into the harder branch
    }
    return res;   // Soft Drop condition never satisfied -> groomed away
}

#ifdef JET_OBS_HAVE_NSUBJETTINESS
// ---------------------------------------------------------------------
// N-subjettiness: tau_1, tau_2, tau_3 with one-pass winner-take-all k_T
// axes and the NORMALIZED measure (dimensionless; beta exponent; R0 =
// the jet radius, supplied by the caller). Requires fastjet-contrib's
// Nsubjettiness package -- only compiled when JET_OBS_HAVE_NSUBJETTINESS
// is defined (see top-of-file notes and plot_unbias_weights.cc).
// ---------------------------------------------------------------------
struct NSubResult {
    double tau1 = -1.0;
    double tau2 = -1.0;
    double tau3 = -1.0;
};

inline NSubResult nsubjettiness(const std::vector<double> &cpt,
                                const std::vector<double> &ceta,
                                const std::vector<double> &cphi,
                                double R0, double beta) {
    NSubResult res;
    std::vector<fastjet::PseudoJet> constituents;
    const size_t nc = cpt.size();
    constituents.reserve(nc);
    for (size_t i = 0; i < nc; ++i) {
        if (cpt[i] <= 0.0) continue;
        if (i >= ceta.size() || i >= cphi.size()) break;
        constituents.push_back(make_pj(cpt[i], ceta[i], cphi[i]));
    }
    if (constituents.empty()) return res;

    fastjet::contrib::OnePass_WTA_KT_Axes axes;
    fastjet::contrib::NormalizedMeasure measure(beta, R0);
    fastjet::contrib::Njettiness nsub(axes, measure);

    res.tau1 = nsub.getTau(1, constituents);
    res.tau2 = nsub.getTau(2, constituents);
    res.tau3 = nsub.getTau(3, constituents);
    return res;
}
#endif  // JET_OBS_HAVE_NSUBJETTINESS

// ---------------------------------------------------------------------
// Broadening / width: generalised angularity lambda_beta^kappa
// (Eq. 5.11, attached reference) with beta=1, kappa=1:
//
//   lambda_1 = Sum_i z_i * (DeltaR_i,jet / R) ,
//   z_i = pt_i / Sum_j pt_j ,
//   DeltaR_i,jet = sqrt( (eta_i - eta_jet)^2 + Dphi(phi_i, phi_jet)^2 ) .
//
// jet_eta/jet_phi MUST be the stored jet axis (same provenance as the
// stored jet mass -- see plot_unbias_weights.cc), never an axis
// re-derived from the constituents here. R is the jet radius, supplied
// by the caller; this file makes no assumption about its value.
// ---------------------------------------------------------------------
inline double broadening(const std::vector<double> &cpt,
                         const std::vector<double> &ceta,
                         const std::vector<double> &cphi,
                         double jet_eta, double jet_phi, double R) {
    const size_t nc = cpt.size();
    double sum_pt = 0.0;
    for (size_t i = 0; i < nc; ++i) if (cpt[i] > 0.0) sum_pt += cpt[i];
    if (sum_pt <= 0.0 || R <= 0.0) return 0.0;

    double lam = 0.0;
    for (size_t i = 0; i < nc; ++i) {
        if (cpt[i] <= 0.0) continue;
        if (i >= ceta.size() || i >= cphi.size()) break;
        const double deta = ceta[i] - jet_eta;
        const double dphi = TVector2::Phi_mpi_pi(cphi[i] - jet_phi);
        const double dR = std::sqrt(deta * deta + dphi * dphi);
        const double z = cpt[i] / sum_pt;
        lam += z * (dR / R);
    }
    return lam;
}

}  // namespace jetobs