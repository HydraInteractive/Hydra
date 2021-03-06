#include <server/clienthandler.hpp>
#define SocketSetSize 4

using namespace BarcodeServer;

int ClientHandler::_generateClientID() {
	return this->_currID++;
}

ClientHandler::ClientHandler() {
	this->_currID = 0;
}

ClientHandler::~ClientHandler() {
	for (size_t i = 0; i < this->_clients.size(); i++) {
		SDLNet_TCP_Close(this->_clients[i]->socket);
		delete this->_clients[i];
	}
	delete this->_msg;
}

void ClientHandler::initialize(char * msg) {
	this->_msg = msg;
	this->_currID = 0;
}

TCPsocket ClientHandler::getSocketFromID(int id) {
	for (size_t i = 0; i < this->_clients.size(); i++) {
		if (this->_clients[i]->id == id) {
			return this->_clients[i]->socket;
		}
	}
	return TCPsocket();
}

int ClientHandler::getActivity() {
	int pending = 0;
	for (size_t i = 0; i < this->_sets.size(); i++) {
		pending += SDLNet_CheckSockets(this->_sets[i].set, 0);
	}
	return pending;
}

std::vector<Hydra::Network::Packet*> ClientHandler::getReceivedData(int pending) {
	std::vector<Hydra::Network::Packet*> packets;
	Hydra::Network::Packet* tmp;
	Hydra::Network::Packet* tmp2;
	size_t curr = 0;
	size_t offset = 0;
	std::vector<int> tmpvec;
	for (size_t i = 0; i < this->_clients.size(); i++) {
		curr = 0;
		offset = 0;
		while (SDLNet_SocketReady(this->_clients[i]->socket)) {
			long lenTmp = SDLNet_TCP_Recv(this->_clients[i]->socket, this->_msg + offset, MAX_NETWORK_LENGTH - offset) + offset;
			if (lenTmp > 0) {
				size_t len = lenTmp;
				while (curr < len && curr < MAX_NETWORK_LENGTH) {
					tmp = (Hydra::Network::Packet*)(&this->_msg[curr]);
					if (curr + tmp->len > len) {
						memmove(_msg, _msg + curr, len - curr);
						offset = len - curr;
						curr = 0;
					}
					curr += tmp->len;
					tmp->client = this->_clients[i]->id;
					tmp2 = (Hydra::Network::Packet*)(new char[tmp->len]);
					memcpy(tmp2, tmp, tmp->len);
					packets.push_back(tmp2);
				}
				if (curr == len)
					offset = 0;
			} else
				tmpvec.push_back(this->_clients[i]->id);
		}
	}
	for (size_t i = 0; i < tmpvec.size(); i++) {
		this->_disconnectedClients.push_back(tmpvec[i]);
		this->disconnectClient(tmpvec[i]);
	}
	return packets;
}

int ClientHandler::addNewConnection(TCPsocket sock) {
	for (size_t i = 0; i < this->_sets.size(); i++) {
		if (this->_sets[i].nrOfClients < SocketSetSize) {
			SDLNet_TCP_AddSocket(this->_sets[i].set, sock);
			this->_clients.push_back(new Client);
			this->_clients[this->_clients.size() - 1]->id = this->_generateClientID();
			this->_clients[this->_clients.size() - 1]->socket = sock;
			this->_clients[this->_clients.size() - 1]->socketSet = i;
			this->_clients[this->_clients.size() - 1]->isDead = false;
			this->_sets[i].nrOfClients++;
			return this->_clients[this->_clients.size() - 1]->id;
		}
	}

	this->_sets.push_back(SocketSet(SDLNet_AllocSocketSet(SocketSetSize)));
	SDLNet_TCP_AddSocket(this->_sets[this->_sets.size() - 1].set, sock);

	this->_clients.push_back(new Client);
	this->_clients[this->_clients.size() - 1]->id = this->_generateClientID();
	this->_clients[this->_clients.size() - 1]->socket = sock;
	this->_clients[this->_clients.size() - 1]->socketSet = this->_sets.size() - 1;
	this->_clients[this->_clients.size() - 1]->isDead = false;
	this->_sets[this->_sets.size() - 1].nrOfClients = 1;

	return this->_clients[this->_clients.size() - 1]->id;
}

int ClientHandler::sendData(char * data, int len, int clientID) {
	int k = SDLNet_TCP_Send(this->getSocketFromID(clientID), data, len);
	if (k > 0)
		return k;
	else {
		this->_disconnectedClients.push_back(clientID);
		this->disconnectClient(clientID);
		return -1;
	}
}

std::vector<int> ClientHandler::getAllClients() {
	std::vector<int> vec;
	for (size_t i = 0; i < this->_clients.size(); i++) {
		vec.push_back(this->_clients[i]->id);
	}
	return vec;
}

std::vector<int> ClientHandler::getDisconnectedClients() {
	std::vector<int> result;
	for (size_t i = 0; i < this->_disconnectedClients.size(); i++) {
		result.push_back(this->_disconnectedClients[i]);
	}
	this->_disconnectedClients.clear();
	return result;
}

void ClientHandler::disconnectClient(int id) {
	for (size_t i = 0; i < this->_clients.size(); i++) {
		if (this->_clients[i]->id == id) {
			SDLNet_TCP_Close(this->_clients[i]->socket);
			SDLNet_TCP_DelSocket(this->_sets[_clients[i]->socketSet].set, _clients[i]->socket);
			this->_sets[_clients[i]->socketSet].nrOfClients--;
			delete this->_clients[i];
			this->_clients.erase(this->_clients.begin() + i);
			break;
		}
	}
}

int ClientHandler::getNrOfClients() {
	return this->_clients.size();
}
