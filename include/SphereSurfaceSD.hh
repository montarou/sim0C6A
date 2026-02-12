#ifndef SphereSurfaceSD_h
#define SphereSurfaceSD_h 1

#include "G4VSensitiveDetector.hh"
#include "G4Step.hh"

#include "G4TouchableHistory.hh"
#include "G4THitsCollection.hh"
#include "G4ThreeVector.hh"
#include <vector>
#include "globals.hh"

#include "SphereSurfaceSDMessenger.hh"


class G4Step;
class G4HCofThisEvent;
class SphereHit;
class SphereSurfaceSDMessenger;

using SphereHitsCollection = G4THitsCollection<SphereHit>;

class SphereSurfaceSD : public G4VSensitiveDetector {
public:
    SphereSurfaceSD(const G4String& name, const G4String& hitsCollectionName);
    virtual ~SphereSurfaceSD()  override;

    virtual void Initialize(G4HCofThisEvent* hitCollection) override;
    virtual G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
    virtual void EndOfEvent(G4HCofThisEvent* hitCollection) override;

    void SetVerbose(G4int level) {fSDVerboseLevel = level;}

private:
    SphereHitsCollection* fHitsCollection = nullptr;
    SphereSurfaceSDMessenger* fSDMessenger = nullptr;
    G4int fSDVerboseLevel = 0;  // 0 = silencieux, 1 = résumé, 2 = debug détaillé
    G4int fHCID;
};

#endif

