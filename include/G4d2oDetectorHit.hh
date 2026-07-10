#ifndef G4d2oDetectorHit_h
#define G4d2oDetectorHit_h 1

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4ThreeVector.hh"
#include "G4Allocator.hh"

class G4d2oDetectorHit : public G4VHit
{
private:
    G4int hitPixelNumber;
    G4int hitDetectorNumber;
    G4int hitTrackID;
    G4int hitTrackParentID;
    G4int hitScatA;
    G4int hitScatZ;
    G4double hitTotalEnergyDeposit;
    G4double hitGlobalTime;
    G4double hitKineticEnergy;
    G4double hitPDGCharge;
    G4double hitBaryonNumber;
    G4double hitLeptonNumber;
    G4double hitDeltaTime;
    G4double hitCalcA;
    char hitParticleName[100];
    char hitProcessName[100];
    char hitProcessTypeName[100];
    char hitMaterialName[100];
    char hitCreatorProcessName[100];
    char hitCreatorProcessType[100];
    G4ThreeVector hitPosition;
    G4ThreeVector hitPrePosition;
    G4ThreeVector hitVertex;
    G4ThreeVector hitMomentumDirection;
    G4ThreeVector hitPostMomentumDirection;

    // PDG and parent type
    G4int hitPDGEncoding;
    G4int hitParentType;

    // Neutron capture members
    G4bool hitIsNeutronCapture;
    G4bool hitCaptureOnHydrogen;
    G4bool hitCaptureOnDeuterium;
    G4double hitCaptureTime;
    G4double hitCaptureGammaEnergy;
    G4double hitCaptureProductEnergy;
    G4double hitCaptureNeutronEnergy;  // NEW: neutron energy at capture
    char hitCaptureProduct[100];
    char hitCaptureVolume[100];
    G4double hitCapturePosX;
    G4double hitCapturePosY;
    G4double hitCapturePosZ;

public:
    G4d2oDetectorHit();
    ~G4d2oDetectorHit();

    inline void* operator new(size_t);
    inline void operator delete(void* aHit);

    void Draw();
    void Print();

    // Existing getters/setters
    void SetPixelNumber(G4int);
    G4int GetPixelNumber();
    void SetDetectorNumber(G4int);
    G4int GetDetectorNumber();
    void SetTrackID(G4int);
    G4int GetTrackID();
    void SetTrackParentID(G4int);
    G4int GetTrackParentID();
    void SetScatA(G4int);
    G4int GetScatA();
    void SetScatZ(G4int);
    G4int GetScatZ();
    void SetTotalEnergyDeposit(G4double);
    G4double GetTotalEnergyDeposit();
    void SetGlobalTime(G4double);
    G4double GetGlobalTime();
    void SetKineticEnergy(G4double);
    G4double GetKineticEnergy();
    void SetPDGCharge(G4double);
    G4double GetPDGCharge();
    void SetBaryonNumber(G4double);
    G4double GetBaryonNumber();
    void SetLeptonNumber(G4double);
    G4double GetLeptonNumber();
    void SetDeltaTime(G4double);
    G4double GetDeltaTime();
    void SetCalcA(G4double);
    G4double GetCalcA();
    void SetParticleName(char*);
    char* GetParticleName();
    void SetProcessName(char*);
    char* GetProcessName();
    void SetProcessTypeName(char*);
    char* GetProcessTypeName();
    void SetMaterialName(char*);
    char* GetMaterialName();
    void SetCreatorProcessName(char*);
    char* GetCreatorProcessName();
    void SetCreatorProcessType(char*);
    char* GetCreatorProcessType();
    void SetMomentumDirection(G4ThreeVector);
    G4ThreeVector GetMomentumDirection();
    void SetPostMomentumDirection(G4ThreeVector);
    G4ThreeVector GetPostMomentumDirection();
    void SetPosition(G4ThreeVector);
    G4ThreeVector GetPosition();
    void SetPrePosition(G4ThreeVector);
    G4ThreeVector GetPrePosition();
    void SetVertex(G4ThreeVector);
    G4ThreeVector GetVertex();

    // PDG and parent type getters/setters
    void SetPDGEncoding(G4int code) { hitPDGEncoding = code; }
    G4int GetPDGEncoding() const { return hitPDGEncoding; }
    void SetParentType(G4int type) { hitParentType = type; }
    G4int GetParentType() const { return hitParentType; }

    // Neutron capture getters/setters
    void SetIsNeutronCapture(G4bool flag) { hitIsNeutronCapture = flag; }
    G4bool GetIsNeutronCapture() const { return hitIsNeutronCapture; }
    void SetCaptureOnHydrogen(G4bool flag) { hitCaptureOnHydrogen = flag; }
    G4bool GetCaptureOnHydrogen() const { return hitCaptureOnHydrogen; }
    void SetCaptureOnDeuterium(G4bool flag) { hitCaptureOnDeuterium = flag; }
    G4bool GetCaptureOnDeuterium() const { return hitCaptureOnDeuterium; }
    void SetCaptureTime(G4double t) { hitCaptureTime = t; }
    G4double GetCaptureTime() const { return hitCaptureTime; }
    void SetCaptureGammaEnergy(G4double e) { hitCaptureGammaEnergy = e; }
    G4double GetCaptureGammaEnergy() const { return hitCaptureGammaEnergy; }
    void SetCaptureProductEnergy(G4double e) { hitCaptureProductEnergy = e; }
    G4double GetCaptureProductEnergy() const { return hitCaptureProductEnergy; }
    void SetCaptureNeutronEnergy(G4double e) { hitCaptureNeutronEnergy = e; }  // NEW
    G4double GetCaptureNeutronEnergy() const { return hitCaptureNeutronEnergy; }  // NEW
    void SetCaptureProduct(const char* prod) { snprintf(hitCaptureProduct, sizeof(hitCaptureProduct), "%s", prod); }
    char* GetCaptureProduct() { return hitCaptureProduct; }
    void SetCaptureVolume(const char* vol) { snprintf(hitCaptureVolume, sizeof(hitCaptureVolume), "%s", vol); }
    char* GetCaptureVolume() { return hitCaptureVolume; }
    void SetCapturePos(G4double x, G4double y, G4double z) { hitCapturePosX = x; hitCapturePosY = y; hitCapturePosZ = z; }
    G4double GetCapturePosX() const { return hitCapturePosX; }
    G4double GetCapturePosY() const { return hitCapturePosY; }
    G4double GetCapturePosZ() const { return hitCapturePosZ; }
};

typedef G4THitsCollection<G4d2oDetectorHit> G4d2oDetectorHitsCollection;
extern G4Allocator<G4d2oDetectorHit> G4d2oDetectorHitAllocator;

inline void* G4d2oDetectorHit::operator new(size_t) {
    void* aHit = (void*)G4d2oDetectorHitAllocator.MallocSingle();
    return aHit;
}
inline void G4d2oDetectorHit::operator delete(void* aHit) {
    G4d2oDetectorHitAllocator.FreeSingle((G4d2oDetectorHit*)aHit);
}

#endif