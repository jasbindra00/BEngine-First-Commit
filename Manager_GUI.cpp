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
#include <array>



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
	while (!file.NextLine().GetFileStream()) {
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
GUIElementPtr Manager_GUI::CreateElement(GUIInterface* parent, const Keys& keys){
	using KeyProcessing::KeyPair;
	std::string elttype = keys.at("ELEMENTTYPE");
	std::string stylefile = keys.at("STYLEFILE");
	if (elttype == "NEWINTERFACE" || elttype == "NESTEDINTERFACE") return std::make_unique<GUIInterface>(parent, this, CreateStyleFromFile(stylefile), keys);
	else {
		auto guielementtype = GUIData::GUITypeData::converter(elttype);
		if (guielementtype == GUIType::NULLTYPE) throw CustomException("ELEMENTTYPE");
		return elementfactory[guielementtype](parent, CreateStyleFromFile(stylefile), keys);
	}
}
GUIInterfacePtr Manager_GUI::CreateInterfaceFromFile(const std::string& interfacefile) {
	using KeyProcessing::KeyPair;
	using KeyProcessing::Keys;
	std::string appenderrorstr{ " in interface file of name " + interfacefile+ ". " };
	FileReader file;
	if (!file.LoadFile(interfacefile)) {
		LOG::Log(LOCATION::MANAGER_GUI, LOGTYPE::ERROR, __FUNCTION__, "Unable to open the interface template file of name " + interfacefile);
		return nullptr;
	}
	std::vector<std::pair<std::string,std::vector<GUIElementPtr>>> interfacehierarchy;
	auto linestream = static_cast<Attributes*>(&file.GetLineStream());
	//check if the first entry is a new interface. 
	file.NextLine();
		auto type = linestream->PeekWord();
		if (type != "{ELEMENTTYPE,NEWINTERFACE}" || type == "{ELEMENTTYPE,NESTEDINTERFACE}") { //cannot begin with nested interface.
			LOG::Log(LOCATION::MANAGER_GUI, LOGTYPE::ERROR, __FUNCTION__, "Unable to identify leading interface" + appenderrorstr + "Leading interfaces must start with INTERFACE. EXITING INTERFACE READ..");
			return nullptr;
		}
	
	file.PutBackLine();
	GUIInterface* leadinginterface{ nullptr };
	int ninterfaces{ 0 };

	while (file.NextLine().GetFileStream()) {
		Keys linekeys = KeyProcessing::ExtractValidKeys(file.ReturnLine());
		KeyProcessing::FillMissingKeys(std::vector<KeyPair>{ {"ELEMENTTYPE", "ERROR"}, { "STYLEFILE", "ERROR" }, { "ELEMENTNAME", "ERROR" },
			{ "POSITIONX","0" }, { "POSITIONY","0" }, { "SIZEX","50" }, { "SIZEY","50" }}, linekeys);
		{
			bool err = false;
			for (auto& key : linekeys) {
				if (key.second == "ERROR") {
					LOG::Log(LOCATION::MAP, LOGTYPE::ERROR, __FUNCTION__, "Invalid essential key " + KeyProcessing::ConstructKeyStr(key.first, key.second) + " for GUIElement initialisation on line " + file.GetLineNumberString() + appenderrorstr+ "DID NOT READ ELEMENT..");
					err = true;
				}
			}
			if (err) continue;
		}
		GUIElementPtr element;
		try { element = CreateElement(leadinginterface, std::move(linekeys)); }
		catch (const CustomException& exception) {
			if (std::string{ exception.what() } == "ELEMENTTYPE") {
				LOG::Log(LOCATION::MANAGER_GUI, LOGTYPE::ERROR, __FUNCTION__, "Unable to read the GUIElement on line " + file.GetLineNumberString() + appenderrorstr + "DID NOT READ ELEMENT..");
			}
			continue;
		}
		if (dynamic_cast<GUIInterface*>(element.get())) {//if the element is an interface
			std::string elttype = linekeys.at("ELEMENTTYPE");
			//create a new structure line
			interfacehierarchy.push_back(std::make_pair(elttype,std::vector<GUIElementPtr>{}));
			//this is now the leading interface. subsequent nested interfaces will be relative to this one.
			leadinginterface = static_cast<GUIInterface*>(element.get());
			//add the interface as the first element within its structure
			interfacehierarchy[ninterfaces].second.emplace_back(std::move(element));
			++ninterfaces;
		}
		//if its an element, add the element to the lastmost active interface structure
		else interfacehierarchy[ninterfaces - 1].second.emplace_back(std::move(element));
	}
	GUIInterface* masterinterface = static_cast<GUIInterface*>(interfacehierarchy[0].second[0].get());
	leadinginterface = masterinterface;
	//link up all the individual interfaces to their elements.
	int interfacenum = 0; //interface
	for (auto& structure : interfacehierarchy) {
		auto currentinterface = static_cast<GUIInterface*>(structure.second[0].get());
		if (structure.second.size() == 1) continue; //the structure has only a single interface. no elt linkage required
		for (int i = structure.second.size() - 1; i > 0; --i) { //now, loop through all of the elements (coming after the first interface entry within the structure)
			auto& element = structure.second[i];
			if (!currentinterface->AddElement(element->GetName(), element)) { 
				LOG::Log(LOCATION::MANAGER_GUI, LOGTYPE::ERROR, __FUNCTION__, "Unable to add the GUIElement of name " + element->GetName() + " to interface of name " + currentinterface->GetName() + " in line " + file.GetLineNumberString() + " in interface file of name " + interfacefile);
				element.reset();
				continue;
			}
		}
		//link this interface up to its parent interface.
		if (interfacenum > 0) { //if not at master interface
			bool successful;
			if (structure.first == "NEWINTERFACE") { 
				leadinginterface = static_cast<GUIInterface*>(structure.second[0].get());  //this is now the leading interface. subsequent nested interfaces are linked to this.
				successful = masterinterface->AddElement(currentinterface->GetName(), structure.second[0]);} //new interfaces are added to the master interface. technically nested relative to the master
			else if (structure.first == "NESTEDINTERFACE"){
				auto tmp = structure.second[0].get(); 
				successful = leadinginterface->AddElement(currentinterface->GetName(), structure.second[0]);
				leadinginterface = static_cast<GUIInterface*>(tmp);
			}
			if (!successful) {
				LOG::Log(LOCATION::MANAGER_GUI, LOGTYPE::ERROR, __FUNCTION__, "Ambiguous element name within interface file of name " + interfacefile);
				structure.second[0].reset();
			}
		}
		++interfacenum;
	}
	return std::unique_ptr<GUIInterface>(static_cast<GUIInterface*>(interfacehierarchy[0].second[0].release())); //return the master interface.
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


