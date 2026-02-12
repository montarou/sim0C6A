#include "SteppingMessenger.hh"
#include "SteppingAction.hh"

#include "G4UIcmdWithAnInteger.hh"
#include "G4UIdirectory.hh"
#include "G4UIcommand.hh"

SteppingMessenger::SteppingMessenger(SteppingAction* stepping)
: fStepping(stepping)
{
    G4UIdirectory* dir = new G4UIdirectory("/stepping/");
    dir->SetGuidance("Contrôle de la verbosité de SteppingAction.");

    fVerboseCmd = new G4UIcmdWithAnInteger("/stepping/verbose", this);
    fVerboseCmd->SetGuidance("Définit le niveau de verbosité pour SteppingAction.");
    fVerboseCmd->SetParameterName("verboseLevel", false);
    fVerboseCmd->SetRange("verboseLevel>=0");
}

SteppingMessenger::~SteppingMessenger()
{
    delete fVerboseCmd;
}

void SteppingMessenger::SetNewValue(G4UIcommand* command, G4String value)
{
    if (command == fVerboseCmd) {
        fStepping->SetVerbose(fVerboseCmd->GetNewIntValue(value));
    }
}
