#include <hydra/system/playersystem.hpp>

#include <imgui/imgui.h>

#include <hydra/ext/openmp.hpp>

#include <hydra/component/playercomponent.hpp>
#include <hydra/component/transformcomponent.hpp>
#include <hydra/component/cameracomponent.hpp>
#include <hydra/component/weaponcomponent.hpp>

using namespace Hydra::System;
using namespace Hydra::Component;

enum Keys {
 H, F, COUNT
};

PlayerSystem::~PlayerSystem() {}

void PlayerSystem::tick(float delta) {
	using world = Hydra::World::World;
	static std::vector<std::shared_ptr<Entity>> entities;
	const Uint8* keysArray = SDL_GetKeyboardState(nullptr);
	bool prevKBFrameState[Keys::COUNT] = {false};

	//Process PlayerComponent
	world::getEntitiesWithComponents<PlayerComponent, TransformComponent, CameraComponent>(entities);
	#pragma omp parallel for
	for (int_openmp_t i = 0; i < (int_openmp_t)entities.size(); i++) {
		auto player = entities[i]->getComponent<PlayerComponent>();
		auto transform = entities[i]->getComponent<TransformComponent>();
		auto camera = entities[i]->getComponent<CameraComponent>();

		player->activeBuffs.onTick(player->maxHealth, player->health);

		if (player->health <= 0)
			player->isDead = true;

		auto weapon = player->getWeapon()->getComponent<Hydra::Component::WeaponComponent>();

		glm::mat4 rotation = glm::mat4_cast(camera->orientation);

		{
			player->velocity = glm::vec3{0};
			if (keysArray[SDL_SCANCODE_W])
				player->velocity.z -= player->movementSpeed;
			if (keysArray[SDL_SCANCODE_S])
				player->velocity.z += player->movementSpeed;

			if (keysArray[SDL_SCANCODE_A])
				player->velocity.x -= player->movementSpeed;
			if (keysArray[SDL_SCANCODE_D])
				player->velocity.x += player->movementSpeed;

			if (keysArray[SDL_SCANCODE_SPACE] && player->onGround){
				player->acceleration.y += 6.0f;
				player->onGround = false;
			}
			if (keysArray[SDL_SCANCODE_H] && !prevKBFrameState[Keys::H])
					player->upgradeHealth();

			if (keysArray[SDL_SCANCODE_F] && !prevKBFrameState[Keys::F]) {
				const glm::vec3 forward = glm::vec3(glm::vec4{0, 0, 1, 0} * rotation);
				auto abilitiesEntity = std::find_if(entities[i]->children.begin(), entities[i]->children.end(), [](Hydra::World::EntityID id) { return Hydra::World::World::getEntity(id)->name == "Abilities"; });
				player->activeAbillies.useAbility(Hydra::World::World::getEntity(*abilitiesEntity).get(), player->position, -forward);
			}

			if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT) && !ImGui::GetIO().WantCaptureMouse) {
				const glm::vec3 forward = glm::vec3(glm::vec4{0, 0, 1, 0} * rotation);

				//TODO: Make pretty?
				glm::quat bulletOrientation = glm::angleAxis(-camera->cameraYaw, glm::vec3(0, 1, 0)) * (glm::angleAxis(-camera->cameraPitch, glm::vec3(1, 0, 0)));
				float bulletVelocity = 1.0f;
				player->activeBuffs.onAttack(bulletVelocity);

				weapon->shoot(player->position, forward, bulletOrientation, bulletVelocity);
			}
		}

		player->acceleration.y -= 10.0f * delta;
		glm::vec4 movementVector = glm::vec4(player->velocity, 0) * rotation;
		movementVector.y = player->acceleration.y;

		player->position += glm::vec3(movementVector) * delta;

		if (player->position.y < 0) {
			player->position.y = 0;
			player->acceleration.y = 0;
			player->onGround = true;
		}

		if (player->firstPerson)
			camera->position = player->position;
		else
			camera->position = player->position + glm::vec3(0, 3, 0) + glm::vec3(glm::vec4{-4, 0, 4, 0} * rotation);

		transform->position = player->position;
		transform->rotation = camera->orientation;

		player->getWeapon()->getComponent<TransformComponent>()->position = player->position + glm::vec3(glm::vec4{player->weaponOffset, 0} * rotation);
		player->getWeapon()->getComponent<TransformComponent>()->rotation = glm::normalize(glm::conjugate(camera->orientation) * glm::quat(glm::vec3(glm::radians(180.0f), 0, glm::radians(180.0f))));
	}

	prevKBFrameState[Keys::H] = !!keysArray[SDL_SCANCODE_H];
	prevKBFrameState[Keys::F] = !!keysArray[SDL_SCANCODE_F];
}

void PlayerSystem::registerUI() {}
