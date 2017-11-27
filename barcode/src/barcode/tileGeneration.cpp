#include <barcode/tileGeneration.hpp>

#include <fstream>
#include <json.hpp>
#include <imgui/imgui.h>
#include <algorithm> //std::shuffle
#include <random> //std::default_random_engine
#include <chrono> //std::chrono::system_clock

#include <hydra/component/meshcomponent.hpp>
#include <hydra/component/aicomponent.hpp>
#include <hydra/component/ghostobjectcomponent.hpp>
#include <hydra/component/pointlightcomponent.hpp>
#include <hydra/component/lifecomponent.hpp>
#include <hydra/component/pickupcomponent.hpp>
#include <hydra/component/textcomponent.hpp>
#include <hydra/component/rigidbodycomponent.hpp>
#include <hydra/component/movementcomponent.hpp>
#include <hydra/component/weaponcomponent.hpp>
#include <hydra/component/spawnpointcomponent.hpp>

using world = Hydra::World::World;

TileGeneration::TileGeneration(std::string middleRoomPath) {
	_obtainRoomFiles();
	pathfindingMap = new bool*[WORLD_MAP_SIZE];
	for (int i = 0; i < WORLD_MAP_SIZE; i++)
	{
		pathfindingMap[i] = new bool[WORLD_MAP_SIZE];
		for (int j = 0; j < WORLD_MAP_SIZE; j++)
		{
			pathfindingMap[i][j] = false;
		}
	}
	_setUpMiddleRoom(middleRoomPath);
}

TileGeneration::~TileGeneration() {
	for (int i = 0; i < WORLD_MAP_SIZE; i++)
	{
		delete[] pathfindingMap[i];
	}
	delete[] pathfindingMap;
}

bool** TileGeneration::buildMap()
{
	_createMapRecursivly(glm::ivec2(ROOM_GRID_SIZE / 2, ROOM_GRID_SIZE / 2));
	_spawnEnemies();
	_clearSpawnPoints();
	return pathfindingMap;
}

void TileGeneration::_createMapRecursivly(glm::ivec2 pos) {

	_roomCounter++;
	
	//Random the rooms vector each time we're about to spawn a new room
	_randomizeRooms();


	//Go through all doors in the room of the tile, create a random room
	//That fits with the door, go through that room and so on, until the next
	//you get to a "end room". Then go back and continue for next door

	//Check if yTilePos or xTilePos == 0. If this is true, limit the rooms it can create
	//or we will go otuside the tile grid
	
	
	if (roomGrid[pos.x][pos.y]->door[NORTH] && roomGrid[pos.x][pos.y + 1] == nullptr)
	{
		bool placed = false;
		//Load all rooms and see if any of them fits
		for (size_t i = 0; i < _roomFileNames.size() && placed == false && _roomCounter < maxRooms; i++) {

			//Take a random room and read it. Don't spawn it until it fits
			//NOTE:: Make a random list and go through it to prevent loading same room multible times
			auto loadedRoom = world::newEntity("Room", world::root());

			BlueprintLoader::load(_roomFileNames[i])->spawn(loadedRoom);
			auto roomC = loadedRoom->getComponent<Hydra::Component::RoomComponent>();
			glm::quat rotation = _rotateRoom(roomC);

			if (roomC->door[SOUTH] == true) {
				if (_checkAdjacents(pos.x, pos.y + 1, roomC))
				{
					placed = true;
					auto t = loadedRoom->getComponent<Hydra::Component::TransformComponent>();
					t->position = _gridToWorld(pos.x, pos.y + 1);
					t->scale = glm::vec3(1, 1, 1);
					t->rotation = rotation;
					roomGrid[pos.x][pos.y + 1] = roomC;
					_insertPathFindingMap(glm::ivec2(pos.x, pos.y + 1));
					_createMapRecursivly(glm::ivec2(pos.x, pos.y + 1));
				}
				else
					loadedRoom->dead = true;
			}			
			else
				loadedRoom->dead = true;
		}
		//If for some reason no room at all fits, spawn a door/rubble/something
		if (placed == false) {
			//Place stuff to cover door
			auto doorBlock = world::newEntity("DoorBlock", world::root());
			doorBlock->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/BlockCube2.mATTIC");
			auto t = doorBlock->addComponent<Hydra::Component::TransformComponent>();
			t->position = _gridToWorld(pos.x, pos.y + 1);
			t->position.z -= 17;
			t->position.y += 3;
			t->scale = glm::vec3(4);
			auto rgbc = doorBlock->addComponent<Hydra::Component::GhostObjectComponent>();
			rgbc->createBox(glm::vec3(3.2f), Hydra::System::BulletPhysicsSystem::CollisionTypes::COLL_WALL, glm::quat());
		}
	}

	if (roomGrid[pos.x][pos.y]->door[WEST] && roomGrid[pos.x + 1][pos.y] == nullptr)
	{
		bool placed = false;
		//Load all rooms and see if any of them fits
		for (size_t i = 0; i < _roomFileNames.size() && placed == false && _roomCounter < maxRooms; i++) {

			//Take a random room and read it. Don't spawn it until it fits
			//NOTE:: Make a random list and go through it to prevent loading same room multible times
			std::shared_ptr<Hydra::World::Entity> loadedRoom = world::newEntity("Room", world::root());

			BlueprintLoader::load(_roomFileNames[i])->spawn(loadedRoom);
			auto roomC = loadedRoom->getComponent<Hydra::Component::RoomComponent>();
			glm::quat rotation = _rotateRoom(roomC);

			if (roomC->door[EAST] == true) {
				if (_checkAdjacents(pos.x + 1, pos.y, roomC))
				{
					placed = true;
					auto t = loadedRoom->getComponent<Hydra::Component::TransformComponent>();
					t->position = _gridToWorld(pos.x + 1, pos.y);
					t->scale = glm::vec3(1, 1, 1);
					t->rotation = rotation;
					roomGrid[pos.x + 1][pos.y] = roomC;
					_insertPathFindingMap(glm::ivec2(pos.x + 1, pos.y));
					_createMapRecursivly(glm::ivec2(pos.x + 1, pos.y));
				}
				else
					loadedRoom->dead = true;
			}
			else
				loadedRoom->dead = true;
		}
		//If for some reason no room at all fits, spawn a door/rubble/something
		if (placed == false) {
			//Place stuff to cover door
			auto doorBlock = world::newEntity("DoorBlock", world::root());
			doorBlock->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/BlockCube2.mATTIC");
			auto t = doorBlock->addComponent<Hydra::Component::TransformComponent>();
			t->position = _gridToWorld(pos.x + 1, pos.y);
			t->position.x -= 17;
			t->position.y += 3;
			t->scale = glm::vec3(4);
			auto rgbc = doorBlock->addComponent<Hydra::Component::GhostObjectComponent>();
			rgbc->createBox(glm::vec3(3.2f), Hydra::System::BulletPhysicsSystem::CollisionTypes::COLL_WALL, glm::quat());
		}
	}

	if (roomGrid[pos.x][pos.y]->door[SOUTH] && roomGrid[pos.x][pos.y - 1] == nullptr)
	{
		bool placed = false;
		//Load all rooms and see if any of them fits
		for (size_t i = 0; i < _roomFileNames.size() && placed == false && _roomCounter < maxRooms; i++) {

			//Take a random room and read it. Don't spawn it until it fits
			//NOTE:: Make a random list and go through it to prevent loading same room multible times
			std::shared_ptr<Hydra::World::Entity> loadedRoom = world::newEntity("Room", world::root());

			BlueprintLoader::load(_roomFileNames[i])->spawn(loadedRoom);
			auto roomC = loadedRoom->getComponent<Hydra::Component::RoomComponent>();
			glm::quat rotation = _rotateRoom(roomC);

			if (roomC->door[NORTH] == true) {
				if (_checkAdjacents(pos.x, pos.y - 1, roomC))
				{
					placed = true;
					auto t = loadedRoom->getComponent<Hydra::Component::TransformComponent>();
					t->position = _gridToWorld(pos.x, pos.y - 1);
					t->scale = glm::vec3(1, 1, 1);
					t->rotation = rotation;
					roomGrid[pos.x][pos.y - 1] = roomC;
					_insertPathFindingMap(glm::ivec2(pos.x, pos.y - 1));
					_createMapRecursivly(glm::ivec2(pos.x, pos.y - 1));
				}
				else
					loadedRoom->dead = true;
			}
			else
				loadedRoom->dead = true;
		}
		//If for some reason no room at all fits, spawn a door/rubble/something
		if (placed == false) {
			//Place stuff to cover door
			auto doorBlock = world::newEntity("DoorBlock", world::root());
			doorBlock->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/BlockCube2.mATTIC");
			auto t = doorBlock->addComponent<Hydra::Component::TransformComponent>();
			t->position = _gridToWorld(pos.x, pos.y - 1);
			t->position.z += 17;
			t->position.y += 3;
			t->scale = glm::vec3(4);
			auto rgbc = doorBlock->addComponent<Hydra::Component::GhostObjectComponent>();
			rgbc->createBox(glm::vec3(3.2f), Hydra::System::BulletPhysicsSystem::CollisionTypes::COLL_WALL, glm::quat());
		}
	}

	if (roomGrid[pos.x][pos.y]->door[EAST] && roomGrid[pos.x - 1][pos.y] == nullptr)
	{
		bool placed = false;
		//Load all rooms and see if any of them fits
		for (size_t i = 0; i < _roomFileNames.size() && placed == false && _roomCounter < maxRooms; i++) {

			//Take a random room and read it. Don't spawn it until it fits
			//NOTE:: Make a random list and go through it to prevent loading same room multible times
			std::shared_ptr<Hydra::World::Entity> loadedRoom = world::newEntity("Room", world::root());

			BlueprintLoader::load(_roomFileNames[i])->spawn(loadedRoom);
			auto roomC = loadedRoom->getComponent<Hydra::Component::RoomComponent>();
			glm::quat rotation = _rotateRoom(roomC);

			if (roomC->door[WEST] == true) {
				if (_checkAdjacents(pos.x - 1, pos.y, roomC))
				{
					placed = true;
					auto t = loadedRoom->getComponent<Hydra::Component::TransformComponent>();
					t->position = _gridToWorld(pos.x - 1, pos.y);
					t->scale = glm::vec3(1, 1, 1);
					t->rotation = rotation;
					roomGrid[pos.x - 1][pos.y] = roomC;
					_insertPathFindingMap(glm::ivec2(pos.x - 1, pos.y));
					_createMapRecursivly(glm::ivec2(pos.x - 1, pos.y));
				}
				else
					loadedRoom->dead = true;
			}
			else
				loadedRoom->dead = true;
		}
		//If for some reason no room at all fits, spawn a door/rubble/something
		if (placed == false) {
			//Place stuff to cover door
			auto doorBlock = world::newEntity("DoorBlock", world::root());
			doorBlock->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/BlockCube2.mATTIC");
			auto t = doorBlock->addComponent<Hydra::Component::TransformComponent>();
			t->position = _gridToWorld(pos.x - 1, pos.y);
			t->position.x += 17;
			t->position.y += 3;
			t->scale = glm::vec3(4);
			auto rgbc = doorBlock->addComponent<Hydra::Component::GhostObjectComponent>();
			rgbc->createBox(glm::vec3(3.2f), Hydra::System::BulletPhysicsSystem::CollisionTypes::COLL_WALL, glm::quat());
		}
	}
}

void TileGeneration::_setUpMiddleRoom(std::string middleRoomPath) {

	auto room = world::newEntity("Middle Room", world::root());
	BlueprintLoader::load(middleRoomPath)->spawn(room); 
	auto roomC = room->getComponent<Hydra::Component::RoomComponent>();
	roomGrid[ROOM_GRID_SIZE / 2][ROOM_GRID_SIZE / 2] = roomC;
	_insertPathFindingMap(glm::ivec2(ROOM_GRID_SIZE / 2, ROOM_GRID_SIZE / 2));
	auto t = room->addComponent<Hydra::Component::TransformComponent>();
	t->position = _gridToWorld(2, 2);
	t->scale = glm::vec3(1, 1, 1);
	_generatePlayerSpawnPoints();
	_spawnLight(t);
}

void TileGeneration::_obtainRoomFiles() {
	//Get the files in order
	std::string path = "assets/room/";
	for (auto & p : std::experimental::filesystem::directory_iterator(path)) {
		_roomFileNames.push_back(p.path().string());
	}

	//_roomFileNames.push_back(path + "centralRoomBigScreen.room");
	//_roomFileNames.push_back(path + "fourwayRoom.room");
	//_roomFileNames.push_back(path + "starterRoom.room");
	//_roomFileNames.push_back(path + "threewayRoom.room");
	//_roomFileNames.push_back(path + "trashedComputerRoom.room");
	//_roomFileNames.push_back(path + "tryTree.room");
	//_roomFileNames.push_back(path + "tryTwo.room");

	_randomizeRooms();

}

void TileGeneration::_randomizeRooms() {

	//Randomize the list 2 times for extra randomness
	for (size_t k = 0; k < 2; k++) {
		for (size_t i = 0; i < _roomFileNames.size(); i++) {
			int randomPos = rand() % _roomFileNames.size();
			std::swap(_roomFileNames[i], _roomFileNames[randomPos]);
		}
	}
}

//Call after creating the first room to spawn players in the same room, call after creating the whole map to randomly spawn players across the whole map
//Returns false if not enough player spawn points were found
bool TileGeneration::_generatePlayerSpawnPoints()
{
	//Get all spawnpoints
	std::vector<std::shared_ptr<Hydra::World::Entity>> entities = std::vector<std::shared_ptr<Hydra::World::Entity>>();
	world::getEntitiesWithComponents<Hydra::Component::SpawnPointComponent, Hydra::Component::TransformComponent>(entities);

	//Randomize order
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	shuffle(entities.begin(), entities.end(), std::default_random_engine(seed));
	for (int i = 0; i < entities.size() && playerSpawns.size() < numberOfPlayers; i++)
	{
		auto sp = entities[i]->getComponent<Hydra::Component::SpawnPointComponent>();
		if (sp->playerSpawn && !entities[i]->dead)
		{
			auto t = entities[i]->getComponent<Hydra::Component::TransformComponent>();
			playerSpawns.push_back(t->getMatrix()[3]);
			entities[i]->dead = true;
		}
	}
	if (playerSpawns.size() < numberOfPlayers)
	{
		return false;
	}
	return true;
}

void TileGeneration::_spawnEnemies() {
	std::vector<std::shared_ptr<Hydra::World::Entity>> entities;
	world::getEntitiesWithComponents<Hydra::Component::SpawnPointComponent, Hydra::Component::TransformComponent>(entities);
	//Randomize order
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	shuffle(entities.begin(), entities.end(), std::default_random_engine(seed));

	int spawned = 0;
	for (int i = 0; i < entities.size() && spawned < numberOfEnemies; i++)
	{
		auto sp = entities[i]->getComponent<Hydra::Component::SpawnPointComponent>();
		if (sp->enemySpawn && !entities[i]->dead)
		{
			auto t = entities[i]->getComponent<Hydra::Component::TransformComponent>();
			_spawnRandomEnemy(t->getMatrix()[3]);
			entities[i]->dead = true;
			spawned++;
		}
	}
	//_spawnPickUps(roomTransform);
	//_spawnLight(roomTransform);
}

void TileGeneration::_spawnRandomEnemy(glm::vec3 pos)
{
	//int randomRobots = rand() % int(MAX_ENEMIES - randomSlowAliens);
	//int randomFastAliens = rand() % int(MAX_ENEMIES - randomRobots);

		auto alienEntity = world::newEntity("SlowAlien1", world::root());
		alienEntity->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/characters/AlienModel.mATTIC");
		auto a = alienEntity->addComponent<Hydra::Component::AIComponent>();
		a->behaviour = std::make_shared<AlienBehaviour>(alienEntity);
		a->behaviour->setPathMap(pathfindingMap);
		a->damage = 4;
		a->behaviour->originalRange = 4;
		a->radius = 1;

		auto h = alienEntity->addComponent<Hydra::Component::LifeComponent>();
		h->maxHP = 80;
		h->health = 80;

		auto m = alienEntity->addComponent<Hydra::Component::MovementComponent>();
		m->movementSpeed = 5.0f;

		auto t = alienEntity->addComponent<Hydra::Component::TransformComponent>();
		t->position = pos;
		t->scale = glm::vec3{ 1,1,1 };

		auto rgbc = alienEntity->addComponent<Hydra::Component::RigidBodyComponent>();
		rgbc->createBox(glm::vec3(0.5f, 1.5f, 0.5f) * t->scale, glm::vec3(0, 1.5, 0), Hydra::System::BulletPhysicsSystem::CollisionTypes::COLL_ENEMY, 100.0f,
			0, 0, 0.6f, 1.0f);
		rgbc->setActivationState(Hydra::Component::RigidBodyComponent::ActivationState::disableDeactivation);
	//
	//for (int i = 0; i < randomFastAliens; i++) {
	//	auto alienEntity = world::newEntity("FastAlien1", world::root());
	//	alienEntity->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/characters/AlienFastModel.mATTIC");
	//	auto a = alienEntity->addComponent<Hydra::Component::AIComponent>();
	//	a->behaviour = std::make_shared<AlienBehaviour>(alienEntity);

	//	auto h = alienEntity->addComponent<Hydra::Component::LifeComponent>();
	//	h->maxHP = 60;
	//	h->health = 60;

	//	auto m = alienEntity->addComponent<Hydra::Component::MovementComponent>();
	//	m->movementSpeed = 10.0f;

	//	auto t = alienEntity->addComponent<Hydra::Component::TransformComponent>();
	//	t->position.x = roomTransform->position.x + i + 2;
	//	t->position.y = 5;
	//	t->position.z = roomTransform->position.z + i + 2;
	//	t->scale = glm::vec3{ 1,1,1 };

	//	auto rgbc = alienEntity->addComponent<Hydra::Component::RigidBodyComponent>();
	//	rgbc->createBox(glm::vec3(0.5f, 1.5f, 0.5f) * t->scale, glm::vec3(0, 1.5, 0), Hydra::System::BulletPhysicsSystem::CollisionTypes::COLL_ENEMY, 100.0f,
	//		0, 0, 0.6f, 1.0f);
	//	rgbc->setActivationState(Hydra::Component::RigidBodyComponent::ActivationState::disableDeactivation);
	//	rgbc->setAngularForce(glm::vec3(0));

	//}


	//for (int i = 0; i < randomRobots; i++) {
	//	auto robotEntity = world::newEntity("Robot1", world::root());
	//	robotEntity->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/characters/RobotModel.mATTIC");
	//	auto a = robotEntity->addComponent<Hydra::Component::AIComponent>();
	//	a->behaviour = std::make_shared<RobotBehaviour>(robotEntity);
	//	a->behaviour->setPathMap(pathfindingMap);
	//	a->damage = 7;
	//	a->behaviour->originalRange = 20;
	//	a->radius = 1;
	//
	//	auto h = robotEntity->addComponent<Hydra::Component::LifeComponent>();
	//	h->maxHP = 70;
	//	h->health = 70;

	//	auto w = robotEntity->addComponent<Hydra::Component::WeaponComponent>();
	//	w->bulletSpread = 0.3f;
	//	w->fireRateRPM = 50;
	//	w->bulletsPerShot = 1;
	//	w->damage = 7;
	//	w->maxmagammo = 100000000;
	//	w->currmagammo = 100000000;
	//	w->maxammo = 100000000;
	//	
	//	auto m = robotEntity->addComponent<Hydra::Component::MovementComponent>();
	//	m->movementSpeed = 3.0f;
	//	auto t = robotEntity->addComponent<Hydra::Component::TransformComponent>();
	//	t->position.x = roomTransform->position.x + i + 2;
	//	t->position.y = 5;
	//	t->position.z = roomTransform->position.z + i + 2;
	//	t->scale = glm::vec3{ 1,1,1 };

	//	auto rgbc = robotEntity->addComponent<Hydra::Component::RigidBodyComponent>();
	//	rgbc->createBox(glm::vec3(0.5f, 1.5f, 0.5f) * t->scale, glm::vec3(0, 1.5, 0), Hydra::System::BulletPhysicsSystem::CollisionTypes::COLL_ENEMY, 100.0f,
	//		0, 0, 0.6f, 1.0f);
	//	rgbc->setActivationState(Hydra::Component::RigidBodyComponent::ActivationState::disableDeactivation);
	//	rgbc->setAngularForce(glm::vec3(0));

	//}
}

void TileGeneration::_clearSpawnPoints()
{
	//Get all spawnpoints
	std::vector<std::shared_ptr<Hydra::World::Entity>> entities = std::vector<std::shared_ptr<Hydra::World::Entity>>();
	world::getEntitiesWithComponents<Hydra::Component::SpawnPointComponent, Hydra::Component::TransformComponent>(entities);

	for (auto e : entities)
	{
		e->dead = true;
	}
}

void TileGeneration::_insertPathFindingMap(glm::ivec2 room)
{
	auto roomC = roomGrid[room.x][room.y];
	int x = room.x * ROOM_MAP_SIZE;
	for (int localX = 0; localX < ROOM_MAP_SIZE; x++, localX++)
	{
		int y = room.y * ROOM_MAP_SIZE;
		for (int localY = 0; localY < ROOM_MAP_SIZE; y++, localY++)
		{
			pathfindingMap[x][y] = roomC->localMap[localX][localY];
		}
	}
}

void TileGeneration::_spawnPickUps(std::shared_ptr<Hydra::Component::TransformComponent>& roomTransform) {

	int randomChance = rand() % 100 + 1;

	if (randomChance < (int)PICKUP_CHANCE) {
		auto pickUpEntity = world::newEntity("PickUp", world::root());
		auto t = pickUpEntity->addComponent<Hydra::Component::TransformComponent>();
		t->position = glm::vec3(roomTransform->position.x, 3.0f, roomTransform->position.z);
		pickUpEntity->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/Lock.mATTIC");
		auto pickUpC = pickUpEntity->addComponent<Hydra::Component::PickUpComponent>();
		auto rgbc = pickUpEntity->addComponent<Hydra::Component::RigidBodyComponent>();
		rgbc->createBox(glm::vec3(2.0f, 1.5f, 1.7f), glm::vec3(0), Hydra::System::BulletPhysicsSystem::CollisionTypes::COLL_PICKUP_OBJECT, 10);
		rgbc->setActivationState(Hydra::Component::RigidBodyComponent::ActivationState::disableDeactivation);

		auto pickupText = world::newEntity("Textpickup", world::root());
		pickupText->addComponent<Hydra::Component::MeshComponent>()->loadMesh("TEXTQUAD");
		pickupText->addComponent<Hydra::Component::TransformComponent>()->setPosition(t->position);
		auto textStuff = pickupText->addComponent<Hydra::Component::TextComponent>();
		textStuff->setText("Perk picked up");
		textStuff->isStatic = true;

	}
}

void TileGeneration::_spawnLight(std::shared_ptr<Hydra::Component::TransformComponent>& roomTransform) {

	auto pointLight1 = world::newEntity("Pointlight1", world::root());
	pointLight1->addComponent<Hydra::Component::TransformComponent>();
	auto t = pointLight1->getComponent<Hydra::Component::TransformComponent>();
	t->position.x = roomTransform->position.x;
	t->position.y = roomTransform->position.y + 7;
	t->position.z = roomTransform->position.z;
	auto p1LC = pointLight1->addComponent<Hydra::Component::PointLightComponent>();

	p1LC->color = glm::vec3(1);

}

glm::quat TileGeneration::_rotateRoom(std::shared_ptr<Hydra::Component::RoomComponent>& room) {
	//glm::angleAxis(rotation, axis(y in your case));

	glm::quat rotation;

	int randomRotateChance = 100;//rand() % 100;

	//Rotate the room 90 degrees
	if (randomRotateChance < 25) {

		std::shared_ptr<Hydra::Component::RoomComponent> tempRoom = std::make_shared<Hydra::Component::RoomComponent>();
		
		//rotate doors
		if (room->door[NORTH] == true)
			tempRoom->door[EAST] = true;
		if (room->door[EAST] == true)
			tempRoom->door[SOUTH] = true;
		if (room->door[SOUTH] == true)
			tempRoom->door[WEST] = true;
		if (room->door[WEST] == true)
			tempRoom->door[NORTH] = true;

		//rotate open walls
		if (room->openWalls[0] == true)
			tempRoom->openWalls[1] = true;
		if (room->openWalls[1] == true)
			tempRoom->openWalls[2] = true;
		if (room->openWalls[2] == true)
			tempRoom->openWalls[3] = true;
		if (room->openWalls[3] == true)
			tempRoom->openWalls[0] = true;

		for (int i = 0; i < 4; i++) {
			room->door[i] = tempRoom->door[i];
			room->openWalls[i] = tempRoom->openWalls[i];
		}
		rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0));

	}
	//Rotate the room 180 degrees
	else if (randomRotateChance >= 25 && randomRotateChance < 50) {

		std::shared_ptr<Hydra::Component::RoomComponent> tempRoom = std::make_shared<Hydra::Component::RoomComponent>();

		//rotate doors
		if (room->door[NORTH] == true)
			tempRoom->door[SOUTH] = true;
		if (room->door[EAST] == true)
			tempRoom->door[WEST] = true;
		if (room->door[SOUTH] == true)
			tempRoom->door[NORTH] = true;
		if (room->door[WEST] == true)
			tempRoom->door[EAST] = true;

		//rotate open walls
		if (room->openWalls[0] == true)
			tempRoom->openWalls[2] = true;
		if (room->openWalls[1] == true)
			tempRoom->openWalls[3] = true;
		if (room->openWalls[2] == true)
			tempRoom->openWalls[0] = true;
		if (room->openWalls[3] == true)
			tempRoom->openWalls[1] = true;

		for (int i = 0; i < 4; i++) {
			room->door[i] = tempRoom->door[i];
			room->openWalls[i] = tempRoom->openWalls[i];
		}
		rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0, 1, 0));

	}
	else if (randomRotateChance >= 50 && randomRotateChance < 75) {

		std::shared_ptr<Hydra::Component::RoomComponent> tempRoom = std::make_shared<Hydra::Component::RoomComponent>();

		//rotate doors
		if (room->door[NORTH] == true)
			tempRoom->door[WEST] = true;
		if (room->door[EAST] == true)
			tempRoom->door[NORTH] = true;
		if (room->door[SOUTH] == true)
			tempRoom->door[EAST] = true;
		if (room->door[WEST] == true)
			tempRoom->door[SOUTH] = true;

		//rotate open walls
		if (room->openWalls[0] == true)
			tempRoom->openWalls[3] = true;
		if (room->openWalls[1] == true)
			tempRoom->openWalls[0] = true;
		if (room->openWalls[2] == true)
			tempRoom->openWalls[1] = true;
		if (room->openWalls[3] == true)
			tempRoom->openWalls[2] = true;

		for (int i = 0; i < 4; i++) {
			room->door[i] = tempRoom->door[i];
			room->openWalls[i] = tempRoom->openWalls[i];
		}
		rotation = glm::angleAxis(glm::radians(270.0f), glm::vec3(0, 1, 0));

	}
	else
		rotation = glm::angleAxis(glm::radians(0.0f), glm::vec3(0, 1, 0));

	return rotation;
}

glm::vec3 TileGeneration::_gridToWorld(int x, int y) {

	float xPos = (ROOM_SIZE * x) - ((ROOM_GRID_SIZE * ROOM_SIZE) / 2) + 17;
	float yPos = (ROOM_SIZE * y) - ((ROOM_GRID_SIZE * ROOM_SIZE) / 2) + 17;
	 
	glm::vec3 finalPos = glm::vec3(xPos, 0, yPos);
	return finalPos;

}

bool TileGeneration::_checkAdjacents(int x, int y, std::shared_ptr<Hydra::Component::RoomComponent>& r) {
	if (r->door[r->NORTH])
	{
		if (y >= ROOM_GRID_SIZE - 1)
			return false;
		if (roomGrid[x][y + 1] != nullptr)
			if (roomGrid[x][y + 1]->door[r->SOUTH] == false)
				return false;
		
	}
	if (r->door[r->EAST])
	{
		if (x <= 0)
			return false;
		if (roomGrid[x - 1][y] != nullptr)
			if(roomGrid[x - 1][y]->door[r->WEST] == false)
				return false;
	}
	if (r->door[r->SOUTH])
	{
		if (y <= 0)
			return false;
		if (roomGrid[x][y - 1] != nullptr)
			if(roomGrid[x][y - 1]->door[r->NORTH] == false)
				return false;
	}
	if (r->door[r->WEST])
	{
		if (x >= ROOM_GRID_SIZE - 1)
			return false;
		if (roomGrid[x + 1][y] != nullptr)
			if(roomGrid[x + 1][y]->door[r->EAST] == false)
				return false;
	}
	return true;
}
