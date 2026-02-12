#include "PrimaryGeneratorMessenger.hh"
#include "PrimaryGeneratorAction.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAnInteger.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorMessenger::PrimaryGeneratorMessenger(PrimaryGeneratorAction* Gun)
:fAction(Gun)
{
  fDirGenerator = new G4UIdirectory("/primariesgenerator/");
  fDirGenerator->SetGuidance("Change Type of Primary Generator");

  fSelectActionCmd = new G4UIcmdWithAnInteger("/primariesgenerator/selectsource",this);
  fSelectActionCmd->SetGuidance("Select primary generator action");
  fSelectActionCmd->SetGuidance("0 Co60");
  fSelectActionCmd->SetGuidance("1 Gamma 100keV");
  fSelectActionCmd->SetGuidance("2 Gamma Spectra");
  fSelectActionCmd->SetGuidance("3 Electron 200keV");
  fSelectActionCmd->SetParameterName("id",false);
  fSelectActionCmd->SetRange("id>=0 && id<5");
  fSelectActionCmd->AvailableForStates(G4State_PreInit,G4State_Idle);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorMessenger::~PrimaryGeneratorMessenger()
{
  delete fSelectActionCmd;
  delete fDirGenerator;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
  if (command == fSelectActionCmd) {
    //G4cout<<"    Avant Commande "<<G4endl;
    G4int SelectedAction = fSelectActionCmd->GetNewIntValue(newValue);
    fAction->SelectAction(SelectedAction);
    //G4cout<<"    Commande "<<SelectedAction<<G4endl;
    }
  }

