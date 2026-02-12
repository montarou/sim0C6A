#ifndef ANALYSIS_MANAGER_SETUP_HH
#define ANALYSIS_MANAGER_SETUP_HH

// Initialisation de l'analyse avec G4AnalysisManager
void SetupAnalysis();

// Finalisation de l'analyse
// Enregistre les données et ferme le fichier
void FinalizeAnalysis();

// Exposer l'ID du ntuple "plane_passages" créé dans SetupAnalysis
// Retourne -1 si non configuré.
int GetPlanePassageNtupleId();

// Exposer l'ID du ntuple "ScorePlane2_passages" créé dans SetupAnalysis
// Retourne -1 si non configuré.
int GetScorePlane2NtupleId();

// Exposer l'ID du ntuple "ScorePlane3_passages" créé dans SetupAnalysis
// Retourne -1 si non configuré.
int GetScorePlane3NtupleId();

// Exposer l'ID du ntuple "ScorePlane4_passages" créé dans SetupAnalysis
// Retourne -1 si non configuré.
int GetScorePlane4NtupleId();

// Exposer l'ID du ntuple "ScorePlane5_passages" créé dans SetupAnalysis
// Retourne -1 si non configuré.
int GetScorePlane5NtupleId();

// Exposer l'ID du ntuple "abs_graphite" (absorptions photoélectriques dans le cône graphite)
int GetAbsGraphiteNtupleId();

// Exposer l'ID du ntuple "abs_inox" (absorptions photoélectriques dans l'inox)
int GetAbsInoxNtupleId();

// =====================================================
// NOUVEAU : Ntuple dédié aux diffusions Compton dans le cône graphite
// Enregistre CHAQUE interaction Compton individuelle avec :
//   - Positions (x, y, z, r)
//   - Énergies avant/après
//   - Directions avant/après (θ, φ)
//   - Angle de diffusion
// =====================================================
int GetComptonConeNtupleId();

// GetScorePlane6NtupleId() supprimé

#endif
