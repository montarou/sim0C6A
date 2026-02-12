#ifndef SurfaceSpectrumSD_hh
#define SurfaceSpectrumSD_hh 1

// Geant4
#include "G4VSensitiveDetector.hh"
#include "globals.hh"

// STL
#include <vector>
#include <string>

#include <set>

// Fwds
class G4Step;
class G4HCofThisEvent;
class G4TouchableHistory;

/**
 * @brief Sensitive detector pour compter les passages d’un plan mince
 *        et remplir à la fois un histogramme binned (en mémoire) et un ntuple (G4Analysis).
 *
 * [DOC] Hypothèses côté ntuple "plane_passages" (même ordre que dans le .cc) :
 *   Col 0 : double x_mm
 *   Col 1 : double y_mm
 *   Col 2 : double z_mm
 *   Col 3 : double E_keV
 *   Col 4 : int    pdg
 *   Col 5 : string name   (si tu ne crées pas cette colonne string, commente FillNtupleSColumn côté .cc)
 */

class SurfaceSpectrumSD : public G4VSensitiveDetector
{
public:
  // ---------------------------------------------------------------------------
  // Ctors / Dtor
  // ---------------------------------------------------------------------------
  SurfaceSpectrumSD(const G4String& name,
                    G4double Emin_keV,
                    G4double Emax_keV,
                    G4int    nBins,
                    G4bool   countOutwardOnly = true);
  ~SurfaceSpectrumSD() override;

  // ---------------------------------------------------------------------------
  // Interface G4VSensitiveDetector
  // ---------------------------------------------------------------------------
  void    Initialize(G4HCofThisEvent* hce) override;
  G4bool  ProcessHits(G4Step* step, G4TouchableHistory*) override;
  void    EndOfEvent(G4HCofThisEvent* hce) override;

  // ---------------------------------------------------------------------------
  // Utilitaires
  // ---------------------------------------------------------------------------
  void Reset();                           // [ADD] remet l’histogramme interne à zéro

  // [ADD] Configuration runtime
  inline void SetArea_cm2(G4double a)        { fArea_cm2 = a; }
  inline void SetPassageNtupleId(G4int id)   { fPassageNtupleId = id; }
  inline void SetVerbose(G4int v)            { fVerbose = v; }

  // [ADD] Accès lecture
  inline G4double Emin_keV()     const { return fEMin_keV; }
  inline G4double Emax_keV()     const { return fEMax_keV; }
  inline G4double BinWidth_keV() const { return fBinWidth_keV; }
  inline G4int    NBins()        const { return fNBins; }
  inline G4bool   OutwardOnly()  const { return fOutwardOnly; }
  inline G4double Area_cm2()     const { return fArea_cm2; }
  inline G4int    PassageNtupleId() const { return fPassageNtupleId; }

  // [ADD] Histogramme interne (lecture seule)
  inline const std::vector<G4double>& Bins() const { return fBins; }

  // Bilan global
  void   PrintSummary() const;

private:
  // ---------------------------------------------------------------------------
  // Paramètres de binning énergie (keV)
  // ---------------------------------------------------------------------------
  G4double fEMin_keV    = 0.0;   // [DOC] borne incluse
  G4double fEMax_keV    = 0.0;   // [DOC] borne exclue
  G4double fBinWidth_keV= 0.0;   // [DOC] (fEMax_keV - fEMin_keV)/fNBins
  G4int    fNBins       = 0;

  // ---------------------------------------------------------------------------
  // Politique de comptage et normalisation
  // ---------------------------------------------------------------------------
  G4bool   fOutwardOnly = true;  // [DOC] si true, n’accepte que dir.z() > 0
  G4double fArea_cm2    = 1.0;   // [DOC] utile pour normaliser un flux, si besoin

  // ---------------------------------------------------------------------------
  // Données internes
  // ---------------------------------------------------------------------------
  std::vector<G4double> fBins;   // [DOC] histogramme (compte par bin)
  G4int   fVerbose      = 0;

  // ID de l’ntuple "plane_passages" (G4Analysis)
  G4int   fPassageNtupleId = -1;

  // ---------------------------------------------------------------------------
  // Compteurs/debug (utilisés par les logs dans le .cc)
  // ---------------------------------------------------------------------------
  G4int   fRowsThisEvent = 0;   // [DOC] lignes ajoutées à l’ntuple durant l’événement
  G4long  fRowsTotal     = 0;   // [DOC] total lignes écrites depuis le début du run (par thread)
  G4int   fDbgMaxPrint   = 10;  // [DOC] imprime les 10 premières lignes puis chaque 1000e

  G4long fCntEnter = 0;   // pas entrant dans le plan (pre!=plan, post==plan)
  G4long fCntLeave = 0;   // pas sortant du plan (pre==plan, post!=plan)
  G4long fCntOut   = 0;   // sous-ensemble leave qui sont "outward" si filtre actif
  G4long fCntRows  = 0;   // lignes réellement écrites dans l’ntuple
  std::set<int> fEventsPrimaryCounted; // événements primaires pour lesquels on a écrit au moins 1 ligne

};


/**
 c l*ass SurfaceSpectrumSD : public G4VSensitiveDetector {
 public:
   SurfaceSpectrumSD(const G4String& name,
   G4double eMin_keV, G4double eMax_keV, G4int nBins,
G4bool countOutwardOnly = true);
~SurfaceSpectrumSD() override = default;

void Initialize(G4HCofThisEvent*) override;
G4bool ProcessHits(G4Step* step, G4TouchableHistory*) override;
void EndOfEvent(G4HCofThisEvent*) override;

// Accumulateur par run (on fera la normalisation en EndOfRunAction)
const std::vector<G4double>& GetBins() const { return fBins; }
G4double GetBinWidth_keV() const { return fBinWidth_keV; }

// Remise à zéro manuelle (depuis RunAction::BeginOfRunAction)
void Reset();

// Aire en cm2 (à renseigner une fois connu hx,hy)
void SetArea_cm2(G4double A) { fArea_cm2 = A; }
G4double GetArea_cm2() const { return fArea_cm2; }

// Compteur de primaires (à incrémenter dans PrimaryGeneratorAction ou RunAction)
void AddPrimaries(G4int n) { fNprim += n; }
G4int GetNprim() const { return fNprim; }

G4double GetEMin_keV() const { return fEMin_keV; }
G4double GetEMax_keV() const { return fEMax_keV; }

// id du ntuple « passages » (x,y,z,Ek,PDG,name)
void SetPassageNtupleId(int id) { fPassageNtupleId = id; }

private:

  // paramètres du spectre
  G4double fEMin_keV, fEMax_keV, fBinWidth_keV;
  G4int    fNBins;
  std::vector<G4double> fBins;   // compte de traversées par bin
  G4double fArea_cm2 = 1.0;
  G4int    fNprim = 0;
  G4bool   fOutwardOnly;

  int fPassageNtupleId = -1;
  // --- Debug counters ---
  G4int  fRowsThisEvent = 0;   // rows (AddNtupleRow) during current event
  G4long fRowsTotal     = 0;   // rows across the whole run (per thread)
  G4int  fDbgMaxPrint   = 10;  // print first 10 rows then every 1000th
  };

  */
#endif // SurfaceSpectrumSD_hh













































