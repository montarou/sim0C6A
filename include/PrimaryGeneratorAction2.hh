#ifndef PrimaryGeneratorAction2_h
#define PrimaryGeneratorAction2_h

#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"
#include "G4ThreeVector.hh"
#include <vector>

class G4ParticleGun;
class G4Event;
class DetectorConstruction;
class G4VSolid;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class PrimaryGeneratorAction2
{
  public:
    PrimaryGeneratorAction2(G4ParticleGun*);
   ~PrimaryGeneratorAction2() = default;

  public:
   void GeneratePrimaries(G4Event*);

  public:
    G4double RejectAccept();
    G4double InverseCumul();
    
    // =====================================================
    // NOUVEAU : Méthode pour générer une position aléatoire
    //           dans le volume de l'anode (source volumique)
    // =====================================================
    G4ThreeVector GeneratePositionInAnode();
    
    // Active/désactive le mode source volumique
    void SetVolumeSourceMode(G4bool mode) { fUseVolumeSource = mode; }
    G4bool GetVolumeSourceMode() const { return fUseVolumeSource; }

  private:
    G4ParticleGun*         fParticleGun = nullptr;

    G4int                  fNPoints = 0; //nb of points
    std::vector<G4double>  fX;           //abscisses X
    std::vector<G4double>  fY;           //values of Y(X)
    std::vector<G4double>  fSlp;         //slopes
    std::vector<G4double>  fYC;          //cumulative function of Y
    G4double               fYmax = 0.;   //max(Y)

    G4double fCosAlphaMin = 0., fCosAlphaMax = 0.;      //solid angle
    G4double fPsiMin = 0., fPsiMax = 0.;
    
    // =====================================================
    // NOUVEAU : Membres pour la source volumique
    // =====================================================
    G4bool fUseVolumeSource = true;  // true = source volumique, false = source ponctuelle
    
    // Bounding box de l'anode (en unités internes Geant4)
    G4double fAnodeXmin = -3.5;  // mm (valeurs par défaut)
    G4double fAnodeXmax =  3.5;
    G4double fAnodeYmin = -3.5;
    G4double fAnodeYmax =  3.5;
    G4double fAnodeZmin = -0.001;
    G4double fAnodeZmax =  0.0;
    
    // Pointeur vers le solide de l'anode (pour le test Inside())
    G4VSolid* fAnodeSolid = nullptr;
    
    // Flag pour initialisation
    G4bool fAnodeInitialized = false;
    
    // Méthode d'initialisation du volume source
    void InitializeAnodeVolume();

  private:
    void InitFunction();
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
