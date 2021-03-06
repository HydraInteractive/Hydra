#pragma once
#include <hydra/ext/api.hpp>
#include <hydra/world/world.hpp>
#include <hydra/renderer/renderer.hpp>

#include <memory>
#include <glm/gtx/transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <hydra/component/transformcomponent.hpp>

using namespace Hydra::World;

// TODO: Implement LOD
namespace Hydra::Component {
	struct HYDRA_GRAPHICS_API ParticleComponent final : public IComponent<ParticleComponent, ComponentBits::Particle> {

		enum class EmitterBehaviour : int { PerSecond = 0, Explosion, SpawnerBeam, MAX_COUNT };
		static constexpr const char* EmitterBehaviourStr[] = { "PerSecond", "Explosion", "SpawnerBeam" };
		enum class ParticleTexture : int { Energy = 0, AlienBlood, Blood, BloodierBlood, AlienHS, Spawner, MAX_COUNT };
		static constexpr const char* ParticleTextureStr[] = { "Energy", "AlienBlood", "Blood", "AlienHS", "Spawner" };

		struct HYDRA_GRAPHICS_API Particle {
			TransformComponent transform;
			glm::vec3 velocity = glm::vec3{0, 0, 0};
			glm::vec3 acceleration = glm::vec3{0, 0, 0}; // TODO: Drop?
			float life = 0.0f;
			float startLife = 0.0f;
			glm::vec2 texOffset1 = glm::vec2(0.0f,0.0f);
			glm::vec2 texOffset2 = glm::vec2(0.0f,0.0f);
			glm::vec2 texCoordInfo = glm::vec2(0.0f,0.0f);
			void respawn(TransformComponent t, glm::vec3 vel, glm::vec3 acc, float l) {
				transform = t;
				// transform.ignoreParent = true;
				velocity = vel;
				acceleration = acc;
				life = startLife = l;
			}
			inline glm::mat4 getMatrix() { return transform.getMatrix(); }
		};

		static constexpr size_t TextureOuterGrid = 6; // 6x6
		static constexpr size_t TextureInnerGrid = 4; // 4x4
		static constexpr float ParticleSize = 1.0f / (TextureInnerGrid * TextureOuterGrid);
		static constexpr size_t MaxParticleAmount = 256;

		float delay = 1.0f; // 0.1 = 10 Particle/Second
		float accumulator = 256.0f;
		glm::vec3 tempVelocity = glm::vec3(1.0f, 1.0f, 1.0f);
		EmitterBehaviour behaviour = EmitterBehaviour::PerSecond;
		ParticleTexture texture = ParticleTexture::Energy;
		Particle particles[MaxParticleAmount];
		glm::vec3 optionalNormal = glm::vec3(0.0f,0.0f,0.0f);

		~ParticleComponent() final;

		void spawnParticles();

		inline const std::string type() const final { return "ParticleComponent"; }

		void serialize(nlohmann::json& json) const final;
		void deserialize(nlohmann::json& json) final;
		void registerUI() final;
	};
};
