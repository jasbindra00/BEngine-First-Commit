#ifndef STATE_GAME_H
#define STATE_GAME_H
#include "State_Base.h"

struct EventDetails;
class State_Game : public State_Base
{
protected:
	using State_Base::transcendency;
	using State_Base::transparency;
	using State_Base::statemgr;
public:
	State_Game(Manager_State* statemanager);
	virtual void Update(const float& dT) override;
	virtual void draw(sf::RenderTarget& target, sf::RenderStates state) const override;
	virtual void OnCreate() override;
	virtual void OnDestroy() override;
	virtual void Activate() override;
	virtual void Deactivate() override;
	virtual void Continue() override;
	virtual void UpdateCamera() override;
	virtual ~State_Game() override;
	void KeyPress(EventDetails* details);
};




#endif