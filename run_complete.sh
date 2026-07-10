#!/bin/bash

# Exit on error
set -e

#------------------------------------------------------------------------------
# Configuration
#------------------------------------------------------------------------------
BASE_DIR="/home/usd.local/manoj.adhikari/GEANT4/G4d2oM2NeutronCapture"
CRYHOME="/home/usd.local/manoj.adhikari/cry_v1.7"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_DIR="${BASE_DIR}/OUTPUT/run_${TIMESTAMP}"
MACRO_FILE="${OUTPUT_DIR}/input.mac"
LOG_FILE="${OUTPUT_DIR}/simulation.log"

#------------------------------------------------------------------------------
# Setup
#------------------------------------------------------------------------------
cd "${BASE_DIR}"
mkdir -p "${OUTPUT_DIR}"

# Load modules
module purge
module load geant4/11.4.0
module load root/6.26.10

# Set CRY
if [[ -d "${CRYHOME}" ]]; then
    export LD_LIBRARY_PATH="${CRYHOME}/lib:${LD_LIBRARY_PATH:-}"
    echo "CRY library loaded from: ${CRYHOME}"
fi

#------------------------------------------------------------------------------
# Check if compiled
#------------------------------------------------------------------------------
if [[ ! -f "${BASE_DIR}/build/G4d2o" ]]; then
    echo "Compiling G4d2o..."
    mkdir -p "${BASE_DIR}/build"
    cd "${BASE_DIR}/build"
    cmake_geant4 ..
    make -j$(nproc)
    cd "${BASE_DIR}"
    echo "Compilation complete"
fi

#------------------------------------------------------------------------------
# Run Simulation
#------------------------------------------------------------------------------
echo "========================================="
echo "Job started at: $(date)"
echo "Host: $(hostname)"
echo "Output directory: ${OUTPUT_DIR}"
echo "========================================="

# Create macro file
echo "/run/beamOn" > "${MACRO_FILE}"

# Run the simulation
echo "Starting simulation..."
./build/G4d2o < "${MACRO_FILE}" > "${LOG_FILE}" 2>&1

#------------------------------------------------------------------------------
# Completion
#------------------------------------------------------------------------------
echo "========================================="
echo "Job finished at: $(date)"
echo "========================================="

# Check results
if grep -q "Number of events processed" "${LOG_FILE}"; then
    echo "Results:"
    grep "Number of events processed" "${LOG_FILE}"
    echo ""
    echo "Last 10 lines of log:"
    tail -10 "${LOG_FILE}"
else
    echo "Warning: No event count found in log"
    echo "Check ${LOG_FILE} for details"
fi

echo ""
echo "Simulation complete!"
echo "Output saved to: ${OUTPUT_DIR}"
echo "Log file: ${LOG_FILE}"
