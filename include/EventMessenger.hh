#ifndef EVENTMESSENGER_HH
#define EVENTMESSENGER_HH

#include "G4UImessenger.hh"
#include "globals.hh"

class G4UIcmdWithAnInteger;
class EventAction;

class EventMessenger : public G4UImessenger {
public:
    EventMessenger(EventAction* action);
    ~EventMessenger() override;

    void SetNewValue(G4UIcommand*, G4String) override;

private:
    EventAction* fEventAction;
    G4UIcmdWithAnInteger* fVerboseCmd;
};

#endif
