#include "G4d2oTrackingAction.hh"
#include "TrackInfoManager.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"
#include "G4SystemOfUnits.hh"
#include "G4VSteppingVerbose.hh"

using namespace CLHEP;

G4d2oTrackingAction::G4d2oTrackingAction() {}
G4d2oTrackingAction::~G4d2oTrackingAction() {}

void G4d2oTrackingAction::PreUserTrackingAction(const G4Track* track) {
    // Get particle name (distinguishes "gamma" from "opticalphoton")
    G4String particleName = track->GetDefinition()->GetParticleName();
    
    G4String creatorProc = "Primary";
    if (track->GetCreatorProcess()) {
        creatorProc = track->GetCreatorProcess()->GetProcessName();
    }
    
    // Store ALL track information including particle name
    // This data is saved to ROOT file
    TrackInfoManager::GetInstance()->AddTrack(
        track->GetTrackID(),                    // trackID
        track->GetDefinition()->GetPDGEncoding(), // pdg
        track->GetParentID(),                   // parentID
        creatorProc,                            // creatorProcess
        track->GetKineticEnergy() / MeV,        // energy (MeV)
        track->GetPosition().x() / cm,          // posX (cm)
        track->GetPosition().y() / cm,          // posY (cm)
        track->GetPosition().z() / cm,          // posZ (cm)
        track->GetGlobalTime() / ns,            // time (ns)
        particleName                            // particleName
    );
    
    // ============================================================
    // SUPPRESS VERBOSE OUTPUT FOR OPTICAL PHOTONS
    // ============================================================
    if (particleName == "opticalphoton") {
        // Turn off verbose output for this particle only
        // This prevents the step-by-step tracking from being printed
        if (G4VSteppingVerbose::GetInstance()) {
            G4VSteppingVerbose::GetInstance()->SetSilent(true);
        }
        return;  // Skip verbose output for optical photons
    }
    
    // For all other particles, ensure verbose output is ON
    if (G4VSteppingVerbose::GetInstance()) {
        G4VSteppingVerbose::GetInstance()->SetSilent(false);
    }
}