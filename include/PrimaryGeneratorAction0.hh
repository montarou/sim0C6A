#ifndef PRIMARYGENERATORACTION0_HH
#define PRIMARYGENERATORACTION0_HH

#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"

class G4ParticleGun;
class G4Event;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class PrimaryGeneratorAction0
{
  public:
    PrimaryGeneratorAction0(G4ParticleGun*);
   ~PrimaryGeneratorAction0() = default ;

  public:
    void GeneratePrimaries(G4Event*);

  private:
   G4double fCosAlphaMin = 0., fCosAlphaMax = 0.;      //solid angle
   G4double fPsiMin = 0., fPsiMax = 0.;


    G4ParticleGun*  fParticleGun = nullptr;
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
