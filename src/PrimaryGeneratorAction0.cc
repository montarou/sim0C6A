#include "PrimaryGeneratorAction0.hh"
#include "PrimaryGeneratorAction.hh"

#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"
#include "globals.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction0::PrimaryGeneratorAction0(G4ParticleGun* gun)
: fParticleGun(gun)
{

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorAction0::GeneratePrimaries(G4Event* anEvent){

    G4ParticleDefinition *particle = fParticleGun->GetParticleDefinition();

    if(particle == G4Geantino::Geantino())
    {
        G4int Z = 27;
        G4int A = 60;

        G4double charge = 0.*eplus;
        G4double energy = 0.*keV;

        G4ParticleDefinition *ion = G4IonTable::GetIonTable()->GetIon(Z,A,energy);

        fParticleGun -> SetParticleDefinition(ion);
        fParticleGun -> SetParticleCharge(charge);
    }
    
    fParticleGun->GeneratePrimaryVertex(anEvent);

    // LOG: Debug output
    {
        const auto eid = anEvent ? anEvent->GetEventID() : -1;
        G4ThreeVector pos = fParticleGun->GetParticlePosition();
        G4ThreeVector dir = fParticleGun->GetParticleMomentumDirection();
        G4double energy = fParticleGun->GetParticleEnergy();
        G4double theta = std::acos(dir.z()) / deg;
        
        // COMMENTÉ pour réduire la taille du fichier log
        /*
        G4cout << "[GEN] event=" << eid
               << " mode=0"
               << " pos(mm)=(" << pos.x()/mm << "," << pos.y()/mm << "," << pos.z()/mm << ")"
               << " dir=(" << dir.x() << "," << dir.y() << "," << dir.z() << ")"
               << " theta=" << theta << " deg"
               << " E=" << energy/keV << " keV"
               << " [Co-60]"
               << G4endl;
        */
    }
}
