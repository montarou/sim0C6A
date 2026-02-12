#include "SurfaceSpectrumSD.hh"
#include "RunAction.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"
#include "G4Threading.hh"
#include "G4Track.hh"
#include "G4ParticleDefinition.hh"
#include "G4VProcess.hh"
#include "G4LogicalVolume.hh"
#include "MyTrackInfo.hh"

// ============================================================================
// [ADD] Helper Master/Worker (ou SEQ) pour les logs
// ============================================================================
namespace {
  inline const char* ThreadTag(){
    #ifdef G4MULTITHREADED
    return G4Threading::IsMasterThread() ? "[MT-MASTER]" : "[MT-WORKER]";
    #else
    return "[SEQ]";
    #endif
  }}

SurfaceSpectrumSD::SurfaceSpectrumSD(const G4String& name,
                                     G4double eMin_keV, G4double eMax_keV, G4int nBins,
                                     G4bool outwardOnly)
: G4VSensitiveDetector(name)
,fEMin_keV(eMin_keV)
,fEMax_keV(eMax_keV)
,fNBins(nBins)
,fOutwardOnly(outwardOnly)
,fPassageNtupleId(-1)
,fCntEnter(0)
,fCntLeave(0)
,fCntOut(0)
,fCntRows(0)
{

  // [ADD] largeur de bin et histogramme
  fBinWidth_keV = (fNBins > 0) ? (fEMax_keV - fEMin_keV)/fNBins : 0.0;
  fBins.assign(std::max(0, fNBins), 0.0);

  if (fDbgMaxPrint <= 0) fDbgMaxPrint = 10;

  G4cout << ThreadTag() << " [SpecSD] ctor: E=[" << fEMin_keV << "," << fEMax_keV
  << "] keV, nBins=" << fNBins
  << " onlyOutward=" << (fOutwardOnly ? "true" : "false")
  << G4endl;
}

SurfaceSpectrumSD::~SurfaceSpectrumSD() = default;


// ============================================================================
// Initialize : reset des compteurs + log
// ============================================================================
void SurfaceSpectrumSD::Initialize(G4HCofThisEvent*)
{
  fRowsThisEvent = 0;  // [ADD] compteur par event
  // COMMENTÉ pour réduire la taille du fichier log
  // auto ev  = G4RunManager::GetRunManager()->GetCurrentEvent();
  // G4int eid = ev ? ev->GetEventID() : -1;
  // G4cout << ThreadTag() << " [SpecSD] Initialize for event " << eid << G4endl;
}

// ============================================================================
// Helper de binning
// ============================================================================
static inline G4int BinIndex(G4double E_keV, G4double Emin, G4double Emax, G4int nBins)
{
  if (E_keV < Emin || E_keV >= Emax || nBins <= 0) return -1;
  const auto w = (Emax - Emin) / nBins;
  const auto idx = static_cast<G4int>((E_keV - Emin)/w);
  return std::min(nBins-1, std::max(0, idx));
}

// ============================================================================
// ProcessHits
// ============================================================================
G4bool SurfaceSpectrumSD::ProcessHits(G4Step* step, G4TouchableHistory*)
{

  if (!step) return false;

  // [ADD] Prépare des raccourcis pour le debug
  const auto* pre     = step->GetPreStepPoint();
  const auto* post    = step->GetPostStepPoint();
  const auto  posPre  = pre->GetPosition();
  const auto  posPost = post->GetPosition();
  const auto  dir     = pre->GetMomentumDirection();
  const auto* prePV   = pre->GetPhysicalVolume();
  const auto* postPV  = post->GetPhysicalVolume();

  // [ADD] Count ENTER/LEAVE plane occurrences (use PV name "physScorePlane")
  const bool enteringPlane =
  ( (!prePV) || (prePV->GetName() != "physScorePlane") ) &&
  ( postPV && postPV->GetName() == "physScorePlane" );
  if (enteringPlane) { ++fCntEnter; }

  // [ADD] Trace léger : appels à ProcessHits (limité à 30 lignes)
  {
    static int dbg_calls = 0;
    if (dbg_calls < 30) {
      G4cout << "[SpecSD::ProcessHits] pre=" << (prePV ? prePV->GetName() : "<null>")
      << " -> post=" << (postPV ? postPV->GetName() : "<null>")
      << " prePos=" << posPre/mm << " mm"
      << " postPos=" << posPost/mm << " mm"
      << " dirZ=" << dir.z()
      << " onlyOutward=" << (fOutwardOnly ? "true" : "false")
      << " ntupleId=" << fPassageNtupleId
      << G4endl;
      ++dbg_calls;
    }
  }

  // [FIX] Ne compter que la **SORTIE** du plan mince :
  //  - prePV == physScorePlane
  //  - postPV != physScorePlane (ou nul)
  const bool leavingPlane =
  (prePV && prePV->GetName() == "physScorePlane") &&
  (!postPV || postPV->GetName() != "physScorePlane");
  if (!leavingPlane) {
    static int dbg_reject_leave = 0;
    if (dbg_reject_leave < 10) {
      G4cout << "[SpecSD] skip (not leaving physScorePlane)"
      << " pre="  << (prePV  ? prePV->GetName()  : "<null>")
      << " post=" << (postPV ? postPV->GetName() : "<null>")
      << G4endl;
      ++dbg_reject_leave;
    }
    return false;
  }

  // [ADD] outward subset counter (only when outward-only filter is active and passed)
  if (fOutwardOnly && dir.z() > 0.) { 
    ++fCntOut;
    
    // [ADD] Incrémenter le compteur de photons transmis dans RunAction
    auto* runManager = G4RunManager::GetRunManager();
    if (runManager) {
      auto* runAction = const_cast<RunAction*>(
        static_cast<const RunAction*>(runManager->GetUserRunAction()));
      if (runAction) {
        runAction->AddTransmittedPhoton();
      }
    }
  }

  // [FIX] Direction monde : garder uniquement le flux sortant vers +Z si demandé
  if (fOutwardOnly && dir.z() <= 0.) {
    static int dbg_reject_inward = 0;
    if (dbg_reject_inward < 10) {
      G4cout << "[SpecSD] REJECT (inward/side) dirZ=" << dir.z() << G4endl;
      ++dbg_reject_inward;
    }
    return false;
  }

  // [KEEP] Énergie au point de sortie (post-step), en keV
  const G4double E_keV = post->GetKineticEnergy()/keV;

  // [KEEP] Binning du spectre
  const G4int ib = BinIndex(E_keV, fEMin_keV, fEMax_keV, fNBins);
  if (ib >= 0) fBins[ib] += 1.0;

  // [FIX] Écriture dans l'ntuple de passages (si actif)
  if (fPassageNtupleId >= 0) {
    auto* man = G4AnalysisManager::Instance();
    if (man && man->IsActive()) {
      // [FIX] Position au point de sortie (post-step)
      const auto pos = post->GetPosition();
      const G4double x_mm = pos.x()/mm;
      const G4double y_mm = pos.y()/mm;
      const G4double z_mm = pos.z()/mm;

      const auto* def     = step->GetTrack()->GetDefinition();
      const G4int pdg     = def ? def->GetPDGEncoding()  : 0;
      const G4String name = def ? def->GetParticleName() : "unknown";

      // [ADD] TrackID, ParentID et processus créateur
      const G4Track* track = step->GetTrack();
      const G4int trackID = track->GetTrackID();
      const G4int parentID = track->GetParentID();
      const G4VProcess* creatorProcess = track->GetCreatorProcess();
      G4String creator_process = "primary";
      if (creatorProcess) {
          creator_process = creatorProcess->GetProcessName();
      }

      // ==================== Remplissage du ntuple plane_passages ====================
      // Structure harmonisée avec les autres ntuples:
      // colonnes : pdg, name, is_secondary, x_mm, y_mm, z_mm, ekin_keV, trackID, parentID, creator_process
      
      // Calculer is_secondary (0 = primaire, 1 = secondaire)
      G4int is_secondary = (parentID == 0) ? 0 : 1;

      // ================================================================
      // [ADD] Lecture du flag Compton dans le cône (via MyTrackInfo)
      // Pour les primaires : distinguer transmission directe vs redirigé Compton
      // ================================================================
      G4int compton_in_cone = 0;
      G4int n_compton_cone  = 0;
      G4double compton_x_mm = 0., compton_y_mm = 0., compton_z_mm = 0.;

      MyTrackInfo* tInfo = static_cast<MyTrackInfo*>(track->GetUserInformation());
      if (tInfo && tInfo->HasComptonInCone()) {
          compton_in_cone = 1;
          n_compton_cone  = tInfo->GetNComptonInCone();
          G4ThreeVector cpos = tInfo->GetLastComptonPos();
          compton_x_mm = cpos.x() / mm;
          compton_y_mm = cpos.y() / mm;
          compton_z_mm = cpos.z() / mm;
      }

      // ================================================================
      // [ADD] Log des primaires redirigés par Compton dans le cône
      // ================================================================
      if (parentID == 0 && compton_in_cone) {
          static G4int sComptonPlaneLog = 0;
          if (sComptonPlaneLog < 200 || sComptonPlaneLog % 5000 == 0) {
              G4cout << "[ScorePlane1][COMPTON_REDIRECTED] #" << sComptonPlaneLog
                     << " | particle: " << name << " (pdg=" << pdg << ")"
                     << " | trackID=" << trackID
                     << " | E_at_plane=" << E_keV << " keV"
                     << " | n_compton_in_cone=" << n_compton_cone
                     << " | last_compton_pos(mm)=("
                         << compton_x_mm << ", "
                         << compton_y_mm << ", "
                         << compton_z_mm << ")"
                     << " | pos_at_plane(mm)=("
                         << pos.x()/mm << ", "
                         << pos.y()/mm << ", "
                         << pos.z()/mm << ")"
                     << G4endl;
          }
          ++sComptonPlaneLog;
      }

      // ================================================================
      // [ADD] Log de l'origine des particules secondaires
      // Pour chaque secondaire atteignant le plan de comptage, on imprime :
      //   - le processus créateur
      //   - les coordonnées du vertex (lieu de la réaction)
      //   - le volume logique où a eu lieu la réaction
      // ================================================================
      if (is_secondary) {
          // Position du vertex (lieu de création du secondaire)
          const G4ThreeVector& vtxPos = track->GetVertexPosition();
          const G4double vtx_x_mm = vtxPos.x() / mm;
          const G4double vtx_y_mm = vtxPos.y() / mm;
          const G4double vtx_z_mm = vtxPos.z() / mm;

          // Volume logique au vertex
          const G4LogicalVolume* vtxLV = track->GetLogicalVolumeAtVertex();
          G4String vtx_volume = (vtxLV) ? vtxLV->GetName() : "unknown";

          // Matériau au vertex
          G4String vtx_material = "unknown";
          if (vtxLV && vtxLV->GetMaterial()) {
              vtx_material = vtxLV->GetMaterial()->GetName();
          }

          // Énergie cinétique au vertex (énergie initiale du secondaire)
          const G4double vtx_ekin_keV = track->GetVertexKineticEnergy() / keV;

          // Log limité : les 50 premiers puis 1 sur 1000
          static G4int sSecLog = 0;
          if (sSecLog < 50 || sSecLog % 1000 == 0) {
              G4cout << "[ScorePlane1][SECONDARY_ORIGIN] #" << sSecLog
                     << " | particle: " << name << " (pdg=" << pdg << ")"
                     << " | trackID=" << trackID << " parentID=" << parentID
                     << " | E_at_plane=" << E_keV << " keV"
                     << " | creator_process: " << creator_process
                     << " | vertex_pos(mm)=(" << vtx_x_mm << ", " << vtx_y_mm << ", " << vtx_z_mm << ")"
                     << " | vertex_Ekin=" << vtx_ekin_keV << " keV"
                     << " | vertex_volume: " << vtx_volume
                     << " | vertex_material: " << vtx_material
                     << G4endl;
          }
          ++sSecLog;
      }

      // Log limité pour vérification (3 premiers seulement)
      static int c=0, maxPrint=3;
      if (c < maxPrint) {
        G4cout << "[plane_passages][fill#" << (c+1) << "] pdg="<<pdg
        << " x="<<x_mm<<" y="<<y_mm<<" E="<<E_keV<<" keV" << G4endl;
        ++c;
      }
      
      // Remplissage dans l'ordre des colonnes définies dans AnalysisManagerSetup.cc
      man->FillNtupleIColumn(fPassageNtupleId, 0, pdg);             // Col 0: pdg (int)
      man->FillNtupleSColumn(fPassageNtupleId, 1, name);            // Col 1: name (string)
      man->FillNtupleIColumn(fPassageNtupleId, 2, is_secondary);    // Col 2: is_secondary (int)
      man->FillNtupleDColumn(fPassageNtupleId, 3, x_mm);            // Col 3: x_mm (double)
      man->FillNtupleDColumn(fPassageNtupleId, 4, y_mm);            // Col 4: y_mm (double)
      man->FillNtupleDColumn(fPassageNtupleId, 5, z_mm);            // Col 5: z_mm (double)
      man->FillNtupleDColumn(fPassageNtupleId, 6, E_keV);           // Col 6: ekin_keV (double)
      man->FillNtupleIColumn(fPassageNtupleId, 7, trackID);         // Col 7: trackID (int)
      man->FillNtupleIColumn(fPassageNtupleId, 8, parentID);        // Col 8: parentID (int)
      man->FillNtupleSColumn(fPassageNtupleId, 9, creator_process); // Col 9: creator_process (string)
      // [ADD] Colonnes Compton dans le cône graphite
      man->FillNtupleIColumn(fPassageNtupleId, 10, compton_in_cone); // Col 10: compton_in_cone (0/1)
      man->FillNtupleIColumn(fPassageNtupleId, 11, n_compton_cone);  // Col 11: n_compton_cone (int)
      man->FillNtupleDColumn(fPassageNtupleId, 12, compton_x_mm);    // Col 12: compton_x_mm (double)
      man->FillNtupleDColumn(fPassageNtupleId, 13, compton_y_mm);    // Col 13: compton_y_mm (double)
      man->FillNtupleDColumn(fPassageNtupleId, 14, compton_z_mm);    // Col 14: compton_z_mm (double)
      man->AddNtupleRow(fPassageNtupleId);

      // [ADD] rows counter and unique primary event marker
      ++fCntRows;
      {
        const auto* tr = step->GetTrack();
        if (tr && tr->GetParentID() == 0) {
          auto* rm = G4RunManager::GetRunManager();
          auto* ev = rm ? rm->GetCurrentEvent() : nullptr;
          const int eid = ev ? ev->GetEventID() : -1;
          fEventsPrimaryCounted.insert(eid);
        }
      }

      // [ADD] Compteurs (sans logs excessifs)
      ++fRowsThisEvent; ++fRowsTotal;
      // Note: Les logs détaillés ont été désactivés pour réduire la taille du fichier .log
      // Décommenter les lignes suivantes pour le débogage si nécessaire:
      // if (fRowsThisEvent <= fDbgMaxPrint) {
      //   auto ev = G4RunManager::GetRunManager()->GetCurrentEvent();
      //   G4int eid = ev ? ev->GetEventID() : -1;
      //   G4cout << ThreadTag() << " [SpecSD] row in event " << eid
      //          << " E_keV=" << E_keV << " pos(mm)=(" << x_mm << "," << y_mm << "," << z_mm << ")" << G4endl;
      // }

      // Confirmation limitée désactivée (décommenter pour débogage)
      // static int dbg_write = 0;
      // if (dbg_write < 5) {
      //   G4cout << "[SpecSD] WROTE row x=" << x_mm << " y=" << y_mm << " z=" << z_mm << G4endl;
      //   ++dbg_write;
      // }
    } else {
      // [WARN] Diag utile si l’analysis est inactive (ne devrait plus arriver)
      static int dbg_inactive = 0;
      if (dbg_inactive < 10) {
        G4cout << "[SpecSD][WARN] Analysis manager inactive — row NOT written"
        << " (ntupleId=" << fPassageNtupleId << ")"
        << G4endl;
        ++dbg_inactive;
      }
    }
  }

  return true;
}

void SurfaceSpectrumSD::EndOfEvent(G4HCofThisEvent*) {
  // COMMENTÉ pour réduire la taille du fichier log
  // auto ev = G4RunManager::GetRunManager()->GetCurrentEvent();
  // G4int eid = ev ? ev->GetEventID() : -1;
  // G4cout << ThreadTag() << " [SpecSD] EndOfEvent eid=" << eid << ": rowsThisEvent=" << fRowsThisEvent << G4endl;
}

void SurfaceSpectrumSD::Reset() {
  std::fill(fBins.begin(), fBins.end(), 0.0);
}

void SurfaceSpectrumSD::PrintSummary() const {
  G4cout << "[SpecSD][SUMMARY] enter=" << fCntEnter
  << " leave=" << fCntLeave
  << " outward=" << fCntOut
  << " rows_written=" << fCntRows
  << " unique_primary_events_counted=" << fEventsPrimaryCounted.size()
  << G4endl;
}

