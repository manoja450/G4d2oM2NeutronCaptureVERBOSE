#include "TrackInfoManager.hh"

TrackInfoManager* TrackInfoManager::GetInstance() {
    static TrackInfoManager instance;
    return &instance;
}

void TrackInfoManager::AddTrack(int trackID, int pdg, int parentID, 
                                G4String creatorProcess, double energy,
                                double posX, double posY, double posZ, double time,
                                G4String particleName) {
    TrackInfo info;
    info.trackID = trackID;
    info.pdg = pdg;
    info.parentID = parentID;
    info.creatorProcess = creatorProcess;
    info.energy = energy;
    info.posX = posX;
    info.posY = posY;
    info.posZ = posZ;
    info.time = time;
    info.parentType = 0;
    info.particleName = particleName;  // NEW: Store particle name
    fTrackMap[trackID] = info;
}

TrackInfo* TrackInfoManager::GetTrackInfoPtr(int trackID) {
    auto it = fTrackMap.find(trackID);
    if (it != fTrackMap.end()) {
        return &(it->second);
    }
    return nullptr;
}

const TrackInfo* TrackInfoManager::GetTrackInfo(int trackID) const {
    auto it = fTrackMap.find(trackID);
    if (it != fTrackMap.end()) {
        return &(it->second);
    }
    return nullptr;
}

bool TrackInfoManager::HasTrack(int trackID) const {
    return fTrackMap.find(trackID) != fTrackMap.end();
}

void TrackInfoManager::Clear() {
    fTrackMap.clear();
}