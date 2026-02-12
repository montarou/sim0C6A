#include "ScorePlane4SD.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4StepPoint.hh"
#include "G4VPhysicalVolume.hh"
#include "G4ParticleDefinition.hh"
#include "G4VProcess.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"
#include "G4Event.hh"
#include "G4Threading.hh"

namespace {
    inline const char* ThreadTag() {
#ifdef G4MULTITHREADED
        return G4Threading::IsMasterThread() ? "[MT-MASTER]" : "[MT-WORKER]";
#else
        return "[SEQ]";
#endif
    }
    
    // ===========================================================================
    // CORRECTION : Fonction pour vérifier si le nom correspond à un WaterRing
    // Les volumes sont nommés "physWaterRing0", "physWaterRing1", etc.
    // ===========================================================================
    inline bool IsWaterRingVolume(const G4String& volName) {
        return (volName.find("physWaterRing") != std::string::npos);
    }
}

ScorePlane4SD::ScorePlane4SD(const G4String& name)
    : G4VSensitiveDetector(name)
{
    G4cout << ThreadTag() << " [ScorePlane4SD] Constructeur: " << name << G4endl;
}

void ScorePlane4SD::Initialize(G4HCofThisEvent*)
{
    fTracksThisEvent.clear();
    
    auto ev = G4RunManager::GetRunManager()->GetCurrentEvent();
    G4int eid = ev ? ev->GetEventID() : -1;
    
    static int dbg = 0;
    if (dbg < 5) {
        G4cout << ThreadTag() << " [ScorePlane4SD] Initialize event " << eid << G4endl;
        ++dbg;
    }
}

G4bool ScorePlane4SD::ProcessHits(G4Step* step, G4TouchableHistory*)
{
    if (!step) return false;

    const G4Track* track = step->GetTrack();
    if (!track) return false;

    const G4StepPoint* preStep = step->GetPreStepPoint();
    const G4StepPoint* postStep = step->GetPostStepPoint();
    if (!preStep || !postStep) return false;

    ++fCntTotal;

    const G4VPhysicalVolume* prePV = preStep->GetPhysicalVolume();
    const G4VPhysicalVolume* postPV = postStep->GetPhysicalVolume();

    // ===========================================================================
    // CORRECTION : Utiliser IsWaterRingVolume() au lieu de comparer à "physScorePlane4"
    // ===========================================================================
    const G4String preName = prePV ? prePV->GetName() : "";
    const G4String postName = postPV ? postPV->GetName() : "";
    
    // Détection de l'entrée dans un volume WaterRing
    const bool enteringVolume = 
        (!IsWaterRingVolume(preName)) && IsWaterRingVolume(postName);

    const bool firstStepInVolume = 
        IsWaterRingVolume(preName) && (preStep->GetStepStatus() == fGeomBoundary);

    if (!enteringVolume && !firstStepInVolume) {
        return false;
    }

    // Filtrer : accepter uniquement les particules allant vers +z
    const G4ThreeVector& dir = preStep->GetMomentumDirection();
    if (dir.z() <= 0.) {
        ++fCntRejected;
        return false;
    }

    // Éviter les doublons pour le même track dans le même événement
    G4int trackID = track->GetTrackID();
    if (fTracksThisEvent.find(trackID) != fTracksThisEvent.end()) {
        return false;
    }
    fTracksThisEvent.insert(trackID);

    ++fCntAccepted;

    // Récupération des informations de la particule
    const G4ParticleDefinition* def = track->GetDefinition();
    const G4int pdg = def ? def->GetPDGEncoding() : 0;
    const G4String name = def ? def->GetParticleName() : "unknown";
    
    const G4int parentID = track->GetParentID();
    const G4int is_secondary = (parentID == 0) ? 0 : 1;
    const G4int trackIDval = track->GetTrackID();
    
    const G4VProcess* creatorProcess = track->GetCreatorProcess();
    G4String creator_process = "primary";
    if (creatorProcess) {
        creator_process = creatorProcess->GetProcessName();
    }

    // Position
    G4ThreeVector pos;
    if (enteringVolume) {
        pos = postStep->GetPosition();
    } else {
        pos = preStep->GetPosition();
    }
    const G4double x_mm = pos.x() / mm;
    const G4double y_mm = pos.y() / mm;

    const G4double ekin_keV = preStep->GetKineticEnergy() / keV;

    // Écriture dans le ntuple
    if (fNtupleId >= 0) {
        auto* man = G4AnalysisManager::Instance();
        if (man && man->IsActive()) {
            man->FillNtupleIColumn(fNtupleId, 0, pdg);
            man->FillNtupleSColumn(fNtupleId, 1, name);
            man->FillNtupleIColumn(fNtupleId, 2, is_secondary);
            man->FillNtupleDColumn(fNtupleId, 3, x_mm);
            man->FillNtupleDColumn(fNtupleId, 4, y_mm);
            man->FillNtupleDColumn(fNtupleId, 5, ekin_keV);
            man->FillNtupleIColumn(fNtupleId, 6, trackIDval);
            man->FillNtupleIColumn(fNtupleId, 7, parentID);
            man->FillNtupleSColumn(fNtupleId, 8, creator_process);
            man->AddNtupleRow(fNtupleId);

            static int dbg_write = 0;
            if (dbg_write < 20) {
                G4cout << "[ScorePlane4SD] WROTE row: pdg=" << pdg 
                       << " name=" << name
                       << " is_secondary=" << is_secondary
                       << " x=" << x_mm << " mm"
                       << " y=" << y_mm << " mm"
                       << " Ekin=" << ekin_keV << " keV"
                       << " volume=" << (enteringVolume ? postName : preName)
                       << G4endl;
                ++dbg_write;
            }
        }
    }

    return true;
}

void ScorePlane4SD::EndOfEvent(G4HCofThisEvent*)
{
    auto ev = G4RunManager::GetRunManager()->GetCurrentEvent();
    G4int eid = ev ? ev->GetEventID() : -1;
    
    static int dbg = 0;
    if (dbg < 5 || fTracksThisEvent.size() > 0) {
        if (dbg < 20) {
            G4cout << ThreadTag() << " [ScorePlane4SD] EndOfEvent " << eid 
                   << ": " << fTracksThisEvent.size() << " particules enregistrées"
                   << G4endl;
            ++dbg;
        }
    }
}

void ScorePlane4SD::PrintSummary() const
{
    G4cout << "[ScorePlane4SD][SUMMARY]"
           << " total=" << fCntTotal
           << " accepted=" << fCntAccepted
           << " rejected=" << fCntRejected
           << G4endl;
}
