#!/bin/bash
#SBATCH --job-name=G4h2o
#SBATCH --partition=compute
#SBATCH --time=14-00:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem=74G
#SBATCH --output=/home/usd.local/manoj.adhikari/GEANT4/G4d2oM2NeutronCaptureVERBOSE/SIMULATIONLOGS/slurm-%j.out
#SBATCH --error=/home/usd.local/manoj.adhikari/GEANT4/G4d2oM2NeutronCaptureVERBOSE/SIMULATIONLOGS/slurm-%j.err

#------------------------------------------------------------------------------
# Configuration
#------------------------------------------------------------------------------
BASE_DIR="/home/usd.local/manoj.adhikari/GEANT4/G4d2oM2NeutronCaptureVERBOSE"
SIMULATION_LOGS="${BASE_DIR}/SIMULATIONLOGS"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
EXECUTABLE="${BASE_DIR}/build/G4d2o"

# beamon.dat is actually named beamOn.dat (with capital O)
BEAMON_FILE="${BASE_DIR}/beamOn.dat"

# CRY data location
CRYHOME="/home/usd.local/manoj.adhikari/software/cry_v1.7"
CRY_DATA_DIR="${CRYHOME}/data"

#------------------------------------------------------------------------------
# Setup
#------------------------------------------------------------------------------
# Create SIMULATIONLOGS directory if it doesn't exist
mkdir -p "${SIMULATION_LOGS}"

# Change to base directory
cd "${BASE_DIR}"

# Load modules (same as interactive)
module purge
module load geant4/11.4.0
module load root/6.26.10

# Set up CRY library
if [[ -d "${CRYHOME}" ]]; then
    export LD_LIBRARY_PATH="${CRYHOME}/lib:${LD_LIBRARY_PATH:-}"
    echo "CRY library loaded from: ${CRYHOME}"
else
    echo "WARNING: CRY library not found at ${CRYHOME}"
fi

if [[ -d "${CRY_DATA_DIR}" ]]; then
    echo "CRY data directory found: ${CRY_DATA_DIR}"
    export CRY_DATA_PATH="${CRY_DATA_DIR}"
else
    echo "WARNING: CRY data directory not found at ${CRY_DATA_DIR}!"
fi

# Extract run number from beamOn.dat - properly extract only the number
RUN_NUMBER="unknown"
if [[ -f "${BEAMON_FILE}" ]]; then
    # Read the first line, take only the number (ignore everything after space or tab)
    RUN_NUMBER=$(head -1 "${BEAMON_FILE}" | awk '{print $1}' | tr -d ' \t')
    echo "Extracted Run Number: ${RUN_NUMBER} from ${BEAMON_FILE}"
else
    echo "WARNING: beamOn.dat not found at ${BEAMON_FILE}!"
    echo "Available .dat files:"
    ls -la "${BASE_DIR}"/*.dat 2>/dev/null || echo "No .dat files found"
fi

# Create output directory with run number
if [[ "${RUN_NUMBER}" != "unknown" ]] && [[ -n "${RUN_NUMBER}" ]]; then
    OUTPUT_DIR="${BASE_DIR}/OUTPUT/run_${RUN_NUMBER}"
    LOG_FILE="${OUTPUT_DIR}/simulation_output_run${RUN_NUMBER}.log"
else
    OUTPUT_DIR="${BASE_DIR}/OUTPUT/run_${TIMESTAMP}"
    LOG_FILE="${OUTPUT_DIR}/simulation_output.log"
fi

mkdir -p "${OUTPUT_DIR}"

#------------------------------------------------------------------------------
# Job Information
#------------------------------------------------------------------------------
echo "========================================="
echo "Job ID      : ${SLURM_JOB_ID:-N/A}"
echo "Node        : $(hostname)"
echo "Start Time  : $(date)"
echo "Run Number  : ${RUN_NUMBER}"
echo "Output Dir  : ${OUTPUT_DIR}"
echo "Executable  : ${EXECUTABLE}"
echo "beamon.dat  : ${BEAMON_FILE}"
echo "CRY Home    : ${CRYHOME}"
echo "CRY Data    : ${CRY_DATA_DIR}"
echo "========================================="

# Display beamOn.dat content
if [[ -f "${BEAMON_FILE}" ]]; then
    echo ""
    echo "beamOn.dat content:"
    cat "${BEAMON_FILE}"
    echo ""
fi
echo "========================================="

#------------------------------------------------------------------------------
# Check Executable
#------------------------------------------------------------------------------
# Check if executable exists
if [[ ! -f "${EXECUTABLE}" ]]; then
    echo "ERROR: Executable not found at ${EXECUTABLE}!"
    echo "Available executables:"
    find "${BASE_DIR}" -name "G4d2o" -type f 2>/dev/null
    exit 1
fi

# Make sure executable is executable
chmod +x "${EXECUTABLE}"

#------------------------------------------------------------------------------
# Run Simulation
#------------------------------------------------------------------------------
echo "Starting simulation at $(date)"
echo "Running: ${EXECUTABLE} > ${LOG_FILE} 2>&1"
echo ""

START_TIME=$(date +%s)

# Copy beamOn.dat to output directory for record keeping
if [[ -f "${BEAMON_FILE}" ]]; then
    cp "${BEAMON_FILE}" "${OUTPUT_DIR}/beamOn.dat"
    echo "beamOn.dat copied to ${OUTPUT_DIR}"
fi

# Run exactly like you do interactively
"${EXECUTABLE}" > "${LOG_FILE}" 2>&1

EXIT_CODE=$?
END_TIME=$(date +%s)
RUNTIME=$((END_TIME - START_TIME))

#------------------------------------------------------------------------------
# Completion
#------------------------------------------------------------------------------
echo "========================================="
echo "Finished    : $(date)"
echo "Runtime     : ${RUNTIME} seconds ($((RUNTIME/60)) minutes)"
echo "Exit Code   : ${EXIT_CODE}"
echo "========================================="

if [ ${EXIT_CODE} -eq 0 ]; then
    echo "✅ Simulation completed successfully!"
else
    echo "❌ Simulation failed with exit code ${EXIT_CODE}"
    echo "Check log file: ${LOG_FILE}"
    echo ""
    echo "Last 30 lines of log:"
    tail -30 "${LOG_FILE}" 2>/dev/null || echo "Log file not found or empty"
    
    # Check for common errors
    if grep -q "CRYData" "${LOG_FILE}" 2>/dev/null; then
        echo ""
        echo "CRY data error detected. Please check CRY installation."
        echo "CRY data should be at: ${CRY_DATA_DIR}"
        echo "Contents of CRY data directory:"
        ls -la "${CRY_DATA_DIR}" 2>/dev/null || echo "Directory not accessible"
    fi
fi

echo ""
echo "Results saved to: ${OUTPUT_DIR}"
echo "Log file: ${LOG_FILE}"
echo "SLURM logs saved to: ${SIMULATION_LOGS}"

# Show any generated ROOT files
echo ""
echo "Generated ROOT files:"
find "${OUTPUT_DIR}" -name "*.root" -type f 2>/dev/null || echo "No ROOT files found in output directory"
find "${BASE_DIR}" -maxdepth 1 -name "*.root" -type f 2>/dev/null | while read rootfile; do
    cp "${rootfile}" "${OUTPUT_DIR}/" 2>/dev/null
    echo "Copied: $(basename ${rootfile})"
done

# Show disk usage
echo ""
echo "Disk usage:"
df -h "${BASE_DIR}"

echo ""
echo "========================================="
echo "To check simulation output:"
echo "  cat ${LOG_FILE}"
echo "  tail -f ${LOG_FILE}"
echo ""
echo "To check SLURM output:"
echo "  cat ${SIMULATION_LOGS}/slurm-${SLURM_JOB_ID}.out"
echo "  cat ${SIMULATION_LOGS}/slurm-${SLURM_JOB_ID}.err"
echo "========================================="