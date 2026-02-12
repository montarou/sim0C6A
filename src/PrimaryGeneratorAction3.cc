#include "PrimaryGeneratorAction3.hh"
#include "PrimaryGeneratorAction.hh"

#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"
#include "globals.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction3::PrimaryGeneratorAction3(G4ParticleGun* gun)
: fParticleGun(gun)
{
  //solid angle
  //
  G4double alphaMin =  0*deg;      //alpha in [0,pi]
  G4double alphaMax = 10*deg;
  fCosAlphaMin = std::cos(alphaMin);
  fCosAlphaMax = std::cos(alphaMax);

  fPsiMin = 0*deg;       //psi in [0, 2*pi]
  fPsiMax = 360*deg;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorAction3::GeneratePrimaries(G4Event* anEvent){

  // Tirage d'un électron
  //
  G4ParticleDefinition*  particle = G4ParticleTable::GetParticleTable()->FindParticle("e-");
  fParticleGun->SetParticleDefinition(particle);

  // Energie des électrons
  //
  fParticleGun->SetParticleEnergy(200.*keV);

  //vertex position fixed
  //
  G4double x0 = 0*mm, y0 = 0*mm, z0 = 0.001*mm;  // source ponctuelle à z = +1 µm

  fParticleGun->SetParticlePosition(G4ThreeVector(x0,y0,z0));

  //direction uniform in solid angle
  //
  G4double cosAlpha = fCosAlphaMin-G4UniformRand()*(fCosAlphaMin-fCosAlphaMax);
  G4double sinAlpha = std::sqrt(1. - cosAlpha*cosAlpha);
  G4double psi = fPsiMin + G4UniformRand()*(fPsiMax - fPsiMin);
  
  G4double thetaDeg = std::acos(cosAlpha) / deg;

  G4double ux = sinAlpha*std::cos(psi),
           uy = sinAlpha*std::sin(psi),
           uz = cosAlpha;

  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(ux,uy,uz));

  // LOG: Afficher les paramètres de génération
  // COMMENTÉ pour réduire la taille du fichier log
  /*
  {
    const auto eid = anEvent ? anEvent->GetEventID() : -1;
    G4cout << "[GEN] event=" << eid
           << " mode=3"
           << " pos(mm)=(" << x0/mm << "," << y0/mm << "," << z0/mm << ")"
           << " dir=(" << ux << "," << uy << "," << uz << ")"
           << " theta=" << thetaDeg << " deg"
           << " E=" << fParticleGun->GetParticleEnergy()/keV << " keV"
           << " [ELECTRON]"
           << G4endl;
  }
  */

  fParticleGun->GeneratePrimaryVertex(anEvent);
}
