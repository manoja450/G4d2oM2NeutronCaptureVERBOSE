#include <fstream>
#include <iostream>
#include <string>

#include "G4d2oSensitiveDetector.hh"
#include "inputVariables.hh"
#include "TrackInfoManager.hh"

#include "G4UnitsTable.hh"
#include "G4VProcess.hh"
#include "G4VParticleChange.hh"
#include "G4HadronicProcess.hh"
#include "G4Nucleus.hh"
#include "G4TrackStatus.hh"

#include "TH1.h"
#include "G4EventManager.hh"
#include "G4UserEventAction.hh"
#include "G4d2oEventAction.hh"
#include "G4SystemOfUnits.hh"

// Helper function to check if G4String contains a substring
inline bool stringContains(const G4String& str, const char* substr) {
    return str.find(substr) != std::string::npos;
}

G4d2oSensitiveDetector::G4d2oSensitiveDetector(G4String sdName, G4String sdMaterial, G4int iType)
    : G4VSensitiveDetector(sdName),
      sensitiveDetectorName(sdName),
      sensitiveMaterialName(sdMaterial),
      collectionID(-1)
{
    G4cerr << "\tInstance of G4d2oSensitiveDetector Constructed." << G4endl;
    G4cerr << "\t\tSensitive Detector Name: " << sensitiveDetectorName << G4endl;
    G4cerr << "\t\tSensitive Material Name: " << sensitiveMaterialName << G4endl;

    hitsCollectionName = sensitiveDetectorName;
    G4cerr << "\t\tHits Collection Name: " << hitsCollectionName << G4endl;

    G4d2oSensitiveDetector::collectionName.insert(hitsCollectionName);

    numHits = 0;
    dateTime = new TDatime();
    dateTime->Set();

    inputVariables *input = inputVariables::GetIVPointer();
    iprint = input->GetPrintStatus();

    if (iprint == 2)
        outfile = input->GetOutputFile();

    // Increased to 200 ms for D2O capture (~104 ms)
    timeCutOff = 200 * ms;
    numEvents = 0;
    sdType = iType;

    collectionID = input->GetNewCollectionID(iType);
    G4cerr << "\t\tHits Collection ID: " << collectionID << G4endl;
}

G4d2oSensitiveDetector::~G4d2oSensitiveDetector()
{
    G4cerr << "Deleting G4d2oSensitiveDetector (" << collectionID << ") ...";
    delete dateTime;
    G4cerr << "done" << G4endl;
}

void G4d2oSensitiveDetector::Initialize(G4HCofThisEvent *HCoE)
{
    if (collectionID < 0)
    {
        printf("ERROR: Hits collection ID not set properly.\n\n");
        exit(0);
    }

    hitsCollection = new G4d2oDetectorHitsCollection(sensitiveDetectorName, hitsCollectionName);
    HCoE->AddHitsCollection(collectionID, hitsCollection);
    numHits = 0;
    numEvents++;
}

G4bool G4d2oSensitiveDetector::ProcessHits(G4Step *theStep, G4TouchableHistory *roHist)
{
    roHist = 0;
    numHits++;

    if (numHits == 1) { sumTime = 0; sumED = 0; }

    G4Track *theTrack = theStep->GetTrack();
    const G4StepPoint *thePreStepPoint = theStep->GetPreStepPoint();
    const G4StepPoint *thePostStepPoint = theStep->GetPostStepPoint();
    theTotalEnergyDeposit = theStep->GetTotalEnergyDeposit();
    theDeltaTime = theStep->GetDeltaTime();

    const G4Material *mat0 = thePreStepPoint->GetMaterial();
    const G4Material *mat = thePostStepPoint->GetMaterial();
    G4String matname = mat0->GetName();
    if (mat) matname = mat->GetName();
    sprintf(theMaterialName, "%s", matname.c_str());

    const G4ParticleDefinition *theParticle = theTrack->GetDefinition();
    theTrackID = theTrack->GetTrackID();
    theTrackParentID = theTrack->GetParentID();
    theMomentumDirection = thePreStepPoint->GetMomentumDirection();
    thePostMomentumDirection = thePostStepPoint->GetMomentumDirection();

    // ============================================================
    // GET PDG ENCODING (for particle identification)
    // ============================================================
    thePDGEncoding = theParticle->GetPDGEncoding();
    
    // Get particle name (distinguishes gamma from opticalphoton)
    G4String particleName = theParticle->GetParticleName();

    // ============================================================
    // DETERMINE PARENT TYPE (using TrackInfoManager)
    // ============================================================
    theParentType = 4; // Default: Unclassified
    if (theTrackParentID > 0) {
        const TrackInfo* info = TrackInfoManager::GetInstance()->GetTrackInfo(theTrackParentID);
        if (info) {
            int parentPDG = std::abs(info->pdg);
            G4String parentProcess = info->creatorProcess;
            G4String parentName = info->particleName;

            // Use stored parent type if available
            if (info->parentType != 0) {
                theParentType = info->parentType;
            }
            // Determine parent type from parent info
            else if (parentPDG == 13) { // Muon
                theParentType = 1;
            } else if (parentPDG == 11) { // Electron
                if (parentProcess == "Decay") {
                    theParentType = 2; // Michel electron
                } else if (parentProcess == "compton" || parentProcess == "phot" ||
                           parentProcess == "conv" || parentProcess == "photonNuclear") {
                    // Check grandparent for neutron capture gamma
                    int grandParentID = info->parentID;
                    theParentType = 6; // EM Shower γ
                    if (grandParentID > 0) {
                        const TrackInfo* grandInfo = TrackInfoManager::GetInstance()->GetTrackInfo(grandParentID);
                        if (grandInfo && grandInfo->creatorProcess == "nCapture") {
                            theParentType = 5; // Neutron capture gamma
                        } else if (grandInfo && grandInfo->creatorProcess == "Primary") {
                            theParentType = 11; // Cosmic ray gamma
                        }
                    }
                } else if (parentProcess == "eIoni" || parentProcess == "mIoni" ||
                           parentProcess == "hIoni" || parentProcess == "muIoni") {
                    theParentType = 7; // Delta ray
                } else {
                    if (parentProcess == "Primary") {
                        theParentType = 8; // Primary electron
                    } else if (parentProcess == "RadioactiveDecay") {
                        theParentType = 9;
                    } else if (parentProcess == "neutronInelastic") {
                        theParentType = 10;
                    } else if (parentProcess == "nCapture") {
                        theParentType = 5; // Direct neutron capture electron
                    } else {
                        theParentType = 3; // Other electron
                    }
                }
            } else if (parentPDG == 22) { // Parent is a photon
                if (parentName == "gamma") {
                    // Check if this gamma came from neutron capture
                    if (parentProcess == "nCapture" || parentProcess == "neutronCapture") {
                        theParentType = 5; // Neutron capture gamma
                    } else {
                        theParentType = 6; // EM Shower gamma
                    }
                } else if (parentName == "opticalphoton") {
                    // Optical photons don't create other optical photons typically
                    theParentType = 4;
                }
            }
        }
    }

    // Information that can be obtained from theParticle
    G4String pName = theParticle->GetParticleName();
    sprintf(theParticleName, "%s", pName.c_str());
    thePDGCharge = theParticle->GetPDGCharge();
    theBaryonNumber = theParticle->GetBaryonNumber();
    theLeptonNumber = theParticle->GetLeptonNumber();

    theGlobalTime = thePostStepPoint->GetGlobalTime();
    theKineticEnergy = thePreStepPoint->GetKineticEnergy();

    if (theGlobalTime > timeCutOff)
        theTrack->SetTrackStatus(fStopAndKill);

    thePrePosition = thePreStepPoint->GetPosition();
    thePosition = thePostStepPoint->GetPosition();

    const G4VProcess *theProcessDefinedStep = (G4VProcess *)thePostStepPoint->GetProcessDefinedStep();
    if (theTrackID != 1)
    {
        const G4VProcess *theCreatorProcess = (G4VProcess *)theTrack->GetCreatorProcess();
        if (theCreatorProcess)
        {
            G4String pName2 = theCreatorProcess->GetProcessName();
            sprintf(theCreatorProcessName, "%s", pName2.c_str());
            pName2 = theCreatorProcess->GetProcessTypeName(theCreatorProcess->GetProcessType());
            sprintf(theCreatorProcessType, "%s", pName2.c_str());
        }
        else
        {
            sprintf(theCreatorProcessName, "Unknown");
            sprintf(theCreatorProcessType, " ");
        }
    }
    else
    {
        sprintf(theCreatorProcessName, "Source");
        sprintf(theCreatorProcessType, " ");
    }

    theDetectorNumber = -1;
    if (theTrack->GetTouchable()->GetHistoryDepth() > 0)
        theDetectorNumber = theTrack->GetTouchable()->GetCopyNumber(2);
    thePixelNumber = 0;

    sprintf(theProcessName, "empty");
    sprintf(theProcessTypeName, "empty");
    if (theProcessDefinedStep)
    {
        G4String pName2 = theProcessDefinedStep->GetProcessName();
        sprintf(theProcessName, "%s", pName2.c_str());
        pName2 = theProcessDefinedStep->GetProcessTypeName(theProcessDefinedStep->GetProcessType());
        sprintf(theProcessTypeName, "%s", pName2.c_str());
    }

    scatA = 0;
    scatZ = 0;

    if (theProcessDefinedStep->GetProcessType() == fHadronic && theProcessDefinedStep->GetProcessSubType() == fHadronElastic)
    {
        if (theStep->GetSecondary() && theStep->GetSecondary()->size())
        {
            std::vector<G4Track *>::const_iterator it;
            for (it = theStep->GetSecondary()->begin(); it != theStep->GetSecondary()->end(); it++)
            {
                if ((*it)->GetDynamicParticle()->GetParticleDefinition()->GetAtomicNumber() == 0 ||
                    (*it)->GetPosition() != thePosition)
                    continue;

                scatA = (*it)->GetDynamicParticle()->GetParticleDefinition()->GetBaryonNumber();
                scatZ = (*it)->GetDynamicParticle()->GetParticleDefinition()->GetPDGCharge();
            }
        }
    }

    calcA = 0;
    if (theProcessDefinedStep->GetProcessType() == fHadronic && theProcessDefinedStep->GetProcessSubType() == fHadronElastic)
    {
        G4double mN = 931.494061;
        G4double alpha = thePreStepPoint->GetKineticEnergy() - thePostStepPoint->GetKineticEnergy();
        G4ThreeVector p1 = thePreStepPoint->GetMomentum();
        G4ThreeVector p1p = thePostStepPoint->GetMomentum();
        G4double beta = (p1 - p1p).mag();
        calcA = (beta * beta - alpha * alpha) / (2.0 * alpha * mN);
    }

    // ============================================================
    // NEUTRON CAPTURE DETECTION
    // ============================================================
    G4bool isNeutronCapture = false;
    G4bool captureOnHydrogen = false;
    G4bool captureOnDeuterium = false;
    G4double captureTime = 0.0;
    G4double captureGammaEnergy = 0.0;
    G4double captureProductEnergy = 0.0;
    G4double captureNeutronEnergy = 0.0;  // NEW: neutron energy at capture
    G4String captureProduct = "";
    G4String captureVolume = thePreStepPoint->GetPhysicalVolume()->GetName();
    G4String captureMaterial = thePreStepPoint->GetMaterial()->GetName();
    G4double capturePosX = 0.0, capturePosY = 0.0, capturePosZ = 0.0;

    // DEBUG: Track neutrons entering water volumes
    if (pName == "neutron")
    {
        if (captureVolume == "h2oPhysV" || captureVolume == "d2oPhysV" ||
            captureVolume == "h2oLogV" || captureVolume == "d2oLogV")
        {
            static int neutronInWaterCount = 0;
            if (neutronInWaterCount < 50 && iprint <= 1)
            {
                G4cout << "[DEBUG] Neutron in " << captureVolume
                       << " | E = " << theKineticEnergy/eV << " eV"
                       << " | Time = " << theGlobalTime/ns << " ns"
                       << G4endl;
                neutronInWaterCount++;
            }
        }
    }

    // Check for capture process (NOT by particle name!)
    const G4VProcess *process = thePostStepPoint->GetProcessDefinedStep();
    if (process && pName == "neutron")
    {
        G4String procName = process->GetProcessName();
        if (procName == "nCapture" || procName == "neutronCapture")
        {
            isNeutronCapture = true;
            captureTime = theGlobalTime / ns;
            capturePosX = thePosition.x() / cm;
            capturePosY = thePosition.y() / cm;
            capturePosZ = thePosition.z() / cm;
            captureNeutronEnergy = theKineticEnergy / eV;  // NEW: store neutron energy in eV

            // Reliable distinction using MATERIAL name (not volume name)
            if (stringContains(captureMaterial, "Heavy") ||
                captureMaterial == "D2O" ||
                captureMaterial == "G4_HEAVY_WATER")
            {
                captureOnDeuterium = true;
                captureOnHydrogen = false;
                captureGammaEnergy = 6.25;          // MeV
                captureProduct = "Tritium";
                captureProductEnergy = 0.0;
            }
            else if (stringContains(captureMaterial, "Water") ||
                     captureMaterial == "H2O" ||
                     captureMaterial == "G4_WATER")
            {
                captureOnHydrogen = true;
                captureOnDeuterium = false;
                captureGammaEnergy = 2.22;          // MeV
                captureProduct = "Deuterium";
                captureProductEnergy = 0.0;
            }

            // Print capture information (first 50 captures)
            static int capCount = 0;
            if (capCount < 50 && iprint <= 1)
            {
                G4cout << "\n=========================================" << G4endl;
                if (captureOnDeuterium)
                    G4cout << "*** DEUTERIUM CAPTURE IN WATER DETECTED ***" << G4endl;
                else if (captureOnHydrogen)
                    G4cout << "*** HYDROGEN CAPTURE IN WATER DETECTED ***" << G4endl;
                else
                    G4cout << "*** NEUTRON CAPTURE IN OTHER MATERIAL ***" << G4endl;
                G4cout << "  Volume: " << captureVolume << G4endl;
                G4cout << "  Material: " << captureMaterial << G4endl;
                G4cout << "  Time: " << captureTime << " ns" << G4endl;
                G4cout << "  Gamma energy: " << captureGammaEnergy << " MeV" << G4endl;
                G4cout << "  Neutron energy: " << captureNeutronEnergy << " eV" << G4endl;
                G4cout << "  Position: (" << capturePosX << ", " << capturePosY << ", " << capturePosZ << ") cm" << G4endl;
                G4cout << "=========================================" << G4endl;
                capCount++;
            }
        }
    }

    if (iprint == 1)
    {
        G4cerr.setf(ios::fixed, ios::floatfield);
        if (numHits == 1 && sdType == 1) { G4cerr << G4endl << "Event # " << numEvents << G4endl; }
        G4cerr << " Hit # " << std::setw(3) << numHits << "  " << std::setw(3) << theTrackID << " " << std::setw(3) << theTrackParentID << " " << std::setw(3) << theDetectorNumber << " " << std::setw(3) << thePixelNumber;
        G4cerr << "  " << std::setw(16) << theCreatorProcessName;
        G4cerr << "  " << std::setw(10) << theParticleName;
        G4cerr.precision(4);
        G4cerr << "   KE = " << std::setw(8) << G4BestUnit(theKineticEnergy, "Energy");
        G4cerr.precision(2);
        G4cerr << "   gt = " << std::setw(6) << G4BestUnit(theGlobalTime, "Time");
        sumTime += theDeltaTime;
        G4cerr << "  " << std::setw(16) << theProcessName;
        G4cerr.precision(4);
        G4cerr << "  Ed = " << std::setw(8) << G4BestUnit(theTotalEnergyDeposit, "Energy");
        sumED += theTotalEnergyDeposit;
        G4cerr << "  Material = " << std::setw(16) << theMaterialName;
        G4cerr.precision(3);
        G4cerr << "   (px,py,pz)= " << std::setw(7) << theMomentumDirection.x() << " " << std::setw(7) << theMomentumDirection.y() << " " << std::setw(7) << theMomentumDirection.z();
        G4cerr.precision(1);
        G4cerr << "   (x,y,z)= " << std::setw(8) << thePosition.x() << " " << std::setw(8) << thePosition.y() << " " << std::setw(8) << thePosition.z();
        G4cerr << "   (x,y,z)= " << std::setw(8) << thePrePosition.x() << " " << std::setw(8) << thePrePosition.y() << " " << std::setw(8) << thePrePosition.z();
        if (isNeutronCapture) { G4cerr << " *** NEUTRON CAPTURE ***"; }
        G4cerr << G4endl;
    }
    if (iprint == 2)
    {
        outfile->setf(ios::fixed, ios::floatfield);
        if (numHits == 1 && sdType == 1) { *outfile << G4endl << "Event # " << numEvents << G4endl; }
        *outfile << " Hit # " << std::setw(3) << numHits << "  " << std::setw(3) << theTrackID << " " << std::setw(3) << theTrackParentID << " " << std::setw(3) << theDetectorNumber << " " << std::setw(3) << thePixelNumber;
        *outfile << "  " << std::setw(16) << theCreatorProcessName;
        *outfile << "  " << std::setw(10) << theParticleName;
        outfile->precision(4);
        *outfile << "   KE = " << std::setw(8) << G4BestUnit(theKineticEnergy, "Energy");
        outfile->precision(2);
        *outfile << "   gt = " << std::setw(6) << G4BestUnit(theGlobalTime, "Time");
        sumTime += theDeltaTime;
        *outfile << "  " << std::setw(16) << theProcessName;
        outfile->precision(4);
        *outfile << "  Ed = " << std::setw(8) << G4BestUnit(theTotalEnergyDeposit, "Energy");
        sumED += theTotalEnergyDeposit;
        *outfile << "  Material = " << std::setw(16) << theMaterialName;
        outfile->precision(3);
        *outfile << "   (px,py,pz)= " << std::setw(7) << theMomentumDirection.x() << " " << std::setw(7) << theMomentumDirection.y() << " " << std::setw(7) << theMomentumDirection.z();
        outfile->precision(1);
        *outfile << "   (x,y,z)= " << std::setw(8) << thePosition.x() << " " << std::setw(8) << thePosition.y() << " " << std::setw(8) << thePosition.z();
        *outfile << "   (x,y,z)= " << std::setw(8) << thePrePosition.x() << " " << std::setw(8) << thePrePosition.y() << " " << std::setw(8) << thePrePosition.z();
        if (isNeutronCapture) { *outfile << " *** NEUTRON CAPTURE ***"; }
        *outfile << G4endl;
    }

    InsertNewHit(isNeutronCapture, captureOnHydrogen, captureOnDeuterium,
                 captureTime, captureGammaEnergy, captureProductEnergy,
                 captureProduct.c_str(), captureVolume.c_str(),
                 capturePosX, capturePosY, capturePosZ, captureNeutronEnergy);

    if (matname == "Photo cathode (arb.)" && pName == "opticalphoton")
        theTrack->SetTrackStatus(fStopAndKill);
    else if (numHits > 1e7)
        theTrack->SetTrackStatus(fStopAndKill);

    return true;
}

void G4d2oSensitiveDetector::InsertNewHit(G4bool isNeutronCapture,
                                           G4bool captureOnHydrogen,
                                           G4bool captureOnDeuterium,
                                           G4double captureTime,
                                           G4double captureGammaEnergy,
                                           G4double captureProductEnergy,
                                           const char* captureProduct,
                                           const char* captureVolume,
                                           G4double capturePosX,
                                           G4double capturePosY,
                                           G4double capturePosZ,
                                           G4double captureNeutronEnergy)
{
    newHit = new G4d2oDetectorHit();
    hitsCollection->insert(newHit);

    newHit->SetPixelNumber(thePixelNumber);
    newHit->SetDetectorNumber(theDetectorNumber);
    newHit->SetTrackID(theTrackID);
    newHit->SetTrackParentID(theTrackParentID);
    newHit->SetScatA(scatA);
    newHit->SetScatZ(scatZ);

    newHit->SetTotalEnergyDeposit(theTotalEnergyDeposit);
    newHit->SetGlobalTime(theGlobalTime);
    newHit->SetKineticEnergy(theKineticEnergy);
    newHit->SetPDGCharge(thePDGCharge);
    newHit->SetLeptonNumber(theLeptonNumber);
    newHit->SetBaryonNumber(theBaryonNumber);
    newHit->SetDeltaTime(theDeltaTime);
    newHit->SetCalcA(calcA);

    newHit->SetParticleName(theParticleName);
    newHit->SetProcessName(theProcessName);
    newHit->SetProcessTypeName(theProcessTypeName);
    newHit->SetMaterialName(theMaterialName);
    newHit->SetCreatorProcessName(theCreatorProcessName);
    newHit->SetCreatorProcessType(theCreatorProcessType);

    newHit->SetMomentumDirection(theMomentumDirection);
    newHit->SetPostMomentumDirection(thePostMomentumDirection);
    newHit->SetPosition(thePosition);
    newHit->SetPrePosition(thePrePosition);
    newHit->SetVertex(theVertex);

    // ============================================================
    // SET PDG ENCODING AND PARENT TYPE
    // ============================================================
    newHit->SetPDGEncoding(thePDGEncoding);
    newHit->SetParentType(theParentType);

    // Set neutron capture information
    newHit->SetIsNeutronCapture(isNeutronCapture);
    newHit->SetCaptureOnHydrogen(captureOnHydrogen);
    newHit->SetCaptureOnDeuterium(captureOnDeuterium);
    newHit->SetCaptureTime(captureTime);
    newHit->SetCaptureGammaEnergy(captureGammaEnergy);
    newHit->SetCaptureProductEnergy(captureProductEnergy);
    newHit->SetCaptureNeutronEnergy(captureNeutronEnergy);  // NEW
    newHit->SetCaptureProduct(captureProduct);
    newHit->SetCaptureVolume(captureVolume);
    newHit->SetCapturePos(capturePosX, capturePosY, capturePosZ);
}

void G4d2oSensitiveDetector::EndOfEvent(G4HCofThisEvent *HCoE)
{
    HCoE = NULL;
}

G4String G4d2oSensitiveDetector::GetSensitiveDetectorName(void)
{
    return sensitiveDetectorName;
}

G4String G4d2oSensitiveDetector::GetSensitiveMaterialName(void)
{
    return sensitiveMaterialName;
}

G4String G4d2oSensitiveDetector::GetHitsCollectionName(void)
{
    return hitsCollectionName;
}