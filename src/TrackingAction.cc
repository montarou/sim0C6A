#include "TrackingAction.hh"
#include "G4Track.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "G4ParticleDefinition.hh"

#include "G4TrackingManager.hh"
#include "G4Trajectory.hh"

TrackingAction::TrackingAction() {}

void TrackingAction::PreUserTrackingAction(const G4Track* track)
{
    // Demande à Geant4 de stocker les trajectoires
    fpTrackingManager->SetStoreTrajectory(true);

    // Crée une nouvelle trajectoire basée sur la particule suivie
    fpTrackingManager->SetTrajectory(new G4Trajectory(track));
}


