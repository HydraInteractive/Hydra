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
#include <hydra/component/particlecomponent.hpp>
#include <hydra/component/bulletcomponent.hpp>
#include <hydra/component/lifecomponent.hpp>
#include <btBulletDynamicsCommon.h>

using namespace Hydra::System;

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

void BulletPhysicsSystem::enable(Hydra::Component::RigidBodyComponent* component) {
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
	default:
		_data->dynamicsWorld->addRigidBody(rigidBody, COLL_NOTHING, COLL_NOTHING);
		break;
	}
}

void BulletPhysicsSystem::disable(Hydra::Component::RigidBodyComponent* component) {
	_data->dynamicsWorld->removeRigidBody(static_cast<btRigidBody*>(component->getRigidBody()));
	component->_handler = nullptr;
}

void BulletPhysicsSystem::tick(float delta) {
	_data->dynamicsWorld->stepSimulation(delta);
	// Gets all collisions happening between all rigidbody entities.
	int numManifolds = _data->dynamicsWorld->getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; i++) {
		btPersistentManifold* contactManifold = _data->dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
		const btCollisionObject* obA = contactManifold->getBody0();
		const btCollisionObject* obB = contactManifold->getBody1();
		Entity* eA = Hydra::World::World::getEntity(obA->getUserIndex()).get();
		Entity* eB = Hydra::World::World::getEntity(obB->getUserIndex()).get();
		if (!eA || !eB)
			continue;

		auto entityBC = eA->getComponent<Hydra::Component::BulletComponent>();
		if (!entityBC)
			if (!(entityBC = eB->getComponent<Hydra::Component::BulletComponent>()))
				continue;

		//btManifoldPoint& pt = contactManfiold->getContactPoint(0);
		// Gets the contact points
		//btManifoldPoint& pt = contactManifold->getContactPoint(0);
		//btVector3 collPosA = pt.getPositionWorldOnA();
		//_spawnParticleEmitterAt(collPosA);
		int numContacts = contactManifold->getNumContacts();
		for (int j = 0; j < numContacts; j++) {
			btManifoldPoint& pt = contactManifold->getContactPoint(j);
			btVector3 collPosB = pt.getPositionWorldOnB();
			btVector3 normalOnB = pt.m_normalWorldOnB;
			_spawnParticleEmitterAt(collPosB, normalOnB);
			
			// Set the bullet entity to dead.
			World::World::World::getEntity(entityBC->entityID)->dead = true;
			// Breaks because just wanna check the first collision.
			break;
		}
	}
}

void BulletPhysicsSystem::_spawnParticleEmitterAt(btVector3 pos, btVector3 normal) {
	auto pE = Hydra::World::World::newEntity("Collision Particle Spawner", Hydra::World::World::rootID);

	pE->addComponent<Hydra::Component::MeshComponent>()->loadMesh("QUAD");
	
	auto pETC = pE->addComponent<Hydra::Component::TransformComponent>();
	pETC->position = glm::vec3(pos.getX(), pos.getY(), pos.getZ());

	auto pEPC = pE->addComponent<Hydra::Component::ParticleComponent>();
	pEPC->delay = 1.0f / 1.0f;
	pEPC->accumulator = 5.0f;
	pEPC->behaviour = Hydra::Component::ParticleComponent::EmitterBehaviour::Explosion;
	pEPC->texture = Hydra::Component::ParticleComponent::ParticleTexture::Blood;
	pEPC->optionalNormal = glm::vec3(normal.getX(), normal.getY(), normal.getZ());

	auto pELC = pE->addComponent<Hydra::Component::LifeComponent>();
	pELC->maxHP = 0.9f;
	pELC->health = 0.9f;
}

void BulletPhysicsSystem::registerUI() {}
