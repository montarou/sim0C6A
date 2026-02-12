#include "EventMessenger.hh"
#include "EventAction.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIdirectory.hh"

EventMessenger::EventMessenger(EventAction* action)
: fEventAction(action)
{
    auto dir = new G4UIdirectory("/event/");
    dir->SetGuidance("Commandes pour EventAction");

    fVerboseCmd = new G4UIcmdWithAnInteger("/event/verbose", this);
    fVerboseCmd->SetGuidance("Définit le niveau de verbosité pour EventAction");
    fVerboseCmd->SetParameterName("verboseLevel", false);
    fVerboseCmd->SetRange("verboseLevel >= 0");
}

EventMessenger::~EventMessenger() {
    delete fVerboseCmd;
}

void EventMessenger::SetNewValue(G4UIcommand* command, G4String value) {
    if (command == fVerboseCmd) {
        fEventAction->SetVerbose(fVerboseCmd->GetNewIntValue(value));
    }
}
