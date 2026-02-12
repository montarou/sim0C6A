#ifndef PrimaryGeneratorAction1_h
#define PrimaryGeneratorAction1_h

#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"

class G4ParticleGun;
class G4Event;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class PrimaryGeneratorAction1
{
  public:
    PrimaryGeneratorAction1(G4ParticleGun*);
   ~PrimaryGeneratorAction1() = default;

  public:
    void GeneratePrimaries(G4Event*);

  private:

    G4double fCosAlphaMin = 0., fCosAlphaMax = 0.;      //solid angle
    G4double fPsiMin = 0., fPsiMax = 0.;

    G4ParticleGun*  fParticleGun = nullptr;
};

#endif
