#ifndef ACTIONINITIALIZATION_HH
#define ACTIONINITIALIZATION_HH

#include "G4VUserActionInitialization.hh"

#include "PrimaryGeneratorAction.hh"
#include "PrimaryGeneratorAction0.hh"

#include "RunAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"
#include "TrackingAction.hh"

class ActionInitialization : public G4VUserActionInitialization
{
public:
        ActionInitialization();
        ~ActionInitialization();

        virtual void Build() const;
        virtual void BuildForMaster() const;
};
#endif
