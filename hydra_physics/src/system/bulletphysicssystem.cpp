// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/**
 * Interface to manage the Bullet3 physics library.
 *
 * License: Mozilla Public License Version 2.0 (https://www.mozilla.org/en-US/MPL/2.0/ OR See accompanying file LICENSE)
 * Authors:
 *  - Dan Printzell
 */
#include <hydra/system/bulletphysicssystem.hpp>

#include <memory>

#include <hydra/component/rigidbodycomponent.hpp>
#include <hydra/component/ghostobjectcomponent.hpp>
#include <hydra/component/particlecomponent.hpp>
#include <hydra/component/bulletcomponent.hpp>
#include <hydra/component/lifecomponent.hpp>
#include <hydra/component/playercomponent.hpp>
#include <hydra/component/pickupcomponent.hpp>
#include <hydra/component/perkcomponent.hpp>
#include <hydra/component/textcomponent.hpp>
#include <hydra/component/ghostobjectcomponent.hpp>
#include <hydra/component/aicomponent.hpp>
#include <hydra/component/meshcomponent.hpp>
#include <hydra/component/spawnercomponent.hpp>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

inline static btQuaternion cast(const glm::quat& r) { return btQuaternion{r.x, r.y, r.z, r.w}; }
inline static btVector3 cast(const glm::vec3& v) { return btVector3{v.x, v.y, v.z}; }

inline static glm::quat cast(const btQuaternion& r) { return glm::quat(r.w(), r.x(), r.y(), r.z()); }
inline static glm::vec3 cast(const btVector3& v) { return glm::vec3(v.x(), v.y(), v.z()); }

using namespace Hydra::System;
using namespace Hydra::Component;

struct BulletPhysicsSystem::Data {
	std::unique_ptr<btDefaultCollisionConfiguration> config;
	std::unique_ptr<btCollisionDispatcher> dispatcher;
	std::unique_ptr<btBroadphaseInterface> broadphase;
	std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
	std::unique_ptr<btDiscreteDynamicsWorld> dynamicsWorld;
};

BulletPhysicsSystem::BulletPhysicsSystem() {
	_data = new Data;
	_data->config = std::make_unique<btDefaultCollisionConfiguration>();
	_data->dispatcher = std::make_unique<btCollisionDispatcher>(_data->config.get());
	_data->broadphase = std::make_unique<btDbvtBroadphase>();
	_data->solver = std::make_unique<btSequentialImpulseConstraintSolver>();
	_data->dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(_data->dispatcher.get(), _data->broadphase.get(), _data->solver.get(), _data->config.get());
	_data->dynamicsWorld->setGravity(btVector3(0, -10, 0));
}

BulletPhysicsSystem::~BulletPhysicsSystem() { delete _data; }

void BulletPhysicsSystem::enable(RigidBodyComponent* component) {
	if (component->_handler)
		return;
	component->_handler = this;
	// Make so addRigidbody takes in collision filter group and what that group collides with.
	btRigidBody* rigidBody = static_cast<btRigidBody*>(component->getRigidBody());
	switch (rigidBody->getUserIndex2())
	{
	case CollisionTypes::COLL_PLAYER:
		_data->dynamicsWorld->addRigidBody(rigidBody, COLL_PLAYER, CollisionCondition::playerCollidesWith);
		break;
	case CollisionTypes::COLL_ENEMY:
		_data->dynamicsWorld->addRigidBody(rigidBody, COLL_ENEMY, CollisionCondition::enemyCollidesWith);
		break;
	case CollisionTypes::COLL_WALL:
		_data->dynamicsWorld->addRigidBody(rigidBody, COLL_WALL, CollisionCondition::wallCollidesWith);
		break;
	case CollisionTypes::COLL_PLAYER_PROJECTILE:
		_data->dynamicsWorld->addRigidBody(rigidBody, COLL_PLAYER_PROJECTILE, CollisionCondition::playerProjCollidesWith);
		break;
	case CollisionTypes::COLL_ENEMY_PROJECTILE:
		_data->dynamicsWorld->addRigidBody(rigidBody, COLL_ENEMY_PROJECTILE, CollisionCondition::enemyProjCollidesWith);
		break;
	case CollisionTypes::COLL_MISC_OBJECT:
		_data->dynamicsWorld->addRigidBody(rigidBody, COLL_MISC_OBJECT, CollisionCondition::miscObjectCollidesWith);
		break;
	case CollisionTypes::COLL_PICKUP_OBJECT:
		_data->dynamicsWorld->addRigidBody(rigidBody, COLL_PICKUP_OBJECT, CollisionCondition::pickupObjectCollidesWith);
		break;
	case CollisionTypes::COLL_FLOOR:
		_data->dynamicsWorld->addRigidBody(rigidBody, COLL_FLOOR, CollisionCondition::floorCollidesWith);
		break;
	case CollisionTypes::COLL_SPAWNER:
		_data->dynamicsWorld->addRigidBody(rigidBody, COLL_SPAWNER, CollisionCondition::spawnerCollidesWith);
		break;
	default:
		_data->dynamicsWorld->addRigidBody(rigidBody, COLL_NOTHING, COLL_NOTHING);
		break;
	}
}

void BulletPhysicsSystem::disable(RigidBodyComponent* component) {
	if (!component->_handler)
		return;
	_data->dynamicsWorld->removeRigidBody(static_cast<btRigidBody*>(component->getRigidBody()));
	component->_handler = nullptr;
}

void Hydra::System::BulletPhysicsSystem::enable(GhostObjectComponent * component){
	component->_handler = this;

	switch (component->ghostObject->getUserIndex2())
	{
	case CollisionTypes::COLL_PLAYER:
		_data->dynamicsWorld->addCollisionObject(component->ghostObject, COLL_PLAYER, CollisionCondition::playerCollidesWith);
		break;
	case CollisionTypes::COLL_ENEMY:
		_data->dynamicsWorld->addCollisionObject(component->ghostObject, COLL_ENEMY, CollisionCondition::enemyCollidesWith);
		break;
	case CollisionTypes::COLL_WALL:
		_data->dynamicsWorld->addCollisionObject(component->ghostObject, COLL_WALL, CollisionCondition::wallCollidesWith);
		break;
	case CollisionTypes::COLL_PLAYER_PROJECTILE:
		_data->dynamicsWorld->addCollisionObject(component->ghostObject, COLL_PLAYER_PROJECTILE, CollisionCondition::playerProjCollidesWith);
		break;
	case CollisionTypes::COLL_ENEMY_PROJECTILE:
		_data->dynamicsWorld->addCollisionObject(component->ghostObject, COLL_ENEMY_PROJECTILE, CollisionCondition::enemyProjCollidesWith);
		break;
	case CollisionTypes::COLL_MISC_OBJECT:
		_data->dynamicsWorld->addCollisionObject(component->ghostObject, COLL_MISC_OBJECT, CollisionCondition::miscObjectCollidesWith);
		break;
	case CollisionTypes::COLL_PICKUP_OBJECT:
		_data->dynamicsWorld->addCollisionObject(component->ghostObject, COLL_PICKUP_OBJECT, CollisionCondition::pickupObjectCollidesWith);
		break;
	case CollisionTypes::COLL_FLOOR:
		_data->dynamicsWorld->addCollisionObject(component->ghostObject, COLL_FLOOR, CollisionCondition::floorCollidesWith);
		break;
	case CollisionTypes::COLL_SPAWNER:
		_data->dynamicsWorld->addCollisionObject(component->ghostObject, COLL_SPAWNER, CollisionCondition::spawnerCollidesWith);
		break;
	//case CollisionTypes::COLL_BOSSHAND:
	//	_data->dynamicsWorld->addCollisionObject(component->ghostObject, COLL_BOSSHAND, CollisionCondition::enemyCollidesWith);
	//	break;
	default:
		_data->dynamicsWorld->addCollisionObject(component->ghostObject, COLL_NOTHING, COLL_NOTHING);
		break;
	}
}

void Hydra::System::BulletPhysicsSystem::disable(GhostObjectComponent * component){
	_data->dynamicsWorld->removeCollisionObject(component->ghostObject);
	component->_handler = nullptr;
}

void* Hydra::System::BulletPhysicsSystem::rayTestFromTo(const glm::vec3& from, const glm::vec3& to)
{
	btVector3 playerPos(cast(from));
	btVector3 directionBoi(cast(to));
	btCollisionWorld::ClosestRayResultCallback* callback = new btCollisionWorld::ClosestRayResultCallback(playerPos, directionBoi);

	_data->dynamicsWorld->rayTest(playerPos, directionBoi, *callback);
	return callback;
}

void BulletPhysicsSystem::tick(float delta) {
	_data->dynamicsWorld->stepSimulation(delta, 3);
	// Gets all collisions happening between all rigidbody entities.
	//_data->dynamicsWorld->stepSimulation(delta, 3, btScalar(1.0) / btScalar(180.));
	int numManifolds = _data->dynamicsWorld->getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; i++) {
		btPersistentManifold* contactManifold = _data->dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
		const btCollisionObject* obA = contactManifold->getBody0();
		const btCollisionObject* obB = contactManifold->getBody1();
		Entity* eA = Hydra::World::World::getEntity(obA->getUserIndex()).get();
		Entity* eB = Hydra::World::World::getEntity(obB->getUserIndex()).get();

		if (!eA || !eB)
			continue;

		BulletComponent* bulletComponent = nullptr;
		LifeComponent* lifeComponent = nullptr;
		PlayerComponent* playerComponent = nullptr;
		PickUpComponent* pickupComponent = nullptr;
		PerkComponent* perkComponent = nullptr;

		if ((bulletComponent = eA->getComponent<BulletComponent>().get()))
			lifeComponent = eB->getComponent<LifeComponent>().get();
		else if ((bulletComponent = eB->getComponent<BulletComponent>().get()))
			lifeComponent = eA->getComponent<LifeComponent>().get();

		playerComponent = eA->getComponent<PlayerComponent>().get();
		if (!playerComponent)
			playerComponent = eB->getComponent<PlayerComponent>().get();

		if ((pickupComponent = eA->getComponent<PickUpComponent>().get()))
			perkComponent = eB->getComponent<PerkComponent>().get();
		else if ((pickupComponent = eB->getComponent<PickUpComponent>().get()))
			perkComponent = eA->getComponent<PerkComponent>().get();

		if (pickupComponent && perkComponent) { // What does this code do? - Stevan.
			if (IEngine::getInstance()->getDeadSystem())
				_spawnText(Hydra::World::World::getEntity(playerComponent->entityID)->getComponent<Hydra::Component::TransformComponent>()->position, "eyyy\n");
			
			_addPickUp(pickupComponent, perkComponent);
		}

		// Gets the contact points
		int numContacts = contactManifold->getNumContacts();
		for (int j = 0; j < numContacts; j++) {
			auto entityAMesh = eA->getComponent<Hydra::Component::MeshComponent>();
			auto entityBMesh = eB->getComponent<Hydra::Component::MeshComponent>();

			btManifoldPoint& pt = contactManifold->getContactPoint(j);
			btVector3 collPosB = pt.getPositionWorldOnB();
			btVector3 normalOnB = pt.m_normalWorldOnB;

			if (playerComponent && normalOnB.y() > 0.7){
				playerComponent->onGround = true;
				if (obA->getUserIndex2() == COLL_FLOOR || obB->getUserIndex2() == COLL_FLOOR) {
					playerComponent->onFloor = true;
				}
				else {
					playerComponent->onFloor = false;
				}
			}
			if (lifeComponent) {
				// Terrible HeadShot code.
				auto rgbcA = eA->getComponent<Hydra::Component::RigidBodyComponent>();
				bool headshot = false;
				float accumulatedDamage = bulletComponent->damage;
				glm::vec3 textColor = { 1.0,1.0,1.0 };
				glm::vec3 textScale = { 10,10,10 };
				Hydra::Component::ParticleComponent::ParticleTexture particleTexture;
				auto compound = static_cast<btCompoundShape*>(static_cast<btRigidBody*>(rgbcA->getRigidBody())->getCollisionShape());
				if (compound->getNumChildShapes() > 1) {
					glm::vec3 child1Pos = rgbcA->getPosition(0);
					glm::vec3 child2Pos = rgbcA->getPosition(1);
					float distance1 = glm::distance(child1Pos, cast(collPosB));
					float distance2 = glm::distance(child2Pos, cast(collPosB));
					if (distance2 < distance1) {
						accumulatedDamage *= 2.0f; // 2.0 multiplier on HS.
						textColor = glm::vec3(4, 0.5, 0.5);
						textScale = glm::vec3(15, 15, 15);
						headshot = true;
					}
				}

				if (!Hydra::IEngine::getInstance()->getDeadSystem()) {
					lifeComponent->applyDamage(accumulatedDamage);
				}

				auto targetEntity = Hydra::World::World::getEntity(lifeComponent->entityID);
				auto aiComponent = targetEntity->getComponent<Hydra::Component::AIComponent>();

				if (!targetEntity->getComponent<Hydra::Component::PlayerComponent>() && aiComponent) {
					_spawnText(cast(collPosB), std::to_string(accumulatedDamage), textColor, textScale);
					switch (aiComponent->behaviour->type) {
					case Hydra::Physics::Behaviour::Behaviour::Type::ALIEN: {
						if (headshot)
							particleTexture = Hydra::Component::ParticleComponent::ParticleTexture::AlienHS;
						else
							particleTexture = Hydra::Component::ParticleComponent::ParticleTexture::AlienBlood;
						_spawnParticleEmitterAt(cast(collPosB), cast(normalOnB), particleTexture);
						break;
					}
					case Hydra::Physics::Behaviour::Behaviour::Type::ROBOT: {
						if (headshot)
							particleTexture = Hydra::Component::ParticleComponent::ParticleTexture::AlienHS;
						else
							particleTexture = Hydra::Component::ParticleComponent::ParticleTexture::Energy;
						_spawnParticleEmitterAt(cast(collPosB), cast(normalOnB), particleTexture);
						break;
					}
					default:
						break;
					}
				} else if (targetEntity->getComponent<SpawnerComponent>()) {
					_spawnText(cast(collPosB), std::to_string(accumulatedDamage), textColor, textScale);
					particleTexture = Hydra::Component::ParticleComponent::ParticleTexture::Energy;
					_spawnParticleEmitterAt(cast(collPosB), cast(normalOnB), particleTexture);
				}
			}

			// Set the bullet entity to dead.
			if (bulletComponent)
				World::World::World::getEntity(bulletComponent->entityID)->dead = true;

			// Breaks because just wanna check the first collision point.
			break;
		}
	}

	entities.clear();
}

void BulletPhysicsSystem::_spawnParticleEmitterAt(const glm::vec3& pos, const glm::vec3& normal, const Hydra::Component::ParticleComponent::ParticleTexture& effect) {
	auto pE = Hydra::World::World::newEntity("Collision Particle Spawner", Hydra::World::World::rootID);

	pE->addComponent<MeshComponent>()->loadMesh("PARTICLEQUAD");

	auto pETC = pE->addComponent<TransformComponent>();
	pETC->position = pos;

	auto pEPC = pE->addComponent<Hydra::Component::ParticleComponent>();
	pEPC->delay = 1.0f / 1.0f;
	pEPC->accumulator = 4;
	pEPC->tempVelocity = glm::vec3(6.0f, 6.0f, 6.0f);
	pEPC->behaviour = Hydra::Component::ParticleComponent::EmitterBehaviour::Explosion;
	pEPC->texture = effect;
	pEPC->optionalNormal = normal;

	auto pELC = pE->addComponent<LifeComponent>();
	pELC->maxHP = pELC->health = 0.9f;
}

void BulletPhysicsSystem::_spawnText(const glm::vec3& pos, const std::string& text, const glm::vec3& color, const glm::vec3& scale) {
	auto textEntity = world::newEntity("Damage", world::root());
	auto transC = textEntity->addComponent<TransformComponent>();
	transC->setPosition(pos);
	transC->setScale(scale);
	textEntity->addComponent<MeshComponent>()->loadMesh("TEXTQUAD");
	auto lifeC = textEntity->addComponent<LifeComponent>();
	lifeC->health = lifeC->maxHP = 1;
	auto textC = textEntity->addComponent<TextComponent>();
	//char buff[64];
	//snprintf(buff, sizeof(buff), "%.0f\x01\x02", text);
	textC->setText(text.substr(0, 2));
	textC->isStatic = false;
	textC->color = color;
}


void Hydra::System::BulletPhysicsSystem::_addPickUp(PickUpComponent * pickupComponent, PerkComponent * perkComponent) {


	switch (pickupComponent->pickUpType)
	{
	case PickUpComponent::PICKUP_RANDOMPERK: {
		std::vector<int> perksNotFound;
		
		for (size_t i = 0; i < perkComponent->AMOUNTOFPERKS; i++){
			bool perkFound = false;
			for (size_t j = 0; j < perkComponent->activePerks.size(); j++){
				if (PerkComponent::Perk(i) == perkComponent->activePerks[j]){
					perkFound = true;
					j = perkComponent->activePerks.size();
				}
			}
			for (size_t j = 0; j < perkComponent->newPerks.size(); j++){
				if (PerkComponent::Perk(i) == perkComponent->newPerks[j]) {
					perkFound = true;
					j = perkComponent->newPerks.size();
				}
			}
			if (!perkFound)
				perksNotFound.push_back(i);
		}

		if (!perksNotFound.empty()){
			int newPerk = rand() % (perksNotFound.size());
			perkComponent->newPerks.push_back(PerkComponent::Perk(perksNotFound[newPerk]));
		}
		if(IEngine::getInstance()->getDeadSystem())
			World::World::World::getEntity(pickupComponent->entityID)->dead = true;
	}
		break;
	case PickUpComponent::PICKUP_HEALTH: {

	}
		break;
	case PickUpComponent::PICKUP_AMMO: {

	}
		break;
	default:
		break;
	}

	if (!IEngine::getInstance()->getDeadSystem()) {

		void(*p)(EntityID id);

		void *tmp = (IEngine::getInstance()->getState()->getTextureLoader());
		typedef void(*fptr)(EntityID);
		fptr my_fptr = reinterpret_cast<fptr>(reinterpret_cast<long long>(tmp));

		my_fptr(pickupComponent->entityID);
	}

}

void BulletPhysicsSystem::registerUI() {}
