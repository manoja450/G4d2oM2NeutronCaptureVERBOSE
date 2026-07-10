#include "G4d2oEventAction.hh"
#include "G4d2oDetectorHit.hh"
#include "G4d2oNeutrinoAlley.hh"
#include "G4d2oSensitiveDetector.hh"
#include "G4d2oDetector.hh"
#include "G4d2oRunAction.hh"
#include "G4d2oNeutronGun.hh"
#include "G4d2oPrimaryGeneratorAction.hh"
#include "G4d2oStepRecorder.hh"
#include "TrackInfoManager.hh"
#include "G4Navigator.hh"
#include "G4TransportationManager.hh"

#include "G4SDManager.hh"
#include "G4HCofThisEvent.hh"
#include "G4UnitsTable.hh"
#include "G4RunManager.hh"

#include "TFile.h"
#include "TMath.h"
#include "TString.h"
#include "TClassTable.h"
#include "inputVariables.hh"

#include <fstream>
#include <iomanip>

// ============================================================
// VERBOSE OUTPUT FILE
// ============================================================
static std::ofstream gVerboseLog;
static int gVerboseEventCount = 0;
static const int MAX_VERBOSE_EVENTS = 50;

// ============================================================
// Helper function to get parent type from track info
// ============================================================
int GetParentTypeFromTrackInfo(const TrackInfo& info, const TrackInfoManager* trackMgr) {
    // Parent type classification:
    // 1 = Muon
    // 2 = Michel e⁻ (muon decay)
    // 3 = EM Shower e⁻ (Compton, Photo, Pair)
    // 4 = Unclassified
    // 5 = Neutron Capture γ
    // 6 = EM Shower γ (Bremsstrahlung)
    // 7 = Delta Ray (Ionization e⁻)
    // 8 = Primary e⁻ (Beam)
    // 9 = Radioactive Decay
    // 10 = Neutron Inelastic
    // 11 = Cosmic Ray γ

    if (info.parentID == 0) {
        return 8; // Primary beam
    }

    const TrackInfo* parentInfo = trackMgr->GetTrackInfo(info.parentID);
    if (!parentInfo) {
        return 4; // Unclassified
    }

    if (parentInfo->parentType != 0) {
        return parentInfo->parentType;
    }

    int parentPDG = std::abs(parentInfo->pdg);
    G4String creatorProc = parentInfo->creatorProcess;
    G4String parentName = parentInfo->particleName;

    if (parentPDG == 22) {
        if (creatorProc == "nCapture" || creatorProc == "neutronCapture") {
            return 5;
        }
        if (creatorProc == "brems" || creatorProc == "Brem" ||
            creatorProc == "eBrem" || creatorProc == "muBrem") {
            return 6;
        }
        if (creatorProc == "Primary") {
            return 11;
        }
        return 6;
    }

    if (parentPDG == 11 || parentPDG == -11) {
        if (creatorProc == "muDecay" || creatorProc == "Decay") {
            return 2;
        }
        if (creatorProc == "eIoni" || creatorProc == "mIoni" ||
            creatorProc == "hIoni" || creatorProc == "muIoni" ||
            creatorProc == "Ionization") {
            return 7;
        }
        if (creatorProc == "compt" || creatorProc == "phot" ||
            creatorProc == "pair" || creatorProc == "conv") {
            return 3;
        }
        if (creatorProc == "Primary") {
            return 8;
        }
        if (creatorProc == "RadioactiveDecay") {
            return 9;
        }
        return 3;
    }

    if (parentPDG == 13 || parentPDG == -13) {
        return 1;
    }

    if (parentPDG == 2112) {
        if (creatorProc == "nInelastic" || creatorProc == "neutronInelastic") {
            return 10;
        }
        return 4;
    }

    if (parentPDG == 2212) {
        return 4;
    }

    return 4;
}

void PropagateParentType(TrackInfoManager* trackMgr) {
    const auto& allTracks = trackMgr->GetAllTracks();
    
    std::vector<int> trackIDs;
    for (const auto& pair : allTracks) {
        trackIDs.push_back(pair.first);
    }
    std::sort(trackIDs.begin(), trackIDs.end());
    
    for (int trackID : trackIDs) {
        TrackInfo* info = trackMgr->GetTrackInfoPtr(trackID);
        if (!info) continue;
        if (info->parentType != 0) continue;
        
        info->parentType = GetParentTypeFromTrackInfo(*info, trackMgr);
        
        if (info->pdg == 22 && (info->creatorProcess == "nCapture" || 
                                 info->creatorProcess == "neutronCapture")) {
            info->parentType = 5;
        }
    }
    
    for (int trackID : trackIDs) {
        TrackInfo* info = trackMgr->GetTrackInfoPtr(trackID);
        if (!info) continue;
        
        if (info->parentID != 0) {
            const TrackInfo* parentInfo = trackMgr->GetTrackInfo(info->parentID);
            if (parentInfo && parentInfo->parentType == 5) {
                info->parentType = 5;
            }
        }
    }
}

G4d2oEventAction::G4d2oEventAction(void)
{
    G4cerr << "\tConstructing G4d2oEventAction...";

    // Open verbose log file
    gVerboseLog.open("simulation_verbose.log", std::ios::out);
    gVerboseLog << "============================================================" << std::endl;
    gVerboseLog << "G4d2o Simulation Verbose Output" << std::endl;
    gVerboseLog << "============================================================" << std::endl;
    gVerboseLog << std::endl;

    HCoE = NULL;
    for (G4int i = 0; i < MAX_SEN_DET; i++)
    {
        detHC[i] = NULL;
        sourceHC[i] = NULL;
        targHC[i] = NULL;
    }

    numEvents = 0;

    input = inputVariables::GetIVPointer();

    G4d2oNeutrinoAlley *detCon = (G4d2oNeutrinoAlley *)G4RunManager::GetRunManager()->GetUserDetectorConstruction();
    theDet = (G4d2oDetector *)detCon->GetDetectorPtr();

    G4d2oRunAction *runAct = (G4d2oRunAction *)G4RunManager::GetRunManager()->GetUserRunAction();
    fout = TFile::Open(runAct->GetOutFileName(), "recreate");

    if (!TClassTable::GetDict("simEvent"))
    {
        G4cout << "Error in G4d2oEventAction::G4d2oEventAction() - class simEvent not found\n" << G4endl;
        exit(0);
    }
    if (!TClassTable::GetDict("simHit"))
    {
        G4cout << "Error in G4d2oEventAction::G4d2oEventAction() - class simHit not found\n" << G4endl;
        exit(0);
    }
    if (!TClassTable::GetDict("simAreaHit"))
    {
        G4cout << "Error in G4d2oEventAction::G4d2oEventAction() - class simAreaHit not found\n" << G4endl;
        exit(0);
    }
    theEventData = new simEvent();

    outTree = new TTree("Sim_Tree", "tree");
    outTree->Branch("eventData", "simEvent", &theEventData);

    numPMTs = theDet->GetTotalPMTS();
    numRows = theDet->GetPMTRows();
    numInEachRow = theDet->GetPMTsInRow();

    if (numPMTs != numRows * numInEachRow)
    {
        printf("\nError in G4d2oEventAction::G4d2oEventAction() - %d x %d != %d for PMT array.\n\n", numRows, numInEachRow, numPMTs);
        exit(0);
    }

    if (numPMTs > 0 && numRows > 0 && numInEachRow > 0)
    {
        hPMTArray = new TH2D("hPMTArray", "PMT hit pattern", numInEachRow, -0.5, numInEachRow - 0.5, numRows, -0.5, numRows - 0.5);
        hPMTNumVsTime = new TH2D("hPMTNumVsTime", "PMT number vs. time", 100, 0, 100, numPMTs, -0.5, numPMTs - 0.5);
    }
    else
    {
        hPMTArray = 0;
        hPMTNumVsTime = 0;
    }
    hPhotonEnergy = new TH1D("hPhotonEnergy", "Photon Energy", 500, 1.0, 10.0);
    hTotalPhotons = new TH1D("hTotalPhotons", "Total photons", 1000, 0, 10000);

    for (Int_t i = 0; i < 2; i++)
        hSidePMT[i] = 0;

    if (input->GetSideLining() == 1 && theDet->SidePMTX() > 0 && theDet->SidePMTZ() > 0)
    {
        for (Int_t i = 0; i < 2; i++)
        {
            hSidePMT[i] = new TH2D(Form("hSidePMT[%d]", i), Form("Side PMT %d", i),
                                   500, -theDet->SidePMTX() / 2.0, theDet->SidePMTX() / 2.0,
                                   500, -theDet->SidePMTZ() / 2.0, theDet->SidePMTZ() / 2.0);
        }
    }

    const G4VUserPrimaryGeneratorAction* constPga = G4RunManager::GetRunManager()->GetUserPrimaryGeneratorAction();
    thePGA = const_cast<G4VUserPrimaryGeneratorAction*>(constPga);

    G4cerr << "done." << G4endl;
}

G4d2oEventAction::~G4d2oEventAction()
{
    G4cout << "Deleting G4d2oEventAction";

    // Close verbose log file
    gVerboseLog << std::endl;
    gVerboseLog << "============================================================" << std::endl;
    gVerboseLog << "End of Verbose Output" << std::endl;
    gVerboseLog << "============================================================" << std::endl;
    gVerboseLog.close();

    if (outTree)
    {
        outTree->AutoSave();

        fout->cd();
        fout->mkdir("histos");
        fout->cd("histos");

        if (hPMTArray)
            hPMTArray->Write("pmtHits");
        if (hPMTNumVsTime)
            hPMTNumVsTime->Write("pmtNumVsTime");
        if (hPhotonEnergy)
            hPhotonEnergy->Write("photonEnergy");
        if (hTotalPhotons)
            hTotalPhotons->Write("photonsPerEvent");

        if (input->GetSideLining() == 1)
        {
            for (Int_t i = 0; i < 2; i++)
                if (hSidePMT[i])
                    hSidePMT[i]->Write(Form("sidePMTHits%d", i));
        }

        fout->mkdir("pmtPositions");
        fout->cd("pmtPositions");
        if (theDet->GetPMTPositionHistogram(0))
            theDet->GetPMTPositionHistogram(0)->Write("pmtPosX");
        if (theDet->GetPMTPositionHistogram(1))
            theDet->GetPMTPositionHistogram(1)->Write("pmtPosY");
        if (theDet->GetPMTPositionHistogram(2))
            theDet->GetPMTPositionHistogram(2)->Write("pmtPosZ");
    }

    G4String theOutFileName = fout->GetName();
    system("mkdir -p scripts");
    ofstream lastFile("scripts/lastFileName.txt");
    lastFile << fout->GetName() << endl;
    lastFile.close();

    delete fout;
    delete gBialkali_ev;

    G4cout << "...done" << G4endl << G4endl;
    G4cout << "\t**********************************************************************************" << G4endl;
    G4cout << "\t***   Output File: " << theOutFileName << " " << G4endl;
    G4cout << "\t**********************************************************************************" << G4endl;
    G4cout << G4endl << G4endl;
}

void G4d2oEventAction::BeginOfEventAction(const G4Event *thisEvent)
{
    fVetoEdep = 0.0;
    ResetNarrowVetoEdep();
    ResetWideVetoEdep();

    TrackInfoManager::GetInstance()->Clear();

    // ============================================================
    // FIX: DO NOT mute the StepRecorder - let it record steps
    // ============================================================
    G4d2oStepRecorder* stepRecorder = dynamic_cast<G4d2oStepRecorder*>(G4VSteppingVerbose::GetInstance());
    if (stepRecorder) {
        stepRecorder->SetSilent(false);  // ← RECORD STEPS!
    }
}

void G4d2oEventAction::EndOfEventAction(const G4Event *thisEvent)
{
    HCoE = thisEvent->GetHCofThisEvent();
    if (HCoE)
    {
        GetHitsCollection();
        ProcessEvent();
    }
    for (int i = 0; i < kNarrowPanels; ++i)
        theEventData->narrow_veto_edep[i] = fNarrowVetoEdep[i];
    for (int i = 0; i < kWidePanels; ++i)
        theEventData->wide_veto_edep[i] = fWideVetoEdep[i];
}

void G4d2oEventAction::GetHitsCollection(void)
{
    for (Int_t i = 0; i < input->numDetHC; i++)
        detHC[i] = (G4d2oDetectorHitsCollection *)HCoE->GetHC(input->detCollID[i]);
    for (Int_t i = 0; i < input->numSrcHC; i++)
        sourceHC[i] = (G4d2oDetectorHitsCollection *)HCoE->GetHC(input->srcCollID[i]);
    for (Int_t i = 0; i < input->numTargHC; i++)
        targHC[i] = (G4d2oDetectorHitsCollection *)HCoE->GetHC(input->targCollID[i]);
}

void G4d2oEventAction::ProcessEvent(void)
{
    theEventData->ClearData();
    ZeroEventVariables();
    numEvents++;
    eventNumber = numEvents;

    // ============================================================
    // VERBOSE LOGGING: Event header
    // ============================================================
    if (gVerboseEventCount < MAX_VERBOSE_EVENTS) {
        gVerboseLog << "\n" << std::string(80, '=') << std::endl;
        gVerboseLog << "EVENT " << eventNumber << std::endl;
        gVerboseLog << std::string(80, '=') << std::endl;
        gVerboseLog << std::endl;
    }

    PropagateParentType(TrackInfoManager::GetInstance());

    G4d2oDetectorHit *thisHit;

    G4ThreeVector initDir(0,0,0);
    G4ThreeVector initPos(0,0,0);
    G4double sourceEnergy = 0;

    G4d2oNeutronGun* neutronGun = dynamic_cast<G4d2oNeutronGun*>(thePGA);
    if (neutronGun) {
        initDir = neutronGun->GetInitialDirection();
        initPos = neutronGun->GetOriginalPosition();
        sourceEnergy = neutronGun->GetSourceEnergy();
    } else {
        G4d2oPrimaryGeneratorAction* primaryAction = dynamic_cast<G4d2oPrimaryGeneratorAction*>(thePGA);
        if (primaryAction) {
            initDir = primaryAction->GetInitialDirection();
            initPos = primaryAction->GetOriginalPosition();
            sourceEnergy = primaryAction->GetSourceEnergy();
        }
    }

    theEventData->direction0.SetXYZ(initDir.x(), initDir.y(), initDir.z());
    theEventData->position0.SetXYZ(initPos.x(), initPos.y(), initPos.z());
    theEventData->sourceParticleEnergy = sourceEnergy;
    theEventData->eventNumber = eventNumber;
    theEventData->vol0 = -1;

    // ============================================================
    // VERBOSE LOGGING: Primary particle info
    // ============================================================
    if (gVerboseEventCount < MAX_VERBOSE_EVENTS) {
        gVerboseLog << "Primary Neutron: " << std::endl;
        gVerboseLog << "  Position: (" << initPos.x()/cm << ", " << initPos.y()/cm << ", " << initPos.z()/cm << ") cm" << std::endl;
        gVerboseLog << "  Energy: " << sourceEnergy/MeV << " MeV" << std::endl;
        gVerboseLog << "  Direction: (" << initDir.x() << ", " << initDir.y() << ", " << initDir.z() << ")" << std::endl;
        gVerboseLog << std::endl;
    }

    G4Navigator *Navigator = G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking();

    G4VPhysicalVolume *volume0 = Navigator->LocateGlobalPointAndSetup(initPos);
    if (volume0->GetName() == "h2oPhysV")
        theEventData->vol0 = 2;
    else if (volume0->GetName() == "d2oPhysV")
        theEventData->vol0 = 1;
    else if (volume0->GetName() == "acrylicPhysV")
        theEventData->vol0 = 3;

    std::map<std::string, int> vmuid = {{"muVetoBI", 0}, {"muVetoBO", 1}, {"muVetoTI", 2}, {"muVetoTO", 3},
                                        {"muVetoLI", 4}, {"muVetoLO", 5}, {"muVetoRI", 6}, {"muVetoRO", 7},
                                        {"muVetoNI", 8}, {"muVetoNO", 9}, {"muVetoFI", 10}, {"muVetoFO", 11}};

    bool hasNeutronCapture = false;
    bool captureOnH = false;
    bool captureOnD = false;
    double captureT = 0.0;
    double captureGammaE = 0.0;
    double captureProductE = 0.0;
    double captureNeutronE = 0.0;
    TString captureProduct = "";
    TString captureVol = "";
    double capPosX = 0.0, capPosY = 0.0, capPosZ = 0.0;

    for (G4int iDetHC = 0; iDetHC < input->numDetHC; iDetHC++)
    {
        if (detHC[iDetHC])
        {
            TString SDname = detHC[iDetHC]->GetSDname().data();
            if (SDname.BeginsWith("muVetoInner") || SDname.BeginsWith("muVetoOuter") || SDname.BeginsWith("muVeto"))
            {
                int muid = 0;
                if (SDname.BeginsWith("muVetoInner")) muid = 0;
                else if (SDname.BeginsWith("muVetoOuter")) muid = 1;
                else muid = vmuid[SDname.Data()];

                for (G4int i = 0; i < detHC[iDetHC]->entries(); i++)
                {
                    thisHit = (*detHC[iDetHC])[i];
                    if (thisHit->GetPDGCharge() != 0)
                    {
                        theEventData->muVetoEnergy[muid] += thisHit->GetTotalEnergyDeposit();
                    }
                }
            }
            else
            {
                for (G4int i = 0; i < detHC[iDetHC]->entries(); i++)
                {
                    thisHit = (*detHC[iDetHC])[i];

                    if (thisHit->GetIsNeutronCapture())
                    {
                        hasNeutronCapture = true;
                        captureOnH = thisHit->GetCaptureOnHydrogen();
                        captureOnD = thisHit->GetCaptureOnDeuterium();
                        captureT = thisHit->GetCaptureTime();
                        captureGammaE = thisHit->GetCaptureGammaEnergy();
                        captureProductE = thisHit->GetCaptureProductEnergy();
                        captureNeutronE = thisHit->GetCaptureNeutronEnergy();
                        captureProduct = thisHit->GetCaptureProduct();
                        captureVol = thisHit->GetCaptureVolume();
                        capPosX = thisHit->GetCapturePosX();
                        capPosY = thisHit->GetCapturePosY();
                        capPosZ = thisHit->GetCapturePosZ();
                    }

                    G4String hitParticleName = thisHit->GetParticleName();
                    
                    // ============================================================
                    // VERBOSE LOGGING: Particle hits
                    // ============================================================
                    if (gVerboseEventCount < MAX_VERBOSE_EVENTS) {
                        G4String volName = thisHit->GetMaterialName();
                        G4double energy = thisHit->GetKineticEnergy();
                        G4double time = thisHit->GetGlobalTime();
                        gVerboseLog << "  Particle: " << hitParticleName 
                                   << " | E = " << energy/eV << " eV"
                                   << " | t = " << time/ns << " ns"
                                   << " | Vol = " << volName
                                   << " | PDG = " << thisHit->GetPDGEncoding()
                                   << std::endl;
                    }
                    
                    if (hitParticleName == "opticalphoton")
                    {
                        G4int hitPMTNumber = thisHit->GetDetectorNumber();
                        G4double hitGlobalTime = thisHit->GetGlobalTime();
                        G4double hitKineticEnergy = thisHit->GetKineticEnergy();
                        
                        G4int hitPDGCode = thisHit->GetPDGEncoding();
                        
                        G4int parentType = 4;
                        
                        G4int trackID = thisHit->GetTrackID();
                        const TrackInfo* trackInfo = TrackInfoManager::GetInstance()->GetTrackInfo(trackID);
                        
                        if (trackInfo) {
                            parentType = trackInfo->parentType;
                        }

                        // PMT QE scaling
                        if (hitPMTNumber == 0 && G4UniformRand() > 0.8329) continue;
                        if (hitPMTNumber == 1 && G4UniformRand() > 0.8197) continue;
                        if (hitPMTNumber == 2 && G4UniformRand() > 0.9205) continue;
                        if (hitPMTNumber == 3 && G4UniformRand() > 0.6747) continue;
                        if (hitPMTNumber == 4 && G4UniformRand() > 0.8538) continue;
                        if (hitPMTNumber == 5 && G4UniformRand() > 1.0000) continue;
                        if (hitPMTNumber == 6 && G4UniformRand() > 0.9951) continue;
                        if (hitPMTNumber == 7 && G4UniformRand() > 0.8934) continue;
                        if (hitPMTNumber == 8 && G4UniformRand() > 0.7615) continue;
                        if (hitPMTNumber == 9 && G4UniformRand() > 1.0000) continue;
                        if (hitPMTNumber == 10 && G4UniformRand() > 0.6164) continue;
                        if (hitPMTNumber == 11 && G4UniformRand() > 1.0000) continue;

                        if (detHC[iDetHC]->GetSDname() == "pmt")
                        {
                            theEventData->AddPMTHit(hitPMTNumber, hitGlobalTime, hitKineticEnergy, parentType);
                            if (hPMTArray)
                                hPMTArray->Fill(hitPMTNumber / numRows, hitPMTNumber % numRows);
                            if (hPMTNumVsTime)
                                hPMTNumVsTime->Fill(hitGlobalTime, hitPMTNumber);
                        }
                        hPhotonEnergy->Fill(hitKineticEnergy / eV);
                    }
                }
            }

            if (detHC[iDetHC]->GetSDname() == "topVetoSD")
            {
                if (detHC[iDetHC]->GetSize() > 0)
                {
                    theEventData->veto_tag = true;
                    theEventData->veto_edep = fVetoEdep;
                }
                else
                {
                    theEventData->veto_tag = false;
                    theEventData->veto_edep = fVetoEdep;
                }
            }

            if (detHC[iDetHC]->GetSDname() == "VetoNu1SD") {
                for (int i=0;i<kNarrowPanels;++i)
                    theEventData->narrow_veto_edep[i] = fNarrowVetoEdep[i];
            }

            if (detHC[iDetHC]->GetSDname() == "VetoNu2SD") {
                for (int i=0;i<kWidePanels;++i)
                    theEventData->wide_veto_edep[i] = fWideVetoEdep[i];
            }
        }
    }

    // ============================================================
    // VERBOSE LOGGING: Capture summary
    // ============================================================
    if (hasNeutronCapture && gVerboseEventCount < MAX_VERBOSE_EVENTS) {
        gVerboseLog << std::endl;
        gVerboseLog << "*** NEUTRON CAPTURE DETECTED ***" << std::endl;
        gVerboseLog << "  Type: " << (captureOnD ? "Deuterium (D2O)" : "Hydrogen (H2O)") << std::endl;
        gVerboseLog << "  Product: " << captureProduct << std::endl;
        gVerboseLog << "  Capture Time: " << captureT << " ns" << std::endl;
        gVerboseLog << "  Gamma Energy: " << captureGammaE << " MeV" << std::endl;
        gVerboseLog << "  Neutron Energy: " << captureNeutronE << " eV" << std::endl;
        gVerboseLog << "  Volume: " << captureVol << std::endl;
        gVerboseLog << "  Position: (" << capPosX << ", " << capPosY << ", " << capPosZ << ") cm" << std::endl;
        gVerboseLog << std::endl;
    }

    // ============================================================
    // STORE NEUTRON CAPTURE INFORMATION
    // ============================================================
    theEventData->neutronCaptured = hasNeutronCapture;
    theEventData->neutronCaptureOnHydrogen = captureOnH;
    theEventData->neutronCaptureOnDeuterium = captureOnD;
    theEventData->neutronCaptureTime = captureT;
    theEventData->neutronCaptureGammaEnergy = captureGammaE;
    theEventData->neutronCaptureProductEnergy = captureProductE;
    theEventData->neutronCaptureProduct = captureProduct;
    theEventData->neutronCaptureVolume = captureVol;
    theEventData->neutronCaptureNHits = theEventData->numHits;
    theEventData->neutronCapturePosX = capPosX;
    theEventData->neutronCapturePosY = capPosY;
    theEventData->neutronCapturePosZ = capPosZ;

    // ============================================================
    // RECORD STEP POINT DATA FROM STEP RECORDER
    // ============================================================
    theEventData->ClearStepData();

    // Get step data from StepRecorder using static methods
    std::vector<int>* trackID = G4d2oStepRecorder::GetTrackID();
    std::vector<int>* stepNumber = G4d2oStepRecorder::GetStepNumber();
    std::vector<int>* volumeCopy = G4d2oStepRecorder::GetVolumeCopy();
    std::vector<int>* processID = G4d2oStepRecorder::GetProcessID();
    std::vector<int>* pdg = G4d2oStepRecorder::GetPDG();
    std::vector<int>* parentID = G4d2oStepRecorder::GetParentID();
    std::vector<double>* posX = G4d2oStepRecorder::GetPosX();
    std::vector<double>* posY = G4d2oStepRecorder::GetPosY();
    std::vector<double>* posZ = G4d2oStepRecorder::GetPosZ();
    std::vector<double>* kineticEnergy = G4d2oStepRecorder::GetKineticEnergy();
    std::vector<double>* energyDeposit = G4d2oStepRecorder::GetEnergyDeposit();
    std::vector<double>* globalTime = G4d2oStepRecorder::GetGlobalTime();
    std::vector<double>* stepLength = G4d2oStepRecorder::GetStepLength();
    
    if (trackID && trackID->size() > 0) {
        // Debug: print how many steps were recorded
        if (eventNumber < 10) {
            G4cout << "[EventAction] Event " << eventNumber 
                   << ": Recording " << trackID->size() << " steps" << G4endl;
        }
        
        for (size_t i = 0; i < trackID->size(); i++) {
            theEventData->AddStepPoint(
                (*trackID)[i],
                (*stepNumber)[i],
                (*volumeCopy)[i],
                (*processID)[i],
                (*pdg)[i],
                (*parentID)[i],
                (*posX)[i],
                (*posY)[i],
                (*posZ)[i],
                (*kineticEnergy)[i],
                (*energyDeposit)[i],
                (*globalTime)[i],
                (*stepLength)[i]
            );
        }
        G4d2oStepRecorder::Clear();
    }

    // ============================================================
    // VERBOSE LOGGING: Event summary
    // ============================================================
    if (gVerboseEventCount < MAX_VERBOSE_EVENTS) {
        gVerboseLog << std::endl;
        gVerboseLog << "Event Summary:" << std::endl;
        gVerboseLog << "  Total PMT Hits: " << theEventData->numHits << std::endl;
        gVerboseLog << "  Neutron Captured: " << (hasNeutronCapture ? "YES" : "NO") << std::endl;
        gVerboseLog << "  Steps Recorded: " << (trackID ? trackID->size() : 0) << std::endl;
        gVerboseLog << std::string(80, '=') << std::endl;
        gVerboseLog << std::endl;
        gVerboseLog.flush();
        gVerboseEventCount++;
    }

    // ============================================================
    // RECORD ALL PARTICLES WITH NAMES
    // ============================================================
    theEventData->ClearAllParticles();
    
    const auto& allTracks = TrackInfoManager::GetInstance()->GetAllTracks();
    for (const auto& pair : allTracks) {
        int trackID = pair.first;
        const TrackInfo& info = pair.second;
        
        std::string particleName = info.particleName.data();
        
        int procID = 0;
        if (info.creatorProcess == "Primary") procID = 0;
        else if (info.creatorProcess == "nCapture") procID = 4131;
        else if (info.creatorProcess == "neutronCapture") procID = 4131;
        else if (info.creatorProcess == "compton") procID = 2013;
        else if (info.creatorProcess == "phot") procID = 2012;
        else if (info.creatorProcess == "conv") procID = 2014;
        else if (info.creatorProcess == "Decay") procID = 6201;
        else if (info.creatorProcess == "eIoni") procID = 2002;
        else if (info.creatorProcess == "mIoni") procID = 2002;
        else if (info.creatorProcess == "hIoni") procID = 2002;
        else if (info.creatorProcess == "muIoni") procID = 2002;
        else if (info.creatorProcess == "RadioactiveDecay") procID = 4210;
        else if (info.creatorProcess == "neutronInelastic") procID = 4121;
        else if (info.creatorProcess == "hadElastic") procID = 4111;
        
        theEventData->AddAllParticle(
            trackID,
            info.pdg,
            info.parentID,
            procID,
            info.energy,
            info.posX, info.posY, info.posZ,
            info.time,
            particleName
        );
    }

    if (eventNumber % 1000 == 0)
    {
        G4cout << "Completed Event " << eventNumber << G4endl;
        if (input->GetPGAType() == 10)
            G4cout << "Special Energy = " << input->GetSpecialEnergy() << G4endl;
    }

    outTree->Fill();
    hTotalPhotons->Fill(theEventData->numHits);
}

void G4d2oEventAction::ZeroEventVariables(void)
{
    eventNumber = 0;
}

double G4d2oEventAction::qe_ev(double energy_ev)
{
    if (!gBialkali_ev) return 0.0;
    if (energy_ev < gBialkali_ev->GetX()[0] || energy_ev > gBialkali_ev->GetX()[gBialkali_ev->GetN() - 1])
        return 0.0;
    return gBialkali_ev->Eval(energy_ev, 0, "");
}