#include "RunAction.hh"
#include "EventAction.hh"
#include "AnalysisManagerSetup.hh"

#include "G4AnalysisManager.hh"
#include "G4AccumulableManager.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"

#include "RunMessenger.hh"

#include "G4Threading.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"
#include "SurfaceSpectrumSD.hh"

#include <fstream>
#include <iostream>

#include "SphereHit.hh"
#include "SteppingAction.hh"  // Pour le suivi step par step

extern G4long gEnterPlanePrim;
extern G4long gLeavePlanePrim;

extern G4long gEnterPlanePrim;
extern G4long gLeavePlanePrim;

// maps définies dans SteppingAction.cc
extern std::map<std::string,int> gLostByProc;
extern std::map<std::string,int> gLostByMat;

// ============================================================================
// [ADD] Helper de logs : SEQ en mono-thread, sinon MT-MASTER / MT-WORKER
// ============================================================================
namespace {
    inline const char* ThreadTag() {
        #ifdef G4MULTITHREADED
        return G4Threading::IsMasterThread() ? "[MT-MASTER]" : "[MT-WORKER]";
        #else
        return "[SEQ]";
        #endif
    }
    // [ADD] Protéger SetupAnalysis() contre une double exécution (si 2 ctors utilisés)
    static bool gAnalysisSetupDone = false;
} // namespace

//  Ce constructeur initialise les accumulateurs globaux utilisés pour compter, sur l’ensemble du run :
//  -   le nombre de particules entrées et ayant interagi dans le Béryllium
//  -   le nombre de particules entrées et ayant interagi dans la sphère d’eau
//  Initialisation des membres avec 0
//  : fTotalEntrantInBe(0), fTotalInteractedInBe(0),
//  fTotalEntrantInWaterSphere(0), fTotalInteractedInWaterSphere(0)
//
//  Ces membres sont des objets G4Accumulable<G4int> (compteurs d’entiers).
//  Ils sont initialisés à 0 au début du run.
//  Ce sont des compteurs globaux au run, contrairement à ceux d’EventAction, qui sont locaux à un événement.
//
//  Pourquoi utiliser G4Accumulable
//  G4AccumulableManager est le gestionnaire central des accumulateurs dans Geant4.
//  Register(...) indique au gestionnaire que ces compteurs doivent être :
//  - initialisés au début du run
//  - fusionnés automatiquement en multithread
//  - accessibles via GetValue() en fin de run
//
//  Ce mécanisme rend ton code compatible avec multithread et single-thread sans changement.
RunAction::RunAction()
: fTotalEntrantInBe(0), fTotalInteractedInBe(0),
fTotalEntrantInWaterSphere(0), fTotalInteractedInWaterSphere(0)
{
    auto accMgr = G4AccumulableManager::Instance();
    accMgr->Register(fTotalEntrantInBe);
    accMgr->Register(fTotalInteractedInBe);
    accMgr->Register(fTotalEntrantInWaterSphere);
    accMgr->Register(fTotalInteractedInWaterSphere);

    accMgr->Register(fNValidParticles_lt_35);
    accMgr->Register(fNValidParticles_gt_35);


    // [ADD] compteur global des primaires (option B)
    accMgr->Register(fPrimariesGenerated);

    fRunMessenger = new RunMessenger(this);

    // -------------------- [ADD] Activation & setup analysis --------------------
    if (!gAnalysisSetupDone) {
        auto* am = G4AnalysisManager::Instance();
        am->SetActivation(true);   // [ADD] ACTIVER avant de créer les ntuples
        SetupAnalysis();           // [ADD] crée les ntuples/colonnes une seule fois
        gAnalysisSetupDone = true;
    }
}

//  RunAction::RunAction(EventAction* eventAction)
//  C’est un constructeur surchargé, utilisé pour créer RunAction avec un lien vers EventAction.
//  Typiquement appelé depuis :
//      - auto eventAction = new EventAction();
//      - auto runAction   = new RunAction(eventAction);
//
//  On indique à Geant4 : "Je veux que ces compteurs soient suivis, fusionnés, et accessibles à la fin du run".
//  Cela permet :
//      - De faire un Reset() automatique au début du run
//      - De faire un Merge() en multithread pour combiner les valeurs
//      - D’utiliser GetValue() pour accéder aux résultats finaux
RunAction::RunAction(EventAction* eventAction)
: fTotalEntrantInBe(0), fTotalInteractedInBe(0),
fTotalEntrantInWaterSphere(0), fTotalInteractedInWaterSphere(0)
{
    auto accMgr = G4AccumulableManager::Instance();
    accMgr->Register(fTotalEntrantInBe);
    accMgr->Register(fTotalInteractedInBe);
    accMgr->Register(fTotalEntrantInWaterSphere);
    accMgr->Register(fTotalInteractedInWaterSphere);

    accMgr->Register(fNValidParticles_lt_35);
    accMgr->Register(fNValidParticles_gt_35);

    // [ADD] compteur global des primaires (option B)
    accMgr->Register(fPrimariesGenerated);

    fRunMessenger = new RunMessenger(this);

    // -------------------- [ADD] Activation & setup analysis --------------------
    if (!gAnalysisSetupDone) {
        auto* am = G4AnalysisManager::Instance();
        am->SetActivation(true);   // [ADD]
        SetupAnalysis();           // [ADD]
        gAnalysisSetupDone = true;
    }
    // (si besoin, stocke eventAction dans un membre ici)
}

RunAction::~RunAction(){
    delete fRunMessenger;}

G4int RunAction::GetTotalEntrantInBe() const {
    return fTotalEntrantInBe.GetValue();}

G4int RunAction::GetTotalInteractedInBe() const {
    return fTotalInteractedInBe.GetValue(); }

G4int RunAction::GetTotalEntrantInWaterSphere() const {
    return fTotalEntrantInWaterSphere.GetValue(); }

G4int RunAction::GetTotalInteractedInWaterSphere() const {
    return fTotalInteractedInWaterSphere.GetValue(); }

void RunAction::BeginOfRunAction(const G4Run* run)
{
    // ==================== Step Tracking ====================
    // Réinitialiser le compteur et imprimer l'en-tête du tableau
    SteppingAction::ResetTrackedParticlesCount();
    // =======================================================

    auto* am = G4AnalysisManager::Instance();

    #ifdef G4MULTITHREADED
    if (!G4Threading::IsMasterThread()) return;
    #endif

    //G4cout << ThreadTag() << " [RUN] BeginOfRunAction: start run "<< run->GetRunID() << G4endl;

    // [ADD] Safety : s’assurer que l’analyse est bien active
    //G4cout << ThreadTag() << " [RUN] IsActive BEFORE = " << am->IsActive() << G4endl; // [LOG]
    am->SetActivation(true);                                                  // [ADD]
    //G4cout << ThreadTag() << " [RUN] IsActive AFTER  = " << am->IsActive() << G4endl; // [LOG]

    // [ADD] Ouvrir (ou rouvrir) le fichier en début de run
    am->OpenFile("output.root");
    //G4cout << ThreadTag() << " [RUN] Opened analysis file: output.root" << G4endl;    // [LOG]

    // [ADD] (optionnel) log d’ID de l’ntuple plane_passages
    //G4cout << "[RUN] plane_passages ntuple id = " << GetPlanePassageNtupleId() << G4endl; // [LOG]


    // Réinitialiser les accumulateurs pour ce run
    G4AccumulableManager::Instance()->Reset();


    // [KEEP] Câblage du SensitiveDetector « SpecSD » vers l’ID de l’ntuple plane_passages
    {
        auto* sdMan = G4SDManager::GetSDMpointer();
        if (auto* sd = dynamic_cast<SurfaceSpectrumSD*>(
            sdMan->FindSensitiveDetector("SpecSD", /*depthSearch*/ false))) {
            sd->SetPassageNtupleId(GetPlanePassageNtupleId());
        G4cout << "[RUN] SpecSD wired to plane_passages ntuple id = "
        << GetPlanePassageNtupleId() << G4endl;                    // [LOG]
        } else {
            G4cout << "[RUN][WARN] SpecSD not found; plane_passages ntuple will not be filled."
            << G4endl;                                                 // [LOG]
        }
    }

    // Initialiser les compteurs pour ce run
     fNValidParticles_lt_35 = 0;
     fNValidParticles_gt_35 = 0;
     
    // Réinitialiser les accumulateurs d'énergie pour ce run
    for (G4int i = 0; i < kNbWaterRings; i++) {
        fTotalEdepRing[i] = 0.0;
        fEdepRing10000[i] = 0.0;
    }
    fTotalEdepWater = 0.0;
    fEdepWater10000 = 0.0;
    fLastHistoFillEvent = -10000;
    
    // Réinitialiser les compteurs de photons transmis
    fTransmitted10000 = 0;
    fTransmittedTotal = 0;
}

//  La fonction RunAction::EndOfRunAction(const G4Run*)est appelée automatiquement
//  par Geant4 à la fin du run (après tous les événements).
//  Elle pemet de :
//      - Fusionner les compteurs globaux (en cas de multithread)
//      - Afficher un résumé du run
//      - Afficher un bilan des hits par événement
//      - Fermer correctement le fichier d’analyse
void RunAction::EndOfRunAction(const G4Run*)
{
    auto* am = G4AnalysisManager::Instance();
    //G4cout << "[RunAction] Fin du run, appel à FinalizeAnalysis()" << G4endl;

    //Fusion des accumulateurs (multithreading)
    //    -Dans un exécution multithread, chaque thread a ses propres accumulateurs (G4Accumulable).
    //    -Cette ligne fusionne tous les compteurs en un total global.
    //    -En mono-thread, cela n’a aucun effet néfaste.


    G4AccumulableManager::Instance()->Merge();

    // Ne logg(er)/écrire/fermer qu’une seule fois (master en MT, sinon SEQ)
    bool isMaster = true;
    #ifdef G4MULTITHREADED
    isMaster = G4Threading::IsMasterThread();
    #endif

    if (isMaster) {
        // (sécurité) s’assurer que l’analyse est bien active pour Write/Close
        if (!am->IsActive()) {
            G4cout << ThreadTag()
            << " [RUN][WARN] Analysis inactive at EndOfRunAction -> forcing activation"
            << G4endl;
            am->SetActivation(true);
        }

        // ==================== RÉSUMÉ GLOBAL ====================
        G4cout << "\n====================[ RUN SUMMARY ]====================\n";

        // (A) Primaires tirés (via accumulable)
        G4cout << "Primaries generated = "
        << fPrimariesGenerated.GetValue() << G4endl;

        // (B) Récap Béryllium
        G4cout << "========== Résumé Béryllium ==========" << G4endl;
        G4cout << "Total particules entrées dans Be    : "
        << fTotalEntrantInBe.GetValue() << G4endl;
        G4cout << "Total interactions dans le Be       : "
        << fTotalInteractedInBe.GetValue() << G4endl;
        G4cout << "======================================" << G4endl;

        // (C) Récap WaterSphere
        G4cout << "================= Résumé WaterSphere ==========" << G4endl;
        G4cout << "Total particules entrées dans WaterSphere    : "
        << fTotalEntrantInWaterSphere.GetValue() << G4endl;
        G4cout << "Total interactions dans le WaterSphereB      : "
        << fTotalInteractedInWaterSphere.GetValue() << G4endl;
        G4cout << "===============================================" << G4endl;

        // Bilan hits par évènement
        for (const auto& kv : fHitsByEvent) {
            auto eventID = kv.first;
            const auto& hits = kv.second;
            G4double totalEdep = 0.0;
            for (const auto& hit : hits) totalEdep += hit.GetEdep();

            if (fRunVerbose == 1) {
                G4cout << "→ Event #" << eventID
                << " : " << hits.size() << " hits, "
                << "E_dep total = " << totalEdep / keV << " keV" << G4endl;
            }
        }

        // Récap “spectre émission”
        G4cout << "========== Résumé spectre emission ==========" << G4endl;
        G4cout << "Total particules inferieure a 3.5 keV      : "
        << fNValidParticles_lt_35.GetValue() << G4endl;
        G4cout << "Total particules superieure a 3.5 keV      : "
        << fNValidParticles_gt_35.GetValue() << G4endl;
        G4cout << "=============================================" << G4endl;

        // Compteurs côté Stepping : primaires au plan
        G4cout << "[STEP][SUMMARY] enter_plane_prim=" << gEnterPlanePrim
        << " leave_plane_prim=" << gLeavePlanePrim << G4endl;


        auto* sdm = G4SDManager::GetSDMpointer();
        if (auto* sd = dynamic_cast<SurfaceSpectrumSD*>(sdm->FindSensitiveDetector("SpecSD", false))) {
            sd->PrintSummary();

            // [LOSS] Pertes de primaires avant z=60 mm : ventilation
            if (!gLostByProc.empty() || !gLostByMat.empty()) {
                G4cout << "[LOSS][BY-PROC]" << G4endl;
                for (const auto& kv : gLostByProc) {
                    G4cout << "  " << kv.first << " : " << kv.second << G4endl;
                }
                G4cout << "[LOSS][BY-MAT]" << G4endl;
                for (const auto& kv : gLostByMat) {
                    G4cout << "  " << kv.first << " : " << kv.second << G4endl;
                }
            } else {
                G4cout << "[LOSS] no primary lost before z=60 mm (maps empty)" << G4endl;
            }
        } else {
            G4cout << "[WARN] SpecSD not found in SDManager at EndOfRunAction()" << G4endl;
        }

        G4cout << "=======================================================\n";

        // ==================== Step Tracking Summary ====================
        G4cout << "\n================================================================================\n"
               << "FIN DU SUIVI STEP PAR STEP - " << SteppingAction::GetTrackedParticlesCount() 
               << " evenements suivis (max=" 
               << SteppingAction::GetMaxTrackedParticles() << ")\n"
               << "================================================================================\n" 
               << G4endl;
        // ===============================================================

        // ==================== Remplissage des histogrammes de DOSE (fin de run) ====================
        // Conversion : Edep (keV) -> Dose (pGy)
        // Dose = Edep / masse
        // 1 keV = 1.602e-16 J
        // 1 Gy = 1 J/kg = 1e12 pGy
        // Donc: Dose [pGy] = Edep [keV] * 1.602e-16 / masse [kg] * 1e12
        //                  = Edep [keV] * 1.602e-4 / masse [kg]
        //                  = Edep [keV] * 1.602e-4 / (masse [g] * 1e-3)
        //                  = Edep [keV] * 1.602e-1 / masse [g]
        //                  = Edep [keV] * 0.1602 / masse [g]
        constexpr G4double keV_to_pGy_per_gram = 0.1602;
        
        // H3: Dose totale dans l'eau (run complet) - en pGy
        G4double dose_total_run_pGy = fTotalEdepWater * keV_to_pGy_per_gram / kMassTotalWater;
        am->FillH1(3, dose_total_run_pGy);
        
        // H5-H9: Dose par anneau (run complet) - en pGy
        for (G4int i = 0; i < kNbWaterRings; i++) {
            G4double dose_ring_run_pGy = fTotalEdepRing[i] * keV_to_pGy_per_gram / kMassRing[i];
            am->FillH1(5 + i, dose_ring_run_pGy);
        }
        
        // Afficher le résumé des doses (en nGy pour la lisibilité, 1 nGy = 1000 pGy)
        G4double dose_total_run_nGy = dose_total_run_pGy / 1000.0;
        G4cout << "\n==================== RÉSUMÉ DOSE ====================\n";
        G4cout << "Énergie totale déposée dans l'eau : " << fTotalEdepWater << " keV\n";
        G4cout << "Dose totale dans l'eau (run)      : " << dose_total_run_pGy << " pGy = " 
               << dose_total_run_nGy << " nGy\n";
        G4cout << "Dose par anneau (run) :\n";
        for (G4int i = 0; i < kNbWaterRings; i++) {
            G4double dose_ring_pGy = fTotalEdepRing[i] * keV_to_pGy_per_gram / kMassRing[i];
            G4double dose_ring_nGy = dose_ring_pGy / 1000.0;
            G4cout << "  Anneau " << i << " (r=" << 2*i << "-" << 2*(i+1) << "mm) : "
                   << fTotalEdepRing[i] << " keV -> " << dose_ring_pGy << " pGy = " 
                   << dose_ring_nGy << " nGy\n";
        }
        G4cout << "=====================================================\n";
        // ====================================================================================

        // 3) Écriture / fermeture du ROOT (une seule fois)
        G4cout << ThreadTag() << " [RUN] EndOfRunAction: about to Write()" << G4endl;
        am->Write();
        G4cout << ThreadTag() << " [RUN] EndOfRunAction: Write() done" << G4endl;

        G4cout << ThreadTag() << " [RUN] EndOfRunAction: about to CloseFile(false)" << G4endl;
        am->CloseFile(false);
        G4cout << ThreadTag() << " [RUN] EndOfRunAction: CloseFile(false) done" << G4endl;
    }

}

//  Cette méthode est appelée à la fin de chaque événement, pour :
//  - récupérer les compteurs locaux (contenus dans EventAction)
//  - les ajouter aux compteurs globaux (dans RunAction)
//  Cela permet d’avoir un bilan total du run en cumulant les données événement par événement.
void RunAction::UpdateFromEvent(const EventAction* event)
{
    //  Sécurité : si le pointeur vers EventAction est nul, on ne fait rien pour éviter un crash.
    if (!event) return;

    if (fRunVerbose== 1) {
        G4cout << " \n[DEBUG RunAction]  " <<  G4endl;
    G4cout << " \n[DEBUG RunAction] AVANT update : fTotalEntrantInBe = " << fTotalEntrantInBe.GetValue() << G4endl;
    G4cout << " \n[DEBUG RunAction] event->GetNbEntrantInBe() = " << event->GetNbEntrantInBe() << G4endl;}

    //  Mise à jour des accumulateurs
    //fTotalEntrantInBe = fTotalEntrantInBe.GetValue() + event->GetNbEntrantInBe();
    //fTotalInteractedInBe = fTotalInteractedInBe.GetValue() + event->GetNbInteractedInBe();
    //fTotalEntrantInWaterSphere = fTotalEntrantInWaterSphere.GetValue() + event->GetNbEntrantInWaterSphere();
    //fTotalInteractedInWaterSphere = fTotalInteractedInWaterSphere.GetValue() + event->GetNbInteractedInWaterSphere();
    //G4cout << "[RUNACTION] APRÈS update : fTotalEntrantInBe = " << fTotalEntrantInBe.GetValue() << G4endl;

    fTotalEntrantInBe += event->GetNbEntrantInBe();
    fTotalInteractedInBe += event->GetNbInteractedInBe();
    fTotalEntrantInWaterSphere += event->GetNbEntrantInWaterSphere();
    fTotalInteractedInWaterSphere += event->GetNbInteractedInWaterSphere();

}

//  La fonction suivante est une méthode simple mais importante dans ta classe RunAction :
//  Cette méthode permet d’associer tous les SphereHit d’un événement donné à son eventID,
//  en les enregistrant dans une structure de données interne à RunAction. Cela permet :
//    - De stocker les hits pour chaque événement
//    - De les analyser ou résumer à la fin du run (EndOfRunAction)
//    - D’accéder facilement à tous les hits d’un événement spécifique
//
//  eventID : identifiant de l’événement courant, fourni par Geant4 (ex: 0, 1, 2…)
//  hits : vecteur de SphereHit, contenant tous les hits détectés dans la sphère d’eau durant cet événement
void RunAction::AddHitsForEvent(G4int eventID, const std::vector<SphereHit>& hits)
{
    //  Stockage dans une map
    //  fHitsByEvent est un std::map<G4int, std::vector<SphereHit>> (déclarée dans RunAction.hh)
    //  Cette ligne associe le vecteur hits à l’eventID dans la map
    //  Si une entrée existait déjà pour cet eventID, elle est remplacée.
    fHitsByEvent[eventID] = hits;
}
//  À la fin du run (EndOfRunAction), on peut écrire :
//  for (const auto& [eventID, hits] : fHitsByEvent)
//    Et faire un bilan par événement, par exemple :
//    nombre de hits
//    énergie déposée
//    statistiques par position ou particule

// [ADD] Option B : méthode const pour compter les primaires (appelée depuis PrimaryGenerator)
//      fPrimariesGenerated est 'mutable' dans RunAction.hh pour autoriser l'incrément en méthode const.
void RunAction::CountPrimary() const
{
    auto* self = const_cast<RunAction*>(this);
  ++(self->fPrimariesGenerated);
}

void RunAction::IncrementValid1Particles() const {
    auto* self = const_cast<RunAction*>(this);
    ++(self->fNValidParticles_lt_35);
}

void RunAction::IncrementValid2Particles() const {
    auto* self = const_cast<RunAction*>(this);
    ++(self->fNValidParticles_gt_35);
}

// ==================== Gestion de l'énergie déposée et dose ====================

void RunAction::AddEdepFromEvent(const G4double* edepRing, G4double edepTotal)
{
    // Accumuler l'énergie pour le run complet
    for (G4int i = 0; i < kNbWaterRings; i++) {
        fTotalEdepRing[i] += edepRing[i];
        fEdepRing10000[i] += edepRing[i];
    }
    fTotalEdepWater += edepTotal;
    fEdepWater10000 += edepTotal;
}

G4double RunAction::GetTotalEdepRing(G4int ringIndex) const
{
    if (ringIndex >= 0 && ringIndex < kNbWaterRings) {
        return fTotalEdepRing[ringIndex];
    }
    return 0.0;
}

G4double RunAction::GetTotalEdepWater() const
{
    return fTotalEdepWater;
}

void RunAction::CheckAndFillDoseHistograms(G4int eventID)
{
    // Remplir les histogrammes de dose tous les 10000 événements
    if (eventID > 0 && eventID % 10000 == 0 && eventID != fLastHistoFillEvent) {
        fLastHistoFillEvent = eventID;
        
        auto analysisManager = G4AnalysisManager::Instance();
        
        // Conversion : Edep (keV) -> Dose (pGy)
        // 1 keV = 1.602e-16 J
        // 1 Gy = 1 J/kg = 1e12 pGy
        // Donc: Dose [pGy] = Edep [keV] * 1.602e-16 / masse [kg] * 1e12
        //                  = Edep [keV] * 1.602e-4 / masse [kg]
        //                  = Edep [keV] * 1.602e-4 / (masse [g] * 1e-3)
        //                  = Edep [keV] * 1.602e-1 / masse [g]
        //                  = Edep [keV] * 0.1602 / masse [g]
        constexpr G4double keV_to_pGy_per_gram = 0.1602;
        
        // H4: Dose totale dans l'eau (par 10000 événements)
        G4double dose_total = fEdepWater10000 * keV_to_pGy_per_gram / kMassTotalWater;
        analysisManager->FillH1(4, dose_total);
        
        // H10-H14: Dose par anneau (par 10000 événements)
        G4double dose_ring[kNbWaterRings];
        for (G4int i = 0; i < kNbWaterRings; i++) {
            dose_ring[i] = fEdepRing10000[i] * keV_to_pGy_per_gram / kMassRing[i];
            analysisManager->FillH1(10 + i, dose_ring[i]);
        }
        
        // ===== AFFICHAGE PROGRESS TOUS LES 10000 ÉVÉNEMENTS =====
        G4cout << "[PROGRESS] Event " << eventID 
               << " | Transmitted: " << fTransmitted10000
               << " | Edep(keV): Tot=" << fEdepWater10000
               << " R0=" << fEdepRing10000[0]
               << " R1=" << fEdepRing10000[1]
               << " R2=" << fEdepRing10000[2]
               << " R3=" << fEdepRing10000[3]
               << " R4=" << fEdepRing10000[4]
               << " | Dose(pGy): Tot=" << dose_total
               << " R0=" << dose_ring[0]
               << " R1=" << dose_ring[1]
               << " R2=" << dose_ring[2]
               << " R3=" << dose_ring[3]
               << " R4=" << dose_ring[4]
               << G4endl;
        
        // Réinitialiser les accumulateurs pour les prochains 10000 événements
        for (G4int i = 0; i < kNbWaterRings; i++) {
            fEdepRing10000[i] = 0.0;
        }
        fEdepWater10000 = 0.0;
        fTransmitted10000 = 0;
    }
}


















