// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/**
* EnemyComponent/AI.
*
* License: Mozilla Public License Version 2.0 (https://www.mozilla.org/en-US/MPL/2.0/ OR See accompanying file LICENSE)
* Authors:
*  - Dan Printzell
*/
#include <hydra/component/aicomponent.hpp>
#include <imgui/imgui.h>

using namespace Hydra::World;
using namespace Hydra::Component;

EnemyComponent::EnemyComponent(IEntity* entity) : IComponent(entity), _enemyID(EnemyTypes::Alien), _position(0,0,0), _health(1), _damage(0), _range(1.0f) {}

EnemyComponent::EnemyComponent(IEntity* entity, EnemyTypes enemyID, glm::vec3 pos, int hp, int dmg, float range) : IComponent(entity), _enemyID(enemyID),  _position(pos), _health(hp), _damage(dmg), _range(range){
	_velocityX = 0;
	_velocityY = 0;
	_velocityZ = 0;
	_startPosition = pos;
	_patrolPointReached = false;
	_falling = false;
	_pathState = IDLE;
	_originalRange = range;
	_shootTimer = SDL_GetTicks();
	entity->addComponent<Hydra::Component::TransformComponent>(pos);
	entity->addComponent<Hydra::Component::WeaponComponent>();

	for (int i = 0; i < WORLD_SIZE; i++)
	{
		for (int j = 0; j < WORLD_SIZE; j++)
		{
			_map[i][j] = 0;
		}
	}

	for (int i = 0; i < 10; i++)
	{
		_map[10][10+i] = 1;
	}
}

EnemyComponent::~EnemyComponent() {
	delete _pathFinding;
}

void EnemyComponent::tick(TickAction action, float delta) {
	// If you only have one TickAction in 'wantTick' you don't need to check the tickaction here.

	auto enemy = entity->getComponent<Component::TransformComponent>();
	std::shared_ptr<Hydra::World::IEntity> playerEntity = getPlayerComponent();
	auto player = playerEntity->getComponent<Component::PlayerComponent>();

	_velocityX = 0;
	_velocityY = 0;
	_velocityZ = 0;
	_debugState = _pathState;

	if (glm::length(enemy->getPosition() - player->getPosition()) > 50)
	{
		_pathState = IDLE;
	}

	switch (_enemyID)
	{
		case EnemyTypes::Alien:
		{
			switch (_pathState)
			{
			case IDLE:
			{
				if (glm::length(enemy->getPosition() - player->getPosition()) < 50)
				{
					_timer = SDL_GetTicks();
					_pathFinding->intializedStartGoal = false;
					_pathFinding->foundGoal = false;
					_pathFinding->clearPathToGoal();
					_pathState = SEARCHING;
				}
			}break;
			case SEARCHING:
			{
				if (SDL_GetTicks() > _timer + 5000)
				{
					_timer = SDL_GetTicks();
					_pathState = IDLE;
				}
				if (glm::length(enemy->getPosition() - player->getPosition()) < _range)
				{
					_isAtGoal = true;
					_pathFinding->foundGoal = true;
					_pathState = ATTACKING;
				}
				_pathFinding->findPath(enemy->getPosition(), player->getPosition(), _map);

				_isAtGoal = false;
				if (_pathFinding->foundGoal)
				{
					if (!_pathFinding->_pathToEnd.empty())
					{
						_targetPos = _pathFinding->_pathToEnd[0];
					}
					_pathState = FOUND_GOAL;
				}
			}break;
			case FOUND_GOAL:
			{
				
				if (!_isAtGoal)
				{
					if (!_pathFinding->_pathToEnd.empty())
					{
						glm::vec3 targetDistance = _pathFinding->nextPathPos(enemy->getPosition(), getRadius()) - enemy->getPosition();

						_angle = atan2(targetDistance.x, targetDistance.z);
						_rotation = glm::angleAxis(_angle, glm::vec3(0, 1, 0));

						glm::vec3 direction = glm::normalize(targetDistance);


						_velocityX = (10.0f * direction.x) * delta;
						_velocityZ = (10.0f * direction.z) * delta;

						if (glm::length(enemy->getPosition() - player->getPosition()) <= _range)
						{
							_isAtGoal = true;
							_pathFinding->foundGoal = true;
							_pathState = ATTACKING;
						}

						if (glm::length(enemy->getPosition() - _targetPos) <= 8.5f)
						{
							_pathFinding->intializedStartGoal = false;
							_pathFinding->foundGoal = false;
							_pathFinding->clearPathToGoal();
							_pathState = SEARCHING;
							_timer = SDL_GetTicks();
						}
						else if (glm::length(player->getPosition() - _targetPos) > 15.0f)
						{
							_pathFinding->intializedStartGoal = false;
							_pathFinding->foundGoal = false;
							_pathFinding->clearPathToGoal();
							_pathState = SEARCHING;
							_timer = SDL_GetTicks();
						}
					}
				}
			}break;
			case ATTACKING:
			{
				if (glm::length(enemy->getPosition() - player->getPosition()) >= _range)
				{
					_pathFinding->intializedStartGoal = false;
					_pathFinding->foundGoal = false;
					_pathFinding->clearPathToGoal();
					_pathState = SEARCHING;
					_timer = SDL_GetTicks();
				}
				else
				{
					std::mt19937 rng(rd());
					std::uniform_int_distribution<> randDmg(_damage - 1, _damage + 2);
					if (SDL_GetTicks() > _shootTimer + 1500)
					{
						player->applyDamage(randDmg(rng));
						_shootTimer = SDL_GetTicks();
					}

					glm::vec3 playerDir = enemy->getPosition() - player->getPosition();
					playerDir = glm::normalize(playerDir);
					_angle = atan2(playerDir.x, playerDir.z);
					_rotation = glm::angleAxis(_angle, glm::vec3(0, 1, 0));
				}
			}break;
			}

			_position = _position + glm::vec3(_velocityX, _velocityY, _velocityZ);
			enemy->setPosition(_position);
			enemy->setRotation(_rotation);

		}break;
		case EnemyTypes::Robot:
		{
			switch (_pathState)
			{
			case IDLE:
			{
				if (glm::length(enemy->getPosition() - player->getPosition()) < 50)
				{
					_timer = SDL_GetTicks();
					_pathFinding->intializedStartGoal = false;
					_pathFinding->foundGoal = false;
					_pathFinding->clearPathToGoal();
					_pathState = SEARCHING;
				}
			}break;
			case SEARCHING:
			{
				if (SDL_GetTicks() > _timer + 5000)
				{
					_timer = SDL_GetTicks();
					_pathState = IDLE;
				}

				if (glm::length(enemy->getPosition() - player->getPosition()) < _range)
				{
					_isAtGoal = true;
					_pathState = ATTACKING;
				}

				_pathFinding->findPath(enemy->getPosition(), player->getPosition(), _map);

				_isAtGoal = false;
				if (_pathFinding->foundGoal)
				{
					if (!_pathFinding->_pathToEnd.empty())
					{
						_targetPos = _pathFinding->_pathToEnd[0];
					}
					_pathState = FOUND_GOAL;
				}
			}break;
			case FOUND_GOAL:
			{
				if (!_isAtGoal)
				{
					if (!_pathFinding->_pathToEnd.empty())
					{
						glm::vec3 targetDistance = _pathFinding->nextPathPos(enemy->getPosition(), getRadius()) - enemy->getPosition();

						_angle = atan2(targetDistance.x, targetDistance.z);
						_rotation = glm::angleAxis(_angle, glm::vec3(0, 1, 0));

						glm::vec3 direction = glm::normalize(targetDistance);

						_velocityX = (4.0f * direction.x) * delta;
						_velocityZ = (4.0f * direction.z) * delta;


						if (glm::length(enemy->getPosition() - player->getPosition()) < _range)
						{
							_isAtGoal = true;
							_pathFinding->foundGoal = true;
							_pathState = ATTACKING;
						}

						if (glm::length(enemy->getPosition() - _targetPos) <= 8.5f)
						{
							_pathFinding->intializedStartGoal = false;
							_pathFinding->foundGoal = false;
							_pathFinding->clearPathToGoal();
							_pathState = SEARCHING;
							_timer = SDL_GetTicks();
						}
						else if (glm::length(player->getPosition() - _targetPos) > 15.0f)
						{
							_pathFinding->intializedStartGoal = false;
							_pathFinding->foundGoal = false;
							_pathFinding->clearPathToGoal();
							_pathState = SEARCHING;
							_timer = SDL_GetTicks();
						}
					}
				}
			}break;
			case ATTACKING:
			{
				if (glm::length(enemy->getPosition() - player->getPosition()) > _range)
				{
					_pathFinding->intializedStartGoal = false;
					_pathFinding->foundGoal = false;
					_pathFinding->clearPathToGoal();
					_pathState = SEARCHING;
					_timer = SDL_GetTicks();
				}
				else
				{
					auto weapon = entity->getComponent<Component::WeaponComponent>();

					glm::vec3 playerDir = enemy->getPosition() - player->getPosition();
					playerDir = glm::normalize(playerDir);
					if (SDL_GetTicks() > _shootTimer + 2000)
					{
						weapon->shoot(_position, playerDir, glm::quat(), 5.0f);
						_shootTimer = SDL_GetTicks();
					}
					_angle = atan2(playerDir.x, playerDir.z);
					_rotation = glm::angleAxis(_angle, glm::vec3(0, 1, 0));
				}
			}break;
			}

			_playerSeen = _checkLine(_map, enemy->getPosition(), player->getPosition());

			if (_playerSeen == false)
			{
				if (_range > 4.0f)
				{
					_range -= 1.0f;
				}
			}
			else
			{
				_range = _originalRange;
			}

			_position = _position + glm::vec3(_velocityX, _velocityY, _velocityZ);
			enemy->setPosition(_position);
			enemy->setRotation(_rotation);

		}break;
		case EnemyTypes::AlienBoss:
		{
			switch (_pathState)
			{
			case IDLE:
			{
				if (glm::length(enemy->getPosition() - player->getPosition()) < 50)
				{
					_timer = SDL_GetTicks();
					_pathState = SEARCHING;
				}
			}break;
			case SEARCHING:
			{
				if (SDL_GetTicks() > _timer + 5000)
				{
					_timer = SDL_GetTicks();
					_pathState = IDLE;
				}

				if (glm::length(enemy->getPosition() - player->getPosition()) < _range)
				{
					_isAtGoal = true;
					_pathState = ATTACKING;
				}

				_pathFinding->findPath(enemy->getPosition(), player->getPosition(), _map);
				_isAtGoal = false;

				if (_pathFinding->foundGoal)
				{
					if (!_pathFinding->_pathToEnd.empty())
					{
						_targetPos = _pathFinding->_pathToEnd[0];
					}
					_pathState = FOUND_GOAL;
				}
			}break;
			case FOUND_GOAL:
			{
				if (!_isAtGoal)
				{
					if (!_pathFinding->_pathToEnd.empty())
					{

						glm::vec3 targetDistance = _pathFinding->nextPathPos(enemy->getPosition(), getRadius()) - enemy->getPosition();

						_angle = atan2(targetDistance.x, targetDistance.z);
						_rotation = glm::angleAxis(_angle, glm::vec3(0, 1, 0));

						glm::vec3 direction = glm::normalize(targetDistance);

						_velocityX = (7.0f * direction.x) * delta;
						_velocityZ = (7.0f * direction.z) * delta;

						if (glm::length(enemy->getPosition() - player->getPosition()) < _range)
						{
							_isAtGoal = true;
							_pathFinding->foundGoal = true;
							_pathState = ATTACKING;
						}

						if (glm::length(enemy->getPosition() - _targetPos) <= 8.5f)
						{
							_pathFinding->intializedStartGoal = false;
							_pathFinding->foundGoal = false;
							_pathFinding->clearPathToGoal();
							_pathState = SEARCHING;
							_timer = SDL_GetTicks();
						}
						else if (glm::length(player->getPosition() - _targetPos) > 15.0f)
						{
							_pathFinding->intializedStartGoal = false;
							_pathFinding->foundGoal = false;
							_pathFinding->clearPathToGoal();
							_pathState = SEARCHING;
							_timer = SDL_GetTicks();
						}
					}
				}
			}break;
			case ATTACKING:
			{
				if (glm::length(enemy->getPosition() - player->getPosition()) > _range)
				{
					_pathFinding->intializedStartGoal = false;
					_pathFinding->foundGoal = false;
					_pathFinding->clearPathToGoal();
					_pathState = SEARCHING;
					_timer = SDL_GetTicks();
				}
				else
				{
					glm::vec3 playerDir = enemy->getPosition() - player->getPosition();
					playerDir = glm::normalize(playerDir);
					_angle = atan2(playerDir.x, playerDir.z);
					_rotation = glm::angleAxis(_angle, glm::vec3(0, 1, 0));
				}
			}break;
			}

			_position = _position + glm::vec3(_velocityX, _velocityY, _velocityZ);
			enemy->setPosition(_position);
			enemy->setRotation(_rotation);

		}break;
	}

	 //debug for pathfinding
		//int tempX = enemy->getPosition().x;
		//int tempZ = enemy->getPosition().z;
		//_map[tempX][tempZ] = 2;
		//for (int i = 0; i < _pathFinding->_visitedList.size(); i++)
		//{
		//	_map[(int)_pathFinding->_visitedList[i]->m_xcoord][(int)_pathFinding->_visitedList[i]->m_zcoord] = 3;
		//}
		//for (int i = 0; i < _pathFinding->_openList.size(); i++)
		//{
		//	_map[(int)_pathFinding->_openList[i]->m_xcoord][(int)_pathFinding->_openList[i]->m_zcoord] = 3;
		//}
		//for (int i = 0; i < _pathFinding->_pathToEnd.size(); i++)
		//{
		//	_map[(int)_pathFinding->_pathToEnd[i].x][(int)_pathFinding->_pathToEnd[i].z] = 3;
		//}
}

glm::vec3 EnemyComponent::getPosition()
{
	auto enemy = entity->getComponent<Component::TransformComponent>();

	return enemy->getPosition();
}

float EnemyComponent::getRadius()
{
	return 1.0f;
}

std::shared_ptr<Hydra::World::IEntity> EnemyComponent::getPlayerComponent()
{
	std::shared_ptr<Hydra::World::IEntity> player;
	auto world = entity->getParent()->getChildren();
	for (size_t i = 0; i < world.size(); i++) {
		if (world[i]->getName() == "Player") {
			player = world[i];
		}
	}
	return player;
}

void EnemyComponent::serialize(nlohmann::json& json) const {
	json = {
		{ "position",{ _position.x, _position.y, _position.z } },
		{ "startPosition",{ _startPosition.x, _startPosition.y, _startPosition.z } },
		{ "velocityX", _velocityX },
		{ "velocityY", _velocityY },
		{ "velocityZ", _velocityZ },
		{ "enemyID", (int)_enemyID },
		{ "pathState", (int)_pathState },
		{ "damage", _damage },
		{ "health", _health },
		{ "range", _range },
		{ "Original range", _originalRange }

	};
	for (size_t i = 0; i < 64; i++)
	{
		for (size_t j = 0; j < 64; j++)
		{
			json["map"][i][j] = _map[i][j];
		}
	}
}

void EnemyComponent::deserialize(nlohmann::json& json) {
	auto& pos = json["position"];
	_position = glm::vec3{ pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>() };

	auto& startpos = json["startPosition"];
	_startPosition = glm::vec3{ startpos[0].get<float>(), startpos[1].get<float>(), startpos[2].get<float>() };

	_velocityX = json["velocityX"].get<float>();
	_velocityY = json["velocityY"].get<float>();
	_velocityZ = json["velocityZ"].get<float>();

	_range = json["range"].get<float>();
	_originalRange = json["Original range"].get<float>();

	_enemyID = (EnemyTypes)json["enemyID"].get<int>();
	_pathState = (PathState)json["pathState"].get<int>();
	_damage = json["damage"].get<int>();
	_health = json["health"].get<int>();
	for (size_t i = 0; i < 64; i++)
	{
		for (size_t j = 0; j < 64; j++)
		{
			_map[i][j] = json["map"][i][j].get<int>();
		}
	}
}

// Register UI buttons in the debug UI
// Note: This function won't always be called
void EnemyComponent::registerUI() {
	ImGui::InputFloat("X", &_position.x);
	ImGui::InputFloat("Y", &_position.y);
	ImGui::InputFloat("Z", &_position.z);
	ImGui::InputFloat("startY", &_startPosition.y);
	ImGui::InputInt("pathState", &_debugState);
	ImGui::InputFloat("targetX", &_targetPos.x);
	ImGui::InputFloat("targetY", &_targetPos.y);
	ImGui::InputFloat("targetZ", &_targetPos.z);
	ImGui::Checkbox("isAtGoal", &_isAtGoal);
	ImGui::Checkbox("playerCanBeSeen", &_playerSeen);
	ImGui::InputFloat("range", &_range);
}

int Hydra::Component::EnemyComponent::getWall(int x, int y)
{
	int result = 0;
	if (_map[x][y] == 1)
	{
		result = 1;
	}
	else if (_map[x][y] == 2)
	{
		result = 2;
	}
	else if (_map[x][y] == 3)
	{
		result = 3;
	}
	return result;
}

bool Hydra::Component::EnemyComponent::_checkLine(int levelmap[WORLD_SIZE][WORLD_SIZE], glm::vec3 A, glm::vec3 B)
{
	// New code, not optimal
	double x = B.x - A.x;
	double z = B.z - A.z;
	double len = std::sqrt((x*x) + (z*z));

	if (!len)
		return true;

	double unitx = x / len;
	double unitz = z / len;

	x = A.x;
	z = A.z;
	for (double i = 1; i < len; i += 1)
	{
		if (levelmap[(int)x][(int)z] == 1)
		{
			return false;
		}

		x += unitx;
		z += unitz;
	}

	return true;

	// Old code
	/*bool steep = (fabs(B.z - A.z) > fabs(B.x - A.x));
	if (steep)
	{
		std::swap(A.x, A.z);
		std::swap(B.x, B.z);
	}

	if (A.x > B.x)
	{
		std::swap(A.x, B.x);
		std::swap(A.z, B.z);
	}

	float dx = B.x - A.x;
	float dz = fabs(B.z - A.z);

	float error = dx / 2.0f;
	int zStep = (A.z < B.z) ? 1 : -1;
	int z = (int)A.z;

	int maxX = (int)B.x;

	int x;
	for (x = (int)A.x; x < maxX; x++)
	{
		if (steep)
		{
			_map[x][z] = 3;
			if (levelmap[x][z] == 1) return false;
		}
		else
		{
			_map[x][z] = 3;
			if (levelmap[z][x] == 1) return false;
		}

		error -= dz;
		if (error < 0)
		{
			z += zStep;
			error += dx;
		}
	}

	return true;*/
}


