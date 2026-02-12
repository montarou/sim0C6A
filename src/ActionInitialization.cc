#include "ActionInitialization.hh"
#include "G4TrackingManager.hh"
#include "G4Trajectory.hh"
#include "G4TrajectoryContainer.hh"
#include "G4UserTrackingAction.hh"
#include "TrackingAction.hh"

ActionInitialization::ActionInitialization()
{}

ActionInitialization::~ActionInitialization()
{}

void ActionInitialization::BuildForMaster() const
{
    RunAction *runAction = new RunAction();
    SetUserAction(runAction);
}

void ActionInitialization::Build() const
{

    auto generator = new PrimaryGeneratorAction();
    SetUserAction(generator);

    auto eventAction = new EventAction();
    auto runAction = new RunAction(eventAction);

    // AJOUTER CETTE LIGNE
    eventAction->SetRunAction(runAction);

    SetUserAction(eventAction);
    SetUserAction(runAction);

    auto steppingAction = new SteppingAction(eventAction);
    SetUserAction(steppingAction);

    auto trackingAction = new TrackingAction();
    SetUserAction(trackingAction);

}
