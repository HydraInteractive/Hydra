#include <hydra/network/netclient.hpp>
#include <hydra/world/world.hpp>
#include <hydra/component/meshcomponent.hpp>
#include <hydra/component/rigidbodycomponent.hpp>
#include <hydra/system/bulletphysicssystem.hpp>
#include <hydra/component/ghostobjectcomponent.hpp>
#include <hydra/component/cameracomponent.hpp>
#include <hydra/component/bulletcomponent.hpp>
#include <hydra/component/playercomponent.hpp>
#include <hydra/component/lifecomponent.hpp>
#include <hydra/component/roomcomponent.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <hydra/engine.hpp>

using namespace Hydra::Network;
using world = Hydra::World::World;

bool NetClient::running = false;
NetClient::updatePVS_f NetClient::updatePVS = nullptr;
NetClient::onWin_f NetClient::onWin = nullptr;
NetClient::onWin_f NetClient::onNoPVS = nullptr;
NetClient::onNewEntity_f NetClient::onNewEntity = nullptr;
NetClient::updatePathMap_f NetClient::updatePathMap = nullptr;
NetClient::updatePath_f NetClient::updatePath = nullptr;
void* NetClient::userdata = nullptr;

TCPClient NetClient::_tcp;
EntityID NetClient::_myID;
std::map<ServerID, EntityID> NetClient::_IDs;
std::map<ServerID, nlohmann::json> NetClient::_bullets;

void NetClient::enableEntity(Entity* ent) {
	if (!ent)
		return;

	Hydra::System::BulletPhysicsSystem* bulletsystem = (Hydra::System::BulletPhysicsSystem*)Hydra::IEngine::getInstance()->getState()->getPhysicsSystem();
	Hydra::Component::RigidBodyComponent* rgb = ent->getComponent<Hydra::Component::RigidBodyComponent>().get();
	if (rgb)
		bulletsystem->enable(rgb);

	auto tc = ent->getComponent<TransformComponent>();
	auto ghostobject = ent->getComponent<GhostObjectComponent>();
	if (tc && ghostobject) {
		ghostobject->updateWorldTransform();
		bulletsystem->enable(static_cast<Hydra::Component::GhostObjectComponent*>(ghostobject.get()));
	}
	std::vector<EntityID> children = ent->children;
	for (size_t i = 0; i < children.size(); i++)
		enableEntity(world::getEntity(children[i]).get());
}

void NetClient::_sendUpdatePacket() {
	Entity* tmp = world::getEntity(_myID).get();
	if (tmp) {
		ClientUpdatePacket cpup{};
		auto tc = tmp->getComponent<Hydra::Component::TransformComponent>();
		auto cc = tmp->getComponent<Hydra::Component::CameraComponent>();
		cpup.ti.pos = tc->position;
		const float PLAYER_HEIGHT = 3.25; // Make sure to update gamestate.cpp and netclient.cpp
		cpup.ti.pos.y -= PLAYER_HEIGHT;
		cpup.ti.scale = tc->scale;
		//cpup.ti.rot = tc->rotation;
		cpup.ti.rot = glm::angleAxis(cc->cameraYaw - 1.6f /* Player model fix */, glm::vec3(0, -1, 0));;

		_tcp.send(&cpup, cpup.len);
	}
}
void NetClient::sendEntity(EntityID ent) {
	nlohmann::json json;
	Entity* entptr = world::getEntity(ent).get();
	entptr->serialize(json);
	std::vector<uint8_t> vec = json.to_msgpack(json);

	ClientSpawnEntityPacket* packet = (ClientSpawnEntityPacket*)new char[sizeof(ClientSpawnEntityPacket) + vec.size()];
	*packet = ClientSpawnEntityPacket(vec.size());

	memcpy(packet->data, vec.data(), vec.size());

	_tcp.send((char*)packet, packet->len);

	delete[](char*)packet;
}

void NetClient::_resolvePackets() {
	std::vector<Packet*> packets = _tcp.receiveData();
	Hydra::Component::TransformComponent* tc;
	std::vector<EntityID> children;
	Entity* ent = nullptr;
	Packet* serverUpdate = nullptr;
	for (size_t i = 0; i < packets.size(); i++) {
		auto& p = packets[i];
		switch (p->type) {
		case PacketType::ServerInitialize:
			children = world::root()->children;
			for (size_t i = 0; i < children.size(); i++) {
				ent = world::getEntity(children[i]).get();
				if (ent->name == "Player") {
					_myID = children[i];
					break;
				}
			}
			_IDs[((ServerInitializePacket*)p)->entityid] = _myID;
			tc = ent->getComponent<Hydra::Component::TransformComponent>().get();
			tc->setPosition(((ServerInitializePacket*)p)->ti.pos);
			tc->setRotation(((ServerInitializePacket*)p)->ti.rot);
			tc->setScale(((ServerInitializePacket*)p)->ti.scale);

			if (auto rb = ent->getComponent<Hydra::Component::RigidBodyComponent>(); rb)
				rb->refreshTransform();
			if (auto go = ent->getComponent<Hydra::Component::GhostObjectComponent>(); go)
				go->updateWorldTransform();
			break;
		case PacketType::ServerUpdate:
			serverUpdate = p;
			break;
		case PacketType::ServerPlayer:
			_addPlayer(p);
			break;
		case PacketType::ServerSpawnEntity:
			_resolveServerSpawnEntityPacket((ServerSpawnEntityPacket*)p);
			break;
		case PacketType::ServerDeleteEntity:
			_resolveServerDeleteEntityPacket((ServerDeleteEntityPacket*)p);
			break;
		case PacketType::ServerUpdateBullet: {
			auto ubp = (ServerUpdateBulletPacket*)p;
			_bullets[ubp->serverPlayerID] = nlohmann::json::from_msgpack(std::vector<uint8_t>(&ubp->data[0], &ubp->data[ubp->size()]));
			break;
		}
		case PacketType::ServerShoot: {
			auto ss = (ServerShootPacket*)p;
			auto b = world::newEntity("EXTERNAL BULLET", world::root());
			b->deserialize(_bullets[ss->serverPlayerID]);
			auto bc = b->getComponent<Hydra::Component::BulletComponent>();
			bc->direction = ss->direction;
			auto tc = b->getComponent<Hydra::Component::TransformComponent>();
			tc->position = ss->ti.pos;
			tc->scale = ss->ti.scale;
			tc->rotation = ss->ti.rot;
			auto r = b->getComponent<Hydra::Component::RigidBodyComponent>();
			if (r) {
				r->setLinearVelocity(bc->direction*bc->velocity);
				static_cast<Hydra::System::BulletPhysicsSystem*>(Hydra::IEngine::getInstance()->getState()->getPhysicsSystem())->enable(r.get());
				r->setGravity(glm::vec3(0, 0, 0));
			}
			break;
		}
		case PacketType::ServerFreezePlayer: {
			auto sfp = (ServerFreezePlayerPacket*)p;
			if (sfp->action == ServerFreezePlayerPacket::Action::win) {
				if (onWin)
					onWin(userdata);
			}
			else if (sfp->action == ServerFreezePlayerPacket::Action::noPVS){

				printf("ASODIJAISFDJISADJFISJIODFJIOSDJFIJSIODFJIOSJIODFJSIODFJISOJFIOSD");
				if (onNoPVS) {

					printf("222222222222");
					onNoPVS(userdata);
				}
			}
			else
				for (auto p : Hydra::Component::PlayerComponent::componentHandler->getActiveComponents())
					((Hydra::Component::PlayerComponent*)p.get())->frozen = sfp->action == ServerFreezePlayerPacket::Action::freeze;
			break;
		}
		case PacketType::ServerInitializePVS: {
			if (!updatePVS)
				break;
			auto sipp = (ServerInitializePVSPacket*)p;
			nlohmann::json json = nlohmann::json::parse(sipp->data, sipp->data + sipp->size());
			updatePVS(std::move(json), userdata);
			break;
		}
		case PacketType::ServerPathMap: {
			if (!updatePathMap)
				break;
			auto spm = (ServerPathMapPacket*)p;
			bool* map = new bool[WORLD_MAP_SIZE * WORLD_MAP_SIZE];
			for (int i = 0; i < WORLD_MAP_SIZE * WORLD_MAP_SIZE; i++)
			{
				map[i] = spm->data[i];
			}

			updatePathMap((bool*)map, userdata);
			break;
		}
		case PacketType::ServerAIInfo: {
			if (!updatePath)
				break;
			auto sai = (ServerAIInfoPacket*)p;
			std::vector<glm::ivec2> openList;
			std::vector<glm::ivec2> closedList;
			std::vector<glm::ivec2> pathToend;
			//std::cout << "Vec2s recieved:" << sai->openList << " " << sai->closedList << " " << sai->pathToEnd << std::endl;
			size_t i = 0;
			while (i < sai->openList * 2)
			{
				int x = sai->data[i];
				i++;
				int y = sai->data[i];
				i++;
				openList.push_back(glm::ivec2(x, y));
			}
			while (i < (sai->openList + sai->closedList) * 2)
			{
				int x = sai->data[i];
				i++;
				int y = sai->data[i];
				i++;
				closedList.push_back(glm::ivec2(x, y));
			}
			while (i < (sai->openList + sai->closedList + sai->pathToEnd) * 2)
			{
				int x = sai->data[i];
				i++;
				int y = sai->data[i];
				i++;
				pathToend.push_back(glm::ivec2(x, y));
			}
			updatePath(openList, closedList, pathToend, userdata);
			break;
		}
		default:
			printf("UNKNOWN PACKET: %s\n", (p->type < PacketType::MAX_COUNT ? PacketTypeName[p->type] : "(unk)"));
			break;
		}
	}

	if (serverUpdate)
		_updateWorld(serverUpdate);

	for (size_t i = 0; i < packets.size(); i++)
		delete[] (char*)packets[i];
}

//ADD ENTITES IN ANY OTHER PLACE THAN ROOT?
void NetClient::_resolveServerSpawnEntityPacket(ServerSpawnEntityPacket* entPacket) {
	nlohmann::json json = nlohmann::json::from_msgpack(std::vector<uint8_t>(&entPacket->data[0], &entPacket->data[entPacket->size()]));
	Entity* ent = world::newEntity("SERVER CREATED (ERROR)", world::root()).get();
	ent->deserialize(json);
	_IDs[entPacket->id] = ent->id;

	enableEntity(ent);
	if (onNewEntity)
		onNewEntity(ent, userdata);
}

void NetClient::_resolveServerDeleteEntityPacket(ServerDeleteEntityPacket* delPacket) {
	std::vector<EntityID> children = world::root()->children;
	for (size_t i = 0; i < children.size(); i++)
		if (children[i] == _IDs[delPacket->id]) {
			world::getEntity(children[i])->dead = true;

			_bullets.erase(delPacket->id);
			_IDs.erase(delPacket->id);
			break;
		}
}

void NetClient::_updateWorld(Packet * updatePacket) {
	std::vector<EntityID> children = world::root()->children;
	ServerUpdatePacket* sup = (ServerUpdatePacket*)updatePacket;
	Hydra::Component::TransformComponent* tc;
	for (size_t k = 0; k < sup->nrOfEntUpdates(); k++) {
		if (_IDs[sup->data[k].entityid] == _myID) {
			ServerUpdatePacket::EntUpdate& entupdate = (ServerUpdatePacket::EntUpdate&)sup->data[k];
			LifeComponent* life = world::getEntity(_myID)->getComponent<LifeComponent>().get();
			if (life)
				life->health = entupdate.life;
			continue;
		}
		else if (_IDs[sup->data[k].entityid] == 0) {
			printf("Error updating entity: %zu\n", _IDs[sup->data[k].entityid]);

			auto ent = world::newEntity("ERROR: UNKOWN ENTITY", world::root());
			_IDs[sup->data[k].entityid] = ent->id;
			auto mesh = ent->addComponent<MeshComponent>();
			mesh->loadMesh("assets/objects/characters/AlienModel2.mATTIC");
			auto transform = ent->addComponent<TransformComponent>();
			transform->position = { ((ServerUpdatePacket::EntUpdate&)sup->data[k]).ti.pos.x, ((ServerUpdatePacket::EntUpdate&)sup->data[k]).ti.pos.y, ((ServerUpdatePacket::EntUpdate&)sup->data[k]).ti.pos.z };

			continue;
		}
		for (size_t i = 0; i < children.size(); i++) {
			if (children[i] == _IDs[((ServerUpdatePacket::EntUpdate&)sup->data[k]).entityid]) {
				auto ent = world::getEntity(children[i]);
				ServerUpdatePacket::EntUpdate& entupdate = (ServerUpdatePacket::EntUpdate&)sup->data[k];
				tc = ent->getComponent<Hydra::Component::TransformComponent>().get();
				if (tc) {
					tc->position = { entupdate.ti.pos.x,entupdate.ti.pos.y,entupdate.ti.pos.z };
					tc->setRotation(entupdate.ti.rot);
					tc->setScale(entupdate.ti.scale);
				}

				LifeComponent* life = world::getEntity(children[i])->getComponent<LifeComponent>().get();
				if (life)
					life->health = entupdate.life;

				if (auto rb = ent->getComponent<Hydra::Component::RigidBodyComponent>(); rb) {
					rb->refreshTransform();
					rb->setActivationState(Hydra::Component::RigidBodyComponent::ActivationState::disableSimulation);
				}

				if (auto go = ent->getComponent<Hydra::Component::GhostObjectComponent>(); go)
					go->updateWorldTransform();

				auto mesh = world::getEntity(children[i])->getComponent<MeshComponent>();
				if (mesh)
					mesh->animationIndex = entupdate.animationIndex;
				break;
			}
		}
	}
}

void NetClient::_addPlayer(Packet * playerPacket) {
	ServerPlayerPacket* spp = (ServerPlayerPacket*)playerPacket;
	char* c = new char[spp->nameLength() + 1];
	memcpy(c, spp->name, spp->nameLength());
	c[spp->nameLength()] = '\0';

	Entity* ent = world::newEntity(c, world::root()).get();
	//ent->setID(spp->entID);
	_IDs[spp->entID] = ent->id;
	Hydra::Component::TransformComponent* tc = ent->addComponent<Hydra::Component::TransformComponent>().get();
	auto mesh = ent->addComponent<Hydra::Component::MeshComponent>();
	mesh->loadMesh("assets/objects/characters/PlayerModel2.mATTIC");
	tc->setPosition(spp->ti.pos);
	tc->setRotation(spp->ti.rot);
	tc->setScale(spp->ti.scale);

	auto rbc = ent->addComponent<Hydra::Component::RigidBodyComponent>();
	rbc->createBox(glm::vec3(1.0f, 2.0f, 1.0f), glm::vec3(0, 2, 0), Hydra::System::BulletPhysicsSystem::CollisionTypes::COLL_PLAYER, 100,
		0, 0, 0.0f, 0);
	rbc->setAngularForce(glm::vec3(0, 0, 0));

	rbc->setActivationState(Hydra::Component::RigidBodyComponent::ActivationState::disableSimulation);
	auto bulletPhysWorld = static_cast<Hydra::System::BulletPhysicsSystem*>(IEngine::getInstance()->getState()->getPhysicsSystem());
	bulletPhysWorld->enable(rbc.get());
	delete[] c;
}

bool NetClient::initialize(char* ip, int port) {
	SDLNet_Init();
	if (_tcp.initialize(ip, port)) {
		_myID = 0;
		NetClient::running = true;
		return true;
	}
	NetClient::running = false;
	return false;
}

void NetClient::shoot(Hydra::Component::TransformComponent * tc, const glm::vec3& direction) {
	if (NetClient::_tcp.isConnected()) {
		ClientShootPacket csp{};

		csp.direction = direction;
		csp.ti.pos = tc->position;
		csp.ti.scale = tc->scale;
		csp.ti.rot = tc->rotation;

		NetClient::_tcp.send(&csp, csp.len);
	}
}

void NetClient::updateBullet(EntityID newBulletID) {
	nlohmann::json json;
	Entity* entptr = world::getEntity(newBulletID).get();
	entptr->serialize(json);
	std::vector<uint8_t> vec = json.to_msgpack(json);
	ClientUpdateBulletPacket* packet = (ClientUpdateBulletPacket*)new char[sizeof(ClientUpdateBulletPacket) + vec.size()];
	*packet = ClientUpdateBulletPacket(vec.size());

	memcpy(packet->data, vec.data(), vec.size());

	_tcp.send((char*)packet, sizeof(ClientUpdateBulletPacket) + vec.size());
	delete[](char*)packet;
}

void NetClient::run() {
	{//Receive packets
		_resolvePackets();
	}

	//SendUpdate packet
	{
		_sendUpdatePacket();
	}

	//Nåt
	{
		//du gillar sovpotatisar no?
	}
}

void NetClient::requestAIInfo(Hydra::World::EntityID id)
{
	ServerID serverID = 0;
	for (auto i : _IDs)
	{
		if (i.second == id)
		{
			serverID = i.first;
			break;
		}
	}
	if (serverID == 0)
	{
		return;
	}
	ClientRequestAIInfoPacket packet{};
	packet.serverEntityID = serverID;
	NetClient::_tcp.send((char*)&packet, packet.len);
}

void NetClient::reset() {
	if (!running)
		return;
	_tcp.close();
	_IDs.clear();
	updatePVS = nullptr;
	onWin = nullptr;
	onNewEntity = nullptr;
	updatePathMap = nullptr;
	userdata = nullptr;
	running = false;
}
