#ifndef STEPPINGACTION_HH
#define STEPPINGACTION_HH

#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include <set>

#include "DetectorConstruction.hh"
#include "EventAction.hh"

#include "SteppingMessenger.hh"
#include "globals.hh"

class SteppingAction : public G4UserSteppingAction
{
public:
    SteppingAction(EventAction* eventAction);
    ~SteppingAction();

    virtual void UserSteppingAction(const G4Step*);
    void SetVerbose(G4int level){fSteppingVerboseLevel = level;};

    // ==================== Step Tracking (print to output) ====================
    static void ResetTrackedParticlesCount();
    static G4int GetTrackedParticlesCount();
    static void SetMaxTrackedParticles(G4int n);
    static G4int GetMaxTrackedParticles();

private:
    EventAction *fEventAction;

    G4int fSteppingVerboseLevel = 0;
    SteppingMessenger* fSteppingMessenger;

    // ==================== Step Tracking ====================
    static G4int fMaxTrackedEvents;           // Nombre max d'événements à suivre
    static G4int fTrackedEventsCount;         // Compteur d'événements suivis
    static std::set<G4int> fTrackedEventIDs;  // Set des eventID suivis
    static std::set<G4int> fTrackedTrackIDs;  // Set des trackID à suivre dans l'événement courant
    static G4int fCurrentEventID;             // EventID courant
    
    void PrintStepInfo(const G4Step* step, G4int eventID);
    G4bool ShouldTrackParticle(const G4Track* track, G4int eventID);

};
#endif
