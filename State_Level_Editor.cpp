#include "State_Level_Editor.h"
#include "Manager_GUI.h"
#include "GameStateData.h"
#include "Manager_Event.h"
#include "GUIButton.h"
#include "GUILabel.h"
#include "Window.h"



State_LevelEditor::State_LevelEditor(Manager_State* statemgr, Manager_GUI* guimanager):State_Base(statemgr,guimanager){
	guimanager->GetContext()->eventmanager->RegisterBindingCallable(GameStateData::GameStateType::LEVELEDITOR, "Activate_Pop_Up", [this](EventDetails* details) {this->ActivatePopUp(details); });
	guimanager->GetContext()->eventmanager->RegisterBindingCallable(GameStateData::GameStateType::LEVELEDITOR, "Deactivate_Pop_Up", [this](EventDetails* details) {this->DeactivatePopUp(details); });
	guimanager->GetContext()->eventmanager->RegisterBindingCallable(GameStateData::GameStateType::LEVELEDITOR, "Confirm_Spritesheet", [this](EventDetails* details) {this->ConfirmButtonPopUp(details); });
	guimgr->GetContext()->eventmanager->RegisterBindingCallable(GameStateData::GameStateType::LEVELEDITOR, "Move_Selector_Left", [this](EventDetails* details) {this->MoveSelectorLeft(details); });
	guimgr->GetContext()->eventmanager->RegisterBindingCallable(GameStateData::GameStateType::LEVELEDITOR, "Move_Selector_Right", [this](EventDetails* details) {this->MoveSelectorRight(details); });
	guimgr->GetContext()->eventmanager->RegisterBindingCallable(GameStateData::GameStateType::LEVELEDITOR, "Deactivate_Atlas_Map", [this](EventDetails* details) {this->DeactivateAtlasMap(details); });
	guimgr->GetContext()->eventmanager->RegisterBindingCallable(GameStateData::GameStateType::LEVELEDITOR, "Activate_Atlas_Map", [this](EventDetails* details) {this->ActivateAtlasMap(details); });
	guimgr->GetContext()->eventmanager->RegisterBindingCallable(GameStateData::GameStateType::LEVELEDITOR, "Crop_Sprite", [this](EventDetails* details) {this->CropSprite(details); });
}
void State_LevelEditor::Draw(sf::RenderTarget& target){
	GetInterface<RIGHT_PANEL>()->DrawToLayer(GUIData::GUILayerType::CONTENT, { &tile_selector });
	if (active_map == nullptr) return;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::X)) map_view.zoom(0.99);
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z)) map_view.zoom(1.01);
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) x_off += inc;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) x_off -= inc;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) y_off += inc;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) y_off -= inc;
	map_screen.setView(map_view);
	map_screen.clear(sf::Color::Red);
	for (unsigned int y = 0; y < active_map->map_dimensions.y; ++y) {
		for (unsigned int x = 0; x < active_map->map_dimensions.x; ++x) {
			//Iterate through each. Draw them onto the window.
			empty_tile.setPosition(sf::Vector2f{ static_cast<float>((x * active_map->tile_dimensions.x) + x_off), static_cast<float>((y * active_map->tile_dimensions.y) + y_off) });
			map_screen.draw(empty_tile);
		}
	}
	map_screen.display();
	target.draw(map_sprite);

}

void State_LevelEditor::Update(const float& dT){

}
void State_LevelEditor::Activate(){

}
void State_LevelEditor::Deactivate(){
}
void State_LevelEditor::OnCreate() {
	//Create the interfaces.
	interfaces.at(static_cast<unsigned int>(STATEINTERFACES::BOT_PANEL)) = guimgr->CreateInterfaceFromFile(GameStateData::GameStateType::LEVELEDITOR, "Interface_StateLevelEditor_Bot_Panel.txt");
	interfaces.at(static_cast<unsigned int>(STATEINTERFACES::POP_UP_PANEL)) = guimgr->CreateInterfaceFromFile(GameStateData::GameStateType::LEVELEDITOR, "Interface_StateLevelEditor_PopUp_Panel.txt");
	interfaces.at(static_cast<unsigned int>(STATEINTERFACES::TOP_PANEL)) = guimgr->CreateInterfaceFromFile(GameStateData::GameStateType::LEVELEDITOR, "Interface_StateLevelEditor_Top_Panel.txt");
	interfaces.at(static_cast<unsigned int>(STATEINTERFACES::RIGHT_PANEL)) = guimgr->CreateInterfaceFromFile(GameStateData::GameStateType::LEVELEDITOR, "Interface_StateLevelEditor_Right_Panel.txt");
	//Configure the textfield predicates.
	guimgr->GetElement<GUITextfield>(GameStateType::LEVELEDITOR, { "MAP_NAME", "POP_UP_PANEL" })->SetPredicates(static_cast<unsigned int>(Utility::CharacterCheckData::STRING_PREDICATE::FILE_NAME));
	guimgr->GetElement<GUITextfield>(GameStateType::LEVELEDITOR, { "SPRITE_SHEET_NAME", "POP_UP_PANEL" })->SetPredicates(static_cast<unsigned int>(Utility::CharacterCheckData::STRING_PREDICATE::FILE_NAME));
	guimgr->GetElement<GUITextfield>(GameStateType::LEVELEDITOR, { "TEXTURE_X_FIELD", "POP_UP_PANEL" })->SetPredicates(static_cast<unsigned int>(Utility::CharacterCheckData::STRING_PREDICATE::NUMBER));
	guimgr->GetElement<GUITextfield>(GameStateType::LEVELEDITOR, { "TEXTURE_Y_FIELD", "POP_UP_PANEL" })->SetPredicates(static_cast<unsigned int>(Utility::CharacterCheckData::STRING_PREDICATE::NUMBER));
	guimgr->GetElement<GUITextfield>(GameStateType::LEVELEDITOR, { "MAP_X_DIMENSION", "POP_UP_PANEL" })->SetPredicates(static_cast<unsigned int>(Utility::CharacterCheckData::STRING_PREDICATE::NUMBER));
	guimgr->GetElement<GUITextfield>(GameStateType::LEVELEDITOR, { "MAP_Y_DIMENSION", "POP_UP_PANEL" })->SetPredicates(static_cast<unsigned int>(Utility::CharacterCheckData::STRING_PREDICATE::NUMBER));

	guimgr->SetActiveInterfacesEnable(GetInterface<POP_UP_PANEL>(), false);
	tile_selector.setPosition(GetInterface<RIGHT_PANEL>()->GetElement<GUILabel>("ATLAS_VIEW")->GetGlobalPosition());
	tile_selector.setFillColor(sf::Color::Transparent);
	tile_selector.setOutlineColor(sf::Color::Green);
	tile_selector.setOutlineThickness(2);
	empty_tile.setFillColor(sf::Color::Color(200, 200, 200, 255));
	empty_tile.setOutlineColor(sf::Color::Color(0, 0, 0, 90));
	empty_tile.setOutlineThickness(2);
	empty_tile.setSize(sf::Vector2f{ 32,32 });

	//Initialise the Map Canvas
	auto win = guimgr->GetContext()->window->GetRenderWindow();
	using namespace GUILayerData;
	//Interface specific calculations for canvas positioning
	map_canvas = std::make_unique<GUILayerData::GUILayer>(GUILayerType::CONTENT, sf::Vector2f{ static_cast<float>(win->getSize().x * 0.75), static_cast<float>(win->getSize().y * 0.67) });
	map_canvas->SetPosition(sf::Vector2f{ 0, 0.08 * win->getSize().y });
	map_sprite.setTexture(map_screen.getTexture());
	map_sprite.setOrigin(sf::Vector2f{ static_cast<float>(map_sprite.getTextureRect().width / 2), static_cast<float>(map_sprite.getTextureRect().height / 2) });
	map_sprite.setPosition(sf::Vector2f{ static_cast<float>(win->getSize().x * 0.75 / 2), static_cast<float>(win->getSize().y * 0.67 / 2) });
	map_view = map_screen.getView();
}
void State_LevelEditor::OnDestroy(){

}
void State_LevelEditor::UpdateCamera()
{

}
void State_LevelEditor::Continue()
{

}
UserMap* State_LevelEditor::CreateNewMap(const std::string& map_name, const sf::Vector2u& map_tile_dimensions,const sf::Vector2i& tiledimensions, std::shared_ptr<sf::Texture> tile_sheet){
	auto new_map = std::make_unique<UserMap>(map_tile_dimensions, tiledimensions,tile_sheet);
	auto ptr = new_map.get();
	user_maps[map_name] = std::move(new_map);
	active_map = ptr;
	return ptr;
}
void State_LevelEditor::ActivatePopUp(EventData::EventDetails* details){
	GUIInterface* panel = guimgr->GetInterface(GameStateData::GameStateType::LEVELEDITOR, "POP_UP_PANEL");
	panel->SetHidden(false);
	guimgr->SetActiveInterfacesEnable(panel, false);
}
void State_LevelEditor::DeactivatePopUp(EventData::EventDetails* details) {
	GetInterface<POP_UP_PANEL>()->SetHidden(true);
	guimgr->SetActiveInterfacesEnable(GetInterface<POP_UP_PANEL>(), true);
}
void State_LevelEditor::ConfirmButtonPopUp(EventData::EventDetails* details){

	auto POP_UP = GetInterface<POP_UP_PANEL>();

	//Check if the input map name is available.
	if (std::string map_name = POP_UP->GetElement<GUITextfield>("MAP_NAME")->GetTextfieldString(); !map_name.empty() && user_maps.find(map_name) == user_maps.end()) {
		//Pull the map X dimension.
//Pull the map Y dimension.
		auto map_x_field = POP_UP->GetElement<GUITextfield>("MAP_X_DIMENSION");
		if (auto map_x_dimension_string = map_x_field->GetTextfieldString(); !map_x_dimension_string.empty() && map_x_dimension_string != map_x_field->GetDefaultTextfieldString()) {
			auto map_y_field = POP_UP->GetElement<GUITextfield>("MAP_Y_DIMENSION");
			if (auto map_y_dimension_string = map_y_field->GetTextfieldString(); !map_y_dimension_string.empty() && map_y_dimension_string != map_y_field->GetDefaultTextfieldString()) {
				unsigned int map_x_dimension = std::stoi(map_x_dimension_string);
				unsigned int map_y_dimension = std::stoi(map_y_dimension_string);
				//Check if the Texture_X atlas dimension is not defaulted, and is below MAX_TILE_X_DIMENSION.
				auto texture_x_field = POP_UP->GetElement<GUITextfield>("TEXTURE_X_FIELD");
				if (std::string texture_x_field_string = texture_x_field->GetTextfieldString(); !texture_x_field_string.empty() && texture_x_field_string != texture_x_field->GetDefaultTextfieldString()) {
					if (auto texture_x_dimension = std::stoi(texture_x_field_string); texture_x_dimension <= MAX_TILE_DIMENSION) {
						//Check if the Texture_Y atlas dimension is not defaulted, and is below MAX_TILE_Y_DIMENSION
						auto texture_y_field = POP_UP->GetElement<GUITextfield>("TEXTURE_Y_FIELD");
						if (auto texture_y_field_string = texture_y_field->GetTextfieldString();  !texture_y_field_string.empty() && texture_y_field_string != texture_y_field->GetDefaultTextfieldString()) {
							if (auto texture_y_dimension = std::stoi(texture_y_field_string); texture_y_dimension <= MAX_TILE_DIMENSION) {
								if (std::string atlas_map_name = POP_UP->GetElement<GUITextfield>("SPRITE_SHEET_NAME")->GetTextfieldString(); !atlas_map_name.empty()) {
									//Check if the atlas map is a valid and registered resource.
									if (std::shared_ptr<sf::Texture> atlas_sheet = LoadSheet(atlas_map_name); atlas_sheet != nullptr) {
										UserMap* newmap = CreateNewMap(map_name, sf::Vector2u{ std::move(map_x_dimension), std::move(map_y_dimension) }, sf::Vector2i{ std::move(texture_x_dimension), std::move(texture_y_dimension) }, std::move(atlas_sheet));
										//Configure the tile selector sizes based on the user input.
										//Atlas map is scaled to fit the ATLAS_VIEW element. We must find the tile dimension relative to this element -> this is the selector size.
										sf::Vector2f atlas_view_size{ GetInterface<RIGHT_PANEL>()->GetElement<GUILabel>("ATLAS_VIEW")->GetSize() };
										auto atlas_actual_size = static_cast<sf::Vector2i>(active_map->tile_sheet->getSize());
										tile_selector.setSize(sf::Vector2f{ (atlas_view_size.x / atlas_actual_size.x) * active_map->tile_dimensions.x, (atlas_view_size.y / atlas_actual_size.y) * active_map->tile_dimensions.y });
										DeactivatePopUp(details);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
//we need to toggle 
//AND || && SUPPORT TO EVENT MANAGER!!!!
void State_LevelEditor::MoveSelectorLeft(EventData::EventDetails* details){
	if (GetInterface<RIGHT_PANEL>()->GetElement<GUILabel>("ATLAS_VIEW")->GetActiveState() != GUIState::NEUTRAL) SetSelectorPosition(true);
}
sf::Vector2i State_LevelEditor::GetSelectorAtlasCoords() {
	//The position of the selector is relative to the right panel.
	//Find the position of the tile selector relative to the atlas_view.
	sf::Vector2f atlas_coords = tile_selector.getPosition() - GetInterface<RIGHT_PANEL>()->GetElement<GUILabel>("ATLAS_VIEW")->GetLocalPosition();
	//Divide by the scaled selector size to get the atlas co-ordinates.
	atlas_coords.x = std::ceil(atlas_coords.x / tile_selector.getSize().x);
	atlas_coords.y   = std::ceil(atlas_coords.y / tile_selector.getSize().y);
	return static_cast<sf::Vector2i>(atlas_coords);
}
void State_LevelEditor::SetSelectorPosition(const bool& left) {
	//ATLAS_VIEW can only be manipulated when it has been clicked by the user.
	auto ATLAS_VIEW = GetInterface<RIGHT_PANEL>()->GetElement<GUILabel>("ATLAS_VIEW");
	if (ATLAS_VIEW->GetActiveState() == GUIState::NEUTRAL) return;
	//Limit the speed as the cycle speed is too fast.
	{
		sf::Clock c;
		while (c.getElapsedTime().asSeconds() < 0.17) {}
	}
	//Convert the tile_selector position into tile_sheet array co-ordinates.
	auto atlas_max_tile_dimensions = sf::Vector2i{ static_cast<int>(ATLAS_VIEW->GetSize().x / tile_selector.getSize().x), static_cast<int>(ATLAS_VIEW->GetSize().y / tile_selector.getSize().y) };
	auto tile_selector_atlas_pos = GetSelectorAtlasCoords();
	//Move the selector left or right, and wrap it such that it traverses the atlas map in a contiguous fashion.
	if (left) {
		tile_selector_atlas_pos.x -= 1;
		if (tile_selector_atlas_pos.x < 0) {
			if (tile_selector_atlas_pos.y == 0) return; //At top left of atlas map.
			else {
				tile_selector_atlas_pos.x = atlas_max_tile_dimensions.x - 1;
				tile_selector_atlas_pos.y -= 1;
			}
		}
	}
	else {
		tile_selector_atlas_pos.x += 1;
		if (tile_selector_atlas_pos.x > atlas_max_tile_dimensions.x - 1) {
			if (tile_selector_atlas_pos.y == atlas_max_tile_dimensions.y - 1) return; //At bottom right of atlas map.
			else {
				tile_selector_atlas_pos.x = 0;
				tile_selector_atlas_pos.y += 1;
			}
		}
	}
	//Change the texture rect of the ACTIVE_TILE element to reflect the tile which lies underneath the selector.
	sf::Vector2i atlas_sheet_coords{ static_cast<int>(tile_selector_atlas_pos.x * active_map->tile_dimensions.x), static_cast<int>(tile_selector_atlas_pos.y * active_map->tile_dimensions.y) };
	GetInterface<RIGHT_PANEL>()->GetElement<GUILabel>("ACTIVE_TILE")->GetActiveStyle().ReadIn(STYLE_ATTRIBUTE::BG_TEXTURE_RECT, sf::IntRect{ std::move(atlas_sheet_coords), active_map->tile_dimensions });
	tile_selector.setPosition(sf::Vector2f{ static_cast<float>(tile_selector_atlas_pos.x * tile_selector.getSize().x), static_cast<float>(tile_selector_atlas_pos.y * tile_selector.getSize().y) } + ATLAS_VIEW->GetLocalPosition());
	//Eye candy : change the color of the selector based on whether it has already been cropped or not.
	unsigned int selector_array_position = AtlasCoordToArray(tile_selector_atlas_pos);
	//Check if the array_position has been registered as a TileID already.
	auto tile_registered = std::find_if(active_map->registered_tiles.begin(), active_map->registered_tiles.end(), [&selector_array_position](const std::pair<DefaultProperties, Tile>& p) {
		return p.first.master_tile_reference == selector_array_position;
		});
	if (tile_registered == active_map->registered_tiles.end()) tile_selector.setOutlineColor(sf::Color::Green);
	else tile_selector.setOutlineColor(sf::Color::Red);
}
void State_LevelEditor::MoveSelectorRight(EventData::EventDetails* details){//CHANGE EVENT MANAGER TO SUPPORT &| BINDINGS
	if (GetInterface<RIGHT_PANEL>()->GetElement<GUILabel>("ATLAS_VIEW")->GetActiveState() != GUIState::NEUTRAL) SetSelectorPosition(false);
}
std::shared_ptr<sf::Texture> State_LevelEditor::LoadSheet(const std::string& sheetname){
	std::shared_ptr<sf::Texture> atlas_map_ptr = guimgr->context->GetResourceManager<sf::Texture>()->RequestResource(sheetname);
	if (atlas_map_ptr == nullptr) return nullptr;
	auto atlas_view = GetInterface<RIGHT_PANEL>()->GetElement<GUILabel>("ATLAS_VIEW");
	for (int i = 0; i < 3; ++i) {
		atlas_view->GetStyle(static_cast<GUIState>(i)).ReadIn(STYLE_ATTRIBUTE::BG_TEXTURE_NAME, sheetname);
		atlas_view->GetStyle(static_cast<GUIState>(i)).ReadIn(STYLE_ATTRIBUTE::BG_TEXTURE_RECT, sf::IntRect{ sf::Vector2i{0,0}, static_cast<sf::Vector2i>(atlas_map_ptr->getSize()) });
	}
	GetInterface<RIGHT_PANEL>()->GetElement<GUILabel>("ACTIVE_TILE")->GetActiveStyle().ReadIn(STYLE_ATTRIBUTE::BG_TEXTURE_NAME, sheetname);
	return atlas_map_ptr;
}
void State_LevelEditor::ActivateAtlasMap(EventData::EventDetails* details){
	SetSelectorPosition(true);
	SetSelectorPosition(false);
}
void State_LevelEditor::DeactivateAtlasMap(EventData::EventDetails* details){
	tile_selector.setOutlineColor(sf::Color::Transparent);
}
void State_LevelEditor::CropSprite(EventData::EventDetails* details){
	//If the color of the selector is red, then the tile has been cropped already.
	if (tile_selector.getOutlineColor() == sf::Color::Red) return;
	 //Transform the selector co-ordinates into atlas map array co-ordinates.
	auto tile_sheet_size = static_cast<sf::Vector2i>(active_map->tile_sheet->getSize());
	sf::Vector2i atlas_unit_dimensions{ static_cast<int>(tile_sheet_size.x / active_map->tile_dimensions.x), tile_sheet_size.y / active_map->tile_dimensions.y };
	sf::Vector2i selector_atlas_coords = GetSelectorAtlasCoords();
	auto selector_array_coords = AtlasCoordToArray(selector_atlas_coords);
	
	Tile x(active_map->tile_sheet.get(), sf::IntRect{ sf::Vector2i{selector_atlas_coords.x * active_map->tile_dimensions.x, selector_atlas_coords.y * active_map->tile_dimensions.y}, active_map->tile_dimensions });
	active_map->registered_tiles.emplace_back(std::make_pair(DefaultProperties(std::move(selector_array_coords)), std::move(x)));
	//Tile has been successfully cropped.
	tile_selector.setOutlineColor(sf::Color::Red);
}
void State_LevelEditor::UnCropSprite(EventData::EventDetails* details){

}
