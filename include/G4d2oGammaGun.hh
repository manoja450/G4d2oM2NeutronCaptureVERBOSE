#ifndef G4d2oGammaGun_h
#define G4d2oGammaGun_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "globals.hh"
#include "G4ThreeVector.hh"

#include "G4d2oNeutrinoAlley.hh"
#include "G4d2oDetector.hh"
#include "inputVariables.hh"
#include "ReplayTools.h"

class G4d2oGammaGun : public G4VUserPrimaryGeneratorAction
{
public:
    G4d2oGammaGun();
    ~G4d2oGammaGun();

    virtual void GeneratePrimaries(G4Event* anEvent);

    G4ThreeVector GetInitialDirection() const { return initDir; }
    G4ThreeVector GetOriginalPosition() const { return SourcePosition; }
    G4double GetSourceEnergy() const { return sourceEnergy; }
    
    // Set gamma energy (for different calibration points)
    void SetGammaEnergy(G4double energy) { gammaEnergy = energy; }
    G4double GetGammaEnergy() const { return gammaEnergy; }
    
    ReplayTools* GetReplayTool() const { return etfTimer; }

private:
    void FindWaterVolume();
    
    inputVariables* input;
    G4ParticleGun* particleGun;
    G4ThreeVector initDir;
    G4ThreeVector SourcePosition;
    G4double sourceEnergy;
    G4double gammaEnergy;
    G4int totalEvents;
    G4int theEventNum;
    ReplayTools* etfTimer;
    G4double cthetaRange1, cthetaRange2;
    
    // Water volume properties
    G4ThreeVector waterCenter;
    G4double waterRadius;
    G4double waterHalfHeight;
    G4bool waterVolumeFound;
};

#endif
