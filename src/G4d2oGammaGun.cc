#include "G4d2oGammaGun.hh"
#include "inputVariables.hh"
#include "TMath.h"
#include "TRandom.h"
#include "TVector3.h"
#include "TSystem.h"

#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4RunManager.hh"
#include "G4Navigator.hh"
#include "G4TransportationManager.hh"
#include "globals.hh"
#include "Randomize.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalVolumeStore.hh"

G4d2oGammaGun::G4d2oGammaGun()
{
    G4cout << "\tConstructing G4d2oGammaGun (Gamma Calibration Source)..." ;
    
    input = inputVariables::GetIVPointer();
    G4int irand = input->GetRandomStatus();
    if(irand==0) gRandom->SetSeed(1);
    if(irand==1) gRandom->SetSeed(0);
    totalEvents = input->GetNumberOfEvents();

    G4int n_particle = 1;
    particleGun = new G4ParticleGun(n_particle);
    
    // Set particle to gamma
    G4String particleName = "gamma";
    G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
    particleGun->SetParticleDefinition(particleTable->FindParticle(particleName));
    
    // ============================================================
    // SET GAMMA ENERGY HERE FOR CALIBRATION
    // Change this value for different calibration points:
    //   0.5 MeV, 1.0 MeV, 2.22 MeV (H capture), 6.25 MeV (D capture)
    // ============================================================
    gammaEnergy = 2.22 * MeV;  // Default: H capture gamma
    // gammaEnergy = 6.25 * MeV;  // Uncomment for D capture gamma
    // gammaEnergy = 1.0 * MeV;   // Uncomment for 1 MeV calibration
    // gammaEnergy = 0.5 * MeV;   // Uncomment for 0.5 MeV calibration
    
    sourceEnergy = gammaEnergy;
    particleGun->SetParticleEnergy(sourceEnergy);
    
    SourcePosition.setX(0);
    SourcePosition.setY(0);
    SourcePosition.setZ(0);
    
    G4double thetaAngle1 = 0.0;
    G4double thetaAngle2 = 180.0;
    
    cthetaRange1 = cos(thetaAngle1*deg);
    cthetaRange2 = cos(thetaAngle2*deg);
    
    // Water dimensions (same as neutron gun)
    waterRadius = 33.655 * cm;
    waterHalfHeight = 67.4624 * cm;
    waterCenter.setX(0);
    waterCenter.setY(0);
    waterCenter.setZ(0);
    waterVolumeFound = false;

    etfTimer = new ReplayTools();
    etfTimer->PrepareETFTimer(5000, input->GetNumberOfEvents());
    
    G4cout << "Gamma energy set to: " << gammaEnergy / MeV << " MeV" << G4endl;
    G4cout << "done." << G4endl;
}

G4d2oGammaGun::~G4d2oGammaGun()
{
    G4cout<<"Deleting G4d2oGammaGun...";
    delete particleGun;
    delete etfTimer;
    G4cout<<"done."<<G4endl;
}

void G4d2oGammaGun::FindWaterVolume()
{
    G4PhysicalVolumeStore* pvStore = G4PhysicalVolumeStore::GetInstance();
    if (!pvStore) return;
    
    G4VPhysicalVolume* waterVol = pvStore->GetVolume("d2oPhysV");
    if (!waterVol) waterVol = pvStore->GetVolume("h2oPhysV");
    
    if (waterVol) {
        waterCenter = waterVol->GetTranslation();
        
        G4LogicalVolume* logVol = waterVol->GetLogicalVolume();
        G4VSolid* solid = logVol->GetSolid();
        G4Tubs* tubs = dynamic_cast<G4Tubs*>(solid);
        
        if (tubs) {
            waterRadius = tubs->GetOuterRadius();
            waterHalfHeight = tubs->GetZHalfLength();
        }
        
        waterVolumeFound = true;
        
        G4cout << "Found water volume: " << waterVol->GetName() << G4endl;
        G4cout << "  Center: (" << waterCenter.x()/cm << ", " 
               << waterCenter.y()/cm << ", " << waterCenter.z()/cm << ") cm" << G4endl;
        G4cout << "  Radius: " << waterRadius/cm << " cm" << G4endl;
        G4cout << "  Half-height: " << waterHalfHeight/cm << " cm" << G4endl;
    }
}

void G4d2oGammaGun::GeneratePrimaries(G4Event* anEvent)
{
    theEventNum = anEvent->GetEventID();
    etfTimer->SetCurrentEvent(theEventNum);
    gSystem->ProcessEvents();
    
    if(anEvent->GetEventID()==0 && input->GetPrintStatus()!=3) 
        etfTimer->StartUpdateTimer();
    
    // Find water volume on first event (optional)
    if (!waterVolumeFound) {
        FindWaterVolume();
    }
    
    G4double px, py, pz;
    G4double ctheta, stheta, phi;

    // Use the gamma energy set in constructor
    sourceEnergy = gammaEnergy;
    particleGun->SetParticleEnergy(sourceEnergy);
  
    // ============================================================
    // USE SAME POSITION AS NEUTRON GUN (Michel electron position)
    // This is where water volume actually is!
    // ============================================================
    // Position based on Michel electron gun (where water is located)
    // Y ~ 110 cm offset, X/Z within 33.655 cm radius
    G4double maxRadius = 33.655;  // cm (water radius)
    G4double randvarn1 = maxRadius * sqrt(G4UniformRand());  // radius (cm)
    G4double randvarn2 = 2.0 * TMath::Pi() * G4UniformRand();  // angle (rad)
    G4double randvarn3 = 89.9275 * (-1.0 + 2.0 * G4UniformRand());  // z variation (cm)
    
    G4double x = randvarn1 * cos(randvarn2);
    G4double y = 110.73384 + randvarn1 * sin(randvarn2);
    G4double z = -80.74526 + randvarn3;
    
    SourcePosition.setX(x * cm);
    SourcePosition.setY(y * cm);
    SourcePosition.setZ(z * cm);
    
    particleGun->SetParticlePosition(SourcePosition);
    
    // ============================================================
    // ISOTROPIC DIRECTION for gammas
    // ============================================================
    ctheta = G4UniformRand() * (cthetaRange1 - cthetaRange2) + cthetaRange2;
    phi = 2.0 * TMath::Pi() * G4UniformRand();
    stheta = sqrt(1.0 - ctheta * ctheta);
    
    pz = ctheta;
    py = stheta * cos(phi);
    px = stheta * sin(phi);
    
    initDir.setX(px);
    initDir.setY(py);
    initDir.setZ(pz);
    particleGun->SetParticleMomentumDirection(initDir);
    
    // Debug output for first 10 events
    if(theEventNum < 10) {
        G4Navigator* Navigator = G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking();
        G4VPhysicalVolume* volume = Navigator->LocateGlobalPointAndSetup(SourcePosition);
        G4String volName = volume ? volume->GetName() : "UNKNOWN";
        G4String matName = volume ? volume->GetLogicalVolume()->GetMaterial()->GetName() : "UNKNOWN";
        
        G4cout << "Event " << theEventNum 
               << ": Gamma E = " << sourceEnergy / MeV << " MeV"
               << ", Pos = (" << SourcePosition.x() / cm 
               << ", " << SourcePosition.y() / cm 
               << ", " << SourcePosition.z() / cm << ") cm"
               << ", Volume = " << volName
               << ", Material = " << matName
               << G4endl;
    }
    
    particleGun->GeneratePrimaryVertex(anEvent);
}
