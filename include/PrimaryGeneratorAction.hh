#ifndef PRIMARYGENERATORACTION_HH
#define PRIMARYGENERATORACTION_HH

#include "G4VUserPrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleTable.hh"
#include "G4Geantino.hh"
#include "G4IonTable.hh"
#include "globals.hh"


class PrimaryGeneratorAction0;
class PrimaryGeneratorAction1;
class PrimaryGeneratorAction2;
class PrimaryGeneratorAction3;
class PrimaryGeneratorMessenger;

class G4ParticleGun;
class G4Event;

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
public:
    PrimaryGeneratorAction();
    ~PrimaryGeneratorAction();

    void GeneratePrimaries(G4Event*) override;

    G4ParticleGun* GetParticleGun() { return fParticleGun; };
    void SelectAction(G4int i) { fSelectedAction = i; };

    G4int GetSelectedAction()  { return fSelectedAction; };

    PrimaryGeneratorAction0*  GetAction0() { return fAction0; };
    PrimaryGeneratorAction1*  GetAction1() { return fAction1; };
    PrimaryGeneratorAction2*  GetAction2() { return fAction2; };
    PrimaryGeneratorAction3*  GetAction3() { return fAction3; };

private:
    G4ParticleGun *fParticleGun= nullptr;

    PrimaryGeneratorAction0* fAction0 = nullptr;
    PrimaryGeneratorAction1* fAction1 = nullptr;
    PrimaryGeneratorAction2* fAction2 = nullptr;
    PrimaryGeneratorAction3* fAction3 = nullptr;

    G4int fSelectedAction = 1;

    PrimaryGeneratorMessenger* fGunMessenger = nullptr;
};
#endif
