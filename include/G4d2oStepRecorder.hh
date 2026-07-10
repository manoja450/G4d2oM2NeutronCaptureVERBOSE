#ifndef G4d2oStepRecorder_h
#define G4d2oStepRecorder_h 1

#include "G4SteppingVerbose.hh"
#include <vector>

class G4d2oStepRecorder : public G4SteppingVerbose
{
public:
    G4d2oStepRecorder();
    virtual ~G4d2oStepRecorder();
    
    virtual void TrackingStarted() override;
    virtual void StepInfo() override;
    
    // Clear all recorded data for current event
    static void Clear();
    
    // Get recorded data (static for global access)
    static std::vector<int>* GetTrackID() { return &fTrackID; }
    static std::vector<int>* GetStepNumber() { return &fStepNumber; }
    static std::vector<int>* GetVolumeCopy() { return &fVolumeCopy; }
    static std::vector<int>* GetProcessID() { return &fProcessID; }
    static std::vector<int>* GetPDG() { return &fPDG; }
    static std::vector<int>* GetParentID() { return &fParentID; }
    static std::vector<double>* GetPosX() { return &fPosX; }
    static std::vector<double>* GetPosY() { return &fPosY; }
    static std::vector<double>* GetPosZ() { return &fPosZ; }
    static std::vector<double>* GetKineticEnergy() { return &fKineticEnergy; }
    static std::vector<double>* GetEnergyDeposit() { return &fEnergyDeposit; }
    static std::vector<double>* GetGlobalTime() { return &fGlobalTime; }
    static std::vector<double>* GetStepLength() { return &fStepLength; }
    
    static size_t GetSize() { return fTrackID.size(); }
    void SetSilent(bool silent) { fSilent = silent; }
    bool IsSilent() const { return fSilent; }
    
    // Helper to print step data for debugging
    void PrintFirstSteps(int n = 10);
    
private:
    static std::vector<int> fTrackID;
    static std::vector<int> fStepNumber;
    static std::vector<int> fVolumeCopy;
    static std::vector<int> fProcessID;
    static std::vector<int> fPDG;
    static std::vector<int> fParentID;
    static std::vector<double> fPosX;
    static std::vector<double> fPosY;
    static std::vector<double> fPosZ;
    static std::vector<double> fKineticEnergy;
    static std::vector<double> fEnergyDeposit;
    static std::vector<double> fGlobalTime;
    static std::vector<double> fStepLength;
    
    bool fSilent;
};

#endif