#include "ScorePlane2SD.hh"

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
}

ScorePlane2SD::ScorePlane2SD(const G4String& name)
    : G4VSensitiveDetector(name)
{
    G4cout << ThreadTag() << " [ScorePlane2SD] Constructeur: " << name << G4endl;
}

void ScorePlane2SD::Initialize(G4HCofThisEvent*)
{
    // Reset du set de tracks pour ce nouvel événement
    fTracksThisEvent.clear();
    
    auto ev = G4RunManager::GetRunManager()->GetCurrentEvent();
    G4int eid = ev ? ev->GetEventID() : -1;
    
    static int dbg = 0;
    if (dbg < 5) {
        G4cout << ThreadTag() << " [ScorePlane2SD] Initialize event " << eid << G4endl;
        ++dbg;
    }
}

G4bool ScorePlane2SD::ProcessHits(G4Step* step, G4TouchableHistory*)
{
    if (!step) return false;

    const G4Track* track = step->GetTrack();
    if (!track) return false;

    const G4StepPoint* preStep = step->GetPreStepPoint();
    const G4StepPoint* postStep = step->GetPostStepPoint();
    if (!preStep || !postStep) return false;

    // Compteur total
    ++fCntTotal;

    // Vérifier qu'on ENTRE dans le volume (preStep hors du volume, postStep dans le volume)
    // ou qu'on est au premier step dans le volume
    const G4VPhysicalVolume* prePV = preStep->GetPhysicalVolume();
    const G4VPhysicalVolume* postPV = postStep->GetPhysicalVolume();

    const G4String targetName = "physScorePlane2";
    
    // On veut détecter l'ENTRÉE dans le volume
    const bool enteringVolume = 
        ((!prePV) || (prePV->GetName() != targetName)) &&
        (postPV && postPV->GetName() == targetName);

    // Alternative : on est déjà dans le volume au preStep (premier step)
    const bool firstStepInVolume = 
        (prePV && prePV->GetName() == targetName) &&
        (preStep->GetStepStatus() == fGeomBoundary);

    if (!enteringVolume && !firstStepInVolume) {
        return false;  // Pas une entrée dans le volume
    }

    // Vérifier la direction : on ne garde que les particules allant vers +Z
    const G4ThreeVector& dir = preStep->GetMomentumDirection();
    if (dir.z() <= 0.) {
        ++fCntRejected;
        static int dbg_reject = 0;
        if (dbg_reject < 10) {
            G4cout << "[ScorePlane2SD] REJECT (dir.z <= 0): dir.z=" << dir.z() << G4endl;
            ++dbg_reject;
        }
        return false;
    }

    // Vérifier qu'on n'a pas déjà compté cette particule dans cet événement
    G4int trackID = track->GetTrackID();
    if (fTracksThisEvent.find(trackID) != fTracksThisEvent.end()) {
        // Déjà comptée
        return false;
    }
    fTracksThisEvent.insert(trackID);

    ++fCntAccepted;

    // Récupérer les informations à enregistrer
    const G4ParticleDefinition* def = track->GetDefinition();
    const G4int pdg = def ? def->GetPDGEncoding() : 0;
    const G4String name = def ? def->GetParticleName() : "unknown";
    
    // is_secondary : 0 = primaire, 1 = secondaire
    // ParentID == 0 signifie que c'est une particule primaire
    const G4int parentID = track->GetParentID();
    const G4int is_secondary = (parentID == 0) ? 0 : 1;
    
    // TrackID
    const G4int trackIDval = track->GetTrackID();
    
    // Processus créateur (pour les secondaires)
    const G4VProcess* creatorProcess = track->GetCreatorProcess();
    G4String creator_process = "primary";
    if (creatorProcess) {
        creator_process = creatorProcess->GetProcessName();
    }

    // Position à l'entrée (preStep ou postStep selon le cas)
    G4ThreeVector pos;
    if (enteringVolume) {
        pos = postStep->GetPosition();  // Position juste après l'entrée
    } else {
        pos = preStep->GetPosition();   // Premier step dans le volume
    }
    const G4double x_mm = pos.x() / mm;
    const G4double y_mm = pos.y() / mm;

    // Énergie cinétique à l'entrée
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

            // Debug log (limité)
            static int dbg_write = 0;
            if (dbg_write < 20) {
                G4cout << "[ScorePlane2SD] WROTE row: pdg=" << pdg 
                       << " name=" << name
                       << " is_secondary=" << is_secondary
                       << " x=" << x_mm << " mm"
                       << " y=" << y_mm << " mm"
                       << " Ekin=" << ekin_keV << " keV"
                       << " trackID=" << trackIDval
                       << " parentID=" << parentID
                       << " creator=" << creator_process
                       << G4endl;
                ++dbg_write;
            }
        }
    }

    return true;
}

void ScorePlane2SD::EndOfEvent(G4HCofThisEvent*)
{
    auto ev = G4RunManager::GetRunManager()->GetCurrentEvent();
    G4int eid = ev ? ev->GetEventID() : -1;
    
    static int dbg = 0;
    if (dbg < 5 || fTracksThisEvent.size() > 0) {
        if (dbg < 20) {
            G4cout << ThreadTag() << " [ScorePlane2SD] EndOfEvent " << eid 
                   << ": " << fTracksThisEvent.size() << " particules enregistrées"
                   << G4endl;
            ++dbg;
        }
    }
}

void ScorePlane2SD::PrintSummary() const
{
    G4cout << "[ScorePlane2SD][SUMMARY]"
           << " total=" << fCntTotal
           << " accepted=" << fCntAccepted
           << " rejected=" << fCntRejected
           << G4endl;
}
