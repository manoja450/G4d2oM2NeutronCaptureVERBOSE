G4d2o – Geant4 Simulation of COHERENT D₂O Detector
Current Status (July 2026)
This repository contains the Geant4 simulation for the COHERENT D₂O detector. The original geometry was created by Igor Bernardi and reflects the detector as‑built. The code is maintained for neutron capture studies, with extensions for directional neutron beams and comprehensive analysis tools.

The main branch contains the stable detector geometry and physics lists. Development work on neutron simulations is being carried out on the development branch, with a focus on understanding neutron backgrounds and capture fractions in the D₂O volume.

Overview
This Geant4 simulation models the COHERENT D₂O detector, including:

Inner D₂O cylinder (heavy water) surrounded by

Acrylic vessel

Outer H₂O layer (light water)

Steel vessel

Lead shielding

PMTs and veto panels

The simulation supports various primary particle sources:

Electron, muon, cosmic, and photon guns (existing)

Neutron gun with configurable source modes (new):

Uniform volume generation (inside D₂O or H₂O)

Directional beams from top (downward)

Directional beams from side (−X direction, inward)

The neutron simulation is designed to study:

Neutron capture fractions in different detector volumes (D₂O, H₂O, acrylic, steel, shielding)

Capture time distributions and lifetimes

Penetration depth and spatial distributions of neutron captures

The effect of external neutron sources on the D₂O signal

Installation
Prerequisites
Geant4 version 11.2.1 or later (installation guide)

ROOT (with dictionary support)

CMake 3.10+

CRY (optional, for cosmic-ray simulations) – CRY website

Build Instructions
Clone the repository:

bash
git clone https://github.com/COHERENT/G4d2o.git
cd G4d2o
git checkout development   # or your working branch
Copy the beam-on configuration file and set up the build script:

bash
cp beamOn0.dat beamOn.dat
cp setupBuild.sh.bak setupBuild.sh
Edit setupBuild.sh to set the correct paths for Geant4 and ROOT.

Build the application:

bash
. ./setupBuild.sh makefile
./compileApp makefile
Run a simulation:

bash
./G4d2o
Notes on CRY Installation
If you plan to run cosmic-ray simulations, install CRY version 1.7 and source the setup script in your shell configuration:

bash
source /path/to/cry_v1.7/setup
If you encounter linking errors, edit CMakeLists.txt:

Replace ${CRYHOME} with $ENV{CRYHOME}

Add the CRY library explicitly:

cmake
file(GLOB crylibs $ENV{CRYHOME}/lib/libCRY.a)
target_link_libraries G4d2o ${Geant4_LIBRARIES} ${ROOT_LIBRARIES} simEvent ${crylibs}
Neutron Simulation: Overview and Usage
Neutron Gun Modes
The neutron source is configured via compile‑time flags in G4d2oNeutronGun.hh:

Mode	Flag	Description
Volume uniform	fVolumeMode = true	Neutrons generated uniformly inside the D₂O cylinder (original behaviour)
Top beam	fTopBeamMode = true	Neutrons placed on a plane just inside the top of the H₂O layer, aimed downward
Side beam	fSideBeamMode = true	Neutrons placed on a plane just inside the −X side of the H₂O layer, aimed inward (+X)
How to switch: Edit the flags in G4d2oNeutronGun.hh, rebuild, and run.

Beam Source Geometry
The source positions are sampled to ensure neutrons start inside the H₂O volume (just inside the outer steel vessel), mimicking external neutrons entering the detector from the environment:

Top beam: source plane at Z = center_Z + H2O_half_height - 0.5 cm

Side beam: source plane at X = center_X - H2O_radius + 1.0 cm

The H₂O geometry is hardcoded in the neutron gun (values from the detector construction):

text
H2O center: (0, 110.73384, -80.74526) cm
H2O radius: 44.45 cm
H2O half-height: 95.3135 cm
Analysis Tools
A comprehensive Python analysis script (analyze_captures.py) is provided to parse the simulation log files and generate publication‑quality figures. The script:

Parses the Geant4 log to extract neutron capture events (initial position, capture position, and volume).

Checks that all nCaptureHP lines are recovered.

Generates 3D and 2D projection plots (XY, XZ, YZ) showing:

Initial neutron positions

Neutron capture locations, colour‑coded by volume

Capture fraction vs. initial cylindrical radius

Initial radius distribution

Neutron displacement (production → capture)

Applies a D₂O‑only cut to study events that both start and capture inside the D₂O cylinder, producing the same set of figures for that subset.

Usage:

bash
python analyze_captures.py
The script expects a log file named simulation_output_run*.log (set the variable LOG_FILE at the top of the script). It will generate multiple PNG figures and print summary statistics.

Geometry and Physics
The detector geometry was implemented by Igor Bernardi and reflects the actual COHERENT D₂O detector dimensions. The geometry hierarchy is:

text
worldVol_logV (AIR)
  └── hallConcreteVol_logV (CONCRETE)
        └── hallOpening_logV (AIR)
              └── detPhysV (detector)
                    └── totalDetLogV (AIR)
                          └── shieldingPhysV (LEAD)
                                └── outerVesselPhysV (STEEL)
                                      └── outerAirselPhysV (AIR)
                                            └── h2oPhysV (H2O)
                                                  └── tyvekPhysV (TYVEK)
                                                  └── tyvekCapBotPhysV (TYVEK)
                                                  └── acrylicPhysV (PMMA)
                                                        └── d2oPhysV (D2O)
Physics lists include:

Standard electromagnetic processes (G4EmStandardPhysics)

Hadronic physics (QGSP_BIC_HP for high‑precision neutron transport)

Optical processes (Cherenkov, scintillation, absorption, Rayleigh scattering, boundary processes)

Neutron capture (nCaptureHP) is enabled for hydrogen and deuterium.

Key Results from Neutron Simulations
External neutrons (top/side beams) are overwhelmingly captured in H₂O and acrylic before reaching the D₂O core.

The D₂O‑only events are very few, confirming the small external background contribution to the D₂O signal.

The capture time distribution shows a clear ∼200 μs component from hydrogen capture, consistent with light water.

The expected heavy water capture lifetime (∼104 ms for 99.92% D₂O purity) is not observed in the short time window, but ongoing work extends the analysis to longer time scales.

File Structure
text
G4d2o/
├── include/                 # Header files
│   ├── G4d2oNeutronGun.hh   # Neutron gun configuration
│   └── ...
├── src/                     # Source files
│   ├── G4d2oNeutronGun.cc   # Neutron gun implementation
│   └── ...
├── data/                    # Output ROOT files and logs
├── scripts/                 # Analysis scripts (ROOT macros, Python)
│   └── analyze_captures.py  # Neutron capture analysis
├── opticalData/             # Water optical properties
├── beamOn.dat               # Runtime configuration
├── setupBuild.sh            # Build script
└── README.md                # This file
Contributing and Contacts
Original geometry and physics: Igor Bernardi (completed)

Neutron simulation extensions: [Manoj Adhikari] (current maintainer)

For questions about the neutron simulation or analysis tools, please contact the current maintainer.

Known Issues and Future Work
The CRY module is no longer maintained; use with caution.

The analysis script currently relies on log file parsing – future improvements may use direct ROOT file output.

The neutron gun currently uses fixed H₂O geometry values; a future version should query the geometry dynamically.

Extending the time window in the analysis to capture the full heavy‑water lifetime is ongoing.

Acknowledgments
This work is part of the COHERENT collaboration. Thanks to Igor Bernardi for the detector geometry, Rebecca Rapp for Geant4 guidance, and the collaboration for ongoing support.
