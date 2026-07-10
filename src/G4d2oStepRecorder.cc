#include "G4d2oStepRecorder.hh"
#include "G4StepPoint.hh"
#include "G4VProcess.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4Step.hh"
#include <iomanip>

// Static member initialization
std::vector<int> G4d2oStepRecorder::fTrackID;
std::vector<int> G4d2oStepRecorder::fStepNumber;
std::vector<int> G4d2oStepRecorder::fVolumeCopy;
std::vector<int> G4d2oStepRecorder::fProcessID;
std::vector<int> G4d2oStepRecorder::fPDG;
std::vector<int> G4d2oStepRecorder::fParentID;
std::vector<double> G4d2oStepRecorder::fPosX;
std::vector<double> G4d2oStepRecorder::fPosY;
std::vector<double> G4d2oStepRecorder::fPosZ;
std::vector<double> G4d2oStepRecorder::fKineticEnergy;
std::vector<double> G4d2oStepRecorder::fEnergyDeposit;
std::vector<double> G4d2oStepRecorder::fGlobalTime;
std::vector<double> G4d2oStepRecorder::fStepLength;

G4d2oStepRecorder::G4d2oStepRecorder()
    : G4SteppingVerbose(), fSilent(false)
{
    G4cout << "✓ G4d2oStepRecorder constructed (recording steps)" << G4endl;
    Clear();
}

G4d2oStepRecorder::~G4d2oStepRecorder()
{
    G4cout << "G4d2oStepRecorder destroyed" << G4endl;
    Clear();
}

void G4d2oStepRecorder::Clear()
{
    fTrackID.clear();
    fStepNumber.clear();
    fVolumeCopy.clear();
    fProcessID.clear();
    fPDG.clear();
    fParentID.clear();
    fPosX.clear();
    fPosY.clear();
    fPosZ.clear();
    fKineticEnergy.clear();
    fEnergyDeposit.clear();
    fGlobalTime.clear();
    fStepLength.clear();
}

void G4d2oStepRecorder::TrackingStarted()
{
    // Record step 0 (initial step before any movement)
    if (fTrack) {
        // Get particle name
        G4String particleName = fTrack->GetDefinition()->GetParticleName();
        
        // Only print TrackingStarted for non-optical particles
        if (particleName != "opticalphoton") {
            G4cout << "[StepRecorder] TrackingStarted: Track " << fTrack->GetTrackID() 
                   << ", PDG " << fTrack->GetDefinition()->GetPDGEncoding()
                   << ", Energy " << fTrack->GetKineticEnergy()/keV << " keV"
                   << ", Particle: " << particleName
                   << ", Time: " << fTrack->GetGlobalTime()/ns << " ns"
                   << G4endl;
        }
        
        fTrackID.push_back(fTrack->GetTrackID());
        fStepNumber.push_back(0);
        fVolumeCopy.push_back(0);
        fProcessID.push_back(0);
        fPDG.push_back(fTrack->GetDefinition()->GetPDGEncoding());
        fParentID.push_back(fTrack->GetParentID());
        fPosX.push_back(fTrack->GetPosition().x() / cm);
        fPosY.push_back(fTrack->GetPosition().y() / cm);
        fPosZ.push_back(fTrack->GetPosition().z() / cm);
        fKineticEnergy.push_back(fTrack->GetKineticEnergy() / keV);
        fEnergyDeposit.push_back(0);
        fGlobalTime.push_back(fTrack->GetGlobalTime() / ns);
        fStepLength.push_back(0);
    }
    
    if (!fSilent) {
        G4SteppingVerbose::TrackingStarted();
    }
}

void G4d2oStepRecorder::StepInfo()
{
    if (!fTrack || !fStep) return;
    
    // Check if this is an optical photon
    G4String particleName = fTrack->GetDefinition()->GetParticleName();
    bool isOpticalPhoton = (particleName == "opticalphoton");
    
    G4StepPoint* prePoint = fStep->GetPreStepPoint();
    G4StepPoint* postPoint = fStep->GetPostStepPoint();
    const G4VProcess* process = postPoint->GetProcessDefinedStep();
    
    int procID = 0;
    G4String processName = "NoProcess";
    if (process) {
        procID = process->GetProcessType() * 1000 + process->GetProcessSubType();
        processName = process->GetProcessName();
    }
    
    int volCopy = 0;
    G4String volumeName = "Unknown";
    if (postPoint->GetPhysicalVolume()) {
        volCopy = postPoint->GetPhysicalVolume()->GetCopyNo();
        volumeName = postPoint->GetPhysicalVolume()->GetName();
    }
    
    // Get global time (this is the time since event start)
    G4double globalTime = postPoint->GetGlobalTime() / ns;  // in nanoseconds
    G4double kineticEnergy = postPoint->GetKineticEnergy() / keV;
    G4double energyDeposit = fStep->GetTotalEnergyDeposit() / keV;
    G4double stepLength = fStep->GetStepLength() / cm;
    
    // ============================================================
    // ALWAYS RECORD STEP DATA (even for optical photons)
    // ============================================================
    fTrackID.push_back(fTrack->GetTrackID());
    fStepNumber.push_back(fTrack->GetCurrentStepNumber());
    fVolumeCopy.push_back(volCopy);
    fProcessID.push_back(procID);
    fPDG.push_back(fTrack->GetDefinition()->GetPDGEncoding());
    fParentID.push_back(fTrack->GetParentID());
    fPosX.push_back(postPoint->GetPosition().x() / cm);
    fPosY.push_back(postPoint->GetPosition().y() / cm);
    fPosZ.push_back(postPoint->GetPosition().z() / cm);
    fKineticEnergy.push_back(kineticEnergy);
    fEnergyDeposit.push_back(energyDeposit);
    fGlobalTime.push_back(globalTime);
    fStepLength.push_back(stepLength);
    
    // ============================================================
    // SKIP VERBOSE PRINTING FOR OPTICAL PHOTONS
    // ============================================================
    if (isOpticalPhoton) {
        return;  // Data recorded, but no verbose output
    }
    
    // ============================================================
    // PRINT STEP INFORMATION FOR NON-OPTICAL PARTICLES
    // ============================================================
    static int stepCount = 0;
    if (stepCount < 50 && !fSilent) {
        G4cout << "[StepRecorder] Step " << std::setw(3) << fStepNumber.back()
               << ": Track " << std::setw(3) << fTrackID.back()
               << ", PDG " << std::setw(6) << fPDG.back()
               << ", Particle: " << particleName
               << ", E = " << std::setw(8) << std::setprecision(4) << kineticEnergy << " keV"
               << ", dE = " << std::setw(8) << std::setprecision(4) << energyDeposit << " keV"
               << ", t = " << std::setw(10) << std::setprecision(2) << globalTime << " ns"
               << ", Pos (" << std::setw(6) << std::setprecision(1) << fPosX.back()
               << ", " << std::setw(6) << std::setprecision(1) << fPosY.back()
               << ", " << std::setw(6) << std::setprecision(1) << fPosZ.back() << ") cm"
               << ", StepL = " << std::setw(6) << std::setprecision(2) << stepLength << " cm"
               << ", Vol = " << volumeName
               << ", Proc = " << processName
               << G4endl;
        stepCount++;
    }
    
    // Call parent class for non-optical particles
    if (!fSilent) {
        G4SteppingVerbose::StepInfo();
    }
}

void G4d2oStepRecorder::PrintFirstSteps(int n)
{
    size_t size = fTrackID.size();
    G4cout << "\n=== StepRecorder: First " << std::min(n, (int)size) << " steps ===" << G4endl;
    for (int i = 0; i < std::min(n, (int)size); i++) {
        G4cout << "Step " << i 
               << ": Track " << fTrackID[i]
               << ", PDG " << fPDG[i]
               << ", Energy " << fKineticEnergy[i] << " keV"
               << ", Pos (" << fPosX[i] << ", " << fPosY[i] << ", " << fPosZ[i] << ") cm"
               << ", Time " << fGlobalTime[i] << " ns"
               << G4endl;
    }
    G4cout << "==========================================" << G4endl;
}