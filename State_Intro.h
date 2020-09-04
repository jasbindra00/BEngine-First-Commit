#ifndef STATE_INTRO_H
#define STATE_INTRO_H
#include "State_Base.h"
#include <SFML/Graphics.hpp>


class State_Intro : public State_Base {
protected:
	using State_Base::state_view;
	using State_Base::transparency;
	using State_Base::transcendency;
	
	sf::RectangleShape shape;
	sf::Clock tick;
public:
	State_Intro(Manager_State* statemgr, Manager_GUI* guimgr);
	void OnDestroy() override;
	void OnCreate() override;
	void Update(const float& dT) override;
	void Draw(sf::RenderTarget& target) override;
	void UpdateCamera() override;
	void Continue() override;
	void Activate() override;
	void Deactivate() override;

};


#endif