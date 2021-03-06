#pragma once
#include <hydra/pathing/pathfinding.hpp>
#include <hydra/world/world.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <hydra/component/componentmanager.hpp>
#include <hydra/component/rigidbodycomponent.hpp>
#include <hydra/ext/api.hpp>
#include <memory>
#include <random>

namespace Hydra::Physics::Behaviour {
	class HYDRA_PHYSICS_API Behaviour
	{
	public:
		Behaviour(std::shared_ptr<Hydra::World::Entity> enemy);
		Behaviour();
		virtual ~Behaviour();

		enum class Type { ALIEN, ROBOT, BOSS_HAND, BOSS_ARMS, STATINARY_BOSS };
		Type type = Type::ALIEN;

		enum { IDLE, SEARCHING, MOVING, ATTACKING };
		unsigned int state = IDLE;

		enum BossPhase { CLAWING, SPITTING, SPAWNING, CHILLING };
		BossPhase bossPhase = BossPhase::CLAWING;

		enum HandPhases { IDLEHAND, SMASH, SWIPE, HANDCANON, COVER, RETURN };
		HandPhases handPhase = HandPhases::RETURN;

		enum ArmPhases { CHILL, AIM, SHOOT };
		ArmPhases armPhase = ArmPhases::CHILL;

		enum StatinoaryBossPhases { NOTHING, SPAWN };
		StatinoaryBossPhases stationaryPhase = StatinoaryBossPhases::NOTHING;

		float idleTimer = 0.0;
		float attackTimer = 0.0;
		float newPathTimer = 0.0;
		float newPathDelay = 1.0;
		float spawnTimer = 0.0;
		float phaseTimer = 0.0;
		float regainRange = 0.0;

		std::random_device rd;
		bool playerSeen = false;
		bool isAtGoal = false;
		int oldMapPosX = 0;
		int oldMapPosZ = 0;

		int handID = 0;
		bool hasRequiredComponents = false;

		bool playerUnreachable = false;
		PathFinding* pathFinding = nullptr;
		bool doDiddeliDoneDatPathfinding = false;

		float range = 1.0f;
		float savedRange = 4.0f;
		float originalRange = 1.0f;
		glm::quat rotation = glm::quat();

		virtual void run(float dt) = 0;
		void setEnemyEntity(std::shared_ptr<Hydra::World::Entity> enemy);
		void setTargetPlayer(std::shared_ptr<Hydra::World::Entity> player);
		virtual void setPathMap(bool** map);
	protected:
		struct ComponentSet
		{
			Hydra::World::Entity* entity = nullptr;
			Hydra::Component::TransformComponent* transform = nullptr;
			Hydra::Component::MeshComponent* meshComp = nullptr;
			Hydra::Component::WeaponComponent* weapon = nullptr;
			Hydra::Component::LifeComponent* life = nullptr;
			Hydra::Component::MovementComponent* movement = nullptr;
			Hydra::Component::AIComponent* ai = nullptr;
			Hydra::Component::RigidBodyComponent* rigidBody = nullptr;
		};
		ComponentSet thisEnemy = ComponentSet();
		ComponentSet targetPlayer = ComponentSet();

		glm::vec2 flatVector(glm::vec3 vec);
		void move(glm::vec3 target);
		virtual bool refreshRequiredComponents();
		virtual unsigned int idleState(float dt);
		virtual unsigned int searchingState(float dt);
		virtual unsigned int movingState(float dt);
		virtual unsigned int attackingState(float dt);
		virtual void executeTransforms();
		virtual void resetAnimationOnStart(int animationIndex);
	};

	class HYDRA_PHYSICS_API AlienBehaviour final : public Behaviour
	{
	public:
		AlienBehaviour(std::shared_ptr<Hydra::World::Entity> enemy);
		AlienBehaviour();
		~AlienBehaviour();
		void run(float dt);

		unsigned int attackingState(float dt) final;
	};

	class HYDRA_PHYSICS_API RobotBehaviour final : public Behaviour
	{
	public:
		RobotBehaviour(std::shared_ptr<Hydra::World::Entity> enemy);
		RobotBehaviour();
		~RobotBehaviour();
		void run(float dt);
		unsigned int idleState(float dt) final;
		unsigned int attackingState(float dt) final;
	private:
		bool refreshRequiredComponents() final;
	};

	class HYDRA_PHYSICS_API BossHand_Left final : public Behaviour
	{
	public:
		BossHand_Left(std::shared_ptr<Hydra::World::Entity> enemy);
		BossHand_Left();
		~BossHand_Left();

		const float originalHeight = 40.0f;
		glm::vec3 basePosition[2] = { glm::vec3(30 + 150, originalHeight, 40 + 150),  glm::vec3(30 + 150, originalHeight, -40 + 150) };
		glm::vec3 swipePosition[2] = { glm::vec3(135, 1, 210),glm::vec3(140, 1, 90) };
		glm::vec3 swipeFinish[2] = { glm::vec3(135, 1, 160),glm::vec3(135, 1, 140) };
		glm::vec3 canonPosition[2] = { glm::vec3(30 + 150, originalHeight, 25 + 150), glm::vec3(30 + 150, originalHeight, -25 + 150) };
		glm::vec3 coverPosition[2] = { glm::vec3(170, 20, 155), glm::vec3(170, 20, 145) };
		glm::vec3 smashPosition[2] = { glm::vec3(0), glm::vec3(0) };
		glm::vec3 lastPosition[2] = { glm::vec3(0), glm::vec3(0) };


		float idleTimer = 0.0f;
		float coverTimer = 0.0f;
		float stunTimer = 0.0f;
		float waitToSmashTimer = 0.0f;
		float stuckTimer = 0.0f;
		bool stunned = false;
		bool swiping = false;
		bool smashing = false;
		bool covering = false;
		bool shooting = false;
		bool rotateToCover = false;
		bool hit = false;
		bool inBasePosition = true;
		int spawnAmount = 0;
		int targetedPlayer = 0;
		int randomNrOfShots = 0;
		int shotsFired = 0;

		

		void run(float dt);
		void move(glm::vec3 target);

		unsigned int idleState(float dt) final;
		unsigned int smashState(float dt);
		unsigned int swipeState(float dt);
		unsigned int canonState(float dt);
		unsigned int coverState(float dt);
		unsigned int returnState(float dt);

		void rotateAroundAxis(float newRotation, glm::vec3(direction));

	private:
		bool refreshRequiredComponents() final;
	};

	class HYDRA_PHYSICS_API BossArm final : public Behaviour
	{
	public:
		BossArm(std::shared_ptr<Hydra::World::Entity> enemy);
		BossArm();
		~BossArm();

		const float originalHeight = 20.0f;

		glm::vec3 playerDir;


		float idleTimer = 0.0f;
		float aimTimer = 0.0f;
		float waitTimer = 0.0f;
		bool shot = false;

		void run(float dt);

		unsigned int idleState(float dt) final;
		unsigned int aimState(float dt);
		unsigned int shootState(float dt);

		void executeTransforms();
		void updateRigidBodyPosition();

	private:
		bool refreshRequiredComponents() final;
	};

	class HYDRA_PHYSICS_API StationaryBoss final : public Behaviour
	{
	public:
		StationaryBoss(std::shared_ptr<Hydra::World::Entity> enemy);
		StationaryBoss();
		~StationaryBoss();

		void run(float dt);

		int randomAliens, randomRobots;
		int maxSpawn = 4;

		std::vector<glm::vec3> spawnPositions;

		float spawnEnemiesAtPercentage[3] = { 2800.f, 1000.0f, 500.0f };
		int spawnIndex = 0;
		float idleTimer = 0.0f;
		unsigned int idleState(float dt) final;
		//unsigned int shootingState(float dt) final;
		unsigned int spawnState(float dt);

		void applySpawnPositions();
	private:
		bool refreshRequiredComponents() final;
	};
}