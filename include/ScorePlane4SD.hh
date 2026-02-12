#ifndef ScorePlane4SD_hh
#define ScorePlane4SD_hh 1

#include "G4VSensitiveDetector.hh"
#include "globals.hh"
#include <set>

class G4Step;
class G4HCofThisEvent;
class G4TouchableHistory;

/**
 * @brief Sensitive detector pour le plan de comptage ScorePlane4 (z = +68 mm)
 *        Enregistre les particules traversant le plan dans le sens +Z
 */
class ScorePlane4SD : public G4VSensitiveDetector
{
public:
    ScorePlane4SD(const G4String& name);
    ~ScorePlane4SD() override = default;

    void Initialize(G4HCofThisEvent* hce) override;
    G4bool ProcessHits(G4Step* step, G4TouchableHistory*) override;
    void EndOfEvent(G4HCofThisEvent* hce) override;

    // Configuration
    void SetNtupleId(G4int id) { fNtupleId = id; }
    G4int GetNtupleId() const { return fNtupleId; }

    // Statistiques
    void PrintSummary() const;

private:
    G4int fNtupleId = -1;

    G4long fCntTotal = 0;
    G4long fCntAccepted = 0;
    G4long fCntRejected = 0;
    
    std::set<G4int> fTracksThisEvent;
};

#endif
