#include "PrimaryGeneratorAction.hh"

#include "PrimaryGeneratorAction0.hh"
#include "PrimaryGeneratorAction1.hh"
#include "PrimaryGeneratorAction2.hh"
#include "PrimaryGeneratorAction3.hh"

#include "PrimaryGeneratorMessenger.hh"

#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"

#include "G4TrackingManager.hh"
#include "G4Trajectory.hh"
#include "G4TrajectoryContainer.hh"
#include "G4UserTrackingAction.hh"

#include "Randomize.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction()
{
    G4int n_particle = 1;
    fParticleGun  = new G4ParticleGun(n_particle);

    fSelectedAction = 2;

    fParticleGun->SetParticlePosition(G4ThreeVector(0., 0., 0.001));  // z = +1 µm par défaut

    fAction0 = new PrimaryGeneratorAction0(fParticleGun);
    fAction1 = new PrimaryGeneratorAction1(fParticleGun);
    fAction2 = new PrimaryGeneratorAction2(fParticleGun);
    fAction3 = new PrimaryGeneratorAction3(fParticleGun);

    //create a messenger for this class
    fGunMessenger = new PrimaryGeneratorMessenger(this);

}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
    delete fAction0;
    delete fAction1;
    delete fAction2;
    delete fAction3;

    delete fGunMessenger;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event *anEvent)
{
    //G4cout << "ISelected generator fAction " << fSelectedAction<<G4endl;
    switch(fSelectedAction)
    {
        case 0:
            fAction0->GeneratePrimaries(anEvent);
            break;
        case 1:
            fAction1->GeneratePrimaries(anEvent);
            break;
        case 2:
            fAction2->GeneratePrimaries(anEvent);
            break;
        case 3:
            fAction3->GeneratePrimaries(anEvent);
            break;
        default:
            G4cerr << "Invalid generator fAction" << G4endl;
    }
}
