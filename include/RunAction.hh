#ifndef RUNACTION_HH
#define RUNACTION_HH

#include "G4UserRunAction.hh"
#include "G4Types.hh"
#include "g4root_defs.hh"
#include "G4AnalysisManager.hh"
#include "G4Accumulable.hh"
#include "G4AccumulableManager.hh"

#include <vector>
#include <map>
#include <fstream>

class G4Run;
class G4Event;
class RunMessenger;
class SphereHit;
class EventAction;

class RunAction : public G4UserRunAction
{
    public:
        RunAction();                          // pour BuildForMaster
        RunAction(EventAction* eventAction);  // pour Build
        virtual ~RunAction();

        virtual void BeginOfRunAction(const G4Run*) override;
        virtual void EndOfRunAction(const G4Run*) override;

        void AddToEntrantInBe(G4int n) { fTotalEntrantInBe += n; }
        void AddToInteractedInBe(G4int n) { fTotalInteractedInBe += n; }

        void AddToEntrantInWaterSphere(G4int n) { fTotalEntrantInWaterSphere += n; }
        void AddToInteractedInWaterSphere(G4int n) { fTotalInteractedInWaterSphere += n;}

        void UpdateFromEvent(const EventAction* event);

        // Ajoute les hits d’un événement
        void AddHitsForEvent(G4int eventID, const std::vector<SphereHit>& hits);

        G4int GetTotalEntrantInBe() const;
        G4int GetTotalInteractedInBe() const;
        G4int GetTotalEntrantInWaterSphere() const;
        G4int GetTotalInteractedInWaterSphere() const;

        void SetVerbose(G4int val) { fRunVerbose = val; }

        void IncrementValid1Particles() const;
        G4int GetNValid1Particles() const { return fNValidParticles_lt_35.GetValue(); }

        void IncrementValid2Particles() const;
        G4int GetNValid2Particles() const { return fNValidParticles_gt_35.GetValue(); }

        void CountPrimary() const;

        // ==================== Énergie déposée dans les anneaux d'eau ====================
        static const G4int kNbWaterRings = 5;
        
        // Masses des anneaux en grammes (eau = 1 g/cm³)
        // Anneau 0: r=0-2mm, V = π×4×3 mm³ = 0.0377 cm³
        // Anneau 1: r=2-4mm, V = π×12×3 mm³ = 0.1131 cm³
        // Anneau 2: r=4-6mm, V = π×20×3 mm³ = 0.1885 cm³
        // Anneau 3: r=6-8mm, V = π×28×3 mm³ = 0.2639 cm³
        // Anneau 4: r=8-10mm, V = π×36×3 mm³ = 0.3393 cm³
        static constexpr G4double kMassRing[kNbWaterRings] = {
            0.03770,  // anneau 0 en grammes
            0.11310,  // anneau 1
            0.18850,  // anneau 2
            0.26389,  // anneau 3
            0.33929   // anneau 4
        };
        static constexpr G4double kMassTotalWater = 0.94248;  // π×100×3 mm³ = 0.942 cm³
        
        // Ajouter l'énergie déposée d'un événement
        void AddEdepFromEvent(const G4double* edepRing, G4double edepTotal);
        
        // Accesseurs pour les énergies accumulées
        G4double GetTotalEdepRing(G4int ringIndex) const;
        G4double GetTotalEdepWater() const;
        
        // Pour les histogrammes par 10000 événements
        void CheckAndFillDoseHistograms(G4int eventID);
        
        // Compteur de photons transmis (mis à jour depuis SurfaceSpectrumSD)
        void AddTransmittedPhoton() { fTransmitted10000++; fTransmittedTotal++; }
        G4long GetTransmittedTotal() const { return fTransmittedTotal; }

    private:

        mutable G4Accumulable<G4int> fNValidParticles_lt_35;
        mutable G4Accumulable<G4int> fNValidParticles_gt_35;

        G4int fRunVerbose = 0;
        RunMessenger* fRunMessenger;

        EventAction* fEventAction = nullptr;

        G4Accumulable<G4int> fTotalEntrantInBe;
        G4Accumulable<G4int> fTotalInteractedInBe;

        G4Accumulable<G4int> fTotalEntrantInWaterSphere;
        G4Accumulable<G4int> fTotalInteractedInWaterSphere;

        //Ajoute : stockage des hits par événement
        std::map<G4int, std::vector<SphereHit>> fHitsByEvent;

        // ← mutable pour autoriser l’incrément depuis une méthode const
        mutable G4Accumulable<G4long> fPrimariesGenerated;

        // ==================== Accumulation des énergies déposées ====================
        G4double fTotalEdepRing[kNbWaterRings] = {0., 0., 0., 0., 0.};
        G4double fTotalEdepWater = 0.;
        
        // Pour les histogrammes par 10000 événements
        G4double fEdepRing10000[kNbWaterRings] = {0., 0., 0., 0., 0.};
        G4double fEdepWater10000 = 0.;
        G4int fLastHistoFillEvent = -10000;  // pour suivre quand on a rempli les histos
        
        // Compteur de photons transmis par lot de 10000 événements
        G4long fTransmitted10000 = 0;
        G4long fTransmittedTotal = 0;

};
#endif
