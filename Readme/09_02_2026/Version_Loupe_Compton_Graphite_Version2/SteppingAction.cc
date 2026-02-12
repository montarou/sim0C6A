// SteppingAction.cc  — version insert-only avec ajout (B) "Compter côté stepping"
#include "SteppingAction.hh"
#include "EventAction.hh"
#include "DetectorConstruction.hh"

#include "G4EventManager.hh"
#include "G4Event.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4LogicalVolume.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh"
#include "G4TrackStatus.hh"
#include "G4VProcess.hh"
#include "G4Material.hh"

#include "MyTrackInfo.hh"
#include "G4Threading.hh"
#include "RunAction.hh"
#include "SteppingMessenger.hh"
#include "AnalysisManagerSetup.hh"

#include <cfloat>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <iomanip>

// ============================================================================
// [B] Compteurs globaux visibles depuis RunAction (pour le bilan de fin de run)
//    -> Dans RunAction::EndOfRunAction, déclare :
//       extern G4long gEnterPlanePrim, gLeavePlanePrim;
//       G4cout << "[STEP][SUMMARY] enter_plane_prim=" << gEnterPlanePrim
//              << " leave_plane_prim=" << gLeavePlanePrim << G4endl;
// ============================================================================
G4long gEnterPlanePrim = 0;
G4long gLeavePlanePrim = 0;


// [LOSS] Comptage de primaires perdus avant z=60 mm (par processus et matériau)
std::map<std::string,int> gLostByProc;
std::map<std::string,int> gLostByMat;


// Sécurisation MT (optionnelle mais recommandée)
#ifdef G4MULTITHREADED
#include "G4AutoLock.hh"
namespace { G4Mutex gPlanePrimMutex = G4MUTEX_INITIALIZER; }
namespace { G4Mutex gLossMapMutex   = G4MUTEX_INITIALIZER; }
namespace { G4Mutex gStepTrackingMutex = G4MUTEX_INITIALIZER; }
#endif

// ==================== Step Tracking - Membres statiques ====================
G4int SteppingAction::fTrackedEventsCount = 0;
G4int SteppingAction::fMaxTrackedEvents = 10;
std::set<G4int> SteppingAction::fTrackedEventIDs;
std::set<G4int> SteppingAction::fTrackedTrackIDs;
G4int SteppingAction::fCurrentEventID = -1;



//  Constructeur => hérites de G4UserSteppingAction.
//  crée un objet de la classe SteppingAction, en enregistrant un pointeur vers une instance de EventAction.
//      - Cela permet à SteppingAction de communiquer avec EventAction,
//        typiquement pour mettre à jour des compteurs ou transmettre des informations
//
//  SteppingAction::SteppingAction(EventAction* eventAction) est le constructeur de la classe SteppingAction.
//      - Il prend un pointeur vers un EventAction comme argument.
//      - et fourni ce pointeur dans ActionInitialization :
//  Inclusion d'une commande par messenger pour ajuster le niveau de verbose
//  La variable est fVerboseLevel initiliser à 1
SteppingAction::SteppingAction(EventAction* eventAction)
: G4UserSteppingAction(), fEventAction(eventAction)
{
    fSteppingMessenger = new SteppingMessenger(this);
    fSteppingVerboseLevel = 1;
}
//  G4UserSteppingAction() appelles ile constructeur de la classe de base : G4UserSteppingAction.
//  Cela est obligatoire car SteppingAction hérite de G4UserSteppingAction.
//
//  - fEventAction(eventAction) est une initialisation directe du membre fEventAction (qui est de type EventAction*).
//    Cela signifie que l’objet SteppingAction gardera un accès direct à EventAction, pour appeler ses méthodes comme :
//      fEventAction->IncrementNbEntrantInBe();
//
//  Pourquoi passer EventAction à SteppingAction ?
//  Parce que SteppingAction :
//      - Observe ce qui se passe à chaque G4Step (par ex. entrée/sortie de volumes)
//      - Mais c’est EventAction qui accumule les informations sur l’événement
//      - Donc SteppingAction agit comme un capteur, et transmet les résultats à EventAction

//  Destructeur
//  Efface proprement le Messenger
SteppingAction::~SteppingAction()
{
    delete fSteppingMessenger;
}

// ==================== Step Tracking - Méthodes statiques ====================

void SteppingAction::ResetTrackedParticlesCount()
{
    #ifdef G4MULTITHREADED
    G4AutoLock lock(&gStepTrackingMutex);
    #endif
    fTrackedEventsCount = 0;
    fTrackedEventIDs.clear();
    fTrackedTrackIDs.clear();
    fCurrentEventID = -1;
    
    G4cout << "\n"
           << "========================================================================================================\n"
           << "                         SUIVI STEP PAR STEP DES " << fMaxTrackedEvents << " PREMIERS EVENEMENTS\n"
           << "========================================================================================================\n"
           << std::setw(4)  << "Evt"
           << std::setw(5)  << "Trk"
           << std::setw(4)  << "Stp"
           << std::setw(8)  << "Part"
           << std::setw(4)  << "Sec"
           << std::setw(10) << "Ekin(keV)"
           << "  " << std::left << std::setw(22) << "PreVolume" << std::right
           << std::setw(26) << "PrePos(mm)"
           << "  " << std::left << std::setw(22) << "PostVolume" << std::right
           << std::setw(26) << "PostPos(mm)"
           << G4endl;
    G4cout << std::string(138, '-') << G4endl;
}

G4int SteppingAction::GetTrackedParticlesCount()
{
    return fTrackedEventsCount;
}

void SteppingAction::SetMaxTrackedParticles(G4int n)
{
    fMaxTrackedEvents = n;
}

G4int SteppingAction::GetMaxTrackedParticles()
{
    return fMaxTrackedEvents;
}

G4bool SteppingAction::ShouldTrackParticle(const G4Track* track, G4int eventID)
{
    if (!track) return false;
    
    G4int trackID = track->GetTrackID();
    G4int parentID = track->GetParentID();
    
    // Nouvel événement ?
    if (eventID != fCurrentEventID) {
        #ifdef G4MULTITHREADED
        G4AutoLock lock(&gStepTrackingMutex);
        #endif
        
        // Vérifier si on a déjà atteint le max d'événements
        if (fTrackedEventsCount >= fMaxTrackedEvents) {
            return false;
        }
        
        // Vérifier si cet événement est déjà suivi
        if (fTrackedEventIDs.find(eventID) == fTrackedEventIDs.end()) {
            // Nouvel événement à suivre
            fTrackedEventIDs.insert(eventID);
            fTrackedEventsCount++;
            fTrackedTrackIDs.clear();  // Reset les trackID pour ce nouvel événement
            fCurrentEventID = eventID;
            
            G4cout << "\n>>> Evenement #" << eventID 
                   << " (total suivi: " << fTrackedEventsCount << "/" << fMaxTrackedEvents << ") <<<\n" << G4endl;
        } else {
            fCurrentEventID = eventID;
        }
    }
    
    // Vérifier si cet événement est dans la liste des événements suivis
    if (fTrackedEventIDs.find(eventID) == fTrackedEventIDs.end()) {
        return false;
    }
    
    // Si c'est une particule primaire (parentID == 0)
    if (parentID == 0) {
        if (fTrackedTrackIDs.find(trackID) == fTrackedTrackIDs.end()) {
            #ifdef G4MULTITHREADED
            G4AutoLock lock(&gStepTrackingMutex);
            #endif
            fTrackedTrackIDs.insert(trackID);
        }
        return true;
    }
    
    // Si c'est une secondaire, suivre si parent est suivi
    if (fTrackedTrackIDs.find(parentID) != fTrackedTrackIDs.end()) {
        if (fTrackedTrackIDs.find(trackID) == fTrackedTrackIDs.end()) {
            #ifdef G4MULTITHREADED
            G4AutoLock lock(&gStepTrackingMutex);
            #endif
            fTrackedTrackIDs.insert(trackID);
        }
        return true;
    }
    
    return false;
}

void SteppingAction::PrintStepInfo(const G4Step* step, G4int eventID)
{
    const G4Track* track = step->GetTrack();
    const G4StepPoint* prePoint = step->GetPreStepPoint();
    const G4StepPoint* postPoint = step->GetPostStepPoint();
    
    if (!track || !prePoint || !postPoint) return;
    
    G4int trackID = track->GetTrackID();
    G4int parentID = track->GetParentID();
    G4int stepNumber = track->GetCurrentStepNumber();
    G4String particleName = track->GetDefinition()->GetParticleName();
    G4int isSecondary = (parentID == 0) ? 0 : 1;
    G4double ekin = prePoint->GetKineticEnergy() / CLHEP::keV;
    
    G4String preVolumeName = "OutOfWorld";
    G4String postVolumeName = "OutOfWorld";
    
    if (prePoint->GetTouchableHandle()->GetVolume()) {
        preVolumeName = prePoint->GetTouchableHandle()->GetVolume()->GetName();
    }
    if (postPoint->GetTouchableHandle()->GetVolume()) {
        postVolumeName = postPoint->GetTouchableHandle()->GetVolume()->GetName();
    }
    
    // Tronquer les noms de volumes longs (max 20 caractères)
    const size_t maxVolNameLen = 20;
    if (preVolumeName.length() > maxVolNameLen) {
        preVolumeName = preVolumeName.substr(0, maxVolNameLen-2) + "..";
    }
    if (postVolumeName.length() > maxVolNameLen) {
        postVolumeName = postVolumeName.substr(0, maxVolNameLen-2) + "..";
    }
    
    // Tronquer le nom de particule si nécessaire
    if (particleName.length() > 6) {
        particleName = particleName.substr(0, 6);
    }
    
    G4ThreeVector prePos = prePoint->GetPosition();
    G4ThreeVector postPos = postPoint->GetPosition();
    
    std::ostringstream prePosStr, postPosStr;
    prePosStr << std::fixed << std::setprecision(2) 
              << "(" << prePos.x()/mm << "," << prePos.y()/mm << "," << prePos.z()/mm << ")";
    postPosStr << std::fixed << std::setprecision(2) 
               << "(" << postPos.x()/mm << "," << postPos.y()/mm << "," << postPos.z()/mm << ")";
    
    G4cout << std::setw(4)  << eventID
           << std::setw(5)  << trackID
           << std::setw(4)  << stepNumber
           << std::setw(8)  << particleName
           << std::setw(4)  << isSecondary
           << std::setw(10) << std::fixed << std::setprecision(3) << ekin
           << "  " << std::left << std::setw(20) << preVolumeName << std::right
           << std::setw(26) << prePosStr.str()
           << "  " << std::left << std::setw(20) << postVolumeName << std::right
           << std::setw(26) << postPosStr.str()
           << G4endl;
}

//  Cette fonction UserSteppingAction(const G4Step* step) est appelée à chaque
//  step de chaque particule.
void SteppingAction::UserSteppingAction(const G4Step *step)
{
    // Vérifications de base
    if (!step) return;

    G4int eventID = G4EventManager::GetEventManager()->GetConstCurrentEvent()->GetEventID();

    auto track     = step->GetTrack();

    // ==================== Step Tracking des 10 premiers événements ====================
    if (track && ShouldTrackParticle(track, eventID)) {
        PrintStepInfo(step, eventID);
    }
    // ==================== Fin Step Tracking ====================

    // Ajout du 15/07
    MyTrackInfo* trackInfo = static_cast<MyTrackInfo*>(track->GetUserInformation());
    if (!trackInfo) {
        if (fSteppingVerboseLevel == 1) {
            G4cout << "[DEBUG SteppingAction] MyTrackInfo est nul → création..." << G4endl;
        }
        trackInfo = new MyTrackInfo();
        track->SetUserInformation(trackInfo);

        const G4VProcess* creator = track->GetCreatorProcess();
        G4String pname = (creator ? creator->GetProcessName() : "primary");
        if (fSteppingVerboseLevel == 1) {
            G4cout << "[DEBUG SteppingAction] Processus créateur = " << pname << G4endl;
        }

        trackInfo->SetCreatorProcess(pname);
    }

    if (track->GetCurrentStepNumber() == 1){
        if (fSteppingVerboseLevel == 1) {
            G4cout<<"\n[DEBUG SteppingAction Step 1 Event]"<<eventID<<"\n Vertex="<<track->GetVertexPosition()/mm<<G4endl;}
    }

    //  Récupère le point de départ du step (G4StepPoint*).
    //  Ce point donne :
    //      -La position avant le step
    //      -Le volume traversé
    //      -L’énergie avant le step
    //      -Le temps absolu (GetGlobalTime())
    auto prePoint  = step->GetPreStepPoint();

    //  Récupère le point d’arrivée du step (G4StepPoint*).
    //  Ce point donne :
    //      -La position finale
    //      -Le volume d’arrivée
    //      -L’énergie restante
    //      -Le processus ayant causé le step
    auto postPoint = step->GetPostStepPoint();

    //  Vérifie que les pointeurs ne sont pas nullptr.
    //  Si l’un d’eux est nul,
    //        => cela signifie qu’il y a un problème avec ce step (par exemple un track détruit,
    //        => ou mal initialisé).
    //  Dans ce cas,
    //        => on sort immédiatement de la méthode pour éviter un crash ou une erreur de segmentation.
    if (!track || !prePoint || !postPoint) return;

    // Récupères les volumes traversés :
    //  Chaque StepPoint (prePoint, postPoint) est associé à un objet Touchable (volume touché).
    //  GetVolume() te donne le volume physique (G4VPhysicalVolume*) auquel le point appartient.
    //  Ces volumes ont été placés dans la géométrie avec G4PVPlacement.
    //  récupères donc dans quel volume physique le step commence (pre) et se termine (post).
    auto preVolumeHandle  = prePoint->GetTouchableHandle()->GetVolume();
    auto postVolumeHandle = postPoint->GetTouchableHandle()->GetVolume();

    //  Vérification de validité
    //  Si la particule sort du monde ou traverse une zone indéfinie, les pointeurs peuvent être nuls.
    //  On arrête ici pour éviter un crash.
    if (!preVolumeHandle || !postVolumeHandle) return;

    //  Accès aux volumes logiques
    //  Les volumes physiques sont des instances placées dans l’espace.
    //  Les volumes logiques sont leurs modèles (forme, matériau, nom, SD...).
    //  On obtient les modèles de volumes correspondants aux points d’entrée et de sortie.
    G4LogicalVolume* preLogic  = preVolumeHandle->GetLogicalVolume();
    G4LogicalVolume* postLogic = postVolumeHandle->GetLogicalVolume();

    //  Vérification de validité
    //  Ces noms ont été définis dans ta géométrie, souvent via SetName(...).
    if (!preLogic || !postLogic) return;

     //  Récupération des noms de volumes
    G4String namePre  = preLogic->GetName();
    G4String namePost = postLogic->GetName();

    //  Récupération des matériaux
    G4Material* materialPre  = preLogic->GetMaterial();
    G4Material* materialPost = postLogic->GetMaterial();

    //  Lecture des noms de matériaux
    //  Cette ligne évite les crashs si materialPre ou materialPost est nul.
    //  Cela permet d’avoir des chaînes valides, par exemple pour affichage ou comparaison.
    G4String matNamePre  = materialPre  ? materialPre->GetName()  : "null";
    G4String matNamePost = materialPost ? materialPost->GetName() : "null";

    // ==================== COMPTON DANS LE CÔNE GRAPHITE ====================
    // Détection : si un primaire (parentID == 0) subit une diffusion Compton
    // (processus "compt") dans le volume logique "logicConeCompton",
    // on marque ce track via MyTrackInfo pour pouvoir le distinguer
    // des primaires transmis directement lorsqu'il atteint le plan de scoring.
    //
    // Note Geant4 : lors d'un Compton, le photon diffusé CONTINUE comme le
    // même track (même trackID, parentID == 0). Seul l'électron de recul est
    // créé comme secondaire. C'est pourquoi il faut tagger le track ici.
    // ==================================================================
    {
        const G4VProcess* procDefined = postPoint->GetProcessDefinedStep();
        if (procDefined && track->GetParentID() == 0
            && procDefined->GetProcessName() == "compt"
            && namePre == "logicConeCompton")
        {
            MyTrackInfo* info = static_cast<MyTrackInfo*>(track->GetUserInformation());
            if (info) {
                info->SetComptonInCone(true);
                info->IncrementNComptonInCone();
                info->SetLastComptonPos(postPoint->GetPosition());
                info->SetLastComptonEkin(prePoint->GetKineticEnergy());

                // Log limité : les 100 premiers puis 1 sur 10000
                static G4int sComptonConeLog = 0;
                #ifdef G4MULTITHREADED
                G4AutoLock lock(&gPlanePrimMutex);
                #endif
                if (sComptonConeLog < 100 || sComptonConeLog % 10000 == 0) {
                    G4ThreeVector cpos = postPoint->GetPosition();
                    G4cout << "[STEP][COMPTON_IN_CONE] #" << sComptonConeLog
                           << " | event=" << eventID
                           << " | trackID=" << track->GetTrackID()
                           << " | n_compt=" << info->GetNComptonInCone()
                           << " | E_before=" << prePoint->GetKineticEnergy()/keV << " keV"
                           << " | E_after=" << postPoint->GetKineticEnergy()/keV << " keV"
                           << " | pos(mm)=(" << cpos.x()/mm << ", "
                                              << cpos.y()/mm << ", "
                                              << cpos.z()/mm << ")"
                           << G4endl;
                }
                ++sComptonConeLog;
            }
        }
    }
    // ==================== FIN COMPTON CÔNE ====================

    // [ADD] LV pointer trace désactivé pour réduire la verbosité
    // Décommenter pour débogage:
    // if (namePre == "logicScorePlane" || namePost == "logicScorePlane") {
    //     G4cout << "[STEP] LV pre@" << preLogic << "  post@" << postLogic << G4endl;
    // }

     // DEBUG STEP  : position, nom de volume logique et matériaux
    G4ThreeVector prePos  = prePoint->GetPosition();
    G4ThreeVector postPos = postPoint->GetPosition();
    if (fSteppingVerboseLevel == 1) {
        G4cout << " \n[DEBUG SteppingAction] Event]  " << eventID<< G4endl;
        G4cout << " \n[DEBUG SteppingAction] Step : " << track->GetCurrentStepNumber()<< G4endl;
        G4cout << " \n[DEBUG SteppingAction] PreStep position : " << prePos / mm << " mm" << G4endl;
        G4cout << " \n[DEBUG SteppingAction] PostStep position: " << postPos / mm << " mm" << G4endl;
        G4cout << " \n[DEBUG SteppingAction] PreStep volume   : " << namePre << ", matériau : " << matNamePre << G4endl;
        G4cout << " \n[DEBUG SteppingAction] PostStep volume  : " << namePost << ", matériau : " << matNamePost << G4endl;
        if (track->GetCurrentStepNumber() == 1){
        G4cout <<" \n[DEBUG SteppingAction] Step 1 Event "<<eventID<<" \n namePre ="<<namePre<<" \n namePost ="<<namePost<<G4endl;
        G4cout <<" \n[DEBUG SteppingAction] Step 1 Event "<<eventID<<" \n prePos="<<prePos/mm<<" \n postPos="<<postPos/mm<<G4endl;
        G4ThreeVector vertexPosition = track->GetVertexPosition();
        G4cout <<" \n[DEBUG SteppingAction] Step 1 Event "<<eventID<<" \n Vertex="<<vertexPosition/mm<<G4endl;}
    }

      // [TRACE] Frontières d'entrée/sortie dans logicScorePlane ===
    do {
        const auto* track = step->GetTrack();
        if (!track || track->GetParentID()!=0) break; // primaire seulement

        const auto pre  = step->GetPreStepPoint();
        const auto post = step->GetPostStepPoint();
        if (!pre || !post) break;

        const auto* prePV  = pre->GetPhysicalVolume();
        const auto* postPV = post->GetPhysicalVolume();
        const auto* preLV  = prePV  ? prePV->GetLogicalVolume()  : nullptr;
        const auto* postLV = postPV ? postPV->GetLogicalVolume() : nullptr;

        const bool enter = (preLV  && preLV->GetName()  != "logicScorePlane") &&
        (postLV && postLV->GetName() == "logicScorePlane") &&
        (post->GetStepStatus()==fGeomBoundary);

        const bool leave = (preLV  && preLV->GetName()  == "logicScorePlane") &&
        (postLV && postLV->GetName() != "logicScorePlane") &&
        (post->GetStepStatus()==fGeomBoundary);

        static int seen = 0, maxPrint = 5;  // Réduit de 40 à 5
        if ((enter || leave) && seen < maxPrint) {
            const auto& rpre  = pre->GetPosition();
            const auto& rpost = post->GetPosition();
            G4cout << "[TRACE][PLANE " << (enter?"ENTER":"LEAVE") << "] evt="
            << (G4RunManager::GetRunManager()->GetCurrentEvent()
            ? G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID() : -1)
            << " zPre=" << rpre.z()/mm << " zPost=" << rpost.z()/mm << " mm"
            << G4endl;
            ++seen;
        }
    } while(0);

    // [TRACE] Croisement du plan z=60 mm par le primaire ===
    do {
        const auto* track = step->GetTrack();
        if (!track) break;
        if (track->GetParentID() != 0) break; // primaire uniquement

        const auto pre  = step->GetPreStepPoint();
        const auto post = step->GetPostStepPoint();
        if (!pre || !post) break;

        const auto& rpre  = pre->GetPosition();
        const auto& rpost = post->GetPosition();

        // Pour un faisceau +Z : croisement si z_pre <= 60 et z_post >= 60
        const G4double zPlane = 60.0*mm;
        if (rpre.z() <= zPlane && rpost.z() >= zPlane) {
            const auto* prePV  = pre->GetPhysicalVolume();
            const auto* postPV = post->GetPhysicalVolume();
            const auto* preLV  = prePV  ? prePV->GetLogicalVolume()  : nullptr;
            const auto* postLV = postPV ? postPV->GetLogicalVolume() : nullptr;

            static int seen = 0, maxPrint = 5; // Réduit de 40 à 5
            if (seen < maxPrint) {
                G4cout << "[TRACE][Z=60] evt=" << (G4RunManager::GetRunManager()->GetCurrentEvent()
                ? G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID() : -1)
                << " preZ=" << rpre.z()/mm  << " postZ=" << rpost.z()/mm << " mm"
                << G4endl;
                ++seen;
            }
        }
    } while(0);

    // Comptage entrée/sortie du plan pour PRIMAIRES uniquement (ParentID==0)
    // Compteurs globaux à définir ailleurs (par ex. en haut du .cc) :
    //    G4long gEnterPlanePrim = 0;
    //    G4long gLeavePlanePrim = 0;
    extern G4long gEnterPlanePrim;
    extern G4long gLeavePlanePrim;

    // --- Compteurs entrée/sortie plan pour PRIMAIRES (ParentID==0) ---
    // Reconnaissance du plan via LV ("logicScorePlane") ou PV ("physScorePlane")
    {
        const bool preIsPlane  = (namePre  == "logicScorePlane")  ||
        (preVolumeHandle  && preVolumeHandle->GetName()  == "physScorePlane");
        const bool postIsPlane = (namePost == "logicScorePlane")  ||
        (postVolumeHandle && postVolumeHandle->GetName() == "physScorePlane");
        const bool isPrimary   = (track->GetParentID() == 0);

    if (isPrimary) {
    // ENTER : !preIsPlane && postIsPlane
            if (!preIsPlane && postIsPlane) {
            #ifdef G4MULTITHREADED
                { G4AutoLock lock(&gPlanePrimMutex); ++gEnterPlanePrim; }
            #else
                ++gEnterPlanePrim;
            #endif

            static int dbgEnter = 0;
                if (dbgEnter < 10 && fSteppingVerboseLevel == 1) {
                    const auto pos = postPoint->GetPosition();
                    G4cout << "[STEP][ENTER][prim] -> plane at ("
                    << pos.x()/mm << "," << pos.y()/mm << "," << pos.z()/mm << ") mm" << G4endl;
                    ++dbgEnter;
                }
            }

    // LEAVE : preIsPlane && !postIsPlane
            if (preIsPlane && !postIsPlane) {
            #ifdef G4MULTITHREADED
                { G4AutoLock lock(&gPlanePrimMutex); ++gLeavePlanePrim; }
            #else
                ++gLeavePlanePrim;
            #endif

            static int dbgLeave = 0;
                if (dbgLeave < 10 && fSteppingVerboseLevel == 1) {
                    const auto pos = prePoint->GetPosition();
                    G4cout << "[STEP][LEAVE][prim] <- plane from ("
                    << pos.x()/mm << "," << pos.y()/mm << "," << pos.z()/mm << ") mm" << G4endl;
                    ++dbgLeave;
            }
        }
    }
    }

    // DEBUG STEP  : position, nom de volume logique et matériaux
    if (fSteppingVerboseLevel == 1) {
        G4cout << " \n[DEBUG SteppingAction] Event]  " << eventID<< G4endl;
        G4cout << " \n[DEBUG SteppingAction] Step : " << track->GetCurrentStepNumber()<< G4endl;
        G4cout << " \n[DEBUG SteppingAction] PreStep position : " << prePos / mm << " mm" << G4endl;
        G4cout << " \n[DEBUG SteppingAction] PostStep position: " << postPos / mm << " mm" << G4endl;
        G4cout << " \n[DEBUG SteppingAction] PreStep volume   : " << namePre << ", matériau : " << matNamePre << G4endl;
        G4cout << " \n[DEBUG SteppingAction] PostStep volume  : " << namePost << ", matériau : " << matNamePost << G4endl;
        if (track->GetCurrentStepNumber() == 1){
            G4cout <<" \n[DEBUG SteppingAction] Step 1 Event "<<eventID<<" \n namePre ="<<namePre<<" \n namePost ="<<namePost<<G4endl;
            G4cout <<" \n[DEBUG SteppingAction] Step 1 Event "<<eventID<<" \n prePos="<<prePos/mm<<" \n postPos="<<postPos/mm<<G4endl;
            G4ThreeVector vertexPosition = track->GetVertexPosition();
            G4cout <<" \n[DEBUG SteppingAction] Step 1 Event "<<eventID<<" \n Vertex="<<vertexPosition/mm<<G4endl;}
    }


    G4double energy = track->GetKineticEnergy();
    auto analysisManager = G4AnalysisManager::Instance();

    //  Détection de l’entrée dans MiniX-TubeXFenetreBeryllium-Beryllium
    //  Si la particule entre dans le Beryllium,
    //  on incrémente un compteur et remplis un histogramme (ID 2).
    //
    //  namePre est le nom du volume logique traversé avant le step.
    //  namePost est le volume après le step.
    //  Donc :
    //      Si la particule n’était pas dans le Béryllium avant (namePre != ...)
    //      Et qu’elle se trouve dedans après (namePost == ...)
    //      Alors, la particule vient juste d’entrer dans le volume Béryllium.
    if (namePre != "MiniX-TubeXFenetreBeryllium-Beryllium" &&
        namePost == "MiniX-TubeXFenetreBeryllium-Beryllium") {
        if (fSteppingVerboseLevel == 1) {
            G4cout<<"🔸[DEBUG SteppingAction] Une particule entre dans MiniX-TubeXFenetreBeryllium-Beryllium !"<< G4endl;
            G4cout <<" \n[DEBUG SteppingAction] energy = "<<energy/keV<<G4endl;
        }

    //  Incrémentation du compteur
    //  IncrementNbEntrantInBe() met à jour un compteur par événement (dans EventAction)
    //  qui suit combien de particules sont entrées dans le Béryllium durant cet événement.
    //
    //  Récupère la valeur avec GetNbEntrantInBe()

      fEventAction->IncrementNbEntrantInBe();

      auto runAction = static_cast<const RunAction*>(G4RunManager::GetRunManager()->GetUserRunAction());
      if (runAction) {
          if (fSteppingVerboseLevel == 1) {
              G4cout << "[DEBUG SteppingAction] Entrée Béryllium : courant = "
          << fEventAction->GetNbEntrantInBe()
          << ", total global = " << runAction->GetTotalEntrantInBe() << G4endl;}
      }

    //  Position et type de particule
    //  position_input : position exacte où la particule entre dans le Béryllium (point d’arrivée du step).
    //  pname : nom de la particule (par exemple "e-", "gamma", "proton")
    G4ThreeVector position_input = postPoint->GetPosition();
    G4String pname = track->GetParticleDefinition()->GetParticleName();

    if (fSteppingVerboseLevel == 1) {
        G4cout<<"[DEBUG SteppingAction]"<<"→ Position input :"<<position_input/mm<<"mm"<<G4endl;
        G4cout<<"[DEBUG SteppingAction]"<<"→ Particule :"<< pname<<", Énergie input :"<<energy/keV<<"keV"<<G4endl;}

    //  Remplissage d’un histogramme
    //  Enregistres dans un histogramme 1D (ID = 2)
    //    l’énergie cinétique de la particule au moment de l’entrée dans le Béryllium.
    //  Cela permet de tracer ensuite une distribution des énergies d’entrée dans ce volume.
    // [SUPPRIMÉ] // [SUPPRIMÉ] if (analysisManager)
        // [SUPPRIMÉ] analysisManager->FillH1(2, energy);  // histogramme ID 2
    }


    // Détection de la sortie dans MiniX-TubeXFenetreBeryllium-Beryllium
    //
    //  namePre est le nom du volume logique dans lequel était la particule avant le step.
    //  namePost est celui dans lequel elle est après le step.
    //  Donc cette condition signifie :
    //  La particule était dans le Béryllium
    //  Elle n’y est plus après le step
    //    C’est donc une détection de sortie du Béryllium.
    if (namePre == "MiniX-TubeXFenetreBeryllium-Beryllium" &&
        namePost != "MiniX-TubeXFenetreBeryllium-Beryllium") {

    //  Position de sortie
    //
    //  G4ThreeVector position_output = postPoint->GetPosition();
    //  postPoint représente le point d’arrivée du step.
    //  récupères ici la position exacte où la particule quitte le volume Béryllium.
    //
    //  Cette info peut être utilisée pour :
    //        Tracer une trajectoire
    //        Calculer un angle de sortie
    //        Ou pour filtrer les particules sortant par une direction donnée
    //
    //  Nom de la particule
    //
    //  G4String pname = eventIDtrack->GetParticleDefinition()->GetParticleName();
    //  On récupère le nom du type de particule (ex: "e-", "gamma", "proton")
    //  Cela peut servir à analyser uniquement certains types de particules sortantes.
    //
    //  G4cout << "🔸 Une particule sort de MiniX-TubeXFenetreBeryllium-Beryllium !" << G4endl;
    //  G4cout << " → Position output : " << position_output / mm << " mm" << G4endl;
    //  G4cout << " → Particule : " << pname << ", Énergie output : " << energy / keV << " keV" << G4endl;
    //
    //  Remplissage d’un histogramme
    //  enregistre l’énergie cinétique de la particule au moment de sa sortie dans un histogramme (ID = 3).
    //  Cela permet de visualiser la distribution des énergies à la sortie du Béryllium,
    //    pour comparaison avec l’entrée (ID = 2).
    // [SUPPRIMÉ] // [SUPPRIMÉ] if (analysisManager)
        // [SUPPRIMÉ] analysisManager->FillH1(3, energy);  // histogramme ID 3
    }

    // Détection d’interaction dans le Béryllium
    // Teste si l’un des deux points du step est dans le Béryllium
    if (namePre == "MiniX-TubeXFenetreBeryllium-Beryllium" ||
        namePost == "MiniX-TubeXFenetreBeryllium-Beryllium") {

    // Dépôt d'énergie et processus défini
    G4double edep = step->GetTotalEnergyDeposit();
    const G4VProcess* process = postPoint->GetProcessDefinedStep();

    if (process)
        G4String procName = process->GetProcessName();
        if (fSteppingVerboseLevel == 1) {
            G4cout<<"\n[DEBUG SteppingAction] 💥 → Processus : "<<process->GetProcessName()<< G4endl;
            G4cout<<"\n[DEBUG SteppingAction] 💥 → Dépôt d’énergie : "<<edep/keV<<"keV"<< G4endl;}

    // Vérifie s’il y a eu interaction : dépôt d’énergie ou processus physique réel

    if ((edep > 0.0 && edep < DBL_MAX) ||
        (process && process->GetProcessName() != "Transportation" && process->GetProcessName() != "msc")){
        if (fSteppingVerboseLevel == 1) {
            G4cout << "\n[DEBUG SteppingAction] 💥 Interaction dans le Béryllium !" << G4endl;}
        // Filtrer les processus non physiques
        if (process->GetProcessName() != "Transportation" && process->GetProcessName() != "msc") {
            if (fSteppingVerboseLevel == 1) {
                G4cout << "\n[DEBUG SteppingAction] 💥 Interaction dans le Béryllium !" << G4endl;
                G4cout << " [DEBUG SteppingAction] → Particule : "<<track->GetParticleDefinition()->GetParticleName()<<G4endl;
                G4cout << " [DEBUG SteppingAction] → Processus : "<<process->GetProcessName()<<G4endl;
                G4cout << " [DEBUG SteppingAction] → Dépôt d’énergie : "<<edep/keV<<" keV"<<G4endl;
            }

        // ✅ Boucle sur les secondaires produits
        const auto* secondaries = step->GetSecondaryInCurrentStep();
        if (secondaries && !secondaries->empty()) {
            if (fSteppingVerboseLevel == 1) {
                G4cout<<"[DEBUG SteppingAction] → Secondaires produits :"<<secondaries->size()<<G4endl;}
            for (const auto* sec : *secondaries) {
                if (fSteppingVerboseLevel == 1) {
                    G4cout << "[DEBUG SteppingAction]   ↪ "<< sec->GetParticleDefinition()->GetParticleName()
                    <<", E = "<<sec->GetKineticEnergy()/keV<<" keV"
                    <<", créé par : "
                    <<(sec->GetCreatorProcess()
                    ? sec->GetCreatorProcess()->GetProcessName()
                    : "N/A")
                    << G4endl;}
            }
        }

        // Mise à jour du compteur d’interactions
        fEventAction->IncrementNbInteractedInBe();

        // [SUPPRIMÉ] // [SUPPRIMÉ] if (analysisManager)
            // [SUPPRIMÉ] analysisManager->FillH1(4, edep);  // Histogramme ID 4 : énergie déposée
        }
    }
    }

    if (namePre == "MiniX-TubeXFenetreBeryllium-Beryllium" &&
    namePost != "MiniX-TubeXFenetreBeryllium-Beryllium") {
        if (fSteppingVerboseLevel == 1) {
            G4cout << "[DEBUG SteppingAction] → Particule : "<<track->GetParticleDefinition()->GetParticleName()<<", Énergie output :"<< energy/keV<<"keV"<< G4endl;}
    }

    // ==================== Dépôt d'énergie dans les anneaux d'eau ====================
    // Détecte les dépôts d'énergie dans les couronnes d'eau (logicWaterRing0 à logicWaterRing4)
    // et les transmet à EventAction pour calcul de dose
    // 
    // CORRECTION [30/01/2026] : Utiliser namePre au lieu de namePost car 
    // step->GetTotalEnergyDeposit() retourne l'énergie déposée PENDANT le step,
    // c'est-à-dire dans le volume PRE-step (où la particule commence), 
    // PAS dans le volume POST-step (où elle arrive).
    {
        // Vérifier si on est dans un anneau d'eau
        G4int ringIndex = -1;
        if (namePre.find("logicWaterRing") != std::string::npos) {
            // Extraire le numéro de l'anneau (dernier caractère)
            char lastChar = namePre[namePre.length() - 1];
            if (lastChar >= '0' && lastChar <= '4') {
                ringIndex = lastChar - '0';
            }
        }
        
        if (ringIndex >= 0) {
            G4double edepWater = step->GetTotalEnergyDeposit();
            if (edepWater > 0.0 && edepWater < DBL_MAX) {
                // Transmettre l'énergie déposée à EventAction
                if (fEventAction) {
                    fEventAction->AddEdepToRing(ringIndex, edepWater / keV);  // en keV
                    
                    if (fSteppingVerboseLevel == 1) {
                        G4cout << "[DOSE] Edep dans anneau " << ringIndex 
                               << " : " << edepWater/keV << " keV" << G4endl;
                    }
                }
            }
        }
    }
    // ==================== Fin dépôt d'énergie anneaux d'eau ====================

    G4ThreeVector pos_1 = prePoint->GetPosition();
    G4ThreeVector pos_2 = postPoint->GetPosition();

    G4int tid = G4Threading::G4GetThreadId();

    if (fSteppingVerboseLevel == 2) {
        G4cout<<"[DEBUG SteppingAction] [Thread "<<tid<<"] Event #"<<eventID<<" → de "<<namePre<<" → " << namePost<< ", TrackID = " << track->GetTrackID() << G4endl;
    }
    //

    // 🔍 Affichage des volumes traversés à chaque step
    if (fSteppingVerboseLevel == 2) {
        G4cout<<"[DEBUG SteppingAction] Event #"<<eventID<<"[Trace] de "<<namePre<<" ("<<matNamePre<<")"<<" → "<<namePost<<" ("<< matNamePost<<")"<<G4endl;
        G4cout<<"[DEBUG SteppingAction] Event #"<<eventID<<" → Position input : "<<pos_1/mm<<" mm"<<" → Position output : "<<pos_2 / mm<<" mm"<<G4endl;
    }

    // Récupération ou création du TrackInfo
    //  Ce bloc de code gère l’attachement et la gestion d'informations personnalisées
    //  (via une classe MyTrackInfo) à chaque particule (G4Track),
    //  pour suivre son historique (par exemple : entrée dans un volume, processus créateur...).
    //
    //  Geant4 permet d'attacher à chaque G4Track un objet utilisateur via :
    //  utilise ici avec ta classe personnalisée MyTrackInfo, dérivée de G4VUserTrackInformation.
    //
    // MyTrackInfo* trackInfo = static_cast<MyTrackInfo*>(track->GetUserInformation());
    //
    // Si c’est le premier step du track
    // création d' un MyTrackInfo et
    // stockage du processus créateur (primary, compt, eBrem, etc.)
    // 17/5  MyTrackInfo* trackInfo = static_cast<MyTrackInfo*>(track->GetUserInformation());
    //  Création de l’objet si inexistant
    //
    //  Si le pointeur était nul, on crée un objet MyTrackInfo
    //  Et on l’associe au track avec SetUserInformation(...)
    //  Cela garantit que chaque track a ses propres infos personnalisées (comme "est-il déjà entré dans le cube ?")

    if (!trackInfo) {
        if (fSteppingVerboseLevel == 1) {
            G4cout<<"[DEBUG SteppingAction] MyTrackInfo est nul → création..."<< G4endl;}
        trackInfo = new MyTrackInfo();
        track->SetUserInformation(trackInfo);

        //  Enregistrement du processus créateur
        //  récupères ici le processus qui a généré ce track, s’il s’agit d’un secondaire.
        //  Si creator == nullptr, c’est une particule primaire.
        const G4VProcess* creator = track->GetCreatorProcess();
        G4String pname = (creator ? creator->GetProcessName() : "primary");
        if (fSteppingVerboseLevel == 1) {
            G4cout<<"[DEBUG SteppingAction] Processus créateur = "<< pname<<G4endl;}
        //  Stockage du nom du processus dans MyTrackInfo
        //  Si c’est une particule secondaire, on enregistre son processus d’origine ("compt", "eBrem", "eIoni"…).
        //  Sinon on note "primary".
        if (creator) {
            G4String pname = creator->GetProcessName();
            if (pname.length() > 100) {
                if (fSteppingVerboseLevel == 1) {
                    G4cerr<<"[DEBUG SteppingAction] ⚠️ Nom de processus anormalement long → "<<pname<<G4endl;}
            }
            trackInfo->SetCreatorProcess(pname);
        } else {
            trackInfo->SetCreatorProcess("primary");
        }
    }

    // Détection de l’entrée dans le cube
    // Si le track n’est pas encore marqué comme "entré dans le cube"
    // Et que son postStep est dans "logicWaterCube" → alors :
    // On marque l’entrée (SetEnteredCube(true))
    // Cela sert à :
    // Enregistrer l’entrée uniquement la première fois
    // Éviter de compter plusieurs fois si la particule repasse ou rebondit
    if (!trackInfo->HasEnteredCube() && namePost == "logicWaterCube") {
        trackInfo->SetEnteredCube(true);
    }

    // Détection de l’entrée dans la sphère et remplir histogramme 0
    //
    //  trackInfo->HasEnteredSphere() :
    //  Retourne true si ce track est déjà entré dans la sphère.
    //  Donc ici, on ne veut agir que la première fois qu’un track entre.
    //
    //  namePost == "logicsphereWater" :
    //  La particule arrive dans la sphère après le step.
    //
    //  Marquer l’entrée une seule fois :
    if (!trackInfo->HasEnteredSphere() && namePost == "logicsphereWater") {
        trackInfo->SetEnteredSphere(true);
        //  Met à jour le MyTrackInfo pour ce track.
        //  Cela garantit qu’on n’entrera plus jamais dans ce bloc pour ce track,
        //  même si la particule repasse plusieurs fois dans la sphère.
        // [SUPPRIMÉ] // [SUPPRIMÉ] if (analysisManager)
            // [SUPPRIMÉ] analysisManager->FillH1(0, energy);  // Histogramme 0 : entrée sphère
            //  Enregistre l’énergie cinétique de la particule au moment de l’entrée dans la sphère.
            //  Histogramme ID 0 → par convention dans ton projet, utilisé pour les énergies d’entrée dans la sphère.
            //  Cela permet de tracer la distribution énergétique des particules entrantes dans la sphère.

        // Incrémenter compteur entrée sphère
        fEventAction->IncrementNbEntrantInWaterSphere();
        G4int NbEntrantInWaterSphere = fEventAction->GetNbEntrantInWaterSphere();
        if (fSteppingVerboseLevel == 1) {
            G4cout<<"NbEntrantInWaterSphere : "<<NbEntrantInWaterSphere<<G4endl;}
    }

    // Remplir histogramme 1 à la sortie de la sphère
    // Détecter les particules qui sortent de la sphère vers le cube (sans interaction)
    // namePre == "logicsphereWater" → la particule est dans la sphère
    // namePost == "logicWaterCube" → elle retourne dans le cube (le volume parent)
    // enregistre l’énergie au moment de la sortie de la sphère
    //
    // Cela permet de comparer histogramme 0 (entrée) et 1 (sortie)
    // pour analyser l’atténuation ou la perte d’énergie.
    if (namePre == "logicsphereWater" && namePost == "logicWaterCube") {
        // [SUPPRIMÉ] // [SUPPRIMÉ] if (analysisManager)
            // [SUPPRIMÉ] analysisManager->FillH1(1, energy);  // Histogramme 1 : sortie sphère
    }
    // Détection d’interaction dans la sphère
    // La particule est restée dans "logicsphereWater" pendant le step
    // Et :
    //     Elle a déposé de l’énergie
    //     Ou elle a subi un processus physique réel (process ≠ "Transportation")
    //
    // Affiche un log : type de processus + énergie déposée
    // IncrementNbInteractedInWaterSphere() → compteur par événement
    // FillH1(5, edep) → histogramme de l’énergie déposée dans la sphère
    //
    //if (namePre == "logicsphereWater" && namePost == "logicsphereWater") {
    //    G4double edep = step->GetTotalEnergyDeposit();
    //    const G4VProcess* process = postPoint->GetProcessDefinedStep();
    //
    //    if (edep > 0. || (process && process->GetProcessName() != "Transportation")) {
    //        G4cout <<  " \n💦 Interaction dans WaterSphere !" << G4endl;
    //        if (process)
    //            G4cout << " → Processus : " << process->GetProcessName() << G4endl;
    //        G4cout << " → Dépôt d’énergie : " << edep / keV << " keV" << G4endl;

    if (namePre == "logicsphereWater" && namePost == "logicsphereWater") {
        G4double edep = step->GetTotalEnergyDeposit();
        const G4VProcess* process = postPoint->GetProcessDefinedStep();
        //G4String pname = track->GetParticleDefinition()->GetParticleName();

        if (process) {
            //G4String procName = process->GetProcessName();

            // Filtrer les processus non physiques
            if (process->GetProcessName() != "Transportation" && process->GetProcessName() != "msc") {
                if (fSteppingVerboseLevel == 1) {
                    G4cout<<"\n[DEBUG SteppingAction] 💦 Interaction dans WaterSphere !"<<G4endl;
                    G4cout<<" [DEBUG SteppingAction] → Particule : "<<track->GetParticleDefinition()->GetParticleName()<<G4endl;
                    G4cout<<" [DEBUG SteppingAction] → Processus : "<<process->GetProcessName()<<G4endl;
                    G4cout<<" [DEBUG SteppingAction] → Dépôt d’énergie :"<<edep/keV<<" keV"<< G4endl;}

                // ✅ Boucle sur les secondaires produits
                const auto* secondaries = step->GetSecondaryInCurrentStep();
                if (secondaries && !secondaries->empty()) {
                    if (fSteppingVerboseLevel == 1) {
                        G4cout<<"[DEBUG SteppingAction] → Secondaires produits : "<<secondaries->size()<<G4endl;}
                    for (const auto* sec : *secondaries) {
                        if (fSteppingVerboseLevel == 1) {
                            G4cout<<"[DEBUG SteppingAction] ↪ "<<sec->GetParticleDefinition()->GetParticleName()
                            <<", E ="<<sec->GetKineticEnergy()/keV<<" keV"
                            <<", créé par : "
                            <<(sec->GetCreatorProcess()
                            ? sec->GetCreatorProcess()->GetProcessName()
                            : "N/A")
                            << G4endl;}
                    }
                }

            // Incrémenter compteur d'interaction
            fEventAction->IncrementNbInteractedInWaterSphere();
            G4int NbInteractedInWaterSphere = fEventAction->GetNbInteractedInWaterSphere();
            if (fSteppingVerboseLevel == 1) {
                G4cout<<"[DEBUG SteppingAction] NbInteractedtInWaterSphere : "<<NbInteractedInWaterSphere<<G4endl;
            }
            // [SUPPRIMÉ] // [SUPPRIMÉ] if (analysisManager)
                // [SUPPRIMÉ] analysisManager->FillH1(5, edep);
            }
        }
    }

    // Transmettre au EventAction si primaire au 1er step
    // Transfert du TrackInfo au EventAction si primaire au 1er step
    // Transmettre les informations personnalisées du track primaire (MyTrackInfo) à EventAction,
    // afin qu’elles soient disponibles dans EndOfEventAction() pour les ntuples ou statistiques.
    //
    // Track->GetTrackID() == 1 :
    // Le track ID = 1 correspond à la particule primaire de l’événement.
    // track->GetCurrentStepNumber() == 1 :
    // On est à son tout premier step.
    //
    // Cela évite d'appeler plusieurs fois SetTrackInfo() inutilement.
    //
    //  Appelle fEventAction->SetTrackInfo(...), qui copie les infos utiles :
    //  nom du processus créateur ("primary", "compt"…)
    //  booléens : HasEnteredCube(), HasEnteredSphere(), etc.
    //
    //  Cela permet ensuite dans EventAction::EndOfEventAction() d’enregistrer :
    if (track->GetTrackID() == 1 && track->GetCurrentStepNumber() == 1) {
        fEventAction->SetTrackInfo(trackInfo);
        if (fSteppingVerboseLevel == 1) {
            G4cout<<"[DEBUG SteppingAction] SetTrackInfo (initial) pour track primaire"<<G4endl;}}

    // Tuer les particules sortant du cube vers l'extérieur
    // Arrêter les particules qui quittent définitivement le cube)
    // par un volume non pertinent (autre que la sphère d’eau).
    // namePre == "logicWaterCube" : la particule était dans le volume logique du cube
    // namePost != "logicWaterCube" : elle en sort
    // namePost != "logicsphereWater" : elle ne passe pas dans la sphère → donc elle sort vraiment du système
    //
    // Action
    // track->SetTrackStatus(fStopAndKill) :
    // Tue immédiatement la particule
    // Empêche Geant4 de continuer à la propager (gain de performance)
    if (namePre == "logicWaterCube" &&
        namePost != "logicWaterCube" &&
        namePost != "logicsphereWater") {
        if (fSteppingVerboseLevel == 1) {
            G4cout <<"[DEBUG SteppingAction] ☠️ Particule tuée (sortie définitive de logicWaterCube)"<<G4endl;
            G4cout <<"[DEBUG SteppingAction] de "<<namePre<<" → "<<namePost<<G4endl;}
        track->SetTrackStatus(fStopAndKill);
    }

    // Test de l sortie de la sphere logicEnveloppeGDML
    if (namePre == "logicEnveloppeGDML" && namePost != "logicEnveloppeGDML") {
        G4ThreeVector sortie = postPoint->GetPosition();
        G4String particleName = track->GetParticleDefinition()->GetParticleName();
        G4double energyOut = track->GetKineticEnergy();
        G4bool isPrimary = (track->GetParentID() == 0);

        G4double r     = sortie.mag();
        G4double theta = sortie.theta();
        G4double phi   = sortie.phi();

        G4double thetaDeg = theta / deg;
        G4double phiDeg   = phi / deg;
        G4double cosTheta = std::cos(theta);

        if (fSteppingVerboseLevel == 1) {
            G4cout << "\n[DEBUG SteppingAction] 🚪 Sortie de la sphère EnveloppeGDML détectée !" << G4endl;
            G4cout << "  → Position sortie : " << sortie / mm << " mm" << G4endl;
            G4cout << "  → Particule       : " << particleName << G4endl;
            G4cout << "  → Énergie         : " << energyOut / keV << " keV" << G4endl;
        }

        // Filtrage : primaire
    if (isPrimary) {
        if (fSteppingVerboseLevel == 1) {
            G4cout<<"\n[DEBUG SteppingAction] ✅ Sortie d'une particule primaire ("<<particleName<<")de l'enveloppe GDML"<<G4endl;
            G4cout<<"r → "<<r<<"theta → "<<thetaDeg<<"phi → "<<phiDeg<<G4endl;}
        // [SUPPRIMÉ] // [SUPPRIMÉ] if (analysisManager)
        {
            if (fSteppingVerboseLevel == 1) {
                G4cout<<"\n[DEBUG SteppingAction] ✅ Remplissage des histos →"<<G4endl;}
            // [SUPPRIMÉ] analysisManager->FillH1(7, thetaDeg);
            // [SUPPRIMÉ] analysisManager->FillH1(8, phiDeg);

        }

    } else {
        if (fSteppingVerboseLevel == 1) {
        G4cout<<"\n[DEBUG SteppingAction] 🌀 Sortie d'une particule secondaire ("<<particleName<<") de l'enveloppe GDML"<<G4endl;
        G4cout<<"r → "<<r<<"theta → "<<thetaDeg<<"phi → "<<phiDeg<<G4endl;}
        // [SUPPRIMÉ] // [SUPPRIMÉ] if (analysisManager)
        {
            if (fSteppingVerboseLevel == 1) {
                G4cout<<"\n[DEBUG SteppingAction] 🌀 Remplissage des histos →"<<G4endl;}
            // [SUPPRIMÉ] analysisManager->FillH1(9, thetaDeg);
            // [SUPPRIMÉ] analysisManager->FillH1(10, phiDeg);
        }

    }
    }

    //  Debug (optionnel)
    //G4int eventID = G4EventManager::GetEventManager()->GetConstCurrentEvent()->GetEventID();
    if (fSteppingVerboseLevel == 1) {
        G4cout << "[DEBUG SteppingAction]  Event #"<<eventID<<" — trackID: "<<track->GetTrackID()<< G4endl;
        G4cout << "[DEBUG SteppingAction] → de "<<namePre<<" → "<<namePost<<G4endl;
      if (trackInfo->HasEnteredCube())
        G4cout << "[DEBUG SteppingAction] ✓ Entré dans cube\n";
      if (trackInfo->HasEnteredSphere())
        G4cout << "[DEBUG SteppingAction] ✓ Entré dans sphère\n";
    }

    G4String creator = trackInfo->GetCreatorProcess();
    if (creator.empty()) creator = "unknown";

    if (fSteppingVerboseLevel == 1) {
        G4cout<<"[DEBUG SteppingAction] → Processus créateur : "<<creator<<G4endl;}



    // [TRACE] Fin de piste PRIMAIRE : process + matériau ===
    // [FIX] Appel FINAL de SetTrackInfo pour capturer l'état des flags
    //       HasEnteredCube() et HasEnteredSphere() à la fin du tracking
    do {
        if (track->GetParentID()!=0) break; // primaires uniquement
        const bool died     = (track->GetTrackStatus()==fStopAndKill);
        const bool outWorld = (postPoint->GetStepStatus()==fWorldBoundary);
        if (!(died || outWorld)) break;

        // [FIX] Transmettre l'état FINAL du trackInfo à EventAction
        // Maintenant HasEnteredCube() et HasEnteredSphere() reflètent
        // si la particule est réellement entrée dans ces volumes
        if (trackInfo && fEventAction) {
            fEventAction->SetTrackInfo(trackInfo);
            if (fSteppingVerboseLevel == 1) {
                G4cout << "[DEBUG SteppingAction] SetTrackInfo (FINAL) - "
                       << "cube=" << trackInfo->HasEnteredCube()
                       << " sphere=" << trackInfo->HasEnteredSphere() << G4endl;
            }
        }

        const auto* proc = postPoint->GetProcessDefinedStep();
        const auto* pv   = postPoint->GetPhysicalVolume();
        const auto* lv   = pv ? pv->GetLogicalVolume() : nullptr;
        const auto* mat  = postPoint->GetMaterial();

        // [LOSS] Si le primaire s'arrête avant le plan z=60 mm, comptabiliser (proc + matériau)
        do {
            constexpr G4double zPlane = 60.*mm;
            const G4double zPost = postPoint->GetPosition().z();
            if (zPost >= zPlane) break; // pas "perdu avant le plan"
            const auto* preMat   = prePoint->GetMaterial();
            const auto* stepProc = postPoint->GetProcessDefinedStep();
            const std::string mname = preMat ? std::string(preMat->GetName()) : "Unknown";
            const std::string pname = stepProc ? std::string(stepProc->GetProcessName()) : "Unknown";
            #ifdef G4MULTITHREADED
            { G4AutoLock lock(&gLossMapMutex); ++gLostByMat[mname]; ++gLostByProc[pname]; }
            #else
            ++gLostByMat[mname];
            ++gLostByProc[pname];
            #endif
        } while(0);

        // ==================== ABSORPTIONS PHOTOELECTRIQUES ====================
        // Remplissage des ntuples abs_graphite et abs_inox
        // pour chaque primaire absorbé par effet photoélectrique
        // dans le cône graphite ou dans l'inox (porte-collimateur, enveloppe)
        // ==================================================================
        do {
            // Uniquement les absorptions photoélectriques
            if (!proc || proc->GetProcessName() != "phot") break;

            const G4ThreeVector absPos = postPoint->GetPosition();
            const G4double abs_x_mm = absPos.x() / mm;
            const G4double abs_y_mm = absPos.y() / mm;
            const G4double abs_z_mm = absPos.z() / mm;
            const G4double abs_ekin_keV = prePoint->GetKineticEnergy() / keV;

            // Récupérer info Compton depuis MyTrackInfo
            G4int had_compton = 0;
            G4int n_compton   = 0;
            if (trackInfo) {
                had_compton = trackInfo->HasComptonInCone() ? 1 : 0;
                n_compton   = trackInfo->GetNComptonInCone();
            }

            auto* man = G4AnalysisManager::Instance();
            if (!man || !man->IsActive()) break;

            // --- Absorption dans le cône graphite ---
            if (namePre == "logicConeCompton") {
                const G4int ntupleId = GetAbsGraphiteNtupleId();
                if (ntupleId >= 0) {
                    man->FillNtupleIColumn(ntupleId, 0, eventID);         // eventID
                    man->FillNtupleIColumn(ntupleId, 1, track->GetTrackID()); // trackID
                    man->FillNtupleDColumn(ntupleId, 2, abs_ekin_keV);    // ekin_keV
                    man->FillNtupleDColumn(ntupleId, 3, abs_x_mm);       // x_mm
                    man->FillNtupleDColumn(ntupleId, 4, abs_y_mm);       // y_mm
                    man->FillNtupleDColumn(ntupleId, 5, abs_z_mm);       // z_mm
                    man->FillNtupleIColumn(ntupleId, 6, had_compton);    // had_compton_in_cone
                    man->FillNtupleIColumn(ntupleId, 7, n_compton);      // n_compton_in_cone
                    man->AddNtupleRow(ntupleId);

                    static G4int sAbsGraphLog = 0;
                    if (sAbsGraphLog < 50 || sAbsGraphLog % 10000 == 0) {
                        G4cout << "[STEP][ABS_GRAPHITE] #" << sAbsGraphLog
                               << " | event=" << eventID
                               << " | E=" << abs_ekin_keV << " keV"
                               << " | pos(mm)=(" << abs_x_mm << ", "
                                                  << abs_y_mm << ", "
                                                  << abs_z_mm << ")"
                               << " | had_compton=" << had_compton
                               << " | n_compton=" << n_compton
                               << G4endl;
                    }
                    ++sAbsGraphLog;
                }
                break;
            }

            // --- Absorption dans l'inox SS304 ---
            const auto* preMat = prePoint->GetMaterial();
            if (preMat && preMat->GetName() == "StainlessSteel304") {
                const G4int ntupleId = GetAbsInoxNtupleId();
                if (ntupleId >= 0) {
                    man->FillNtupleIColumn(ntupleId, 0, eventID);         // eventID
                    man->FillNtupleIColumn(ntupleId, 1, track->GetTrackID()); // trackID
                    man->FillNtupleDColumn(ntupleId, 2, abs_ekin_keV);    // ekin_keV
                    man->FillNtupleDColumn(ntupleId, 3, abs_x_mm);       // x_mm
                    man->FillNtupleDColumn(ntupleId, 4, abs_y_mm);       // y_mm
                    man->FillNtupleDColumn(ntupleId, 5, abs_z_mm);       // z_mm
                    man->FillNtupleSColumn(ntupleId, 6, namePre);        // volume logique
                    man->FillNtupleIColumn(ntupleId, 7, had_compton);    // had_compton_in_cone
                    man->FillNtupleIColumn(ntupleId, 8, n_compton);      // n_compton_in_cone
                    man->AddNtupleRow(ntupleId);

                    static G4int sAbsInoxLog = 0;
                    if (sAbsInoxLog < 50 || sAbsInoxLog % 10000 == 0) {
                        G4cout << "[STEP][ABS_INOX] #" << sAbsInoxLog
                               << " | event=" << eventID
                               << " | E=" << abs_ekin_keV << " keV"
                               << " | vol=" << namePre
                               << " | pos(mm)=(" << abs_x_mm << ", "
                                                  << abs_y_mm << ", "
                                                  << abs_z_mm << ")"
                               << " | had_compton=" << had_compton
                               << " | n_compton=" << n_compton
                               << G4endl;
                    }
                    ++sAbsInoxLog;
                }
                break;
            }
        } while(0);
        // ==================== FIN ABSORPTIONS PHOTOELECTRIQUES ====================

        // COMMENTÉ pour réduire la taille du fichier log
        /*
        static int seen=0, maxPrint=60;
        if (seen < maxPrint) {
        const auto* rm = G4RunManager::GetRunManager();
        const int eid  = (rm && rm->GetCurrentEvent()) ? rm->GetCurrentEvent()->GetEventID() : -1;


        const G4String lvName  = lv  ? lv->GetName()
        : G4String(outWorld ? "<WorldBoundary>" : "<none>");
        const G4String matName = mat ? mat->GetName()
        : G4String(outWorld ? "<vacuum/world>" : "<none>");
        const G4String procName = proc ? proc->GetProcessName()
        : G4String(outWorld ? "WorldBoundary" : "Unknown");

        G4cout << "[TRACE][END-PRIMARY] evt=" << eid
        << " z=" << postPoint->GetPosition().z()/mm << " mm"
        << " LV="  << lvName
        << " MAT=" << matName
        << " PROC="<< procName
        << G4endl;

            }
        */
    } while(0);

}
