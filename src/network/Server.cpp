#include <noz/network/Server.h>

namespace noz::network
{
    Server::Server()
        : _enetHost(nullptr)
        , _state(ServerState::Stopped)
        , _host("")
        , _port(0)
        , _maxClients(32)
        , _nextClientId(1)
    {
    }

    Server::~Server()
    {
        stop();
    }

    bool Server::start(const std::string& host, uint16_t port, uint32_t maxClients)
    {
        if (_state != ServerState::Stopped)
        {
            std::cerr << "Server already running or starting" << std::endl;
            return false;
        }

        _state = ServerState::Starting;
        _host = host;
        _port = port;
        _maxClients = maxClients;

        // Set up address
        ENetAddress address;
        enet_address_set_host_ip(&address, host.c_str());
        address.port = port;

        // Create host
        _enetHost = enet_host_create(&address, maxClients, 2, 0, 0);
        if (!_enetHost)
        {
            _state = ServerState::Stopped;
            return false;
        }

        _state = ServerState::Running;
        return true;
    }

    void Server::stop()
    {
        if (_state == ServerState::Stopped)
            return;

        _state = ServerState::Stopping;

        if (_enetHost)
        {
            // Disconnect all clients
            for (auto& [id, client] : _clients)
                if (client.connected)
                    enet_peer_disconnect(client.peer, 0);

            // Wait for disconnects
            ENetEvent event;
            while (enet_host_service(_enetHost, &event, 3000) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_RECEIVE:
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    // Remove client from our list
                    for (auto it = _clients.begin(); it != _clients.end(); ++it)
                    {
                        if (it->second.peer == event.peer)
                        {
                            if (_clientDisconnectHandler)
                                _clientDisconnectHandler(it->second);

							_clients.erase(it);
                            break;
                        }
                    }
                    break;
                case ENET_EVENT_TYPE_NONE:
                    break;
                }
            }

            // Force disconnect remaining clients
            for (auto& [id, client] : _clients)
            {
                enet_peer_reset(client.peer);
            }

            enet_host_destroy(_enetHost);
            _enetHost = nullptr;
        }

        _clients.clear();
        _state = ServerState::Stopped;
        std::cout << "Server stopped" << std::endl;
    }

    bool Server::send(uint32_t clientId, const std::vector<uint8_t>& data, uint32_t channel, bool reliable)
    {
        auto it = _clients.find(clientId);
        if (it == _clients.end() || !it->second.connected)
            return false;

        return send(it->second.peer, data, channel, reliable);
    }

    bool Server::send(ENetPeer* peer, const std::vector<uint8_t>& data, uint32_t channel, bool reliable)
    {
		assert(peer);

        if (_state != ServerState::Running)
            return false;

        auto* packet = enet_packet_create(
            data.data(),
            data.size(),
            reliable ? ENET_PACKET_FLAG_RELIABLE : 0
        );

        if (!packet)
            return false;

        int result = enet_peer_send(peer, channel, packet);
        if (result < 0)
            return false;

        return true;
    }

    bool Server::broadcast(const std::vector<uint8_t>& data, uint32_t channel, bool reliable)
    {
		assert(_enetHost);

        if (_state != ServerState::Running)
            return false;

        auto* packet = enet_packet_create(
            data.data(),
            data.size(),
            reliable ? ENET_PACKET_FLAG_RELIABLE : 0
        );

        if (!packet)
            return false;

        enet_host_broadcast(_enetHost, channel, packet);
        return true;
    }

    void Server::DisconnectClient(uint32_t clientId, uint32_t reason)
    {
        auto it = _clients.find(clientId);
        if (it != _clients.end() && it->second.connected)
        {
            enet_peer_disconnect(it->second.peer, reason);
        }
    }

    void Server::update()
    {
		assert(_enetHost);

        if (_state != ServerState::Running)
            return;

        ENetEvent event;
        while (enet_host_service(_enetHost, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
				handleConnect(event);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
				handleDisconnect(event);
                break;

            case ENET_EVENT_TYPE_RECEIVE:
				handleReceive(event);
                break;

            case ENET_EVENT_TYPE_NONE:
                break;
            }
        }
    }

	void Server::handleConnect(const ENetEvent& event)
	{
		// Create new client info
		ClientInfo client;
		client.id = _nextClientId++;
		client.peer = event.peer;
		client.connected = true;
		client.ping = 0;
		client.lastSeen = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();

		// Get client address
		char addressStr[256];
		enet_address_get_host_ip(&event.peer->address, addressStr, sizeof(addressStr));
		client.address = addressStr;
		client.port = event.peer->address.port;

		_clients[client.id] = client;

		Log::info("server", "client " + std::to_string(client.id) + " connected from " + client.address + ":" + std::to_string(client.port));

		if (_clientConnectHandler)
			_clientConnectHandler(client);
	}

	void Server::handleDisconnect(const ENetEvent& event)
	{
		// Find and remove client
		for (auto it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (it->second.peer == event.peer)
			{
				if (_clientDisconnectHandler)
					_clientDisconnectHandler(it->second);

				_clients.erase(it);
				break;
			}
		}
	}

	void Server::handleReceive(const ENetEvent& event)
	{
		for (auto& [id, client] : _clients)
		{
			if (client.peer == event.peer)
			{
				std::vector<uint8_t> data(event.packet->data, event.packet->data + event.packet->dataLength);

				// Update client info
				client.lastSeen = std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::system_clock::now().time_since_epoch()).count();
				client.ping = event.peer->roundTripTime;

				if (_messageHandler && !data.empty())
					_messageHandler(client,  data);

				break;
			}
		}

		enet_packet_destroy(event.packet);
	}

    const ClientInfo* Server::client(uint32_t clientId) const
    {
        auto it = _clients.find(clientId);
        return (it != _clients.end()) ? &it->second : nullptr;
    }

	void Server::setMessageHandler(ServerMessageHandler handler)
	{
		_messageHandler = handler;
	}

	void Server::setClientConnectHandler(ClientConnectHandler handler)
	{
		_clientConnectHandler = handler;
	}

	void Server::setClientDisconnectHandler(ClientDisconnectHandler handler)
	{
		_clientDisconnectHandler = handler;
	}
}