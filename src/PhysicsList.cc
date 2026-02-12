#include "PhysicsList.hh"

#include "G4EmStandardPhysics_option4.hh"
#include "G4EmDNAPhysics.hh"
#include "G4OpticalPhysics.hh"
#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"

#include "G4RegionStore.hh"
#include "G4Region.hh"
#include "G4ParticleDefinition.hh"
#include "G4ProcessManager.hh"
#include "G4SystemOfUnits.hh"

#include "G4LossTableManager.hh"
#include "G4EmConfigurator.hh"
#include "G4PhysicsListHelper.hh"

#include "G4ParticleTypes.hh"
#include "G4StepLimiter.hh"

PhysicsList::PhysicsList()
{
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4OpticalPhysics());
    RegisterPhysics(new G4DecayPhysics());
    RegisterPhysics(new G4RadioactiveDecayPhysics());
}

void PhysicsList::ConstructProcess()
{
    G4VModularPhysicsList::ConstructProcess();

    //Ajout du StepLimiter
    auto particleIterator = GetParticleIterator();
    particleIterator->reset();
    while ((*particleIterator)()) {
        G4ParticleDefinition* particle = particleIterator->value();
        G4ProcessManager* pmanager = particle->GetProcessManager();
        if (!pmanager) continue;

        // On applique le step limiter à tous les particules chargées + gamma
        if (particle->GetPDGCharge() != 0.0 || particle->GetParticleName() == "gamma") {
            pmanager->AddDiscreteProcess(new G4StepLimiter());
        }
    }

    //DefineDNARegionPhysics();
}

void PhysicsList::SetCuts()
{
    //SetCutsWithDefault();
    SetCutValue(0.1*mm,"e-");
    SetCutValue(0.1*mm,"gamma");
}
/*
void PhysicsList::DefineDNARegionPhysics()
{
    auto dnaPhysics = new G4EmDNAPhysics();
    dnaPhysics->ConstructProcess();

    G4Region* dnaRegion = G4RegionStore::GetInstance()->GetRegion("DNARegion");

    if (dnaRegion) {
        G4cout << ">>> Association de la physique Geant4-DNA à DNARegion." << G4endl;


        // Itération correcte sur les particules
        auto particleIterator = GetParticleIterator();
        particleIterator->reset();


        while ((*particleIterator)()) {
            G4ParticleDefinition* particle = particleIterator->value();
            G4ProcessManager* pmanager = particle->GetProcessManager();
            if (pmanager) {
                auto processList = pmanager->GetProcessList();
                for (size_t i = 0; i < processList->size(); ++i) {
                    G4VProcess* proc = (*processList)[i];
                    if (proc->GetProcessName().find("DNA") != G4String::npos) {
                    }
                }
            }
        }
    } else {
        G4Exception("PhysicsList::DefineDNARegionPhysics()",
                    "DNARegionNotFound", FatalException,
                    "La région DNARegion n'est pas définie.");
    }
}
*/
