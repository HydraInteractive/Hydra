#include <barcode/componentmenu.hpp>

#include <hydra/component/meshcomponent.hpp>
#include <hydra/component/cameracomponent.hpp>
#include <hydra/component/playercomponent.hpp>
#include <hydra/component/weaponcomponent.hpp>
#include <hydra/component/particlecomponent.hpp>
#include <hydra/component/aicomponent.hpp>
#include <hydra/component/lifecomponent.hpp>
#include <hydra/component/lightcomponent.hpp>
#include <hydra/component/pointlightcomponent.hpp>
#include <hydra/component/movementcomponent.hpp>
#include <hydra/component/lifecomponent.hpp>
#include <hydra/component/roomcomponent.hpp>
#include <glm/gtc/type_ptr.hpp>
using world = Hydra::World::World;
ComponentMenu::ComponentMenu()
{
	_entities = std::vector<std::weak_ptr<Hydra::World::Entity>>();
}

ComponentMenu::~ComponentMenu()
{

}

void ComponentMenu::render(bool &openBool, Hydra::System::BulletPhysicsSystem& physicsSystem)
{
	ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiSetCond_Once);
	ImGui::Begin("Add component", &openBool, ImGuiWindowFlags_MenuBar);
	_menuBar();
	ImGui::Columns(3, "Columns");
	ImGui::Text("Select entity");
	for (size_t i = 0; i < _entities.size(); i++)
	{
		if (_entities[i].expired())
		{
			_entities.erase(_entities.begin() + i);
		}
		else if (ImGui::MenuItem(_entities[i].lock()->name.c_str(), "", (_selectedEntity.lock() == _entities[i].lock())))
		{
			_selectedEntity = _entities[i];
			_selectedString = "";
		}
	}
	ImGui::NextColumn();
	ImGui::Text("Select component type");
	if (!_selectedEntity.expired())
	{
		for (size_t i = 0; i < _componentTypes.size(); i++)
		{
			if (ImGui::MenuItem(_componentTypes[i].c_str(), "", (_selectedString == _componentTypes[i])))
			{
				_selectedString = _componentTypes[i];
			}
		}
	}

	ImGui::NextColumn();
	ImGui::Text("Configure component");
	if (_selectedString != "" && !_selectedEntity.expired())
	{
		configureComponent(openBool, _selectedString, physicsSystem);
	}
	ImGui::End();
}

void ComponentMenu::refresh()
{
	_entities.clear();
	auto& entityIDs = getRoomEntity()->children;
	for (size_t i = 0; i < entityIDs.size(); i++)
	{
		_entities.push_back(world::getEntity(entityIDs[i]));
	}
	_selectedEntity = std::weak_ptr<Hydra::World::Entity>();
	_selectedString = "";
}

std::shared_ptr<Hydra::World::Entity> ComponentMenu::getRoomEntity()
{
	std::vector<std::shared_ptr<Hydra::World::Entity>> entities;
	world::getEntitiesWithComponents<Hydra::Component::TransformComponent, Hydra::Component::RoomComponent>(entities);
	if (entities.size() > 0)
	{
		return entities[0];
	}
	return nullptr;
}

void ComponentMenu::configureComponent(bool &openBool, std::string componentType, Hydra::System::BulletPhysicsSystem& physicsSystem)
{
	if (componentType == "Transform")
	{
		if (_selectedEntity.lock()->hasComponent<Hydra::Component::TransformComponent>())
		{
			ImGui::Text("The entity selected already has this component");
		}
		else 
		{
			ImGui::BeginChild("Transform", ImVec2(ImGui::GetWindowContentRegionWidth() *0.3f, ImGui::GetWindowContentRegionMax().y - 160), true);
			ImGui::DragFloat3("Position", glm::value_ptr(transformInput.position));
			ImGui::DragFloat3("Scale", glm::value_ptr(transformInput.scale));
			ImGui::DragFloat4("Rotation", glm::value_ptr(transformInput.rotation));
			ImGui::Checkbox("Ignore parent", &transformInput.ignoreParent);
			ImGui::EndChild();
			ImGui::BeginChild("Confirm", ImVec2(ImGui::GetWindowContentRegionWidth() *0.3f, 25));
			if (ImGui::Button("Finish"))
			{
				auto t = _selectedEntity.lock()->addComponent<Hydra::Component::TransformComponent>();
				t->position = transformInput.position;
				t->scale = transformInput.scale;
				t->rotation = transformInput.rotation;
				t->ignoreParent = transformInput.ignoreParent;
				transformInput = TI();
				openBool = false;
			}
			ImGui::EndChild();
		}
	}
	else if (componentType == "PointLight")
	{
		if (_selectedEntity.lock()->hasComponent<Hydra::Component::PointLightComponent>())
		{
			ImGui::Text("The entity selected already has this component");
		}
		else
		{
			ImGui::BeginChild("Point light", ImVec2(ImGui::GetWindowContentRegionWidth() *0.3f, ImGui::GetWindowContentRegionMax().y - 160), true);
			ImGui::DragFloat3("Colour", glm::value_ptr(pointLightInput.colour), 0.01f);
			ImGui::DragFloat("Constant", &pointLightInput.constant, 0.0001f);
			ImGui::DragFloat("Linear", &pointLightInput.linear, 0.01f);
			ImGui::DragFloat("Quadratic", &pointLightInput.quadratic, 0.0001f);
			ImGui::EndChild();
			ImGui::BeginChild("Confirm", ImVec2(ImGui::GetWindowContentRegionWidth() *0.3f, 25));
			if (ImGui::Button("Finish"))
			{
				auto p = _selectedEntity.lock()->addComponent<Hydra::Component::PointLightComponent>();
				p->color = pointLightInput.colour;
				p->constant = pointLightInput.constant;
				p->linear = pointLightInput.linear;
				p->quadratic = pointLightInput.quadratic;
				pointLightInput = PLI();
				openBool = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset"))
			{

			}
			ImGui::EndChild();
		}
	}

	else if (componentType == "RigidBody")
	{
		if (_selectedEntity.lock()->hasComponent<Hydra::Component::RigidBodyComponent>())
		{
			ImGui::Text("The entity selected already has this component");
		}
		else
		{
			ImGui::BeginChild("RigidBody", ImVec2(ImGui::GetWindowContentRegionWidth() *0.3f, ImGui::GetWindowContentRegionMax().y - 160), true);
			ImGui::DragFloat3("Size", glm::value_ptr(rigidBodyInput.size), 0.01f);
			//ImGui::Checkbox("Ignore parent", &transformInput.ignoreParent);

			//TODO: Selection box for picking collision type
			//TODO: Float input for mass, linear dampening, angular dampening, friction, rolling friction
			ImGui::EndChild();
			ImGui::BeginChild("Confirm", ImVec2(ImGui::GetWindowContentRegionWidth() *0.3f, 25));
			if (ImGui::Button("Finish"))
			{
				auto& t = _selectedEntity.lock()->addComponent<Hydra::Component::RigidBodyComponent>();
				//physicsBox->addComponent<Hydra::Component::RigidBodyComponent>()->createBox(t->scale * 10.0f, Hydra::System::BulletPhysicsSystem::CollisionTypes::COLL_MISC_OBJECT, 10, 0, 0, 1.0f, 1.0f);
				t->createBox(rigidBodyInput.size, Hydra::System::BulletPhysicsSystem::CollisionTypes::COLL_WALL, 100);
				physicsSystem.enable(t.get());
				RBI();
				openBool = false;
			}
			ImGui::EndChild();
		}
	}
}

void ComponentMenu::_menuBar()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Menu"))
		{
			if (ImGui::MenuItem("Refresh", NULL))
			{
				refresh();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}
