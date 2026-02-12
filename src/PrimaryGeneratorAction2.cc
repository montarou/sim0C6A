#include "PrimaryGeneratorAction2.hh"
#include "PrimaryGeneratorAction.hh"
#include "DetectorConstruction.hh"

#include "G4Event.hh"
#include "G4Track.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include "G4AnalysisManager.hh"

#include "RunAction.hh"
#include "G4RunManager.hh"

// NOUVEAU : includes pour le volume source
#include "G4VSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4ThreeVector.hh"


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction2::PrimaryGeneratorAction2(G4ParticleGun* gun)
: fParticleGun(gun)
{
  //solid angle
  //
  G4double alphaMin =  0*deg;      //alpha in [0,pi]
  G4double alphaMax = 60*deg;      // cône de 60°
  fCosAlphaMin = std::cos(alphaMin);
  fCosAlphaMax = std::cos(alphaMax);

  fPsiMin = 0*deg;       //psi in [0, 2*pi]
  fPsiMax = 360*deg;

  // energy distribution
  //
  InitFunction();
  
  // NOUVEAU : Désactiver le mode source volumique (source ponctuelle)
  fUseVolumeSource = false;
  fAnodeInitialized = false;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// =====================================================
// NOUVEAU : Initialisation du volume de l'anode
// =====================================================
void PrimaryGeneratorAction2::InitializeAnodeVolume()
{
    if (fAnodeInitialized) return;
    
    // Récupérer le DetectorConstruction
    const DetectorConstruction* detector = 
        static_cast<const DetectorConstruction*>(
            G4RunManager::GetRunManager()->GetUserDetectorConstruction());
    
    if (!detector) {
        G4cerr << "[PrimaryGeneratorAction2] ERREUR: DetectorConstruction non trouvé!" << G4endl;
        fUseVolumeSource = false;
        return;
    }
    
    // Récupérer le solide de l'anode
    fAnodeSolid = detector->GetAnodeSolid();
    
    if (!fAnodeSolid) {
        G4cerr << "[PrimaryGeneratorAction2] ERREUR: Solide de l'anode non trouvé!" << G4endl;
        G4cerr << "[PrimaryGeneratorAction2] Passage en mode source ponctuelle." << G4endl;
        fUseVolumeSource = false;
        return;
    }
    
    // Récupérer la bounding box
    detector->GetAnodeBoundingBox(fAnodeXmin, fAnodeXmax, 
                                   fAnodeYmin, fAnodeYmax, 
                                   fAnodeZmin, fAnodeZmax);
    
    // Restreindre X et Y à 0.5 mm (centré sur l'origine)
    fAnodeXmin = -0.25*CLHEP::mm;
    fAnodeXmax =  0.25*CLHEP::mm;
    fAnodeYmin = -0.25*CLHEP::mm;
    fAnodeYmax =  0.25*CLHEP::mm;
    
    G4cout << "\n========== INITIALISATION SOURCE VOLUMIQUE ==========" << G4endl;
    G4cout << "Solide de l'anode : " << fAnodeSolid->GetName() << G4endl;
    G4cout << "Bounding box :" << G4endl;
    G4cout << "  X : [" << fAnodeXmin/mm << ", " << fAnodeXmax/mm << "] mm" << G4endl;
    G4cout << "  Y : [" << fAnodeYmin/mm << ", " << fAnodeYmax/mm << "] mm" << G4endl;
    G4cout << "  Z : [" << fAnodeZmin/mm << ", " << fAnodeZmax/mm << "] mm" << G4endl;
    G4cout << "Mode source volumique : ACTIVÉ" << G4endl;
    G4cout << "====================================================\n" << G4endl;
    
    fAnodeInitialized = true;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// =====================================================
// NOUVEAU : Génération d'une position aléatoire dans l'anode
//           utilisant le rejection sampling
// =====================================================
G4ThreeVector PrimaryGeneratorAction2::GeneratePositionInAnode()
{
    // Initialiser si pas encore fait
    if (!fAnodeInitialized) {
        InitializeAnodeVolume();
    }
    
    // Si l'initialisation a échoué, retourner la position par défaut
    if (!fAnodeSolid || !fUseVolumeSource) {
        return G4ThreeVector(0., 0., -0.0005*mm);
    }
    
    G4ThreeVector position;
    G4int maxAttempts = 100000;  // Limite pour éviter boucle infinie
    G4int attempts = 0;
    
    // Rejection sampling : générer des points dans la bounding box
    // et accepter seulement ceux qui sont à l'intérieur du solide
    do {
        G4double x = fAnodeXmin + G4UniformRand() * (fAnodeXmax - fAnodeXmin);
        G4double y = fAnodeYmin + G4UniformRand() * (fAnodeYmax - fAnodeYmin);
        G4double z = fAnodeZmin + G4UniformRand() * (fAnodeZmax - fAnodeZmin);
        
        position.set(x, y, z);
        attempts++;
        
        // Vérifier si le point est à l'intérieur du solide
        // Inside() retourne kInside, kSurface, ou kOutside
        if (fAnodeSolid->Inside(position) != kOutside) {
            break;  // Point accepté
        }
        
    } while (attempts < maxAttempts);
    
    if (attempts >= maxAttempts) {
        G4cerr << "[PrimaryGeneratorAction2] WARNING: Max attempts reached in rejection sampling!" << G4endl;
        G4cerr << "[PrimaryGeneratorAction2] Using fallback position at center of bounding box." << G4endl;
        position.set((fAnodeXmin + fAnodeXmax) / 2.0,
                     (fAnodeYmin + fAnodeYmax) / 2.0,
                     (fAnodeZmin + fAnodeZmax) / 2.0);
    }
    
    return position;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorAction2::GeneratePrimaries(G4Event* anEvent)
{

  // Tirage d'un gamma

  //G4cout << "[Primary Generator DEBUG] case 2 fAction" << G4endl;

  //G4ParticleDefinition* particle = G4ParticleTable::GetParticleTable()->FindParticle("geantino");
  G4ParticleDefinition* particle = G4ParticleTable::GetParticleTable()->FindParticle("gamma");
  //G4ParticleDefinition* particle = G4ParticleTable::GetParticleTable()->FindParticle("e-");
  if (!particle) {
    G4Exception("[Primary Generator DEBUG] PrimaryGeneratorAction1", "NullParticle", FatalException, "gamma not found in particle table.");
  }
  fParticleGun->SetParticleDefinition(particle);

  G4String pname = particle->GetParticleName();
  //G4cout << "[Primary Generator DEBUG] Particle Name =" << pname<<G4endl;
  
  // =====================================================
  // MODIFIÉ : Position de la source
  // =====================================================
  G4double x0, y0, z0;
  
  if (fUseVolumeSource) {
      // Mode source volumique : position aléatoire dans l'anode
      G4ThreeVector pos = GeneratePositionInAnode();
      x0 = pos.x();
      y0 = pos.y();
      z0 = pos.z();
  } else {
      // Mode source ponctuelle (ancien comportement)
      x0 = 0*mm;
      y0 = 0*mm;
      z0 = 0.001*mm;  // z = +1 µm (juste après l'anode)
  }
  
  fParticleGun->SetParticlePosition(G4ThreeVector(x0, y0, z0));

 //G4cout << "positions sources x : " << x0 << " y : " << y0 << " z : " << z0 << G4endl;
  // uniform solid angle

  //direction uniform in solid angle
  G4double cosAlpha = fCosAlphaMin-G4UniformRand()*(fCosAlphaMin-fCosAlphaMax);
  G4double sinAlpha = std::sqrt(1. - cosAlpha*cosAlpha);
  G4double psi = fPsiMin + G4UniformRand()*(fPsiMax - fPsiMin);

  G4double alpha = std::acos(cosAlpha);
  G4double alphaDeg = alpha / deg;

  G4double ux = sinAlpha*std::cos(psi),
  uy = sinAlpha*std::sin(psi),
  uz = cosAlpha;

  // Direction fixée : +Z
  //G4double ux = 0.0;
  //G4double uy = 0.0;
  //G4double uz = 1.0;

  //G4cout << "directions ux : " << ux << " uy : " << uy << " uz : " << uz << G4endl;

  // angle alpha = 0 rad (0°)
  //G4double alphaDeg = 0.0;
  //G4double phiDeg = 0.0;

  //G4cout << "angles alpha : " << alphaDeg << " phi : " << phiDeg << G4endl;

  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(ux,uy,uz));
  //set energy from a tabulated distribution
  //G4double energy = RejectAccept();

  // On SKIPPE l'événement si E <= 3.5 keV : pas de vertex, pas de comptage primaire.
  static constexpr G4double kEcut = 3.5*keV;


  // Tirage énergie avec rejet sous le cut
  G4double energy = 0.;
  do {
    energy = InverseCumul();   // ta loi tabulée existante
  } while (energy <= kEcut);

  // Si on passe le cut, on fixe l'énergie
  fParticleGun->SetParticleEnergy(energy);

  // LOG avant création du vertex (contrôle des valeurs réellement utilisées)
  // COMMENTÉ pour réduire la taille du fichier log
  /*
  {
    const auto eid = anEvent ? anEvent->GetEventID() : -1;
    G4cout << "[GEN] event=" << eid
    << " mode=2"
    << " pos(mm)=(" << x0/mm << "," << y0/mm << "," << z0/mm << ")"
    << " dir=(" << ux << "," << uy << "," << uz << ")"
    << " theta=" << alphaDeg << " deg"
    << " E=" << energy/keV << " keV";
    
    // Indiquer le type de source
    if (fUseVolumeSource) {
        G4cout << " [GAMMA SPECTRE - VOLUME]";
    } else {
        G4cout << " [GAMMA SPECTRE - POINT]";
    }
    G4cout << G4endl;
  }
  */
  // --- Création effective du vertex ---
  fParticleGun->GeneratePrimaryVertex(anEvent);

  // --- Compteurs RunAction : UNIQUEMENT APRES la création du vertex ---
  if (const auto* ra = static_cast<const RunAction*>(
    G4RunManager::GetRunManager()->GetUserRunAction())) {
    // Doivent être 'const' côté RunAction (ou G4Accumulable)
    ra->CountPrimary();             // "un primaire effectivement généré"
    ra->IncrementValid2Particles(); // ton compteur existant
    }

    // --- (facultatif) remplissages analyse/histos existants ---
    if (auto* man = G4AnalysisManager::Instance()) {
      G4double psiDeg = psi / deg;  // conversion en degrés
      
      man->FillH1(0, energy);      // H0: Énergie à l'émission
      man->FillH1(1, alphaDeg);    // H1: Theta à l'émission
      man->FillH1(2, psiDeg);      // H2: Phi à l'émission
    }

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorAction2::InitFunction()
{

  // tabulated function: Y>0, linear per segment, continuous
  fNPoints = 50;
  const G4double xx[] =
  { 1*keV,  2*keV,  3*keV,  4*keV,  5*keV,
    6*keV,  7*keV,  8*keV,  9*keV, 10*keV,
    11*keV, 12*keV, 13*keV, 14*keV, 15*keV,
    16*keV, 17*keV, 18*keV, 19*keV, 20*keV,
    21*keV, 22*keV, 23*keV, 24*keV, 25*keV,
    26*keV, 27*keV, 28*keV, 29*keV, 30*keV,
    31*keV, 32*keV, 33*keV, 34*keV, 35*keV,
    36*keV, 37*keV, 38*keV, 39*keV, 40*keV,
    41*keV, 42*keV, 43*keV, 44*keV, 45*keV,
    46*keV, 47*keV, 48*keV, 49*keV, 50*keV };

  const G4double yy[] =
    { 0.0205648, 63.56, 100.0, 90.93, 76.80, 64.72, 55.15, 47.59, 41.54, 36.61,
      32.53, 29.10, 26.19, 23.68, 21.51, 19.60, 17.91, 16.41, 15.06, 13.85,
      12.75, 11.75, 10.84, 10.00,  9.24,  8.53,  7.87,  7.26,  7.26,  6.16,
      5.66,  5.20,  4.76,  4.35,  3.96,  3.59,  3.25,  2.92,  2.61,  2.31,
      2.03,  1.76,  1.50,  1.26,  1.03,  0.80,  0.59,  0.39,  0.19,  0.00 };

  // copy arrays in std::vector and compute fMax

  fX.resize(fNPoints);
  fY.resize(fNPoints);
  fYmax = 0.;
  for (G4int j = 0; j < fNPoints; j++) {
    fX[j] = xx[j];
    fY[j] = yy[j];
    if (fYmax < fY[j]) fYmax = fY[j];
  };

  // pentes
  fSlp.resize(fNPoints);
  for (G4int j = 0; j < fNPoints - 1; j++) {
      fSlp[j] = (fY[j + 1] - fY[j]) / (fX[j + 1] - fX[j]);
  };

  // cumulée
  fYC.resize(fNPoints);
  fYC[0] = 0.;
  for (G4int j = 1; j < fNPoints; j++) {
          fYC[j] = fYC[j - 1] + 0.5 * (fY[j] + fY[j - 1]) * (fX[j] - fX[j - 1]);
  };
}


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4double PrimaryGeneratorAction2::RejectAccept()
{
  // tabulated function
  // Y is assumed positive, linear per segment, continuous
  // (see Particle Data Group: pdg.lbl.gov --> Monte Carlo techniques)
  //
  G4double Xrndm = 0., Yrndm = 0., Yinter = -1.;

  while (Yrndm > Yinter) {
    //choose a point randomly
    Xrndm = fX[0] + G4UniformRand()*(fX[fNPoints-1] - fX[0]);
    Yrndm = G4UniformRand()*fYmax;
    //find bin
    G4int j = fNPoints-2;
    while ((fX[j] > Xrndm) && (j > 0)) j--;
    //compute Y(x_rndm) by linear interpolation
    Yinter = fY[j] + fSlp[j]*(Xrndm - fX[j]);
  };
  return Xrndm;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4double PrimaryGeneratorAction2::InverseCumul()
{
  // tabulated function
  // Y is assumed positive, linear per segment, continuous
  // --> cumulative function is second order polynomial
  // (see Particle Data Group: pdg.lbl.gov --> Monte Carlo techniques)

  //choose y randomly
  G4double Yrndm = G4UniformRand()*fYC[fNPoints-1];
  //find bin
  G4int j = fNPoints-2;
  while ((fYC[j] > Yrndm) && (j > 0)) j--;
  //y_rndm --> x_rndm :  fYC(x) is second order polynomial
  G4double Xrndm = fX[j];
  G4double a = fSlp[j];
  if (a != 0.) {
    G4double b = fY[j]/a, c = 2*(Yrndm - fYC[j])/a;
    G4double delta = b*b + c;
    G4int sign = 1; if (a < 0.) sign = -1;
    Xrndm += sign*std::sqrt(delta) - b;
  } else if (fY[j] > 0.) {
    Xrndm += (Yrndm - fYC[j])/fY[j];
  };
  return Xrndm;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
