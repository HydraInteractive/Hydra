// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/**
* Weapon stuff
*
* License: Mozilla Public License Version 2.0 (https://www.mozilla.org/en-US/MPL/2.0/ OR See accompanying file LICENSE)
* Authors:
*  - Dan Printzell
*/

#include <hydra/component/weaponcomponent.hpp>
#include <hydra/component/rigidbodycomponent.hpp>
#include <btBulletDynamicsCommon.h>
#include <hydra/engine.hpp>

#include <imgui/imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <random>

using namespace Hydra::World;
using namespace Hydra::Component;

using world = Hydra::World::World;

WeaponComponent::~WeaponComponent() { }

//TODO: (Re)move? to system?
void WeaponComponent::shoot(glm::vec3 position, glm::vec3 direction, glm::quat bulletOrientation, float velocity) {
	if (!_bullets)
		_bullets = world::newEntity("Bullets", entityID);

	if (fireRateTimer > 0)
		return;

	if (bulletSpread == 0.0f) {
		auto bullet = world::newEntity("Bullet", _bullets);
		bullet->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/SmallCargo.mATTIC");
		auto b = bullet->addComponent<Hydra::Component::BulletComponent>();
		b->direction = -direction;
		b->velocity = velocity;
		auto t = bullet->addComponent<Hydra::Component::TransformComponent>();
		t->position = position;
		t->scale = glm::vec3(bulletSize);
		t->rotation = bulletOrientation;

		auto bulletPhysWorld = static_cast<Hydra::System::BulletPhysicsSystem*>(IEngine::getInstance()->getState()->getPhysicsSystem());

		auto rbc = bullet->addComponent<Hydra::Component::RigidBodyComponent>();
		rbc->createBox(glm::vec3(0.5f), 1.0f);
		auto rigidBody = static_cast<btRigidBody*>(rbc->getRigidBody());
		bulletPhysWorld->enable(rbc.get());
		rigidBody->applyCentralForce(btVector3(b->direction.x, b->direction.y, b->direction.z) * 3000);
		rigidBody->setActivationState(DISABLE_DEACTIVATION);
		rigidBody->setGravity(btVector3(0, 0, 0));

	} else {
		for (int i = 0; i < bulletsPerShot; i++) {
			auto bullet = world::newEntity("Bullet", _bullets);
			bullet->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/SmallCargo.mATTIC");

			float phi = ((float)rand() / (float)(RAND_MAX)) * (2.0f*3.14f);
			float distance = ((float)rand() / (float)(RAND_MAX)) * bulletSpread;
			float theta = ((float)rand() / (float)(RAND_MAX)) * 3.14f;

			glm::vec3 bulletDirection = -direction;
			bulletDirection.x += distance * sin(theta) * cos(phi);
			bulletDirection.y += distance * sin(theta) * sin(phi);
			bulletDirection.z += distance * cos(theta);
			bulletDirection = glm::normalize(bulletDirection);

			auto b = bullet->addComponent<Hydra::Component::BulletComponent>();
			b->direction = bulletDirection;
			b->velocity = velocity;

			auto t = bullet->addComponent<Hydra::Component::TransformComponent>();
			t->position = position;
			t->scale = glm::vec3(bulletSize);
			t->rotation = bulletOrientation;

			auto bulletPhysWorld = static_cast<Hydra::System::BulletPhysicsSystem*>(IEngine::getInstance()->getState()->getPhysicsSystem());

			auto rbc = bullet->addComponent<Hydra::Component::RigidBodyComponent>();
			rbc->createBox(glm::vec3(0.5f), 1.0f);
			auto rigidBody = static_cast<btRigidBody*>(rbc->getRigidBody());
			bulletPhysWorld->enable(rbc.get());
			rigidBody->applyCentralForce(btVector3(b->direction.x, b->direction.y, b->direction.z) * 3000);
			rigidBody->setActivationState(DISABLE_DEACTIVATION);
			rigidBody->setGravity(btVector3(0,0,0));
		}
	}
	fireRateTimer = fireRateRPM / 60000.0f;

}

void WeaponComponent::serialize(nlohmann::json& json) const {
	json = {
		{ "fireRateRPM", fireRateRPM },
		{ "bulletSize", bulletSize},
		{ "bulletSpread", bulletSpread},
		{ "bulletsPerShot", bulletsPerShot}
	};
}

void WeaponComponent::deserialize(nlohmann::json& json) {
	fireRateRPM = json["fireRateRPM"].get<int>();
	bulletSize = json["bulletSize"].get<float>();
	bulletSpread = json["bulletSpread"].get<float>();
	bulletsPerShot = json["bulletsPerShot"].get<int>();
}

void WeaponComponent::registerUI() {
	ImGui::DragFloat("Fire Rate RPM", &fireRateRPM);
	ImGui::DragFloat("Bullet Size", &bulletSize, 0.001f);
	ImGui::DragFloat("Bullet Spread", &bulletSpread, 0.001f);
	ImGui::InputInt("Bullets Per Shot", &bulletsPerShot);
	ImGui::DragFloat("DEBUG", &debug);
}
