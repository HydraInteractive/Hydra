#include <server/packets.hpp>
#include <hydra/component/transformcomponent.hpp>
#include <hydra/component/bulletcomponent.hpp>
#include <hydra/component/rigidbodycomponent.hpp>
#include <hydra/component/meshcomponent.hpp>
#include <hydra/system/bulletphysicssystem.hpp>
#include <server/server.hpp>
#include <server/gameserver.hpp>

using namespace BarcodeServer;
using namespace Hydra::Network;

using world = Hydra::World::World;

ServerDeleteEntityPacket* BarcodeServer::createServerDeleteEntityPacket(Hydra::World::EntityID entID) {
	ServerDeleteEntityPacket* sdp = new ServerDeleteEntityPacket();
	sdp->id = entID;
	return sdp;
}

ServerPlayerPacket* BarcodeServer::createServerPlayerPacket(const std::string& name, const TransformInfo& ti) {
	ServerPlayerPacket* spp = (ServerPlayerPacket*)new char[sizeof(ServerPlayerPacket) + name.size()];
	*spp = ServerPlayerPacket(name.size());
	spp->ti = ti;
	memcpy(spp->name, name.c_str(), name.size());
	return spp;
}

void BarcodeServer::resolveClientUpdatePacket(Player* p, ClientUpdatePacket* cup, Hydra::World::EntityID entityID) {
	std::vector<Hydra::World::EntityID> children = World::root()->children;
	if (cup->client == 1) {
		int j = 0;
		j++;
	}
	for (size_t i = 0; i < children.size(); i++) {
		if (children[i] == entityID) {
			auto entity = Hydra::World::World::World::getEntity(children[i]);
			Hydra::Component::TransformComponent* tc = entity->getComponent<Hydra::Component::TransformComponent>().get();
			if (tc != nullptr) {
				auto mesh = entity->getComponent<Hydra::Component::MeshComponent>();

				if (mesh) {
					if (cup->ti.pos == tc->position)
						mesh->animationIndex = p->shootAnimation > 0 ? 2 : 0;
					else {
						mesh->animationIndex = 1;
						p->shootAnimation = 0;
					}
				}
				tc->setPosition(cup->ti.pos);
				tc->setScale(cup->ti.scale);
				tc->setRotation(cup->ti.rot);
			}
			auto rbc = entity->getComponent<Hydra::Component::RigidBodyComponent>();
			if (rbc)
				rbc->refreshTransform();

			return;
		}
	}
}

Hydra::Network::ServerSpawnEntityPacket* BarcodeServer::createServerSpawnEntity(Hydra::World::Entity* ent) {
	std::vector<uint8_t> data;
	nlohmann::json json;
	ent->serialize(json);
	data = json.to_msgpack(json);

	ServerSpawnEntityPacket* packet = (ServerSpawnEntityPacket*)new char[sizeof(ServerSpawnEntityPacket) + data.size()];
	*packet = ServerSpawnEntityPacket(data.size());
	packet->id = ent->id;
	memcpy(packet->data, data.data(), data.size());

	return packet;
}

void BarcodeServer::createAndSendPlayerUpdateBulletPacket(Player * p, Server * s) {
	std::vector<uint8_t> vec = p->bullet.to_msgpack(p->bullet);

	ServerUpdateBulletPacket* packet = (ServerUpdateBulletPacket*)new char[sizeof(ServerUpdateBulletPacket) + vec.size()];
	*packet = ServerUpdateBulletPacket(vec.size());
	packet->serverPlayerID = p->entityid;
	memcpy(packet->data, vec.data(), vec.size());
	s->sendDataToAllExcept((char*)packet, packet->len, p->serverid);
	delete[] packet;
}

void BarcodeServer::createAndSendPlayerShootPacket(Player * p, ClientShootPacket * csp, Server * s) {
	ServerShootPacket* packet = new ServerShootPacket();

	packet->direction = csp->direction;
	packet->ti = csp->ti;
	packet->serverPlayerID = p->entityid;

	printf("sendDataToAllExcept:\n\ttype: ServerShoot\n\tlen: %zu\n", packet->len);
	s->sendDataToAllExcept((char*)packet, sizeof(ServerShootPacket), p->serverid);

	delete packet;
}

Hydra::World::Entity* BarcodeServer::resolveClientSpawnEntityPacket(ClientSpawnEntityPacket* csep, Hydra::World::EntityID id, Server* s) {
	Hydra::World::Entity* ent = World::newEntity("CLIENT CREATED (ERROR)", World::root()).get();
	std::vector<uint8_t> data = std::vector<uint8_t>(&csep->data[0], &csep->data[csep->size()]);

	memcpy(data.data(), csep->data, csep->size());

	nlohmann::json json = nlohmann::json::from_msgpack(data);
	ent->deserialize(json);

	printf("Client Created Entity: \"%s\" with id: %zu\n", ent->name.c_str(), ent->id);

	ServerSpawnEntityPacket* packet = (ServerSpawnEntityPacket*)new char[sizeof(ServerSpawnEntityPacket) + data.size()];
	*packet = ServerSpawnEntityPacket(data.size());
	packet->id = id;
	memcpy(packet->data, data.data(), data.size());

	printf("sendDataToAll:\n\ttype: ServerShoot\n\tlen: %zu\n", packet->len);
	s->sendDataToAll((char*)packet, packet->len);

	delete[] packet;
	return ent;
}

void BarcodeServer::resolveClientUpdateBulletPacket(ClientUpdateBulletPacket * cubp, nlohmann::json& dest) {
	dest = nlohmann::json::from_msgpack(std::vector<uint8_t>(&cubp->data[0], &cubp->data[cubp->size()]));
	printf("Successfully updated bullet.\n");
}

Hydra::World::Entity* BarcodeServer::resolveClientShootPacket(ClientShootPacket * csp, Player * p, Hydra::World::ISystem* physics) {
	Hydra::System::BulletPhysicsSystem* bps = static_cast<Hydra::System::BulletPhysicsSystem*>(physics);
	std::shared_ptr<Hydra::World::Entity> ptr = world::newEntity("BULLET", world::root());
	ptr->deserialize(p->bullet);
	auto otherptr = ptr->getComponent<Hydra::Component::BulletComponent>();
	otherptr->direction = csp->direction;
	auto transform = ptr->getComponent<Hydra::Component::TransformComponent>();
	transform->setPosition(csp->ti.pos);
	transform->setScale(csp->ti.scale);
	transform->setRotation(csp->ti.rot);
	auto r = ptr->getComponent<Hydra::Component::RigidBodyComponent>();
	if (r) {
		r->setLinearVelocity(otherptr->direction * otherptr->velocity);
		bps->enable(r.get());
		r->setGravity(glm::vec3(0, 0, 0));
	}

	if (auto player = world::getEntity(p->entityid); player)
		if (auto mesh = player->getComponent<Hydra::Component::MeshComponent>(); mesh) {
			mesh->animationIndex = 2;
			p->shootAnimation = 0.25;
		}
	return ptr.get();
}
