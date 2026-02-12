#include "RunMessenger.hh"
#include "RunAction.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIdirectory.hh"

RunMessenger::RunMessenger(RunAction* run)
: fRunAction(run)
{
    auto dir = new G4UIdirectory("/run/");
    dir->SetGuidance("Contrôle de la verbosité de RunAction.");

    fVerboseCmd = new G4UIcmdWithAnInteger("/runVerbose/set", this);
    fVerboseCmd->SetGuidance("Définit le niveau de verbosité de RunAction.");
    fVerboseCmd->SetParameterName("verboseLevel", false);
    fVerboseCmd->SetRange("verboseLevel >= 0");
}

RunMessenger::~RunMessenger()
{
    delete fVerboseCmd;
}

void RunMessenger::SetNewValue(G4UIcommand* command, G4String value)
{
    if (command == fVerboseCmd) {
        fRunAction->SetVerbose(fVerboseCmd->GetNewIntValue(value));
    }
}
