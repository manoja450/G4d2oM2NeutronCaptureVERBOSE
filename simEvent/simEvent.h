#ifndef __simEvent_H__
#define __simEvent_H__

#include "TVector3.h"
#include "TClonesArray.h"
#include "TString.h"
#include <vector>
#include <string>

class simHit : public TObject
{
public:
    simHit() { ; }
    simHit(Int_t pNum, Double_t eTime, Double_t phEn, Int_t pType) 
    { 
        Set(pNum, eTime, phEn, pType); 
    }

    void Set(Int_t pNum, Double_t eTime, Double_t phEn, Int_t pType)
    {
        pmtNum = pNum;
        eventTime = eTime;
        photonEnergy = phEn;
        parentType = pType;
    }

    Int_t pmtNum;
    Double_t eventTime;
    Double_t photonEnergy;
    Int_t parentType;

    ClassDef(simHit, 2)
};

class simAreaHit : public simHit
{
public:
    simAreaHit() { ; }
    simAreaHit(Int_t pNum, Double_t eTime, Double_t phEn, TVector3 hitPos, Int_t pType) 
        : simHit(pNum, eTime, phEn, pType) 
    { 
        Set(hitPos); 
    }

    void Set(TVector3 hitPos)
    {
        photonPosition = hitPos;
    }

    void Set(Int_t pNum, Double_t eTime, Double_t phEn, TVector3 hitPos, Int_t pType)
    {
        pmtNum = pNum;
        eventTime = eTime;
        photonEnergy = phEn;
        parentType = pType;
        photonPosition = hitPos;
    }

    TVector3 photonPosition;

    ClassDef(simAreaHit, 2)
};

class simEvent : public TObject
{
public:
    simEvent(Int_t maxHits = 10000);
    virtual ~simEvent();

    static const int kNarrowPanels = 4;
    double narrow_veto_edep[kNarrowPanels] = {0, 0, 0, 0};
    static const int kWidePanels = 4;
    double wide_veto_edep[kWidePanels] = {0, 0, 0, 0};
    Int_t eventNumber;
    TVector3 direction0;
    TVector3 position0;
    int vol0;
    bool veto_tag{false};
    double veto_edep{0.0};

    Double_t sourceParticleEnergy;

    Int_t numHits;
    Int_t numHitsArea;

    Double_t muVetoEnergy[12];

    TClonesArray *pmtHits;
    static TClonesArray *sPMTHits;

    TClonesArray *areaPMTHits;
    static TClonesArray *sAreaPMTHits;

    // ============================================================
    // NEUTRON CAPTURE INFORMATION
    // ============================================================
    bool neutronCaptured;                    // Whether neutron capture occurred
    bool neutronCaptureOnHydrogen;           // Capture on H2O
    bool neutronCaptureOnDeuterium;          // Capture on D2O
    double neutronCaptureTime;               // Time of capture (ns)
    double neutronCaptureGammaEnergy;        // Gamma energy from capture (MeV)
    double neutronCaptureDelay;              // Delay between production and capture (ns)
    TString neutronCaptureVolume;            // Volume where capture occurred
    int neutronCaptureNHits;                 // Number of PMT hits from capture
    double neutronCapturePosX;               // X position of capture (cm)
    double neutronCapturePosY;               // Y position of capture (cm)
    double neutronCapturePosZ;               // Z position of capture (cm)
    double neutronCaptureProductEnergy;      // Energy of capture product (eV)
    TString neutronCaptureProduct;           // Capture product name (Deuterium/Tritium)

    // ============================================================
    // ALL PARTICLES DATA
    // ============================================================
    std::vector<int> allParticleTrackID;
    std::vector<int> allParticlePDG;
    std::vector<int> allParticleParentID;
    std::vector<int> allParticleProcessID;
    std::vector<double> allParticleEnergy;
    std::vector<double> allParticlePosX;
    std::vector<double> allParticlePosY;
    std::vector<double> allParticlePosZ;
    std::vector<double> allParticleTime;
    std::vector<std::string> allParticleName;  // Particle name ("gamma", "opticalphoton", etc.)

    // ============================================================
    // STEP POINT DATA
    // ============================================================
    std::vector<int> stepPointTrackID;
    std::vector<int> stepPointStepNumber;
    std::vector<int> stepPointVolumeCopy;
    std::vector<int> stepPointProcessID;
    std::vector<int> stepPointPDG;
    std::vector<int> stepPointParentID;
    std::vector<double> stepPointPosX;
    std::vector<double> stepPointPosY;
    std::vector<double> stepPointPosZ;
    std::vector<double> stepPointKineticEnergy;
    std::vector<double> stepPointEnergyDeposit;
    std::vector<double> stepPointGlobalTime;
    std::vector<double> stepPointStepLength;

    void AddPMTHit(Int_t pNum, Double_t eTime, Double_t phEn, Int_t pType);
    void AddAreaPMTHit(Int_t pNum, Double_t eTime, Double_t phEn, TVector3 hitPos);
    
    // All particles methods
    void AddAllParticle(int trackID, int pdg, int parentID, int procID,
                        double energy, double posX, double posY, double posZ,
                        double time, const std::string& name);
    void ClearAllParticles();
    
    // Step point methods
    void AddStepPoint(int trackID, int stepNumber, int volumeCopy, int processID,
                      int pdg, int parentID, double posX, double posY, double posZ,
                      double kineticEnergy, double energyDeposit,
                      double globalTime, double stepLength);
    void ClearStepData();
    
    double MeanX() const;
    double MeanY() const;
    double MeanZ() const;
    double TimeRMS() const;
    double WeightO() const;
    double WeightD() const;
    double SourceCosThe() const;
    int NumVetoPairs(double threshold = 5.0 /*MeV*/) const;

    const simHit *GetHit(int ihit) const { return dynamic_cast<simHit *>(pmtHits->At(ihit)); }
    const simAreaHit *GetAHit(int ihit) const { return dynamic_cast<simAreaHit *>(areaPMTHits->At(ihit)); }

    virtual void ClearData();
    virtual void CopyData(simEvent *dataToCopy);
    
    ClassDef(simEvent, 8)
};

#endif