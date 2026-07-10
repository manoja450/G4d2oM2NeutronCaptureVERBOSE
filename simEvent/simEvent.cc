#include "simEvent.h"
#include "TClonesArray.h"
#include "G4d2oGeom.h"
#include <vector>
#include <numeric>
#include <algorithm>
#include <functional>

using namespace std;

ClassImp(simEvent)

TClonesArray *simEvent::sPMTHits = 0;
TClonesArray *simEvent::sAreaPMTHits = 0;

simEvent::simEvent(Int_t maxPMT)
{
  if (!sPMTHits)
    sPMTHits = new TClonesArray("simHit", maxPMT);
  sPMTHits->SetOwner(true);
  pmtHits = sPMTHits;

  if (!sAreaPMTHits)
    sAreaPMTHits = new TClonesArray("simAreaHit", maxPMT);
  sAreaPMTHits->SetOwner(true);
  areaPMTHits = sAreaPMTHits;

  ClearData();
}

simEvent::~simEvent()
{
  pmtHits->Clear("C");
  areaPMTHits->Clear("C");
}

void simEvent::AddPMTHit(Int_t pNum, Double_t eTime, Double_t phEn, Int_t pType)
{
  simHit *theHit = (simHit *)pmtHits->ConstructedAt(numHits++);
  theHit->Set(pNum, eTime, phEn, pType);
}

void simEvent::AddAreaPMTHit(Int_t pNum, Double_t eTime, Double_t phEn, TVector3 hitPos)
{
  simAreaHit *theHit = (simAreaHit *)areaPMTHits->ConstructedAt(numHitsArea++);
  theHit->Set(pNum, eTime, phEn, hitPos, 0);
}

// ============================================================
// ALL PARTICLES METHODS
// ============================================================
void simEvent::AddAllParticle(int trackID, int pdg, int parentID, int procID,
                              double energy, double posX, double posY, double posZ,
                              double time, const std::string& name)
{
    allParticleTrackID.push_back(trackID);
    allParticlePDG.push_back(pdg);
    allParticleParentID.push_back(parentID);
    allParticleProcessID.push_back(procID);
    allParticleEnergy.push_back(energy);
    allParticlePosX.push_back(posX);
    allParticlePosY.push_back(posY);
    allParticlePosZ.push_back(posZ);
    allParticleTime.push_back(time);
    allParticleName.push_back(name);
}

void simEvent::ClearAllParticles()
{
    allParticleTrackID.clear();
    allParticlePDG.clear();
    allParticleParentID.clear();
    allParticleProcessID.clear();
    allParticleEnergy.clear();
    allParticlePosX.clear();
    allParticlePosY.clear();
    allParticlePosZ.clear();
    allParticleTime.clear();
    allParticleName.clear();
}

// ============================================================
// STEP POINT METHODS
// ============================================================
void simEvent::AddStepPoint(int trackID, int stepNumber, int volumeCopy, int processID,
                            int pdg, int parentID, double posX, double posY, double posZ,
                            double kineticEnergy, double energyDeposit,
                            double globalTime, double stepLength)
{
    stepPointTrackID.push_back(trackID);
    stepPointStepNumber.push_back(stepNumber);
    stepPointVolumeCopy.push_back(volumeCopy);
    stepPointProcessID.push_back(processID);
    stepPointPDG.push_back(pdg);
    stepPointParentID.push_back(parentID);
    stepPointPosX.push_back(posX);
    stepPointPosY.push_back(posY);
    stepPointPosZ.push_back(posZ);
    stepPointKineticEnergy.push_back(kineticEnergy);
    stepPointEnergyDeposit.push_back(energyDeposit);
    stepPointGlobalTime.push_back(globalTime);
    stepPointStepLength.push_back(stepLength);
}

void simEvent::ClearStepData()
{
    stepPointTrackID.clear();
    stepPointStepNumber.clear();
    stepPointVolumeCopy.clear();
    stepPointProcessID.clear();
    stepPointPDG.clear();
    stepPointParentID.clear();
    stepPointPosX.clear();
    stepPointPosY.clear();
    stepPointPosZ.clear();
    stepPointKineticEnergy.clear();
    stepPointEnergyDeposit.clear();
    stepPointGlobalTime.clear();
    stepPointStepLength.clear();
}

void simEvent::ClearData()
{
  eventNumber = -1;
  direction0.SetXYZ(0.0, 0.0, 0.0);
  position0.SetXYZ(0.0, 0.0, 0.0);
  sourceParticleEnergy = 0;
  vol0 = 0;
  veto_tag = false;
  veto_edep = 0.0;

  for (int i=0;i<kNarrowPanels;++i) narrow_veto_edep[i] = 0.0;
  for (int i=0;i<kWidePanels;++i) wide_veto_edep[i] = 0.0;
  numHits = 0;
  pmtHits->Clear();

  numHitsArea = 0;
  areaPMTHits->Clear();
  for (int i = 0; i < 12; i++)
    muVetoEnergy[i] = 0.0;

  // ============================================================
  // Clear neutron capture information
  // ============================================================
  neutronCaptured = false;
  neutronCaptureOnHydrogen = false;
  neutronCaptureOnDeuterium = false;
  neutronCaptureTime = 0.0;
  neutronCaptureGammaEnergy = 0.0;
  neutronCaptureDelay = 0.0;
  neutronCaptureVolume = "";
  neutronCaptureNHits = 0;
  neutronCapturePosX = 0.0;
  neutronCapturePosY = 0.0;
  neutronCapturePosZ = 0.0;
  neutronCaptureProductEnergy = 0.0;
  neutronCaptureProduct = "";

  // Clear all particles data
  ClearAllParticles();
  
  // Clear step point data
  ClearStepData();
}

void simEvent::CopyData(simEvent *dataToCopy)
{
  eventNumber = dataToCopy->eventNumber;
  direction0 = TVector3(dataToCopy->direction0);
  position0 = TVector3(dataToCopy->position0);
  sourceParticleEnergy = dataToCopy->sourceParticleEnergy;
  vol0 = dataToCopy->vol0;

  numHits = dataToCopy->numHits;
  pmtHits = (TClonesArray *)dataToCopy->pmtHits->Clone();

  numHitsArea = dataToCopy->numHitsArea;
  areaPMTHits = (TClonesArray *)dataToCopy->areaPMTHits->Clone();
  for (int i = 0; i < 12; i++)
    muVetoEnergy[i] = dataToCopy->muVetoEnergy[i];
    
  // Copy neutron capture information
  neutronCaptured = dataToCopy->neutronCaptured;
  neutronCaptureOnHydrogen = dataToCopy->neutronCaptureOnHydrogen;
  neutronCaptureOnDeuterium = dataToCopy->neutronCaptureOnDeuterium;
  neutronCaptureTime = dataToCopy->neutronCaptureTime;
  neutronCaptureGammaEnergy = dataToCopy->neutronCaptureGammaEnergy;
  neutronCaptureDelay = dataToCopy->neutronCaptureDelay;
  neutronCaptureVolume = dataToCopy->neutronCaptureVolume;
  neutronCaptureNHits = dataToCopy->neutronCaptureNHits;
  neutronCapturePosX = dataToCopy->neutronCapturePosX;
  neutronCapturePosY = dataToCopy->neutronCapturePosY;
  neutronCapturePosZ = dataToCopy->neutronCapturePosZ;
  neutronCaptureProductEnergy = dataToCopy->neutronCaptureProductEnergy;
  neutronCaptureProduct = dataToCopy->neutronCaptureProduct;
  
  // Copy all particles data
  allParticleTrackID = dataToCopy->allParticleTrackID;
  allParticlePDG = dataToCopy->allParticlePDG;
  allParticleParentID = dataToCopy->allParticleParentID;
  allParticleProcessID = dataToCopy->allParticleProcessID;
  allParticleEnergy = dataToCopy->allParticleEnergy;
  allParticlePosX = dataToCopy->allParticlePosX;
  allParticlePosY = dataToCopy->allParticlePosY;
  allParticlePosZ = dataToCopy->allParticlePosZ;
  allParticleTime = dataToCopy->allParticleTime;
  allParticleName = dataToCopy->allParticleName;
  
  // Copy step point data
  stepPointTrackID = dataToCopy->stepPointTrackID;
  stepPointStepNumber = dataToCopy->stepPointStepNumber;
  stepPointVolumeCopy = dataToCopy->stepPointVolumeCopy;
  stepPointProcessID = dataToCopy->stepPointProcessID;
  stepPointPDG = dataToCopy->stepPointPDG;
  stepPointParentID = dataToCopy->stepPointParentID;
  stepPointPosX = dataToCopy->stepPointPosX;
  stepPointPosY = dataToCopy->stepPointPosY;
  stepPointPosZ = dataToCopy->stepPointPosZ;
  stepPointKineticEnergy = dataToCopy->stepPointKineticEnergy;
  stepPointEnergyDeposit = dataToCopy->stepPointEnergyDeposit;
  stepPointGlobalTime = dataToCopy->stepPointGlobalTime;
  stepPointStepLength = dataToCopy->stepPointStepLength;
}

double simEvent::MeanX() const
{
  double x = 0.0;
  for (int ihit = 0; ihit < numHits; ihit++)
  {
    x += G4d2oGeom::Instance()->DetPos(GetHit(ihit)->pmtNum).X();
  }
  x /= numHits;
  return x;
}

double simEvent::MeanY() const
{
  double x = 0.0;
  for (int ihit = 0; ihit < numHits; ihit++)
  {
    x += G4d2oGeom::Instance()->DetPos(GetHit(ihit)->pmtNum).Y();
  }
  x /= numHits;
  return x;
}

double simEvent::TimeRMS() const
{
  std::vector<double> vtimes;
  vtimes.resize(numHits);
  for (int ihit = 0; ihit < numHits; ihit++)
  {
    vtimes[ihit] = GetHit(ihit)->eventTime;
  }
  double sum = std::accumulate(vtimes.begin(), vtimes.end(), 0.0);
  double mean = sum / vtimes.size();

  std::vector<double> diff(vtimes.size());
  std::transform(vtimes.begin(), vtimes.end(), diff.begin(), [mean](double x)
                 { return x - mean; });
  double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
  double stdev = std::sqrt(sq_sum / vtimes.size());
  return stdev;
}

double simEvent::MeanZ() const
{
  double x = 0.0;
  for (int ihit = 0; ihit < numHits; ihit++)
  {
    x += G4d2oGeom::Instance()->DetPos(GetHit(ihit)->pmtNum).Z();
  }
  x /= numHits;
  return x;
}

double simEvent::WeightO() const
{
  return G4d2oGeom::Instance()->ProbO(position0, direction0, sourceParticleEnergy);
}

double simEvent::WeightD() const
{
  return G4d2oGeom::Instance()->ProbD(position0, direction0, sourceParticleEnergy);
}

double simEvent::SourceCosThe() const
{
  return G4d2oGeom::Instance()->ParticleCosThe(position0, direction0);
}

int simEvent::NumVetoPairs(double threshold /*MeV*/) const
{
  int numpairs = 0;
  for (int i = 0; i < 12; i += 2)
  {
    if (muVetoEnergy[i] > threshold && muVetoEnergy[i + 1] > threshold)
      numpairs++;
  }
  return numpairs;
}