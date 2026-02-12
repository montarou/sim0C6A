#ifndef ScorePlane2SD_hh
#define ScorePlane2SD_hh 1

#include "G4VSensitiveDetector.hh"
#include "globals.hh"
#include <set>

class G4Step;
class G4HCofThisEvent;
class G4TouchableHistory;

/**
 * @brief Sensitive detector pour le plan de comptage ScorePlane2
 *        Enregistre les particules traversant le plan dans le sens +Z
 *
 * Ntuple "ScorePlane2_passages" :
 *   - pdg            : code PDG de la particule
 *   - name           : nom de la particule
 *   - is_secondary   : 0 = primaire, 1 = secondaire (issu d'une réaction)
 *   - x_mm           : position X du premier step dans le volume (mm)
 *   - y_mm           : position Y du premier step dans le volume (mm)
 *   - ekin_keV       : énergie cinétique à l'entrée dans le volume (keV)
 *   - trackID        : identifiant unique de la trace dans l'événement
 *   - parentID       : TrackID de la particule parente (0 si primaire)
 *   - creator_process: nom du processus créateur ("primary" si primaire)
 */
class ScorePlane2SD : public G4VSensitiveDetector
{
public:
    ScorePlane2SD(const G4String& name);
    ~ScorePlane2SD() override = default;

    void Initialize(G4HCofThisEvent* hce) override;
    G4bool ProcessHits(G4Step* step, G4TouchableHistory*) override;
    void EndOfEvent(G4HCofThisEvent* hce) override;

    // Configuration
    void SetNtupleId(G4int id) { fNtupleId = id; }
    G4int GetNtupleId() const { return fNtupleId; }

    // Statistiques
    void PrintSummary() const;

private:
    G4int fNtupleId = -1;  // ID du ntuple dans G4AnalysisManager

    // Compteurs pour debug/statistiques
    G4long fCntTotal = 0;      // total de passages détectés
    G4long fCntAccepted = 0;   // passages acceptés (direction +Z)
    G4long fCntRejected = 0;   // passages rejetés (direction -Z ou latérale)
    
    // Pour éviter de compter plusieurs fois la même particule dans le même événement
    std::set<G4int> fTracksThisEvent;  // TrackID des particules déjà comptées
};

#endif // ScorePlane2SD_hh
