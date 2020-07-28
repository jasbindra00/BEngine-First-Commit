#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include "Manager_GUI.h"
#include "Manager_Texture.h"
#include "Manager_Font.h"
#include "GUITextfield.h"
#include "Utility.h"
#include "FileReader.h"
#include "GameStateData.h"
#include "SharedContext.h"
#include "Window.h"
#include "GUIData.h"
#include "EventData.h"




using GameStateData::GameStateType;
using GUIData::GUITypeData::GUIType;
using namespace EventData;
using namespace GUIData;

Manager_GUI::Manager_GUI(SharedContext* cntxt) :context(cntxt) {
	RegisterElementProducer<GUITextfield>(GUIType::TEXTFIELD);
	stateinterfaces[GameStateType::GAME] = Interfaces{};
}
GUIStateStyles Manager_GUI::CreateStyleFromFile(const std::string& stylefile) {
	GUIStateStyles styles;
	FileReader file;
	if(!file.LoadFile(stylefile)){
		LOG::Log(LOCATION::MANAGER_GUI, LOGTYPE::ERROR, __FUNCTION__, "Unable to open the style file of name " + stylefile);
		return styles;
	}
	file.NextLine();
	while (!file.EndOfFile()) {
		auto currentstate = GUIState::NEUTRAL;{//conv scope to function
			auto currentword = file.GetWord();
			if (currentword == "GUIState") {
				unsigned int inputstate;
				file.GetLineStream() >> inputstate;
				currentstate = static_cast<GUIState>(inputstate);
			}
			else if (currentword == "/ENDSTATE") break;
			else { LOG::Log(LOCATION::MANAGER_GUI, LOGTYPE::ERROR, __FUNCTION__, "Unable to find the beginning of state style. Ensure that state style begins with GUIState x"); break;}
		}
		file.NextLine();
		auto word = file.GetWord();
		auto linestream = static_cast<Attributes*>(&file.GetLineStream());
 		while (word != "/ENDSTATE" && !file.EndOfFile()) {
			if (word == "sbg" || word == "tbg") *linestream >> styles[currentstate].background;
			else if (word == "text") *linestream >> styles[currentstate].text;
			else LOG::Log(LOCATION::MANAGER_GUI, LOGTYPE::ERROR, __FUNCTION__, "Unable to recognise state style attribute in stylefile of name " + stylefile + " on line number " + file.GetLineNumberString());
			file.NextLine();
			word = file.GetWord();
			//user may have accidentally forgotten to put in /ENDSTATE
			if (word == "GUIState") {
				LOG::Log(LOCATION::MANAGER_GUI, LOGTYPE::ERROR, __FUNCTION__, "Stylefile of name " + stylefile + " terminated early on linenumber " + file.GetLineNumberString() + " in reading state style - could not find /ENDSTATE ");
				linestream->PutBackPreviousWord();
				break;
			}
 		}	
	}
	file.CloseFile();
  	return styles;
}
GUIElementPtr Manager_GUI::CreateElement(GUIInterface* parent, Attributes& attributes){
	std::string elttype{ attributes.GetWord() };
	std::string stylefile{ attributes.GetWord() };
	if (elttype == "NEWINTERFACE" || elttype == "NESTEDINTERFACE") return std::make_unique<GUIInterface>(parent, this, CreateStyleFromFile(stylefile), attributes);
	else {
		auto guielementtype = GUIData::GUITypeData::converter(elttype);
		if (guielementtype == GUIType::NULLTYPE) {
			LOG::Log(LOCATION::MANAGER_GUI, LOGTYPE::ERROR, __FUNCTION__, "Unable to read the element type");
			return nullptr;
		}
		return elementfactory[guielementtype](parent, CreateStyleFromFile(stylefile), attributes);
	}
}
GUIInterfacePtr Manager_GUI::CreateInterfaceFromFile(const std::string& interfacefile) {
	FileReader file;
	if (!file.LoadFile(interfacefile)) {
		LOG::Log(LOCATION::MANAGER_GUI, LOGTYPE::ERROR, __FUNCTION__, "Unable to open the interface template file of name " + interfacefile);
		return nullptr;
	}
	std::vector<std::pair<std::string,std::vector<GUIElementPtr>>> interfacehierarchy;
	auto linestream = static_cast<Attributes*>(&file.GetLineStream());
	file.NextLine();{
		auto type = linestream->PeekWord();
		if (type != "NEWINTERFACE" || type == "NESTEDINTERFACE") { //cannot begin with nested interface.
			LOG::Log(LOCATION::MANAGER_GUI, LOGTYPE::ERROR, __FUNCTION__, "Unable to identify leading interface in interface file of name " + interfacefile + ". Interface files must start with 'INTERFACE' first)");
			return nullptr;
		}
	}
	file.PutBackLine();
	GUIInterface* leadinginterface{ nullptr };
	int ninterfaces{ 0 };
	while (!file.EndOfFile()) {
		file.NextLine();
		if (file.ReturnLine().empty()) continue;
		auto isnested = linestream->PeekWord();
		auto element = CreateElement(leadinginterface, *linestream);
		if (element == nullptr) continue;
		if (dynamic_cast<GUIInterface*>(element.get())) {
			interfacehierarchy.push_back(std::make_pair(isnested,std::vector<GUIElementPtr>{}));
			leadinginterface = static_cast<GUIInterface*>(element.get());
			interfacehierarchy[ninterfaces].second.emplace_back(std::move(element));
			++ninterfaces;
		}
		else interfacehierarchy[ninterfaces - 1].second.emplace_back(std::move(element));
	}
	GUIInterface* masterinterface = static_cast<GUIInterface*>(interfacehierarchy[0].second[0].get());
	leadinginterface = masterinterface;
	//link up all the individual interfaces to their elements.
	int j = 0;
	for (auto& structure : interfacehierarchy) {
		auto currentinterface = static_cast<GUIInterface*>(structure.second[0].get());
		if (structure.second.size() == 1) continue;
		for (int i = structure.second.size() - 1; i > 0; --i) { //loop throw 
			auto& element = structure.second[i];
			if (!currentinterface->AddElement(element->GetName(), element)) {
				element.reset();
				continue;
			}
		}
		//link up all the individual interfaces to each other.
		/*
		-if NEWINTERFACE, add relative to parent.
		-if NESTEDINTERFACE, add relative to the previously added interface.
		*/
		if (j > 0) { //if not at master interface
			bool successful;
			if (structure.first == "NEWINTERFACE") { //we need to add to the master interface. leading interface becomes this interface
				leadinginterface = static_cast<GUIInterface*>(structure.second[0].get()); 
				successful = masterinterface->AddElement(currentinterface->GetName(), structure.second[0]);}
			else if (structure.first == "NESTEDINTERFACE"){
				auto tmp = structure.second[0].get();
				successful = leadinginterface->AddElement(currentinterface->GetName(), structure.second[0]);
				leadinginterface = static_cast<GUIInterface*>(tmp);
			
			} //need to add to the previous interface
			if (!successful) {
				LOG::Log(LOCATION::MANAGER_GUI, LOGTYPE::ERROR, __FUNCTION__, "Ambiguous element name within interface file of name " + interfacefile);
				structure.second[0].reset();
			}
		}
		++j;
	}
	file.CloseFile();
	return std::unique_ptr<GUIInterface>(static_cast<GUIInterface*>(interfacehierarchy[0].second[0].release()));
}
std::pair<bool,Interfaces::iterator> Manager_GUI::FindInterface(const GameStateType& state, const std::string& interfacename) noexcept{
	auto& interfaces = stateinterfaces.at(state);
	auto foundinterface = std::find_if(interfaces.begin(), interfaces.end(), [interfacename](const auto& p) {
		return p.first == interfacename;
		});
	return (foundinterface == interfaces.end()) ? std::make_pair(false, foundinterface) : std::make_pair(true, foundinterface);
}
bool Manager_GUI::CreateStateInterface(const GameStateType& state, const std::string& name, const std::string& interfacefile){
	auto interfaceexists = FindInterface(state, name);
	if (interfaceexists.first == true) {
		LOG::Log(LOCATION::MANAGER_GUI, LOGTYPE::ERROR, __FUNCTION__, "Interface of name " + name + " within the game state " + std::to_string(Utility::ConvertToUnderlyingType(state)) + " already exists.");
		return false;
	}
	auto interfaceobj = CreateInterfaceFromFile(interfacefile);
	if (interfaceobj == nullptr) return false;
	stateinterfaces[state].emplace_back(std::make_pair(name, std::move(interfaceobj)));
	return true;
}
bool Manager_GUI::RemoveStateInterface(const GameStateType& state, const std::string& name){
	auto foundinterface = FindInterface(state, name);
	if (foundinterface.first == true) {
		LOG::Log(LOCATION::MANAGER_GUI, LOGTYPE::ERROR, __FUNCTION__, "State " + std::to_string(Utility::ConvertToUnderlyingType(state)) + " does not have an interface of name " + name);
		return false;
	}
	stateinterfaces.at(state).erase(foundinterface.second);
	return true;
}
GUIInterface* Manager_GUI::GetInterface(const GameStateType& state, const std::string& interfacename){
	auto foundinterface = FindInterface(state, interfacename);
	if (foundinterface.first == false) return nullptr;
	return foundinterface.second->second.get();
}
bool Manager_GUI::PollGUIEvent(GUIEventInfo& evnt){
	if (guieventqueue.PollEvent(evnt)) return true;
	return false;
}
void Manager_GUI::HandleEvent(const sf::Event& evnt, sf::RenderWindow* winptr) {
	auto evnttype = static_cast<EventType>(evnt.type);
	auto& activeinterfaces = stateinterfaces.at(activestate);
	switch (evnttype) {
	case EventType::MOUSEPRESSED: {
		auto clickcoords = sf::Vector2f{ static_cast<float>(evnt.mouseButton.x), static_cast<float>(evnt.mouseButton.y) };
		SetActiveTextfield(nullptr);
		for (auto& interfaceobj : activeinterfaces) {
			interfaceobj.second->DefocusTextfields();
			if (interfaceobj.second->Contains(clickcoords)) {
				if (interfaceobj.second->GetActiveState() == GUIState::NEUTRAL) {
					interfaceobj.second->OnClick(clickcoords);
				}
			}
			else if (interfaceobj.second->GetActiveState() == GUIState::FOCUSED) {
				interfaceobj.second->OnLeave();
			}
		}
	}
	case EventType::MOUSERELEASED: {
		for (auto& interfaceobj : activeinterfaces) {
			if (interfaceobj.second->GetActiveState() == GUIState::CLICKED) {
				interfaceobj.second->OnRelease();
			}
		}
	}
	case EventType::TEXTENTERED: {
		if (activetextfield != nullptr) {
			activetextfield->AppendChar(evnt.text.unicode);
			std::cout << activetextfield->GetStdString() << std::endl;
		}
	}
	case EventType::KEYPRESSED: {
		if (activetextfield != nullptr) {
			if (evnt.key.code == sf::Keyboard::Key::Backspace) {
				activetextfield->PopChar();
				std::cout << activetextfield->GetStdString() << std::endl;
			}
		}
	}
	}

}

void Manager_GUI::Update(const float& dT){
	globalmouseposition = static_cast<sf::Vector2f>(sf::Mouse::getPosition(*context->window->GetRenderWindow()));
	auto &stategui = stateinterfaces.at(activestate);
	for (auto& interfaceptr : stategui) {
		if (interfaceptr.second->GetName() == "fsdf") {
			interfaceptr.second->Update(dT);
			continue;
		}
		interfaceptr.second->Update(dT);
		
	}
}
void Manager_GUI::Draw() {
	auto& stategui = stateinterfaces.at(activestate);
	for (auto& interface : stategui) {
		interface.second->Render();
	}
}
void Manager_GUI::AddGUIEvent(const GUIEventInfo& evnt){
	guieventqueue.InsertEvent(evnt);
}


