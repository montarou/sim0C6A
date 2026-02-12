#include "PrimaryGeneratorAction1.hh"
#include "PrimaryGeneratorAction.hh"

#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"
#include "globals.hh"
#include "G4AnalysisManager.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction1::PrimaryGeneratorAction1(G4ParticleGun* gun)
: fParticleGun(gun)
{
  //solid angle
  //
  G4double alphaMin =    0*deg;      //alpha in [0,pi]
  G4double alphaMax =   60*deg;      // cône de 60°
  fCosAlphaMin = std::cos(alphaMin);
  fCosAlphaMax = std::cos(alphaMax);

  fPsiMin = 0*deg;       //psi in [0, 2*pi]
  fPsiMax = 360*deg;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorAction1::GeneratePrimaries(G4Event* anEvent){

  // Tirage d'un gamma
  //

  //G4cout << "case 1 fAction" << G4endl;
  //G4ParticleDefinition* particle = G4ParticleTable::GetParticleTable()->FindParticle("e-");
  //if (!particle) {
  //  G4Exception("PrimaryGeneratorAction1", "NullParticle", FatalException, "e- not found in particle table.");
  //}

  //G4ParticleDefinition* particle = G4ParticleTable::GetParticleTable()->FindParticle("geantino");
  G4ParticleDefinition* particle = G4ParticleTable::GetParticleTable()->FindParticle("gamma");
  //G4ParticleDefinition* particle = G4ParticleTable::GetParticleTable()->FindParticle("e-");
  if (!particle) {
    G4Exception("PrimaryGeneratorAction1", "NullParticle", FatalException, "gamma not found in particle table.");
  }

  fParticleGun->SetParticleDefinition(particle);

  // Energie des gammas
  //
  fParticleGun->SetParticleEnergy(10*keV);

  //vertex position fixed
  //
  G4double x0 = 0*mm, y0 = 0*mm, z0 = 0.001*mm;  // source ponctuelle à z = +1 µm

  fParticleGun->SetParticlePosition(G4ThreeVector(x0,y0,z0));


  // ====== Génération isotrope dans le cône autour de z, theta ∈ [0, 12°] ======
  // Tirage uniforme en cos(theta)
  G4double cosTheta = fCosAlphaMin + (fCosAlphaMax - fCosAlphaMin) * G4UniformRand(); // entre 1 et 0
  G4double sinTheta = std::sqrt(1. - cosTheta*cosTheta);

  G4double theta = std::acos(cosTheta);    // en radians
  G4double thetaDeg = theta / deg;         // en degrés


  // Tirage uniforme en phi ∈ [0, 2*pi]
  G4double phi = fPsiMin + G4UniformRand() * (fPsiMax - fPsiMin);

  // Conversion vers vecteur cartésien
  G4double ux = sinTheta * std::cos(phi);
  G4double uy = sinTheta * std::sin(phi);
  G4double uz = cosTheta;


  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(ux,uy,uz));
  //fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0,0,1));

  // LOG: Afficher les paramètres de génération
  // COMMENTÉ pour réduire la taille du fichier log
  /*
  {
    const auto eid = anEvent ? anEvent->GetEventID() : -1;
    G4ThreeVector pos = fParticleGun->GetParticlePosition();
    G4cout << "[GEN] event=" << eid
           << " mode=1"
           << " pos(mm)=(" << pos.x()/mm << "," << pos.y()/mm << "," << pos.z()/mm << ")"
           << " dir=(" << ux << "," << uy << "," << uz << ")"
           << " theta=" << thetaDeg << " deg"
           << " E=" << fParticleGun->GetParticleEnergy()/keV << " keV"
           << " [GAMMA MONO]"
           << G4endl;
  }
  */

  // Enregistrement dans les histogrammes d'émission
  auto analysisManager = G4AnalysisManager::Instance();
  if (analysisManager) {
    G4double energy = fParticleGun->GetParticleEnergy();
    G4double phiDeg = phi / deg;  // conversion en degrés
    
    analysisManager->FillH1(0, energy);      // H0: Énergie à l'émission
    analysisManager->FillH1(1, thetaDeg);    // H1: Theta à l'émission
    analysisManager->FillH1(2, phiDeg);      // H2: Phi à l'émission
  }

  G4ThreeVector pos = fParticleGun->GetParticlePosition();
  //G4cout << "[DEBUG] Source position (tir): " << pos << G4endl;
  //G4cout << "[DEBUG] Source tirée à z = "
  //<< fParticleGun->GetParticlePosition().z()/mm
  //<< " mm, direction = "
  //<< fParticleGun->GetParticleMomentumDirection()
  //<< G4endl;

  fParticleGun->GeneratePrimaryVertex(anEvent);
}
