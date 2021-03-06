#pragma once

#include <hydra/world/world.hpp>
#include <hydra/component/perkcomponent.hpp>


namespace Hydra::System {
	class HYDRA_PHYSICS_API PerkSystem final : public Hydra::World::ISystem{
	public:
		PerkSystem();
		~PerkSystem() final;
		 
		void tick(float delta) final;
		void onPickUp(Hydra::Component::PerkComponent::Perk newPerk, const std::shared_ptr<Hydra::World::Entity>& playerEntity);


		inline const std::string type() const final { return "PerkSystem"; }
		void registerUI() final;


		struct ReadBullet
		{
			enum bulletMesh {BULLET, STAR, TRIDENT, BANANA, DUCK };
			bulletMesh mesh = bulletMesh::BULLET;
			
			float dmg = 0.0f;
			float recoil = 0.0f;
			float ammoCap = 0.0f;
			float bulletSpread = 0.0f;
			float bulletSize = 0.5f;
			float roundsPerMinute = 0.0f;
			float reloadTime = 0.0f;
			float glowIntensity = 0.0f;

			int meshType = 0;

			float bulletColor[4] = { 1.0f };
			int currentMagAmmo = 0;
			int bulletPerShot = 0;
			int ammoPerShot = 0;

			std::string perkDescription;

			bool Multiplier = true;
			bool Adder = false;
			bool glow = false;

		};
		void readFromFile(const char* fileName, ReadBullet &readBullet);
		void PerkChange(ReadBullet& b, const std::shared_ptr<Hydra::World::Entity>& playerEntity);
		bool switchMesh = false;


	private:
		float perkDescriptionTimer = 0;
		std::string perkDescriptionText = "";
	};
}
