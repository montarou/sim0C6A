#include "AnalysisManagerSetup.hh"

#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"

#include "G4SDManager.hh"
#include "SurfaceSpectrumSD.hh"
#include "ScorePlane2SD.hh"
#include "ScorePlane3SD.hh"
#include "ScorePlane4SD.hh"
#include "ScorePlane5SD.hh"
// ScorePlane6SD supprimé
#include "G4Run.hh"

// Variables globales pour stocker les IDs des ntuples
static int g_planePassageNtupleId = -1;
static int g_scorePlane2NtupleId = -1;
static int g_scorePlane3NtupleId = -1;
static int g_scorePlane4NtupleId = -1;
static int g_scorePlane5NtupleId = -1;
// g_scorePlane6NtupleId supprimé
static int g_absGraphiteNtupleId = -1;
static int g_absInoxNtupleId     = -1;
static int g_comptonConeNtupleId = -1;  // NOUVEAU : Compton individuel dans le cône

void SetupAnalysis()
{

    auto man = G4AnalysisManager::Instance();
    auto analysisManager = man;
    man->SetVerboseLevel(1);
    man->SetFirstNtupleId(0);
    man->SetFirstHistoId(0);

    // ==================== Histogrammes 1D ====================
    
    // H0: Énergie des gammas primaires à l'émission
    analysisManager->CreateH1("E_emission", 
        "Énergie des gammas primaires à l'émission", 150, 0., 50.*keV);  // ID 0
    
    // H1: Angle theta des gammas primaires
    analysisManager->CreateH1("theta_emission", 
        "Angle theta des gammas primaires", 180, 0., 180.);  // ID 1 (degrés)
    
    // H2: Angle phi des gammas primaires
    analysisManager->CreateH1("phi_emission", 
        "Angle phi des gammas primaires", 90, -180., 180.);  // ID 2 (degrés)
    
    // ============================================================================
    // HISTOGRAMMES DE DOSE
    // ============================================================================
    // ATTENTION: Les doses sont calculées en pGy dans RunAction::CheckAndFillDoseHistograms()
    // et RunAction::EndOfRunAction() avec la conversion keV_to_pGy_per_gram = 0.1602
    // 
    // Valeurs typiques par 10000 événements:
    //   - Dose totale: ~100-150 pGy
    //   - Dose par anneau: ~150-400 pGy (anneaux 0-3), ~10-50 pGy (anneau 4)
    //
    // Valeurs typiques pour le run complet (10M événements):
    //   - Dose totale: ~100-150 nGy = 100000-150000 pGy
    //   - Dose par anneau: ~100-200 nGy = 100000-200000 pGy
    //
    // CORRECTION [30/01/2026]: Range étendu pour 10M événements
    // ============================================================================
    
    // H3: Dose totale dans tout l'eau (run complet) - en pGy
    // Range étendu: 0-5000000 pGy (= 0-5000 nGy) pour runs jusqu'à 500M événements
    // CORRECTION [31/01/2026]: Plage x10 pour supporter 100M+ événements
    analysisManager->CreateH1("Dose_total_run", 
        "Dose totale dans l'eau (run);Dose (pGy);Counts", 1000, 0., 5000000.);  // ID 3 (pGy)
    
    // H4: Dose totale dans tout l'eau (par 10000 événements) - en pGy
    analysisManager->CreateH1("Dose_total_10000evt", 
        "Dose totale dans l'eau (10000 evt);Dose (pGy);Counts", 200, 0., 300.);  // ID 4 (pGy)
    
    // H5-H9: Dose par anneau (run complet) - en pGy
    // Range étendu: 0-5000000 pGy pour runs jusqu'à 500M événements
    // CORRECTION [31/01/2026]: Plage x10 pour supporter 100M+ événements
    analysisManager->CreateH1("Dose_ring0_run", 
        "Dose anneau 0 (r=0-2mm) run;Dose (pGy);Counts", 1000, 0., 5000000.);  // ID 5 (pGy)
    analysisManager->CreateH1("Dose_ring1_run", 
        "Dose anneau 1 (r=2-4mm) run;Dose (pGy);Counts", 1000, 0., 5000000.);  // ID 6 (pGy)
    analysisManager->CreateH1("Dose_ring2_run", 
        "Dose anneau 2 (r=4-6mm) run;Dose (pGy);Counts", 1000, 0., 5000000.);  // ID 7 (pGy)
    analysisManager->CreateH1("Dose_ring3_run", 
        "Dose anneau 3 (r=6-8mm) run;Dose (pGy);Counts", 1000, 0., 5000000.);  // ID 8 (pGy)
    analysisManager->CreateH1("Dose_ring4_run", 
        "Dose anneau 4 (r=8-10mm) run;Dose (pGy);Counts", 1000, 0., 1000000.);  // ID 9 (pGy)
    
    // H10-H14: Dose par anneau (par 10000 événements) - en pGy
    // Plages adaptées aux valeurs typiques observées:
    //   - Anneaux 0-3: ~100-500 pGy
    //   - Anneau 4: ~0-50 pGy (peu d'énergie déposée en périphérie)
    analysisManager->CreateH1("Dose_ring0_10000evt", 
        "Dose anneau 0 (r=0-2mm) 10000evt;Dose (pGy);Counts", 200, 0., 500.);  // ID 10 (pGy)
    analysisManager->CreateH1("Dose_ring1_10000evt", 
        "Dose anneau 1 (r=2-4mm) 10000evt;Dose (pGy);Counts", 200, 0., 500.);  // ID 11 (pGy)
    analysisManager->CreateH1("Dose_ring2_10000evt", 
        "Dose anneau 2 (r=4-6mm) 10000evt;Dose (pGy);Counts", 200, 0., 500.);  // ID 12 (pGy)
    analysisManager->CreateH1("Dose_ring3_10000evt", 
        "Dose anneau 3 (r=6-8mm) 10000evt;Dose (pGy);Counts", 200, 0., 500.);  // ID 13 (pGy)
    analysisManager->CreateH1("Dose_ring4_10000evt", 
        "Dose anneau 4 (r=8-10mm) 10000evt;Dose (pGy);Counts", 200, 0., 500.);  // ID 14 (pGy)

    // ==================== Ntuple plane_passages ====================
    // Ntuple des passages plan +Z (ScorePlane à z = 18 mm)
    // Structure harmonisée avec les autres ntuples (ScorePlane2, ScorePlane3, etc.)
    // Colonnes : pdg, name, is_secondary, x_mm, y_mm, z_mm, ekin_keV, trackID, parentID, creator_process
    g_planePassageNtupleId = analysisManager->CreateNtuple("plane_passages", "Traversées +Z du plan mince");
    analysisManager->CreateNtupleIColumn(g_planePassageNtupleId, "pdg");             // 0: Code PDG
    analysisManager->CreateNtupleSColumn(g_planePassageNtupleId, "name");            // 1: Nom particule
    analysisManager->CreateNtupleIColumn(g_planePassageNtupleId, "is_secondary");    // 2: 0=primaire, 1=secondaire
    analysisManager->CreateNtupleDColumn(g_planePassageNtupleId, "x_mm");            // 3: Position X (mm)
    analysisManager->CreateNtupleDColumn(g_planePassageNtupleId, "y_mm");            // 4: Position Y (mm)
    analysisManager->CreateNtupleDColumn(g_planePassageNtupleId, "z_mm");            // 5: Position Z (mm)
    analysisManager->CreateNtupleDColumn(g_planePassageNtupleId, "ekin_keV");        // 6: Énergie cinétique (keV)
    analysisManager->CreateNtupleIColumn(g_planePassageNtupleId, "trackID");         // 7: TrackID
    analysisManager->CreateNtupleIColumn(g_planePassageNtupleId, "parentID");        // 8: ParentID
    analysisManager->CreateNtupleSColumn(g_planePassageNtupleId, "creator_process"); // 9: Processus créateur
    // [ADD] Colonnes Compton dans le cône graphite
    analysisManager->CreateNtupleIColumn(g_planePassageNtupleId, "compton_in_cone"); // 10: 1 si Compton dans cône, 0 sinon
    analysisManager->CreateNtupleIColumn(g_planePassageNtupleId, "n_compton_cone");  // 11: Nombre de Compton dans le cône
    analysisManager->CreateNtupleDColumn(g_planePassageNtupleId, "compton_x_mm");    // 12: X dernière diffusion Compton (mm)
    analysisManager->CreateNtupleDColumn(g_planePassageNtupleId, "compton_y_mm");    // 13: Y dernière diffusion Compton (mm)
    analysisManager->CreateNtupleDColumn(g_planePassageNtupleId, "compton_z_mm");    // 14: Z dernière diffusion Compton (mm)
    analysisManager->FinishNtuple(g_planePassageNtupleId);

    // ==================== Ntuple ScorePlane2 ====================
    // Ntuple pour le plan de comptage ScorePlane2 (z = 28 mm)
    // Colonnes : pdg, name, is_secondary, x_mm, y_mm, ekin_keV, trackID, parentID, creator_process
    g_scorePlane2NtupleId = analysisManager->CreateNtuple("ScorePlane2_passages", 
        "Traversées +Z du plan ScorePlane2");
    analysisManager->CreateNtupleIColumn(g_scorePlane2NtupleId, "pdg");           // 0: Code PDG
    analysisManager->CreateNtupleSColumn(g_scorePlane2NtupleId, "name");          // 1: Nom particule
    analysisManager->CreateNtupleIColumn(g_scorePlane2NtupleId, "is_secondary");  // 2: 0=primaire, 1=secondaire
    analysisManager->CreateNtupleDColumn(g_scorePlane2NtupleId, "x_mm");          // 3: Position X (mm)
    analysisManager->CreateNtupleDColumn(g_scorePlane2NtupleId, "y_mm");          // 4: Position Y (mm)
    analysisManager->CreateNtupleDColumn(g_scorePlane2NtupleId, "ekin_keV");      // 5: Énergie cinétique (keV)
    analysisManager->CreateNtupleIColumn(g_scorePlane2NtupleId, "trackID");       // 6: TrackID
    analysisManager->CreateNtupleIColumn(g_scorePlane2NtupleId, "parentID");      // 7: ParentID
    analysisManager->CreateNtupleSColumn(g_scorePlane2NtupleId, "creator_process"); // 8: Processus créateur
    analysisManager->FinishNtuple(g_scorePlane2NtupleId);

    // ==================== Ntuple ScorePlane3 ====================
    // Ntuple pour le plan de comptage ScorePlane3 (z = 38 mm)
    // Colonnes : pdg, name, is_secondary, x_mm, y_mm, ekin_keV, trackID, parentID, creator_process
    g_scorePlane3NtupleId = analysisManager->CreateNtuple("ScorePlane3_passages", 
        "Traversées +Z du plan ScorePlane3");
    analysisManager->CreateNtupleIColumn(g_scorePlane3NtupleId, "pdg");             // 0: Code PDG
    analysisManager->CreateNtupleSColumn(g_scorePlane3NtupleId, "name");            // 1: Nom particule
    analysisManager->CreateNtupleIColumn(g_scorePlane3NtupleId, "is_secondary");    // 2: 0=primaire, 1=secondaire
    analysisManager->CreateNtupleDColumn(g_scorePlane3NtupleId, "x_mm");            // 3: Position X (mm)
    analysisManager->CreateNtupleDColumn(g_scorePlane3NtupleId, "y_mm");            // 4: Position Y (mm)
    analysisManager->CreateNtupleDColumn(g_scorePlane3NtupleId, "ekin_keV");        // 5: Énergie cinétique (keV)
    analysisManager->CreateNtupleIColumn(g_scorePlane3NtupleId, "trackID");         // 6: TrackID
    analysisManager->CreateNtupleIColumn(g_scorePlane3NtupleId, "parentID");        // 7: ParentID
    analysisManager->CreateNtupleSColumn(g_scorePlane3NtupleId, "creator_process"); // 8: Processus créateur
    analysisManager->FinishNtuple(g_scorePlane3NtupleId);

    // ==================== Ntuple WaterRings ====================
    // Ntuple pour les couronnes d'eau (z = 68 mm, fond de l'eau)
    // Remplace l'ancien ScorePlane4
    g_scorePlane4NtupleId = analysisManager->CreateNtuple("WaterRings_passages", 
        "Traversées dans les couronnes d'eau");
    analysisManager->CreateNtupleIColumn(g_scorePlane4NtupleId, "pdg");             // 0: Code PDG
    analysisManager->CreateNtupleSColumn(g_scorePlane4NtupleId, "name");            // 1: Nom particule
    analysisManager->CreateNtupleIColumn(g_scorePlane4NtupleId, "is_secondary");    // 2: 0=primaire, 1=secondaire
    analysisManager->CreateNtupleDColumn(g_scorePlane4NtupleId, "x_mm");            // 3: Position X (mm)
    analysisManager->CreateNtupleDColumn(g_scorePlane4NtupleId, "y_mm");            // 4: Position Y (mm)
    analysisManager->CreateNtupleDColumn(g_scorePlane4NtupleId, "ekin_keV");        // 5: Énergie cinétique (keV)
    analysisManager->CreateNtupleIColumn(g_scorePlane4NtupleId, "trackID");         // 6: TrackID
    analysisManager->CreateNtupleIColumn(g_scorePlane4NtupleId, "parentID");        // 7: ParentID
    analysisManager->CreateNtupleSColumn(g_scorePlane4NtupleId, "creator_process"); // 8: Processus créateur
    analysisManager->FinishNtuple(g_scorePlane4NtupleId);

    // ==================== Ntuple ScorePlane5 ====================
    // Ntuple pour le plan de comptage ScorePlane5 (z = 70 mm)
    g_scorePlane5NtupleId = analysisManager->CreateNtuple("ScorePlane5_passages", 
        "Traversées +Z du plan ScorePlane5");
    analysisManager->CreateNtupleIColumn(g_scorePlane5NtupleId, "pdg");             // 0: Code PDG
    analysisManager->CreateNtupleSColumn(g_scorePlane5NtupleId, "name");            // 1: Nom particule
    analysisManager->CreateNtupleIColumn(g_scorePlane5NtupleId, "is_secondary");    // 2: 0=primaire, 1=secondaire
    analysisManager->CreateNtupleDColumn(g_scorePlane5NtupleId, "x_mm");            // 3: Position X (mm)
    analysisManager->CreateNtupleDColumn(g_scorePlane5NtupleId, "y_mm");            // 4: Position Y (mm)
    analysisManager->CreateNtupleDColumn(g_scorePlane5NtupleId, "ekin_keV");        // 5: Énergie cinétique (keV)
    analysisManager->CreateNtupleIColumn(g_scorePlane5NtupleId, "trackID");         // 6: TrackID
    analysisManager->CreateNtupleIColumn(g_scorePlane5NtupleId, "parentID");        // 7: ParentID
    analysisManager->CreateNtupleSColumn(g_scorePlane5NtupleId, "creator_process"); // 8: Processus créateur
    analysisManager->FinishNtuple(g_scorePlane5NtupleId);

    // ==================== Ntuple abs_graphite ====================
    // Absorptions photoélectriques de primaires dans le cône graphite
    // Rempli depuis SteppingAction quand process=="phot" dans logicConeCompton
    g_absGraphiteNtupleId = analysisManager->CreateNtuple("abs_graphite",
        "Absorptions photoelectriques dans le cone graphite");
    analysisManager->CreateNtupleIColumn(g_absGraphiteNtupleId, "eventID");              // 0
    analysisManager->CreateNtupleIColumn(g_absGraphiteNtupleId, "trackID");              // 1
    analysisManager->CreateNtupleDColumn(g_absGraphiteNtupleId, "ekin_keV");             // 2: E avant absorption
    analysisManager->CreateNtupleDColumn(g_absGraphiteNtupleId, "x_mm");                 // 3
    analysisManager->CreateNtupleDColumn(g_absGraphiteNtupleId, "y_mm");                 // 4
    analysisManager->CreateNtupleDColumn(g_absGraphiteNtupleId, "z_mm");                 // 5
    analysisManager->CreateNtupleIColumn(g_absGraphiteNtupleId, "had_compton_in_cone");  // 6: 1 si Compton avant abs
    analysisManager->CreateNtupleIColumn(g_absGraphiteNtupleId, "n_compton_in_cone");    // 7: nb Compton avant abs
    analysisManager->FinishNtuple(g_absGraphiteNtupleId);

    // ==================== Ntuple abs_inox ====================
    // Absorptions photoélectriques de primaires dans l'inox (SS304)
    // Rempli depuis SteppingAction quand process=="phot" dans les volumes SS304
    g_absInoxNtupleId = analysisManager->CreateNtuple("abs_inox",
        "Absorptions photoelectriques dans l inox SS304");
    analysisManager->CreateNtupleIColumn(g_absInoxNtupleId, "eventID");              // 0
    analysisManager->CreateNtupleIColumn(g_absInoxNtupleId, "trackID");              // 1
    analysisManager->CreateNtupleDColumn(g_absInoxNtupleId, "ekin_keV");             // 2: E avant absorption
    analysisManager->CreateNtupleDColumn(g_absInoxNtupleId, "x_mm");                 // 3
    analysisManager->CreateNtupleDColumn(g_absInoxNtupleId, "y_mm");                 // 4
    analysisManager->CreateNtupleDColumn(g_absInoxNtupleId, "z_mm");                 // 5
    analysisManager->CreateNtupleSColumn(g_absInoxNtupleId, "volume");               // 6: nom du volume logique
    analysisManager->CreateNtupleIColumn(g_absInoxNtupleId, "had_compton_in_cone");  // 7: 1 si Compton avant abs
    analysisManager->CreateNtupleIColumn(g_absInoxNtupleId, "n_compton_in_cone");    // 8: nb Compton avant abs
    analysisManager->FinishNtuple(g_absInoxNtupleId);

    // ==================== Ntuple compton_cone_events ====================
    // NOUVEAU : Enregistre CHAQUE diffusion Compton individuelle dans le cône graphite
    // Permet de tracer :
    //   - Carte (z, r) de toutes les interactions Compton
    //   - Histogramme 2D θ_incident vs θ_diffusé
    //   - Histogramme 2D angle de diffusion vs énergie incidente
    //   - Histogramme ΔE vs E_incident
    //   - Corrélation perte d'énergie / angle de diffusion (formule Compton)
    // =====================================================================
    g_comptonConeNtupleId = analysisManager->CreateNtuple("compton_cone_events",
        "Diffusions Compton individuelles dans le cone graphite");
    analysisManager->CreateNtupleIColumn(g_comptonConeNtupleId, "eventID");            // 0: ID événement
    analysisManager->CreateNtupleIColumn(g_comptonConeNtupleId, "trackID");            // 1: ID du track
    analysisManager->CreateNtupleIColumn(g_comptonConeNtupleId, "n_compton_seq");      // 2: N° séquentiel Compton pour ce track
    analysisManager->CreateNtupleDColumn(g_comptonConeNtupleId, "ekin_before_keV");    // 3: Énergie cinétique AVANT la diffusion (keV)
    analysisManager->CreateNtupleDColumn(g_comptonConeNtupleId, "ekin_after_keV");     // 4: Énergie cinétique APRÈS la diffusion (keV)
    analysisManager->CreateNtupleDColumn(g_comptonConeNtupleId, "delta_ekin_keV");     // 5: Perte d'énergie ΔE = E_before - E_after (keV)
    analysisManager->CreateNtupleDColumn(g_comptonConeNtupleId, "x_mm");               // 6: Position X de l'interaction (mm)
    analysisManager->CreateNtupleDColumn(g_comptonConeNtupleId, "y_mm");               // 7: Position Y de l'interaction (mm)
    analysisManager->CreateNtupleDColumn(g_comptonConeNtupleId, "z_mm");               // 8: Position Z de l'interaction (mm)
    analysisManager->CreateNtupleDColumn(g_comptonConeNtupleId, "r_mm");               // 9: Rayon = sqrt(x²+y²) (mm)
    analysisManager->CreateNtupleDColumn(g_comptonConeNtupleId, "theta_in_deg");       // 10: Angle polaire incident (par rapport à +Z) (deg)
    analysisManager->CreateNtupleDColumn(g_comptonConeNtupleId, "phi_in_deg");         // 11: Angle azimutal incident (deg)
    analysisManager->CreateNtupleDColumn(g_comptonConeNtupleId, "theta_out_deg");      // 12: Angle polaire sortant (deg)
    analysisManager->CreateNtupleDColumn(g_comptonConeNtupleId, "phi_out_deg");        // 13: Angle azimutal sortant (deg)
    analysisManager->CreateNtupleDColumn(g_comptonConeNtupleId, "scatter_angle_deg");  // 14: Angle de diffusion Compton (deg)
    analysisManager->CreateNtupleDColumn(g_comptonConeNtupleId, "cos_scatter");        // 15: cos(angle de diffusion) → vérif. formule Compton
    analysisManager->FinishNtuple(g_comptonConeNtupleId);

    // Ntuple ScorePlane6 supprimé

    //  Raccorder l'ID au SD spectral (maintenant défini)
    if (auto* sd = dynamic_cast<SurfaceSpectrumSD*>(
        G4SDManager::GetSDMpointer()->FindSensitiveDetector("SpecSD", /*warning=*/false))) {
        sd->Reset();
        sd->SetPassageNtupleId(g_planePassageNtupleId);
        // (L'aire [cm^2] est réglée côté DetectorConstruction via specSD->SetArea_cm2(...))
    }

    //  Raccorder l'ID au SD ScorePlane2 (si défini)
    if (auto* sd2 = dynamic_cast<ScorePlane2SD*>(
        G4SDManager::GetSDMpointer()->FindSensitiveDetector("ScorePlane2SD", /*warning=*/false))) {
        sd2->SetNtupleId(g_scorePlane2NtupleId);
        G4cout << "[SetupAnalysis] ScorePlane2SD connecté au ntuple id=" << g_scorePlane2NtupleId << G4endl;
    }

    //  Raccorder l'ID au SD ScorePlane3 (si défini)
    if (auto* sd3 = dynamic_cast<ScorePlane3SD*>(
        G4SDManager::GetSDMpointer()->FindSensitiveDetector("ScorePlane3SD", /*warning=*/false))) {
        sd3->SetNtupleId(g_scorePlane3NtupleId);
        G4cout << "[SetupAnalysis] ScorePlane3SD connecté au ntuple id=" << g_scorePlane3NtupleId << G4endl;
    }

    //  Raccorder l'ID au SD ScorePlane4 (WaterRings) (si défini)
    if (auto* sd4 = dynamic_cast<ScorePlane4SD*>(
        G4SDManager::GetSDMpointer()->FindSensitiveDetector("ScorePlane4SD", /*warning=*/false))) {
        sd4->SetNtupleId(g_scorePlane4NtupleId);
        G4cout << "[SetupAnalysis] ScorePlane4SD (WaterRings) connecté au ntuple id=" << g_scorePlane4NtupleId << G4endl;
    }

    //  Raccorder l'ID au SD ScorePlane5 (si défini)
    if (auto* sd5 = dynamic_cast<ScorePlane5SD*>(
        G4SDManager::GetSDMpointer()->FindSensitiveDetector("ScorePlane5SD", /*warning=*/false))) {
        sd5->SetNtupleId(g_scorePlane5NtupleId);
        G4cout << "[SetupAnalysis] ScorePlane5SD connecté au ntuple id=" << g_scorePlane5NtupleId << G4endl;
    }

    // ScorePlane6SD supprimé
}

void FinalizeAnalysis()
{
    auto man = G4AnalysisManager::Instance();
    #if 0
    // Déplacé dans RunAction::EndOfRunAction()
    man->Write();
    man->CloseFile(false); // false = pratique en mode interactif
    #endif
}

// Implémentation des fonctions getter pour les IDs des ntuples
int GetPlanePassageNtupleId()
{
    return g_planePassageNtupleId;
}

int GetScorePlane2NtupleId()
{
    return g_scorePlane2NtupleId;
}

int GetScorePlane3NtupleId()
{
    return g_scorePlane3NtupleId;
}

int GetScorePlane4NtupleId()
{
    return g_scorePlane4NtupleId;
}

int GetScorePlane5NtupleId()
{
    return g_scorePlane5NtupleId;
}

// GetScorePlane6NtupleId() supprimé

int GetAbsGraphiteNtupleId()
{
    return g_absGraphiteNtupleId;
}

int GetAbsInoxNtupleId()
{
    return g_absInoxNtupleId;
}

int GetComptonConeNtupleId()
{
    return g_comptonConeNtupleId;
}
