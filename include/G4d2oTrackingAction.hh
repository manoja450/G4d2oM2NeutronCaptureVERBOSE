#ifndef G4d2oTrackingAction_h
#define G4d2oTrackingAction_h 1

#include "G4UserTrackingAction.hh"

class G4d2oTrackingAction : public G4UserTrackingAction {
public:
    G4d2oTrackingAction();
    virtual ~G4d2oTrackingAction();
    virtual void PreUserTrackingAction(const G4Track* track);
};

#endif