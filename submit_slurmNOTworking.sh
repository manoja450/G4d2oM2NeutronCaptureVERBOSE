#!/bin/bash
#SBATCH --job-name=G4h2o
#SBATCH --partition=quickq
#SBATCH --time=10:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --mem=34G
#SBATCH --output=/home/usd.local/manoj.adhikari/GEANT4/G4d2oM2NeutronCapture/SLURMOUT/slurm-%j.out
#SBATCH --error=/home/usd.local/manoj.adhikari/GEANT4/G4d2oM2NeutronCapture/SLURMOUT/slurm-%j.err

# Exit on any error
set -e

#------------------------------------------------------------------------------
# Configuration
#------------------------------------------------------------------------------
BASE_DIR="/home/usd.local/manoj.adhikari/GEANT4/G4d2oM2NeutronCapture"
SLURM_DIR="${BASE_DIR}/SLURMOUT"
CRYHOME="/home/usd.local/manoj.adhikari/cry_v1.7"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_DIR="${BASE_DIR}/OUTPUT/run_${TIMESTAMP}"
MACRO_FILE="${OUTPUT_DIR}/input.mac"
LOG_FILE="${OUTPUT_DIR}/simulation.log"

#------------------------------------------------------------------------------
# Setup
#------------------------------------------------------------------------------
# Create required directories
mkdir -p "${OUTPUT_DIR}" "${SLURM_DIR}"

# Change to base directory
cd "${BASE_DIR}"

# Load modules
module purge
module load geant4/11.4.0
module load root/6.26.10

# Set up CRY library if available
if [[ -d "${CRYHOME}" ]]; then
    export LD_LIBRARY_PATH="${CRYHOME}/lib:${LD_LIBRARY_PATH:-}"
    echo "CRY library loaded from: ${CRYHOME}"
fi

#------------------------------------------------------------------------------
# Job Information
#------------------------------------------------------------------------------
echo "========================================="
echo "Job ID      : ${SLURM_JOB_ID:-N/A}"
echo "Job Name    : ${SLURM_JOB_NAME:-G4h2o}"
echo "Node        : $(hostname)"
echo "Start Time  : $(date)"
echo "Output Dir  : ${OUTPUT_DIR}"
echo "========================================="

#------------------------------------------------------------------------------
# Run Simulation
#------------------------------------------------------------------------------
# Create macro file
cat > "${MACRO_FILE}" << 'EOF'
/run/beamOn
EOF

echo "Starting simulation at $(date)"
echo "Macro file: ${MACRO_FILE}"
echo "Log file: ${LOG_FILE}"

# Execute simulation
./build/G4d2o < "${MACRO_FILE}" > "${LOG_FILE}" 2>&1

#------------------------------------------------------------------------------
# Completion
#------------------------------------------------------------------------------
echo "========================================="
echo "Finished    : $(date)"
echo "========================================="

# Extract event count
if grep -q "Number of events processed" "${LOG_FILE}"; then
    grep "Number of events processed" "${LOG_FILE}"
else
    echo "Warning: Event count not found in log file"
fi

echo "Simulation complete. Results saved to: ${OUTPUT_DIR}"
