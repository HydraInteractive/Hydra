#pragma once

#include <hydra/world/world.hpp>

namespace Hydra::System {
	class HYDRA_PHYSICS_API AbilitySystem final : public Hydra::World::ISystem {
	public:
		AbilitySystem();
		~AbilitySystem() final;

		void tick(float delta) final;

		inline const std::string type() const final { return "AbilitySystem"; }
		void registerUI() final;
	};
}
