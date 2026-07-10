#include <stdlib.h>
#include <fstream>
#include <sys/time.h>

#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include "G4d2oPhysicsList.hh"
#include "G4d2oRunAction.hh"
#include "G4d2oNeutrinoAlley.hh"

#include "G4d2oEventAction.hh"
#include "G4d2oStackingAction.hh"
#include "G4d2oStepRecorder.hh"
#include "G4d2oTrackingAction.hh"

#include "G4d2oPhotonGun.hh"
#include "G4d2oElectronGun.hh"
#include "G4d2oElectronGunvD.hh"
#include "G4d2oElectronGunvO.hh"
#include "G4d2oSpecialElectronGun.hh"
#include "G4d2oMichelElectronGun.hh"
#include "G4d2oCosmicGun.hh"
#include "G4d2oMuonGun.hh"
#include "G4d2oNeutronGun.hh"
#include "G4d2oGammaGun.hh"
#include "G4d2oN16Gun.hh"

#include "inputVariables.hh"
#include "TRandom.h"
#include "G4GDMLParser.hh"
#include "G4UImanager.hh"

void export_gdml(std::string const& gdml_filename) {
    G4GDMLParser parser;
    parser.SetEnergyCutsExport(false);
    parser.SetSDExport(true);
    parser.SetOverlapCheck(true);
    parser.SetOutputFileOverwrite(true);
    parser.Write(gdml_filename,
                 G4TransportationManager::GetTransportationManager()
                     ->GetNavigatorForTracking()
                     ->GetWorldVolume()
                     ->GetLogicalVolume(),
                 true);
}

int main(int argc, char** argv) {
    // Read input
    inputVariables *input = inputVariables::GetIVPointer(argc, argv);
    G4int numEvents = input->GetNumberOfEvents();
    G4int ivis = input->GetVisualization();
    G4int irand = input->GetRandomStatus();
    G4int runno = input->GetRunNumber();
    G4int iPhysics = input->GetPhysicsType();
    G4int iNeutronHP = input->GetNeutronHP();
    G4int iPGA = input->GetPGAType();
    G4long userSEED = input->GetUserSEED();

    G4UIExecutive* ui = nullptr;
    if (ivis == 1) ui = new G4UIExecutive(argc, argv);

    G4RunManager *runManager = new G4RunManager;

    // ============================================================
    // STEP RECORDER INITIALIZATION
    // ============================================================
    if (G4VSteppingVerbose::GetInstance() == nullptr) {
        G4d2oStepRecorder* stepVerbose = new G4d2oStepRecorder();
        G4VSteppingVerbose::SetInstance(stepVerbose);
        stepVerbose->SetSilent(false);
        G4cout << "✓ G4d2oStepRecorder initialized (recording steps)" << G4endl;
    } else {
        G4VSteppingVerbose::GetInstance()->SetSilent(false);
        G4cout << "✓ StepRecorder already exists - set to record steps" << G4endl;
    }

    // Random seed
    timeval theTime;
    gettimeofday(&theTime, NULL);
    G4long iseed = (theTime.tv_sec * 1000) + (theTime.tv_usec / 1000);

    if (irand == 0) {
        iseed = 123456789;
        G4cout << "Using fixed seed: " << iseed << " (for debugging)" << G4endl;
        if (userSEED > 0) {
            iseed = userSEED;
            G4cout << "Overriding with user seed: " << iseed << G4endl;
        }
        gRandom->SetSeed(1);
    }
    if (irand == 1) {
        gRandom->SetSeed(0);
    }
    CLHEP::HepRandom::setTheSeed(iseed);

    FILE *outSeed = fopen("seedHistory.dat", "a");
    fprintf(outSeed, "Run %03d: %ld\n", runno, iseed);
    fclose(outSeed);

    // Detector construction
    G4cerr << "Constructing detector..." << G4endl;
    G4d2oNeutrinoAlley *detCon = new G4d2oNeutrinoAlley();
    runManager->SetUserInitialization(detCon);

    // Physics list
    G4cerr << "Constructing physics..." << G4endl;
    runManager->SetUserInitialization(new G4d2oPhysicsList(true, (iNeutronHP == 1)));

    // Tracking action
    runManager->SetUserAction(new G4d2oTrackingAction());

    // Primary generator action
    G4cerr << "Constructing primary generator action..." << G4endl;
    if (iPGA == 0) runManager->SetUserAction(new G4d2oPhotonGun);
    else if (iPGA == 1) runManager->SetUserAction(new G4d2oElectronGun);
    else if (iPGA == 2) runManager->SetUserAction(new G4d2oCosmicGun);
    else if (iPGA == 3) runManager->SetUserAction(new G4d2oMichelElectronGun);
    else if (iPGA == 4) runManager->SetUserAction(new G4d2oMuonGun);
    else if (iPGA == 6) runManager->SetUserAction(new G4d2oN16Gun);
    else if (iPGA == 7) runManager->SetUserAction(new G4d2oNeutronGun);
    else if (iPGA == 8) runManager->SetUserAction(new G4d2oElectronGunvD);
    else if (iPGA == 9) runManager->SetUserAction(new G4d2oElectronGunvO);
    else if (iPGA == 10) runManager->SetUserAction(new G4d2oSpecialElectronGun);
    else if (iPGA == 11) runManager->SetUserAction(new G4d2oGammaGun);

    // Run, event, stacking actions
    runManager->SetUserAction(new G4d2oRunAction);
    runManager->SetUserAction(new G4d2oEventAction);
    runManager->SetUserAction(new G4d2oStackingAction(detCon->GetDetectorPtr()));

    // Initialize G4 kernel
    G4cerr << "Initializing run..." << G4endl;
    runManager->Initialize();

    // ============================================================
    // ENABLE VERBOSE TRACKING OUTPUT
    // ============================================================
    G4UImanager* UI = G4UImanager::GetUIpointer();
    
    // Turn on tracking verbose output
    UI->ApplyCommand("/tracking/verbose 1");
    
    // Turn off event and run verbose to reduce clutter
    UI->ApplyCommand("/run/verbose 0");
    UI->ApplyCommand("/event/verbose 0");
    
    G4cout << "\n============================================================" << G4endl;
    G4cout << "✓ Verbose output settings:" << G4endl;
    G4cout << "  ✅ Neutrons    - TRACKING VISIBLE" << G4endl;
    G4cout << "  ✅ Gammas      - TRACKING VISIBLE" << G4endl;
    G4cout << "  ✅ Electrons   - TRACKING VISIBLE" << G4endl;
    G4cout << "  ✅ Deuterons   - TRACKING VISIBLE" << G4endl;
    G4cout << "  ✅ Tritons     - TRACKING VISIBLE" << G4endl;
    G4cout << "  ❌ Optical Photons - TRACKING SUPPRESSED (data still saved)" << G4endl;
    G4cout << "============================================================\n" << G4endl;

    // Visualization
    G4VisManager *visManager = nullptr;
    if (ivis > 0) {
        visManager = new G4VisExecutive(argc, argv);
        visManager->Initialize();
    }
    if (ivis == 1) {
        UI->ApplyCommand("/control/execute mac/vis-opengl.mac");
        ui->SessionStart();
        delete ui;
    }

    // Run
    runManager->BeamOn(numEvents);

    delete visManager;
    export_gdml("G4d2o-topCylindricalDet.gdml");
    delete runManager;

    return 0;
}