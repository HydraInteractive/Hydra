#include <hydra/system/lightsystem.hpp>

#include <hydra/component/lightcomponent.hpp>

LightSystem::~LightSystem() {}

void LightSystem::tick(float delta) {
	using world = Hydra::World::World;
	static std::vector<std::shared_ptr<Entity>> entities;

	//Process LightComponent
	world::getEntitiesWithComponents<Hydra::Component::LightComponent, Hydra::Component::TransformComponent>(entities);
	#pragma omp parallel for
	for (size_t i = 0; i < entities.size(); i++) {
		auto l = entities[i]->getComponent<Hydra::Component::LightComponent>();
		auto t = entities[i]->getComponent<Hydra::Component::TransformComponent>();
		l->position = t->position;
	}
}

void LightSystem::registerUI() {}