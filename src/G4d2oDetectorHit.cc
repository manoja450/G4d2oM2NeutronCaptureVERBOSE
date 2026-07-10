#include "G4d2oDetectorHit.hh"
#include "G4UnitsTable.hh"

G4Allocator<G4d2oDetectorHit> G4d2oDetectorHitAllocator;

G4d2oDetectorHit::G4d2oDetectorHit()
{
    hitTrackID            = 0;
    hitPixelNumber        = 0;
    hitDetectorNumber     = 0;
    hitTrackParentID      = 0;
    hitTotalEnergyDeposit = 0;
    hitGlobalTime         = 0;
    hitKineticEnergy      = 0;
    hitPDGCharge          = 0;
    hitLeptonNumber       = 0;
    hitBaryonNumber       = 0;
    hitDeltaTime          = 0;
    hitCalcA              = 0;
    
    snprintf(hitParticleName, sizeof(hitParticleName), "%s", "");
    snprintf(hitProcessName, sizeof(hitProcessName), "%s", "");
    snprintf(hitProcessTypeName, sizeof(hitProcessTypeName), "%s", "");
    snprintf(hitMaterialName, sizeof(hitMaterialName), "%s", "");
    snprintf(hitCreatorProcessName, sizeof(hitCreatorProcessName), "%s", "");
    snprintf(hitCreatorProcessType, sizeof(hitCreatorProcessType), "%s", "");

    // PDG and parent type
    hitPDGEncoding = 0;
    hitParentType = 0;

    // Neutron capture members
    hitIsNeutronCapture = false;
    hitCaptureOnHydrogen = false;
    hitCaptureOnDeuterium = false;
    hitCaptureTime = 0.0;
    hitCaptureGammaEnergy = 0.0;
    hitCaptureProductEnergy = 0.0;
    hitCaptureNeutronEnergy = 0.0;  // NEW
    snprintf(hitCaptureProduct, sizeof(hitCaptureProduct), "%s", "");
    snprintf(hitCaptureVolume, sizeof(hitCaptureVolume), "%s", "");
    hitCapturePosX = 0.0;
    hitCapturePosY = 0.0;
    hitCapturePosZ = 0.0;
}

G4d2oDetectorHit::~G4d2oDetectorHit() {}

void G4d2oDetectorHit::SetPixelNumber(G4int thePixelNumber){ hitPixelNumber = thePixelNumber; }
G4int G4d2oDetectorHit::GetPixelNumber(void) { return hitPixelNumber; }

void G4d2oDetectorHit::SetDetectorNumber(G4int theDetectorNumber){ hitDetectorNumber = theDetectorNumber; }
G4int G4d2oDetectorHit::GetDetectorNumber(void) { return hitDetectorNumber; }

void G4d2oDetectorHit::SetTrackID(G4int theTrackID){ hitTrackID = theTrackID; }
G4int G4d2oDetectorHit::GetTrackID(void) { return hitTrackID; }

void G4d2oDetectorHit::SetTrackParentID(G4int theTrackParentID){ hitTrackParentID = theTrackParentID; }
G4int G4d2oDetectorHit::GetTrackParentID(void) { return hitTrackParentID; }

void G4d2oDetectorHit::SetScatA(G4int theScatA){ hitScatA = theScatA; }
G4int G4d2oDetectorHit::GetScatA(void) { return hitScatA; }

void G4d2oDetectorHit::SetScatZ(G4int theScatZ){ hitScatZ = theScatZ; }
G4int G4d2oDetectorHit::GetScatZ(void) { return hitScatZ; }

void G4d2oDetectorHit::SetTotalEnergyDeposit(G4double theEnergyDeposit) { hitTotalEnergyDeposit = theEnergyDeposit; }
G4double G4d2oDetectorHit::GetTotalEnergyDeposit(void) { return hitTotalEnergyDeposit; }

void G4d2oDetectorHit::SetGlobalTime(G4double theGlobalTime) { hitGlobalTime = theGlobalTime; }
G4double G4d2oDetectorHit::GetGlobalTime(void) { return hitGlobalTime; }

void G4d2oDetectorHit::SetKineticEnergy(G4double theKineticEnergy) { hitKineticEnergy = theKineticEnergy; }
G4double G4d2oDetectorHit::GetKineticEnergy(void) { return hitKineticEnergy; }

void G4d2oDetectorHit::SetPDGCharge(G4double thePDGCharge) { hitPDGCharge = thePDGCharge; }
G4double G4d2oDetectorHit::GetPDGCharge(void) { return hitPDGCharge; }

void G4d2oDetectorHit::SetBaryonNumber(G4double theBaryonNumber) { hitBaryonNumber = theBaryonNumber; }
G4double G4d2oDetectorHit::GetBaryonNumber(void) { return hitBaryonNumber; }

void G4d2oDetectorHit::SetLeptonNumber(G4double theLeptonNumber) { hitLeptonNumber = theLeptonNumber; }
G4double G4d2oDetectorHit::GetLeptonNumber(void) { return hitLeptonNumber; }

void G4d2oDetectorHit::SetDeltaTime(G4double theDeltaTime) { hitDeltaTime = theDeltaTime; }
G4double G4d2oDetectorHit::GetDeltaTime(void) { return hitDeltaTime; }

void G4d2oDetectorHit::SetCalcA(G4double theCalcA) { hitCalcA = theCalcA; }
G4double G4d2oDetectorHit::GetCalcA(void) { return hitCalcA; }

void G4d2oDetectorHit::SetParticleName(char *theParticleName){ snprintf(hitParticleName, sizeof(hitParticleName), "%s", theParticleName); }
char* G4d2oDetectorHit::GetParticleName(void) { return hitParticleName; }

void G4d2oDetectorHit::SetProcessName(char *theProcessName){ snprintf(hitProcessName, sizeof(hitProcessName), "%s", theProcessName); }
char* G4d2oDetectorHit::GetProcessName(void) { return hitProcessName; }

void G4d2oDetectorHit::SetProcessTypeName(char *theProcessTypeName){ snprintf(hitProcessTypeName, sizeof(hitProcessTypeName), "%s", theProcessTypeName); }
char* G4d2oDetectorHit::GetProcessTypeName(void) { return hitProcessTypeName; }

void G4d2oDetectorHit::SetMaterialName(char *theMaterialName){ snprintf(hitMaterialName, sizeof(hitMaterialName), "%s", theMaterialName); }
char* G4d2oDetectorHit::GetMaterialName(void) { return hitMaterialName; }

void G4d2oDetectorHit::SetCreatorProcessName(char *theCreatorProcessName){ snprintf(hitCreatorProcessName, sizeof(hitCreatorProcessName), "%s", theCreatorProcessName); }
char* G4d2oDetectorHit::GetCreatorProcessName(void) { return hitCreatorProcessName; }

void G4d2oDetectorHit::SetCreatorProcessType(char *theCreatorProcessType){ snprintf(hitCreatorProcessType, sizeof(hitCreatorProcessType), "%s", theCreatorProcessType); }
char* G4d2oDetectorHit::GetCreatorProcessType(void) { return hitCreatorProcessType; }

void G4d2oDetectorHit::SetMomentumDirection(G4ThreeVector theMomentumDirection){ hitMomentumDirection = theMomentumDirection; }
G4ThreeVector G4d2oDetectorHit::GetMomentumDirection(void) { return hitMomentumDirection; }

void G4d2oDetectorHit::SetPostMomentumDirection(G4ThreeVector thePostMomentumDirection){ hitPostMomentumDirection = thePostMomentumDirection; }
G4ThreeVector G4d2oDetectorHit::GetPostMomentumDirection(void) { return hitPostMomentumDirection; }

void G4d2oDetectorHit::SetPosition(G4ThreeVector thePosition){ hitPosition = thePosition; }
G4ThreeVector G4d2oDetectorHit::GetPosition(void) { return hitPosition; }

void G4d2oDetectorHit::SetPrePosition(G4ThreeVector thePrePosition){ hitPrePosition = thePrePosition; }
G4ThreeVector G4d2oDetectorHit::GetPrePosition(void) { return hitPrePosition; }

void G4d2oDetectorHit::SetVertex(G4ThreeVector theVertex){ hitVertex = theVertex; }
G4ThreeVector G4d2oDetectorHit::GetVertex(void) { return hitVertex; }

void G4d2oDetectorHit::Draw(void) {}
void G4d2oDetectorHit::Print(void) {}