#include "MyTrackInfo.hh"
#include "G4EventManager.hh"
#include "G4PrimaryVertex.hh"
#include "G4PrimaryParticle.hh"
#include "G4Track.hh"
#include "G4TrajectoryContainer.hh"


#include "EventAction.hh"
#include "G4Event.hh"
#include "G4EventManager.hh"
#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh"

#include "SphereHit.hh"
#include "RunAction.hh"


//******************************************************************************************
//  Ce code correspond à la classe EventAction, qui permet de gérer les actions spécifiques
//        à chaque événement dans Geant4, notamment :
//  Initialiser des variables au début de chaque événement
//  Collecter les résultats (hits, compteurs) à la fin de chaque événement
//  Enregistrer des données dans des ntuples ou les transmettre au RunAction
//******************************************************************************************

//  Constructeur et destructeur par défaut.
//
//  Pas de traitement spécifique ici,
//  mais l'objet EventAction est prêt à être utilisé dans ActionInitialization.
//

EventAction::EventAction(){}

EventAction::~EventAction(){}

//  Réinitialise les variables locales à chaque début d'événement.
//  Cela prépare la collecte d'informations pendant le reste de l'événement, via SteppingAction.
void EventAction::BeginOfEventAction(const G4Event*  event)
{
    // Réinitialisation pour chaque événement
    enteredCube = false;
    enteredSphere = false;
    creatorProcess = "unknown";

    fNbEntrantInBe = 0;
    fNbInteractedInBe = 0;
    fNbEntrantInWaterSphere = 0;
    fNbInteractedInWaterSphere = 0;

    // Réinitialisation des énergies déposées dans les anneaux d'eau
    for (G4int i = 0; i < kNbWaterRings; i++) {
        fEdepRing[i] = 0.0;
    }
    fEdepTotalWater = 0.0;

    // 🔍 Récupérer la particule primaire
    G4PrimaryVertex* primaryVertex = event->GetPrimaryVertex();
    if (primaryVertex) {
        G4PrimaryParticle* primary = primaryVertex->GetPrimary();
        if (primary) {
            G4ParticleDefinition* particleDef = primary->GetG4code();
            G4String name = (particleDef ? particleDef->GetParticleName() : "unknown");

            G4ThreeVector mom = primary->GetMomentumDirection();
            G4double energy = primary->GetTotalEnergy();

            if (fEventVerboseLevel == 1) {
            G4cout << "[DEBUG BeginOfEventAction] Particule primaire = " << name << G4endl;
            G4cout << "[DEBUG BeginOfEventAction] Direction         = " << mom << G4endl;
            G4cout << "[DEBUG BeginOfEventAction] Énergie totale    = " << energy / keV << " keV" << G4endl;
            }
        } else {
            if (fEventVerboseLevel == 1) {
            G4cout << "[DEBUG BeginOfEventAction] Pas de particule primaire." << G4endl;
            }
        }
    } else {
        if (fEventVerboseLevel == 1) {
        G4cout << "[DEBUG BeginOfEventAction] Pas de vertex primaire." << G4endl;
        }
    }
}
//  Appelée à la fin de l'événement, pour :
//        - Enregistrer les résultats (ntuples, hits)
//        - Transmettre les données à RunAction

void EventAction::EndOfEventAction(const G4Event* event)
{
    if (fEventVerboseLevel == 1) {
        G4cout << "[DEBUG EndOfEventAction] EndOfEventAction appelé pour EventID = "<<event->GetEventID()<<G4endl;
        G4cout << "[DEBUG EndOfEventAction] NbEntrantInBe = "<<fNbEntrantInBe<<G4endl;
        G4cout << "[DEBUG EndOfEventAction] NbInteractedInBe = "<<fNbInteractedInBe<<G4endl;}

    // Ntuples trackInfo (ID=1), SphereHits (ID=0), SphereStats (ID=2) supprimés
    // Histogrammes supprimés

    if (fRunAction) {
        fRunAction->UpdateFromEvent(this);
        
        // Transmettre l'énergie déposée dans les anneaux d'eau
        fRunAction->AddEdepFromEvent(fEdepRing, fEdepTotalWater);
        
        // Vérifier si on doit remplir les histogrammes de dose (tous les 1000 événements)
        fRunAction->CheckAndFillDoseHistograms(event->GetEventID());
    }

    auto runAction = static_cast<const RunAction*>(G4RunManager::GetRunManager()->GetUserRunAction());
    if (runAction) {
        if (fEventVerboseLevel == 1) {
            G4cout << "\n[DEBUG EndOfEventAction] [EndOfEventAction DEBUG] Compteurs globaux (fin event #" << G4EventManager::GetEventManager()->GetConstCurrentEvent()->GetEventID() << ") :" << G4endl;
            G4cout<<" [DEBUG EndOfEventAction ↪ fTotalEntrantInBe          = "<<runAction->GetTotalEntrantInBe()<<G4endl;
            G4cout<<" [DEBUG EndOfEventAction ↪ fTotalInteractedInBe       = "<<runAction->GetTotalInteractedInBe()<<G4endl;
            G4cout<<" [DEBUG EndOfEventAction ↪ fTotalEntrantInWaterSphere = "<<runAction->GetTotalEntrantInWaterSphere()<<G4endl;
            G4cout<<" [DEBUG EndOfEventAction ↪ fTotalInteractedInWaterSphere = "<<runAction->GetTotalInteractedInWaterSphere()<< G4endl;}
    }

    // SphereSD supprimé - plus d'accès à SphereHitsCollection
}

//  Méthode appelée par SteppingAction pour transmettre à EventAction
//  les informations spécifiques au track primaire

void EventAction::SetTrackInfo(MyTrackInfo* info)
{
    if (!info) return;

    enteredCube = info->HasEnteredCube();
    enteredSphere = info->HasEnteredSphere();

    G4String p = info->GetCreatorProcess();
    creatorProcess = (p.empty() ? "unknown" : p);

    if (fEventVerboseLevel == 1) {
        G4cout<<"[DEBUG SetTrackInfo] ✅ Infos copiées : process="<<creatorProcess<<", cube="<<enteredCube<<", sphère="<<enteredSphere<<G4endl;}
}

