#ifndef EVENTACTION_HH
#define EVENTACTION_HH

#include "G4UserEventAction.hh"
#include "G4THitsCollection.hh"

#include "MyTrackInfo.hh"
#include "g4root_defs.hh"

class G4Event;
class RunAction;

class EventAction : public G4UserEventAction
{
public:
    EventAction();
    ~EventAction();

    virtual void BeginOfEventAction(const G4Event*);
    virtual void EndOfEventAction(const G4Event*);

    void SetTrackInfo(MyTrackInfo* info);

    // Béryllium
    void IncrementNbInteractedInBe() { fNbInteractedInBe++; }
    G4int GetNbInteractedInBe() const { return fNbInteractedInBe; }

    void IncrementNbEntrantInBe() { fNbEntrantInBe++; }
    G4int GetNbEntrantInBe() const { return fNbEntrantInBe; }

    // WaterSphere
    void IncrementNbEntrantInWaterSphere() { fNbEntrantInWaterSphere++; }
    G4int GetNbEntrantInWaterSphere() const { return fNbEntrantInWaterSphere; }

    void IncrementNbInteractedInWaterSphere() { fNbInteractedInWaterSphere++; }
    G4int GetNbInteractedInWaterSphere() const { return fNbInteractedInWaterSphere; }

    // Utilisé dans RunAction (optionnel en mode single-thread)
    //void RegisterToRunAction(RunAction* runAction);

    void SetRunAction(RunAction* runAction) { fRunAction = runAction; }

    void SetVerbose(G4int level) { fEventVerboseLevel = level; }

    // ==================== Énergie déposée dans les anneaux d'eau ====================
    static const G4int kNbWaterRings = 5;
    
    // Ajouter de l'énergie déposée dans un anneau spécifique
    void AddEdepToRing(G4int ringIndex, G4double edep) {
        if (ringIndex >= 0 && ringIndex < kNbWaterRings) {
            fEdepRing[ringIndex] += edep;
            fEdepTotalWater += edep;
        }
    }
    
    // Accesseurs pour l'énergie déposée
    G4double GetEdepRing(G4int ringIndex) const {
        if (ringIndex >= 0 && ringIndex < kNbWaterRings) return fEdepRing[ringIndex];
        return 0.0;
    }
    G4double GetEdepTotalWater() const { return fEdepTotalWater; }


private:

    G4bool enteredCube = false;
    G4bool enteredSphere = false;
    G4String creatorProcess = "unknown";

    // Compteurs pour Beryllium
    G4int fNbInteractedInBe = 0;
    G4int fNbEntrantInBe = 0;

    // Compteurs pour sphère d'eau
    G4int fNbEntrantInWaterSphere = 0;
    G4int fNbInteractedInWaterSphere = 0;

    RunAction* fRunAction = nullptr;

    G4int fEventVerboseLevel = 0;

    G4int    nH   = 0;
    G4double to   = 0.0;
    G4double me   = 0.0;
    G4double ma   = 0.0;
    G4String mo   = "none";

    // Énergie déposée dans les anneaux d'eau (par événement)
    G4double fEdepRing[kNbWaterRings] = {0.0, 0.0, 0.0, 0.0, 0.0};
    G4double fEdepTotalWater = 0.0;

};
#endif
