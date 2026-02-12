#include <iostream>
#include <fstream>
#include <cstdio> // pour freopen

#include "G4RunManagerFactory.hh"
#include "G4RunManager.hh"
#include "G4MTRunManager.hh"
#include "G4Threading.hh"

#include "FTFP_BERT.hh"
#include "G4StepLimiterPhysics.hh"

#include "G4UImanager.hh"
#include "G4UIExecutive.hh"

#include "G4VisManager.hh"
#include "G4VisExecutive.hh"

#include "DetectorConstruction.hh"
#include "PhysicsList.hh"
#include "ActionInitialization.hh"

#include "G4ios.hh"

// ===== RAII guard to redirect all logs (G4cout/G4cerr + std::cout/std::cerr) =====
struct LogGuard {
  std::ofstream f;
  std::streambuf *oldG4Out=nullptr, *oldG4Err=nullptr, *oldStdOut=nullptr, *oldStdErr=nullptr;
  explicit LogGuard(const char* path) : f(path) {
    if (f.is_open()) {
      oldG4Out  = G4cout.rdbuf(f.rdbuf());
      oldG4Err  = G4cerr.rdbuf(f.rdbuf());
      oldStdOut = std::cout.rdbuf(f.rdbuf());
      oldStdErr = std::cerr.rdbuf(f.rdbuf());
    } else {
      // If the file cannot be opened, keep default streams
      G4cerr << "[WARN] Cannot open log file: " << path << G4endl;
    }
  }
  ~LogGuard() {
    if (oldG4Out)  { G4cout.rdbuf(oldG4Out);   oldG4Out=nullptr; }
    if (oldG4Err)  { G4cerr.rdbuf(oldG4Err);   oldG4Err=nullptr; }
    if (oldStdOut) { std::cout.rdbuf(oldStdOut); oldStdOut=nullptr; }
    if (oldStdErr) { std::cerr.rdbuf(oldStdErr); oldStdErr=nullptr; }
    if (f.is_open()) f.close();
  }
};

int main(int argc, char** argv)
{

  // Capture EVERYTHING (banner, geometry init, run, summaries) in a single file
  LogGuard lg("geant4_run_full.log");

  G4String macrofile = "";
  G4UIExecutive* ui  = nullptr;
  if ( argc == 1 ) {     // cas pas de macro file
    ui = new G4UIExecutive(argc, argv);
  } else {
    macrofile = argv[1]; // cas macrofile
  }


  // ✅ Création du run manager avec factory
  auto* runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);

  // ✅ Force l'exécution en mono-thread (si MT activé)
  #ifdef G4MULTITHREADED
  auto* mtManager = dynamic_cast<G4MTRunManager*>(runManager);
  if (mtManager) {
    mtManager->SetNumberOfThreads(1);
    G4cout << "[INFO] Mode mono-thread forcé avec G4MTRunManager." << G4endl;
  }
  #endif

  // Définition de la construction du détecteur
  auto* detector = new DetectorConstruction();
  runManager->SetUserInitialization(detector);

  // Définition de la liste de physique
  //auto* physicsList = new PhysicsList();
  auto physicsList = new FTFP_BERT;
  physicsList->RegisterPhysics(new G4StepLimiterPhysics());
  runManager->SetUserInitialization(physicsList);

  // Définition des actions utilisateur
  runManager->SetUserInitialization(new ActionInitialization());

  // set up visualisation
  G4VisManager* visManager = new G4VisExecutive;
  visManager->Initialize();

  G4UImanager* UImanager = G4UImanager::GetUIpointer();
  if ( ui == nullptr) {
    // macro file: execute
    G4String cmd = "/control/execute ";
    UImanager->ApplyCommand(cmd + macrofile);
  } else {
    // interactive: start session
    UImanager->ApplyCommand("/control/execute init_vis.mac");
    ui->SessionStart();
    delete ui;
  }
  // Lancement du run
  //runManager->BeamOn(10);

  // Fin du programme
  //delete runManager;

  return 0;
}
