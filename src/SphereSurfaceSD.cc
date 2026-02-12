#include "SphereSurfaceSD.hh"
#include "SphereSurfaceSDMessenger.hh"
#include "SphereHit.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4StepPoint.hh"

#include "G4SDManager.hh"
#include "G4RunManager.hh"
#include "G4HCofThisEvent.hh"

#include "G4TouchableHistory.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh"
#include "G4LogicalVolumeStore.hh"

#include "G4ios.hh"

// Définit un détecteur sensible (Sensitive Detector, SD)
// attaché à la surface de la sphère d’eau (logicsphereWater).
//
// SphereSD crée une collection propre pour chaque événement

//  Cette collection peut ensuite être remplie dans ProcessHits()
//  Et analysée en fin d’événement dans EventAction, comme ici :
//
//  auto hitsCollection = static_cast<G4THitsCollection<SphereHit>*>(hce->GetHC(hcID));
//
//  Constructeur
//  Classe qui dérive de G4VSensitiveDetector,
//  la classe de base Geant4 pour les détecteurs sensibles.

//  La signature du constructeur prend deux G4String
//  - name : le nom du détecteur sensible, typiquement "SphereSD"
//  - hitsCollectionName : (pas utilisé dans ton code actuel)
//    le nom logique de la collection de hits
//
//  G4VSensitiveDetector(name) appelle le constructeur de la classe de base
//  et enregistre ce détecteur auprès du système Geant4 avec ce nom (name).
//
//  fHCID(-1) initialise l’identifiant de la collection de hits à -1
//  valeur non encore attribuée (sera défini dans Initialize(...) via G4SDManager)
//
//  collectionName est un membre de G4VSensitiveDetector
//  qui contient le nom de la collection de Hits pour cet SD.
//
//  fHCID est l'identifiant de cette collection
//  (il sera initialisé plus tard).

SphereSurfaceSD::SphereSurfaceSD(const G4String& name, const G4String& hitsCollectionName)
    : G4VSensitiveDetector(name),fHCID(-1)
{
//    collectionName est un membre hérité de G4VSensitiveDetector
//    qui est un std::vector<G4String>.
//
//    - Cette ligne déclare une seule collection de hits que ce détecteur sensible
//    va produire pendant chaque événement, ici appelée "SphereHitsCollection".

//    - C’est ce nom qui sera ensuite utilisé pour récupérer la collection via son ID dans G4HCofThisEvent.

//    - "SphereHitsCollection"
//       permet :
//          D’enregistrer les SphereHit pendant l’événement dans une collection nommée
//          D’accéder ensuite à cette collection dans EventAction, typiquement par :
//          hcID = G4SDManager::GetSDMpointer()->GetCollectionID("SphereSD/SphereHitsCollection");

    collectionName.insert("SphereHitsCollection");
    fSDMessenger = new SphereSurfaceSDMessenger(this);
}

SphereSurfaceSD::~SphereSurfaceSD() {
    delete fSDMessenger;
}

//   Initialize() — appelée au début de chaque événement
//   Crée une nouvelle collection vide de SphereHit pour cet événement.
//   Récupère (ou mémorise) son ID unique via G4SDManager
//   L’ajoute à l’objet G4HCofThisEvent,
//   qui stocke toutes les hits collections de l’événement courant
void SphereSurfaceSD::Initialize(G4HCofThisEvent* hce)
{

//    Création d'une nouvelle collection de hits, de type G4THitsCollection<SphereHit>.
//      -  SphereHit est ta classe de hit personnalisée
//      -  Cette collection enregistrera tous les SphereHit détectés pendant cet événement.
//
//    Elle est identifiée par :
//          - SensitiveDetectorName  "SphereSD"
//          - collectionName[0] "SphereHitsCollection"
//    But : préparer une collection vide pour l’événement courant.
//
    fHitsCollection = new G4THitsCollection<SphereHit>(SensitiveDetectorName, collectionName[0]);

//    Ce bloc récupère et mémorise l’ID numérique (unique) de cette collection de hits.
//
//    fHCID (Hit Collection ID) est initialisé à -1 dans le constructeur
//
//    G4SDManager::GetCollectionID(...) donne l’ID en fonction du nom complet : "SphereSD/SphereHitsCollection".
//
//    But : permettre à Geant4 de retrouver cette collection plus tard via son identifiant unique.

    if (fHCID < 0) {
        fHCID = G4SDManager::GetSDMpointer()->GetCollectionID(fHitsCollection);
    }

//    hce est un pointeur vers G4HCofThisEvent
//    représente le conteneur de toutes les collections de hits de l’événement courant.
//    ajoutes ici la collection (identifiée par fHCID) dans ce conteneur.
//
//    But : faire en sorte que Geant4 sache récupérer ta collection en fin d’événement, par exemple dans EventAction.

    hce->AddHitsCollection(fHCID, fHitsCollection);
}

//  ProcessHits() — appelée à chaque G4Step dans ce volume
//  traites les interactions dans logicsphereWate

G4bool SphereSurfaceSD::ProcessHits(G4Step* step, G4TouchableHistory*) {

    // Débogage : entrée dans la méthode
    //G4cout << "[ProcessHits] Étape détectée." << G4endl;

    // Récupération des informations sur le step
    // pre : point de départ du step
    // post : point d’arrivée du step
    // Ce sont des pointeurs vers des objets G4StepPoint, qui contiennent la position, l'énergie, le volume, le temps, etc.

    G4StepPoint* pre  = step->GetPreStepPoint();
    G4StepPoint* post = step->GetPostStepPoint();

    //Accès aux volumes touchés
    //  - GetTouchableHandle() donne accès à la hiérarchie des volumes au point du step (volume, mère, grand-mère, etc.)
    //  - GetVolume() retourne le volume physique (G4VPhysicalVolume) à cet endroit
    //  - GetLogicalVolume() te donne le volume logique (G4LogicalVolume), qui contient :
    //      Le nom logique
    //      Le matériau
    //      La forme (G4VSolid)
    //      Les SD, champs, etc.
    //
    //  Donc logicalPre et logicalPost sont les volumes logiques traversés pendant ce step.
    //

    auto logicalPre   = pre->GetTouchableHandle()->GetVolume()->GetLogicalVolume();
    auto logicalPost  = post->GetTouchableHandle()->GetVolume()->GetLogicalVolume();

    G4String namePre  = logicalPre->GetName();
    G4String namePost = logicalPost->GetName();

    G4double energy = pre->GetKineticEnergy();
    auto analysisManager = G4AnalysisManager::Instance();
    if (!analysisManager) {
        G4cerr << "[ERREUR] analysisManager est NULL !" << G4endl;
        return true;
    }
    if (fSDVerboseLevel == 1) {}
    // Détection d’une sortie de la sphère (entrée non détectée ici)
    // Si une particule sort de logicsphereWater,
    // On enregistre son énergie dans un histogramme (ID 1).

    if (namePre == "logicsphereWater" && namePost != "logicsphereWater") {
        if (fSDVerboseLevel == 1) {
            G4cout << "[DEBUG ProcessHits] ← Sortie de la sphère à E = "<<energy/MeV<<" MeV"<< G4endl;}
        // [SUPPRIMÉ] analysisManager->FillH1(1, energy);
    }

    auto edep = step->GetTotalEnergyDeposit();
    if (fSDVerboseLevel == 1) {
        G4cout <<"[DEBUG ProcessHits][ProcessHits] ← Energie deposee dans la sphère= "<<edep/MeV<<" MeV"<<G4endl;}

    //pname contient le nom de la particule en cours de step
    //  step->GetTrack() : récupère le G4Track (la particule actuelle)
    //  ->GetParticleDefinition() : accède à la définition (G4ParticleDefinition)
    //  ->GetParticleName() : renvoie le nom en clair (ex: "gamma", "e-", "proton")
    G4String pname = step->GetTrack()->GetParticleDefinition()->GetParticleName();

    // time donne le moment où la particule est à la position pre
    // GetGlobalTime() retourne le temps écoulé depuis le début du run
    //      Unité : temps absolu (typiquement en nanosecondes)
    //      Utile pour mesurer des délais, des trajectoires dans le temps
    G4double time = pre->GetGlobalTime();

    //  vname contient le nom du volume physique traversé, par exemple "spherePV".
    //  pre->GetPhysicalVolume() retourne le volume physique (G4VPhysicalVolume) dans lequel se trouve pre
    //  ->GetName() donne son nom (défini via SetName() dans DetectorConstruction)
    G4String vname = pre->GetPhysicalVolume()->GetName();

    // posDetector est la position du centre du volume sensible dans le repère de son parent.
    //  physVol->GetTranslation() retourne le vecteur de translation du volume dans son parent (
    //  typiquement la position du détecteur dans le monde).
    //  Ce n’est pas la position du point de step, mais celle du volume détecteur lui-même.
    const G4VTouchable *touchable = step->GetPreStepPoint()->GetTouchable();
    G4VPhysicalVolume *physVol = touchable->GetVolume();
    G4ThreeVector posDetector = physVol->GetTranslation();

    // Création du hit
    // On collecte plusieurs infos :
    // Puis on crée un objet SphereHit et on rempli ses champs :
    auto hit = new SphereHit();
    hit->SetEventID(G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID());
    hit->SetPosition(step->GetPreStepPoint()->GetPosition());
    hit->SetEdep(step->GetTotalEnergyDeposit());
    hit->SetEnergy(step->GetPostStepPoint()->GetKineticEnergy());
    hit->SetTrackID(step->GetTrack()->GetTrackID());
    hit->SetParticleName(pname);
    hit->SetTime(time);
    hit->SetVolumeName(vname);


    // Particle PDG code and process info for this step ---
    if (auto* def = step->GetTrack()->GetDefinition()) {
        hit->SetPDG(def->GetPDGEncoding());
    }
    const G4VProcess* proc = nullptr;
    if (step->GetPostStepPoint()) proc = step->GetPostStepPoint()->GetProcessDefinedStep();
    if (!proc && step->GetPreStepPoint()) proc = step->GetPreStepPoint()->GetProcessDefinedStep();
    if (proc) {
        hit->SetProcessName(proc->GetProcessName());
        hit->SetProcessType(static_cast<G4int>(proc->GetProcessType()));
        hit->SetProcessSubType(proc->GetProcessSubType());
    } else {
        hit->SetProcessName("Unknown");
        hit->SetProcessType(-1);
        hit->SetProcessSubType(-1);
    }

    G4int evt = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
   if (fSDVerboseLevel == 1) {
       G4cout << "\n[DEBUG ProcessHits] "<<evt<<G4endl;
        G4cout << "\n[DEBUG ProcessHits] Hits position"<<step->GetPreStepPoint()->GetPosition()<<G4endl;
        G4cout << "\n[DEBUG ProcessHits] edep"<< edep<<G4endl;
        G4cout << "\n[DEBUG ProcessHits] kin energy"<<step->GetPostStepPoint()->GetKineticEnergy()<<G4endl;}

    // Insertion dans la collection
    // Cela ajoute le hit à la collection,
    // qui sera enregistrée dans EventAction.
    fHitsCollection->insert(hit);

    if (fSDVerboseLevel == 1) {
        G4cout << "[DEBUG ProcessHits] Fin OK" << G4endl;}
    return true;
}

// EndOfEvent() — appelée à la fin de l’événement
// parcours tous les hits enregistrés et les imprimes.

void SphereSurfaceSD::EndOfEvent(G4HCofThisEvent*) {

    // fHitsCollection est un pointeur vers G4THitsCollection<SphereHit> — une collection personnalisée.
    // .entries() renvoie le nombre de SphereHit enregistrés pendant cet événement.
    // Nombre de hits enregistrés
    G4int Nentries = fHitsCollection->entries();
    if (fSDVerboseLevel == 1) {
        G4cout << "[DEBUG End Of Event] Nentries = "<<Nentries<< G4endl;}

    //  Boucle sur tous les hits
    for (size_t i = 0; i < fHitsCollection->entries(); ++i) {
        // parcourt chaque SphereHit enregistré dans cet événement.
        // i est simplement l’index dans le tableau.

        if (fSDVerboseLevel == 1) {
            G4cout << "[DEBUG End Of Event] i = "<<i<< G4endl;
            // Affichage du hit
        //  appelle la méthode Print() sur le hit correspondant
        //  Cela suppose que la classe SphereHit a bien une méthode Print() définie
            (*fHitsCollection)[i]->Print();
        }

    }
}
