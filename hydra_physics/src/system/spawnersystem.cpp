#include "hydra/system/spawnersystem.hpp"

#include <hydra/ext/openmp.hpp>

#include <hydra/component/lifecomponent.hpp>
#include <hydra/component/transformcomponent.hpp>
#include <hydra/component/spawnercomponent.hpp>
#include <hydra/component/aicomponent.hpp>
#include <hydra/component/movementcomponent.hpp>
#include <hydra/component/meshcomponent.hpp>
#include <hydra/component/weaponcomponent.hpp>
#include <hydra/component/rigidbodycomponent.hpp>
#include <hydra/engine.hpp>

#include <btBulletDynamicsCommon.h>

using namespace Hydra::System;

SpawnerSystem::SpawnerSystem() {}

SpawnerSystem::~SpawnerSystem() {}

void SpawnerSystem::tick(float delta)
{
	using world = Hydra::World::World;

	//Process SystemComponent
	world::getEntitiesWithComponents<Component::SpawnerComponent, Component::LifeComponent, Component::TransformComponent>(entities);
	for (int_openmp_t i = 0; i < (int_openmp_t)entities.size(); i++) {
		auto life = entities[i]->getComponent<Component::LifeComponent>();
		auto transform = entities[i]->getComponent<Component::TransformComponent>();
		auto spawner = entities[i]->getComponent<Component::SpawnerComponent>();

		spawner->spawnTimer += delta;

		switch (spawner->spawnerID)
		{
		case Component::SpawnerType::AlienSpawner:
		{
			if (spawner->spawnGroup.size() <= 4)
			{
				if (spawner->spawnTimer >= 5)
				{
					auto alienSpawn = world::newEntity("AlienSpawn", world::root());
					auto a = alienSpawn->addComponent <Hydra::Component::AIComponent> ();
					a->behaviour = std::make_shared<AlienBehaviour>(alienSpawn);
					a->damage = 4;
					a->behaviour->originalRange = 4;
					a->radius = 2.0f;

					auto h = alienSpawn->addComponent<Hydra::Component::LifeComponent>();
					h->maxHP = 80;
					h->health = 80;

					auto m = alienSpawn->addComponent<Hydra::Component::MovementComponent>();
					m->movementSpeed = 8.0f;

					auto t = alienSpawn->addComponent<Hydra::Component::TransformComponent>();
					t->position = transform->position;
					t->scale = glm::vec3{ 2,2,2 };

					auto bulletPhysWorld = static_cast<Hydra::System::BulletPhysicsSystem*>(IEngine::getInstance()->getState()->getPhysicsSystem());

					auto rgbc = alienSpawn->addComponent<Hydra::Component::RigidBodyComponent>();
					rgbc->createBox(glm::vec3(0.5f) * t->scale, glm::vec3(0), Hydra::System::BulletPhysicsSystem::CollisionTypes::COLL_ENEMY, 100.0f);
					static_cast<btRigidBody*>(rgbc->getRigidBody())->setActivationState(DISABLE_DEACTIVATION);
					bulletPhysWorld->enable(rgbc.get());
					alienSpawn->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/characters/RobotModel.mATTIC");
					spawner->spawnGroup.push_back(alienSpawn);
					spawner->spawnTimer = 0;
				}
			}
		}break;
		case Component::SpawnerType::RobotSpawner:
		{
			if (spawner->spawnGroup.size() <= 4)
			{
				if (spawner->spawnTimer >= 5)
				{
					auto robotSpawn = world::newEntity("RobotSpawn", world::root());
					auto a = robotSpawn->addComponent<Hydra::Component::AIComponent>();
					a->behaviour = std::make_shared<AlienBehaviour>(robotSpawn);
					a->damage = 7;
					a->behaviour->originalRange = 18;
					a->radius = 1.0f;

					auto h = robotSpawn->addComponent<Hydra::Component::LifeComponent>();
					h->maxHP = 60;
					h->health = 60;

					auto m = robotSpawn->addComponent<Hydra::Component::MovementComponent>();
					m->movementSpeed = 4.0f;

					auto t = robotSpawn->addComponent<Hydra::Component::TransformComponent>();
					t->position = transform->position;
					t->scale = glm::vec3{ 1,1,1 };

					{
						auto weaponEntity = world::newEntity("Weapon", robotSpawn);
						weaponEntity->addComponent<Hydra::Component::WeaponComponent>();
					}

					robotSpawn->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/Gun.mATTIC");
					spawner->spawnGroup.push_back(robotSpawn);
					spawner->spawnTimer = 0;
				}
			}
		}break;
		}

	}

	entities.clear();
}

void SpawnerSystem::registerUI() {}
