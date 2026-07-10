#ifndef TrackInfoManager_hh
#define TrackInfoManager_hh 1

#include <map>
#include <string>
#include "G4String.hh"

struct TrackInfo {
    int trackID;
    int pdg;
    int parentID;
    G4String creatorProcess;
    double energy;
    double posX, posY, posZ;
    double time;
    int parentType;
    G4String particleName;  // NEW: Store particle name ("gamma", "opticalphoton", etc.)
    
    TrackInfo() 
        : trackID(0), pdg(0), parentID(0), energy(0), 
          posX(0), posY(0), posZ(0), time(0), parentType(0) {}
};

class TrackInfoManager {
public:
    static TrackInfoManager* GetInstance();
    
    // Add a track with all info including particle name
    void AddTrack(int trackID, int pdg, int parentID, 
                  G4String creatorProcess, double energy,
                  double posX, double posY, double posZ, double time,
                  G4String particleName = "");
    
    // Get track info (returns pointer, nullptr if not found)
    TrackInfo* GetTrackInfoPtr(int trackID);
    const TrackInfo* GetTrackInfo(int trackID) const;
    
    // Get all tracks
    const std::map<int, TrackInfo>& GetAllTracks() const { return fTrackMap; }
    
    // Clear all tracks
    void Clear();
    
    // Check if track exists
    bool HasTrack(int trackID) const;
    
private:
    TrackInfoManager() {}
    ~TrackInfoManager() {}
    
    std::map<int, TrackInfo> fTrackMap;
};

#endif