#pragma once
/**
* Player stuff
*
* License: Mozilla Public License Version 2.0 (https://www.mozilla.org/en-US/MPL/2.0/ OR See accompanying file LICENSE)
* Authors:
*  - Dan Printzell
*/

#pragma once
#include <hydra/ext/api.hpp>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <hydra/world/world.hpp>
#include <hydra/component/transformcomponent.hpp>
#include <hydra/component/cameracomponent.hpp>

using namespace Hydra::World;

namespace Hydra::Component {
	class HYDRA_API PlayerComponent final : public IComponent{
	public:
		PlayerComponent(IEntity* entity);
		~PlayerComponent() final;

		void tick(TickAction action) final;
		// If you want to add more than one TickAction, combine them with '|' (The bitwise or operator) 
		inline TickAction wantTick() const final { return TickAction::physics; }

		inline const std::string type() const final { return "PlayerComponent"; }

		void serialize(nlohmann::json& json) const final;
		void deserialize(nlohmann::json& json) final;
		void registerUI() final;
	private:
		glm::vec3 _playerPos;
		float _velocityX;
		float _velocityY;
		float _velocityZ;

		int _a = 0;
		bool _b = false;
		float _c = 0.0f;
	};
};