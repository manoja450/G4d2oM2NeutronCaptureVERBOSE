#!/usr/bin/env python
import uproot
import numpy as np
import matplotlib.pyplot as plt
import awkward as ak
import os
import sys
from datetime import datetime
from scipy.optimize import curve_fit
from scipy.stats import norm

# ============================================================
# PLOT STYLE (Thesis / ROOT‑style)
# ============================================================
plt.rcParams.update({
    "font.family": "serif",
    "font.size": 13,
    "axes.labelsize": 17,
    "axes.titlesize": 22,
    "legend.fontsize": 12,
    "xtick.direction": "in",
    "ytick.direction": "in",
    "xtick.top": True,
    "ytick.right": True,
    "axes.linewidth": 1.2,
    "lines.linewidth": 1.5,
    "figure.dpi": 140,
    "figure.figsize": (12.8, 6.2)
})

def exp_decay(t, A, tau, C):
    return A * np.exp(-t / tau) + C

def gauss_with_offset(x, A, mu, sigma, C):
    return A * np.exp(-0.5 * ((x - mu) / sigma) ** 2) + C

# ============================================================
# CONFIGURATION
# ============================================================
MAX_CAPTURES = 50000          # maximum number of captures to process per type
TIME_WINDOW = 0.5             # μs – matching window for electrons
ELECTRON_THRESHOLD = 0.1      # MeV – keep only Compton electrons above this
TIME_MAX = 1600.0             # μs – time histogram upper limit
BIN_WIDTH = 2.0               # μs – bin width for time histogram

# Quality cut for PMT hits
MIN_DISTINCT_PMTS = 2
MIN_TOTAL_HITS = 4

# ============================================================
# FIND ROOT FILE
# ============================================================
root_files = ["Sim_D2ODetector003.root"]
found = None
for f in root_files:
    if os.path.exists(f):
        found = f
        break
if found is None:
    print("No ROOT file found.")
    sys.exit(1)
path = found
print(f"Using: {path}", flush=True)
timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
print(f"Timestamp: {timestamp}", flush=True)

# ============================================================
# LOAD PARTICLE DATA (for capture identification)
# ============================================================
print("Loading particle data...", flush=True)
file = uproot.open(path)
event_data = file["Sim_Tree"]["eventData"]

all_pdg = event_data["allParticlePDG"].array()
all_track = event_data["allParticleTrackID"].array()
all_time = event_data["allParticleTime"].array()
all_energy = event_data["allParticleEnergy"].array()

pdg = ak.to_numpy(ak.flatten(all_pdg))
track = ak.to_numpy(ak.flatten(all_track))
time = ak.to_numpy(ak.flatten(all_time))
energy = ak.to_numpy(ak.flatten(all_energy))

print(f"Total particles: {len(pdg):,}", flush=True)

# ============================================================
# IDENTIFY CAPTURE PRODUCTS: Deuterons (H) and Tritons (D)
# ============================================================
DEUTERON_PDG = 1000010020
TRITIUM_PDG = 1000010030

is_deuteron = (pdg == DEUTERON_PDG)
is_tritium = (pdg == TRITIUM_PDG)

deut_idx = np.where(is_deuteron)[0]
trit_idx = np.where(is_tritium)[0]

print(f"Deuterons (H capture): {len(deut_idx):,}", flush=True)
print(f"Tritons (D capture):   {len(trit_idx):,}", flush=True)

if len(deut_idx) > MAX_CAPTURES:
    deut_idx = deut_idx[:MAX_CAPTURES]
if len(trit_idx) > MAX_CAPTURES:
    trit_idx = trit_idx[:MAX_CAPTURES]

deut_times = time[deut_idx] / 1000.0   # μs
trit_times = time[trit_idx] / 1000.0

print(f"Processing {len(deut_times)} H captures and {len(trit_times)} D captures", flush=True)

# ============================================================
# FIND ELECTRONS (Compton candidates)
# ============================================================
is_electron = (pdg == 11) | (pdg == -11)
electron_idx = np.where(is_electron)[0]
electron_times = time[electron_idx] / 1000.0   # μs
electron_energies = energy[electron_idx]       # MeV

print(f"Total electrons: {len(electron_times):,}", flush=True)

# ============================================================
# MATCH ELECTRONS TO CAPTURES BY TIME (within TIME_WINDOW)
# ============================================================
def match_electrons_to_captures(cap_times, label):
    matched_times = []
    matched_energies = []
    total = len(cap_times)
    print(f"  Matching {total} {label} captures...", flush=True)
    for i, cap_time in enumerate(cap_times):
        if i % 10000 == 0 and i > 0:
            print(f"    Processed {i}/{total}, found {len(matched_times)} electrons", flush=True)
        mask = (electron_times >= cap_time - TIME_WINDOW) & (electron_times <= cap_time + TIME_WINDOW)
        mask = mask & (electron_energies > ELECTRON_THRESHOLD)
        idx = np.where(mask)[0]
        for j in idx:
            matched_times.append(electron_times[j])
            matched_energies.append(electron_energies[j])
    print(f"  Found {len(matched_times)} Compton electrons from {label} captures", flush=True)
    return np.array(matched_times), np.array(matched_energies)

print("\nMatching electrons to H captures...", flush=True)
h_times, h_energies = match_electrons_to_captures(deut_times, "H")

print("Matching electrons to D captures...", flush=True)
d_times, d_energies = match_electrons_to_captures(trit_times, "D")

if len(h_times) == 0 and len(d_times) == 0:
    print("No electrons matched. Try increasing TIME_WINDOW or lowering threshold.", flush=True)
    sys.exit(1)

# ============================================================
# FUNCTION: Perform exponential fits over given ranges (robust)
# ============================================================
def fit_and_plot(ax, times, hist, bin_centers, label, color, ranges, min_points=3):
    """
    Perform exponential fits over specified ranges and plot them.
    ranges: list of (xmin, xmax) tuples.
    min_points: minimum number of bins with non-zero counts required.
    """
    fit_results = {}
    for xmin, xmax in ranges:
        mask = (bin_centers >= xmin) & (bin_centers <= xmax) & (hist > 0)
        n_points = np.sum(mask)
        print(f"  {label} fit {xmin}-{xmax} µs: {n_points} bins with counts > 0", flush=True)
        if n_points < min_points:
            print(f"    Not enough points (need {min_points}), skipping.", flush=True)
            continue
        try:
            popt, _ = curve_fit(exp_decay, bin_centers[mask], hist[mask],
                               p0=[max(hist[mask]), 200, 0],
                               maxfev=5000)
            tau = popt[1]
            fit_results[(xmin, xmax)] = tau
            x_fit = np.linspace(xmin, xmax, 200)
            y_fit = exp_decay(x_fit, *popt)
            ax.plot(x_fit, y_fit, '--', linewidth=2,
                    label=f'{label} fit {xmin}-{xmax} µs: τ={tau:.1f} µs')
            print(f"  {label} fit {xmin}-{xmax} µs: τ = {tau:.1f} µs", flush=True)
        except Exception as e:
            print(f"  Fit failed for {label} {xmin}-{xmax}: {e}", flush=True)
    return fit_results

# ============================================================
# FIGURE 1: COMPTON ELECTRON TIME DISTRIBUTION WITH MULTIPLE FITS
# ============================================================
print("\nGenerating Compton electron time distribution with multiple fits...", flush=True)
fig, ax = plt.subplots()

bins_t = np.arange(0, TIME_MAX + BIN_WIDTH, BIN_WIDTH)
bin_width_t = bins_t[1] - bins_t[0]
bin_centers_t = 0.5 * (bins_t[:-1] + bins_t[1:])

# Histogram for H
if len(h_times) > 0:
    hist_h, _ = np.histogram(h_times, bins=bins_t)
    ax.step(bin_centers_t, hist_h, where='mid', color='#2C3E50', linewidth=1.5,
            label=f'H Capture (n={len(h_times):,})')
    ax.plot(bin_centers_t, hist_h, 'o', color='#2C3E50', markersize=3,
            markerfacecolor='white', markeredgecolor='#2C3E50')
else:
    hist_h = np.zeros(len(bin_centers_t))

# Histogram for D
if len(d_times) > 0:
    hist_d, _ = np.histogram(d_times, bins=bins_t)
    ax.step(bin_centers_t, hist_d, where='mid', color='#E74C3C', linewidth=1.5,
            label=f'D Capture (n={len(d_times):,})')
    ax.plot(bin_centers_t, hist_d, 'o', color='#E74C3C', markersize=3,
            markerfacecolor='white', markeredgecolor='#E74C3C')
else:
    hist_d = np.zeros(len(bin_centers_t))

# ============================================================
# UPDATED FIT RANGES: start from 0 µs and 16 µs
# ============================================================

# Fits for H: start from 0 and 16 µs, up to 800, 1000, 1200 µs
h_ranges = [
    (0, 800), (0, 1000), (0, 1200),
    (16, 800), (16, 1000), (16, 1200)
]
h_fits = fit_and_plot(ax, h_times, hist_h, bin_centers_t, 'H', '#2C3E50', h_ranges, min_points=5)

# Fits for D: start from 0 and 16 µs, up to 800, 1000, 1200 µs
if len(d_times) > 50:
    d_ranges = [
        (0, 800), (0, 1000), (0, 1200),
        (16, 800), (16, 1000), (16, 1200)
    ]
    d_fits = fit_and_plot(ax, d_times, hist_d, bin_centers_t, 'D', '#E74C3C', d_ranges, min_points=3)
else:
    d_fits = {}
    print("Too few D capture events to perform a reliable fit (need >50 matched electrons).", flush=True)

ax.set_xlabel('Compton Electron Creation Time (μs)')
ax.set_ylabel(f'Counts / {bin_width_t:.1f} μs')
ax.set_title('Neutron Capture Time (Compton Electrons) with Multiple Fits')
ax.set_xlim(0, TIME_MAX)
ax.set_yscale('log')
ax.legend(loc='upper right', fontsize=9, ncol=2)
ax.grid(True, linestyle='--', alpha=0.4)
ax.minorticks_on()
ax.tick_params(which='both', direction='in', top=True, right=True)
plt.tight_layout()
plt.savefig(f'fig_compton_times_fits_{timestamp}.png', dpi=300, bbox_inches='tight')
plt.close()
print("✓ Saved fig_compton_times_fits.png", flush=True)

# ============================================================
# FIGURE 2: COMPTON ELECTRON ENERGY SPECTRUM (LOG Y)
# ============================================================
print("Generating Compton electron energy spectrum (log y)...", flush=True)
fig, ax = plt.subplots()
if len(h_energies) > 0:
    bins_e = np.linspace(0, 2.5, 101)
    hist_he, _ = np.histogram(h_energies, bins=bins_e)
    bin_centers_e = 0.5 * (bins_e[:-1] + bins_e[1:])
    ax.step(bin_centers_e, hist_he, where='mid', color='#2C3E50', linewidth=1.5,
            label=f'H (2.22 MeV) n={len(h_energies):,}')
    ax.plot(bin_centers_e, hist_he, 'o', color='#2C3E50', markersize=3,
            markerfacecolor='white', markeredgecolor='#2C3E50')
    ax.axvline(2.0, color='red', linestyle='--', alpha=0.7, label='Compton edge (2.22)')
if len(d_energies) > 0:
    bins_d = np.linspace(0, 6.5, 101)
    hist_de, _ = np.histogram(d_energies, bins=bins_d)
    bin_centers_d = 0.5 * (bins_d[:-1] + bins_d[1:])
    ax.step(bin_centers_d, hist_de, where='mid', color='#E74C3C', linewidth=1.5,
            label=f'D (6.25 MeV) n={len(d_energies):,}')
    ax.plot(bin_centers_d, hist_de, 'o', color='#E74C3C', markersize=3,
            markerfacecolor='white', markeredgecolor='#E74C3C')
    ax.axvline(5.5, color='orange', linestyle='--', alpha=0.7, label='Compton edge (6.25)')

ax.set_xlabel('Electron Energy (MeV)')
ax.set_ylabel('Counts')
ax.set_title('Compton Electron Energy Spectrum')
ax.set_yscale('log')
if len(h_energies) > 0:
    ax.set_xlim(0, 2.5)
else:
    ax.set_xlim(0, 6.5)
ax.legend()
ax.grid(True, linestyle='--', alpha=0.4)
ax.minorticks_on()
ax.tick_params(which='both', direction='in', top=True, right=True)
plt.tight_layout()
plt.savefig(f'fig_compton_energies_{timestamp}.png', dpi=300, bbox_inches='tight')
plt.close()
print("✓ Saved fig_compton_energies.png (log y)", flush=True)

# ============================================================
# PMT HITS FOR CAPTURE EVENTS (with quality cut)
# ============================================================
print("\nProcessing PMT hits for capture events...", flush=True)

pmt_hits_event = event_data["pmtHits"].array()
pmt_num_event = pmt_hits_event["pmtHits.pmtNum"]
pmt_energy_event = pmt_hits_event["pmtHits.photonEnergy"]

n_events_all = len(pmt_num_event)

print(f"Scanning {n_events_all} events...", flush=True)

has_H = []
has_D = []
pass_cut = []

for evt_idx in range(n_events_all):
    pdg_evt = ak.to_numpy(all_pdg[evt_idx]) if len(all_pdg[evt_idx]) > 0 else np.array([])
    has_H.append(np.any(pdg_evt == DEUTERON_PDG))
    has_D.append(np.any(pdg_evt == TRITIUM_PDG))
    
    nums = ak.to_numpy(pmt_num_event[evt_idx])
    if len(nums) >= MIN_TOTAL_HITS:
        distinct = len(np.unique(nums))
        pass_cut.append(distinct >= MIN_DISTINCT_PMTS)
    else:
        pass_cut.append(False)

has_H = np.array(has_H, dtype=bool)
has_D = np.array(has_D, dtype=bool)
pass_cut = np.array(pass_cut, dtype=bool)

print(f"Events with H capture: {np.sum(has_H):,}", flush=True)
print(f"Events with D capture: {np.sum(has_D):,}", flush=True)
print(f"Events passing quality cut: {np.sum(pass_cut):,}", flush=True)

h_mask = has_H & pass_cut
d_mask = has_D & pass_cut

print(f"H capture events passing cut: {np.sum(h_mask):,}", flush=True)
print(f"D capture events passing cut: {np.sum(d_mask):,}", flush=True)

def get_event_data(mask):
    energies = []
    nums = []
    for evt_idx in np.where(mask)[0]:
        e = ak.to_numpy(pmt_energy_event[evt_idx])
        n = ak.to_numpy(pmt_num_event[evt_idx])
        if len(e) > 0:
            energies.extend(e)
            nums.extend(n)
    return np.array(energies), np.array(nums)

h_energies_pmt, h_pmt_nums = get_event_data(h_mask)
d_energies_pmt, d_pmt_nums = get_event_data(d_mask)

h_energies_eV = h_energies_pmt * 1e6
d_energies_eV = d_energies_pmt * 1e6

print(f"H-capture PMT hits: {len(h_energies_pmt):,}", flush=True)
print(f"D-capture PMT hits: {len(d_energies_pmt):,}", flush=True)

# ============================================================
# FIGURE 3: PMT HIT ENERGY SPECTRUM (per hit)
# ============================================================
print("Generating PMT hit energy spectrum...", flush=True)
fig, ax = plt.subplots()
bins_pmt = np.linspace(0, 10, 101)
bin_centers_pmt = 0.5 * (bins_pmt[:-1] + bins_pmt[1:])

if len(h_energies_eV) > 0:
    hist_h, _ = np.histogram(h_energies_eV, bins=bins_pmt)
    ax.step(bin_centers_pmt, hist_h, where='mid', color='#2C3E50', linewidth=1.5,
            label=f'H Capture (n={len(h_energies_eV):,})')
    ax.plot(bin_centers_pmt, hist_h, 'o', color='#2C3E50', markersize=3,
            markerfacecolor='white', markeredgecolor='#2C3E50')
if len(d_energies_eV) > 0:
    hist_d, _ = np.histogram(d_energies_eV, bins=bins_pmt)
    ax.step(bin_centers_pmt, hist_d, where='mid', color='#E74C3C', linewidth=1.5,
            label=f'D Capture (n={len(d_energies_eV):,})')
    ax.plot(bin_centers_pmt, hist_d, 'o', color='#E74C3C', markersize=3,
            markerfacecolor='white', markeredgecolor='#E74C3C')

ax.set_xlabel('Photon Energy (eV)')
ax.set_ylabel('Counts')
ax.set_title('PMT Hit Energy Spectrum (Capture Events)')
ax.set_xlim(0, 10)
ax.legend()
ax.grid(True, linestyle='--', alpha=0.4)
ax.minorticks_on()
ax.tick_params(which='both', direction='in', top=True, right=True)
plt.tight_layout()
plt.savefig(f'fig_pmt_energy_spectrum_{timestamp}.png', dpi=300, bbox_inches='tight')
plt.close()
print("✓ Saved fig_pmt_energy_spectrum.png", flush=True)

# ============================================================
# FIGURE 4: TOTAL PE SPECTRUM (per event) with Gaussian fit (x-axis 0-100)
# ============================================================
print("Generating totalPE spectrum (0-100 P.E.) with Gaussian fit...", flush=True)

def compute_totalPE(mask):
    pe_list = []
    for evt_idx in np.where(mask)[0]:
        n = ak.to_numpy(pmt_num_event[evt_idx])
        pe_list.append(len(n))
    return np.array(pe_list)

h_totalPE = compute_totalPE(h_mask)
d_totalPE = compute_totalPE(d_mask)

for pe_data, label, color in [(h_totalPE, 'H Capture', '#2C3E50'), (d_totalPE, 'D Capture', '#E74C3C')]:
    if len(pe_data) < 20:
        print(f"Too few events ({len(pe_data)}) for {label} totalPE fit. Skipping.", flush=True)
        continue

    pe_pos = pe_data[pe_data > 0]
    if len(pe_pos) < 20:
        print(f"Too few nonzero events for {label} totalPE fit. Skipping.", flush=True)
        continue

    bin_width_pe = 2
    bin_min_pe = 0
    bin_max_pe = 100
    bins_pe = np.arange(bin_min_pe, bin_max_pe + bin_width_pe, bin_width_pe)
    hist_full, _ = np.histogram(pe_data, bins=bins_pe)
    hist_fit, _ = np.histogram(pe_pos, bins=bins_pe)
    centers_fit = 0.5 * (bins_pe[:-1] + bins_pe[1:])

    peak_idx = np.argmax(hist_fit)
    peak_x = centers_fit[peak_idx]
    peak_y = hist_fit[peak_idx]
    if peak_y <= 0:
        continue

    half_max = peak_y / 2.0
    left_idx = peak_idx
    while left_idx > 0 and hist_fit[left_idx] >= half_max:
        left_idx -= 1
    right_idx = peak_idx
    while right_idx < len(hist_fit) - 1 and hist_fit[right_idx] >= half_max:
        right_idx += 1
    if right_idx - left_idx < 5:
        x_left = max(peak_x - 20, bin_min_pe)
        x_right = min(peak_x + 20, bin_max_pe)
        left_idx = max(np.searchsorted(centers_fit, x_left) - 1, 0)
        right_idx = min(np.searchsorted(centers_fit, x_right) + 1, len(hist_fit) - 1)
    else:
        x_left = bins_pe[left_idx]
        x_right = bins_pe[right_idx + 1]

    mask_fit = (centers_fit >= x_left) & (centers_fit <= x_right) & (hist_fit > 0)
    x_fit = centers_fit[mask_fit]
    y_fit = hist_fit[mask_fit].astype(float)
    if len(x_fit) < 5:
        continue

    try:
        c0 = max(0.0, np.percentile(y_fit, 10))
        a0 = max(np.max(y_fit) - c0, 1.0)
        p0 = [a0, peak_x, max((x_right - x_left) / 2.355, 1.0), c0]
        y_err = np.sqrt(np.maximum(y_fit, 1.0))
        popt, pcov = curve_fit(gauss_with_offset, x_fit, y_fit, p0=p0,
                               sigma=y_err, absolute_sigma=True,
                               bounds=([0.0, x_left, 1e-6, 0.0], [np.inf, x_right, np.inf, np.inf]))
        A_fit, mu_fit, sigma_fit, C_fit = popt
        perr = np.sqrt(np.maximum(np.diag(pcov), 0.0))
        mu_err, sigma_err = perr[1], perr[2]
        r_squared = 1 - np.sum((y_fit - gauss_with_offset(x_fit, *popt))**2) / np.sum((y_fit - np.mean(y_fit))**2)
    except Exception as e:
        print(f"Fit failed for {label}: {e}", flush=True)
        continue

    fig, ax = plt.subplots()
    bin_centers = 0.5 * (bins_pe[:-1] + bins_pe[1:])
    step_x = []
    step_y = []
    for i in range(len(bins_pe) - 1):
        step_x.append(bins_pe[i])
        step_x.append(bins_pe[i+1])
        step_y.append(hist_full[i])
        step_y.append(hist_full[i])
    step_x = np.concatenate([[bins_pe[0]], step_x, [bins_pe[-1]]])
    step_y = np.concatenate([[0], step_y, [0]])
    ax.plot(step_x, step_y, color=color, linewidth=1.5, drawstyle='steps-post',
            label=f'{label} (N={len(pe_data):,})')
    ax.plot(bin_centers, hist_full, 'o', color=color, markersize=4,
            markerfacecolor='white', markeredgecolor=color, markeredgewidth=1.2)

    x_smooth = np.linspace(x_left, x_right, 200)
    y_smooth = gauss_with_offset(x_smooth, *popt)
    ax.plot(x_smooth, y_smooth, 'r-', linewidth=2,
            label=f'Fit: μ={mu_fit:.1f}±{mu_err:.1f} P.E.')
    ax.axvline(x_left, color='gray', linestyle='--', alpha=0.5)
    ax.axvline(x_right, color='gray', linestyle='--', alpha=0.5)

    n_zero = np.sum(pe_data == 0)
    info_text = (f'N_{{events}} = {len(pe_data):,}\n'
                 f'N_{{zero}} = {n_zero:,} ({n_zero/len(pe_data)*100:.1f}%)\n'
                 f'Fit range: [{x_left:.0f}, {x_right:.0f}] P.E.\n'
                 f'$\\mu$ = {mu_fit:.1f} $\\pm$ {mu_err:.1f}\n'
                 f'$\\sigma$ = {sigma_fit:.1f} $\\pm$ {sigma_err:.1f}\n'
                 f'R² = {r_squared:.4f}')
    ax.text(0.98, 0.02, info_text, transform=ax.transAxes, fontsize=10,
            verticalalignment='bottom', horizontalalignment='right',
            bbox=dict(boxstyle='round', facecolor='white', alpha=0.9, edgecolor='black'))

    ax.set_xlabel('Total P.E.')
    ax.set_ylabel(f'Counts / {bin_width_pe} P.E.')
    ax.set_title(f'Total PE Spectrum ({label})')
    ax.set_xlim(0, 100)
    ax.legend()
    ax.grid(True, linestyle='--', alpha=0.4)
    ax.minorticks_on()
    ax.tick_params(which='both', direction='in', top=True, right=True)
    plt.tight_layout()
    plt.savefig(f'fig_totalPE_{label.replace(" ", "_")}_{timestamp}.png', dpi=300, bbox_inches='tight')
    plt.close()
    print(f"✓ Saved fig_totalPE_{label.replace(' ', '_')}.png", flush=True)

# ============================================================
# PRINT FIT COMPARISON TABLE
# ============================================================
print("\n" + "="*60, flush=True)
print("FIT COMPARISON TABLE", flush=True)
print("="*60, flush=True)
if h_fits:
    print("H capture fits:")
    # Sort by range for clarity
    for (xmin, xmax), tau in sorted(h_fits.items()):
        print(f"  Range {xmin}-{xmax} µs: τ = {tau:.1f} µs", flush=True)
if d_fits:
    print("D capture fits:")
    for (xmin, xmax), tau in sorted(d_fits.items()):
        print(f"  Range {xmin}-{xmax} µs: τ = {tau:.1f} µs", flush=True)
else:
    print("D capture: No reliable fit could be performed due to low statistics.", flush=True)
    print("   Consider running more events or using a wider binning for D.", flush=True)

# ============================================================
# FINAL SUMMARY
# ============================================================
print("\n" + "="*60, flush=True)
print("SUMMARY", flush=True)
print("="*60, flush=True)
if len(h_times) > 0:
    print(f"H capture: {len(h_times)} Compton electrons, mean time = {np.mean(h_times):.1f} μs", flush=True)
if len(d_times) > 0:
    print(f"D capture: {len(d_times)} Compton electrons, mean time = {np.mean(d_times):.1f} μs", flush=True)

print(f"\nPMT hits in capture events (after quality cut):")
print(f"  H captures: {len(h_energies_pmt):,} hits")
print(f"  D captures: {len(d_energies_pmt):,} hits")

print(f"\nFigures saved:", flush=True)
print(f"  - fig_compton_times_fits_{timestamp}.png (0-{TIME_MAX} μs, multiple fits)", flush=True)
print(f"  - fig_compton_energies_{timestamp}.png (log y)", flush=True)
print(f"  - fig_pmt_energy_spectrum_{timestamp}.png", flush=True)
if len(h_totalPE) >= 20:
    print(f"  - fig_totalPE_H_Capture_{timestamp}.png", flush=True)
if len(d_totalPE) >= 20:
    print(f"  - fig_totalPE_D_Capture_{timestamp}.png", flush=True)
print("="*60, flush=True)