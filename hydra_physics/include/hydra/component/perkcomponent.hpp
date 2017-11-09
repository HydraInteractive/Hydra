/**
* Perk stuff
*
* License: Mozilla Public License Version 2.0 (https://www.mozilla.org/en-US/MPL/2.0/ OR See accompanying file LICENSE)
* Authors:
*  - Dan Printzell
*/
#pragma once
#include <hydra/ext/api.hpp>
#include <hydra/world/world.hpp>
#include <hydra/abilities/abilities.hpp>

using namespace Hydra::World;

enum Perk
{
	PERK_MAGNETICBULLETS,
	PERK_HOMINGBULLETS,
	PERK_GRENADE,
	PERK_MINE,
	PERK_FORCEPUSH,
	PERK_BULLETSPRAY
};

namespace Hydra::Component {
	struct HYDRA_PHYSICS_API PerkComponent final : public IComponent<PerkComponent, ComponentBits::Perk>{
		std::vector<Perk> newPerks;
		std::vector<Perk> activePerks;
		std::vector<BaseAbility*> activeAbilities;
		size_t activeAbility = 0;
		bool usedAbilityLastFrame = 0;

		~PerkComponent() final;
		inline const std::string type() const final { return "PerkComponent"; }
		void serialize(nlohmann::json& json) const final;
		void deserialize(nlohmann::json& json) final;
		void registerUI() final;
	};
};
