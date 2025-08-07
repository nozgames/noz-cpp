/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/network/Client.h>

namespace noz::network
{
    Client::Client()
        : _host(nullptr)
        , _peer(nullptr)
        , _state(ClientState::Disconnected)
        , _serverHost("")
        , _serverPort(0)
    {
    }

    Client::~Client()
    {
        disconnect();
    }

    bool Client::connect(const std::string& host, uint16_t port, uint32_t timeout)
    {
        if (_state != ClientState::Disconnected)
            return false;

        // Create host
        _host = enet_host_create(nullptr, 1, 2, 0, 0);
        if (!_host)
            return false;

        // Set up address
        ENetAddress address;
        enet_address_set_host_ip(&address, host.c_str());
        address.port = port;

        // Connect to server
        _peer = enet_host_connect(_host, &address, 2, 0);
        if (!_peer)
        {
            std::cerr << "Failed to initiate connection to server" << std::endl;
            enet_host_destroy(_host);
            _host = nullptr;
            return false;
        }

		setState(ClientState::Connecting);
        _serverHost = host;
        _serverPort = port;
        return true;
    }

    void Client::disconnect()
    {
        if (_peer && _state == ClientState::Connected)
        {
            enet_peer_disconnect(_peer, 0);
            
            // Wait for disconnect
            ENetEvent event;
            while (enet_host_service(_host, &event, 3000) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_RECEIVE:
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << "Disconnected from server" << std::endl;
                    goto disconnect_complete;
                }
            }
            
            // Force disconnect if timeout
            enet_peer_reset(_peer);
        }

    disconnect_complete:
        if (_host)
        {
            enet_host_destroy(_host);
            _host = nullptr;
        }
        
        _peer = nullptr;
		setState(ClientState::Disconnected);
    }

    bool Client::send(const std::vector<uint8_t>& data, uint32_t channel, bool reliable)
    {
		assert(_peer);
		assert(_state == ClientState::Connected);

        auto* packet = enet_packet_create(
            data.data(),
            data.size(),
            reliable ? ENET_PACKET_FLAG_RELIABLE : 0
        );

        if (!packet)
            return false;

        int result = enet_peer_send(_peer, channel, packet);
        if (result < 0)
            return false;

        return true;
    }

    void Client::update()
    {
		assert(_host);
		assert(_state != ClientState::Disconnected);

        ENetEvent event;
        while (enet_host_service(_host, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
				handleConnect(event);
                break;
                
            case ENET_EVENT_TYPE_RECEIVE:
				handleReceive(event);
                break;
                
            case ENET_EVENT_TYPE_DISCONNECT:
				handleDisconnect(event);
                break;
                
            case ENET_EVENT_TYPE_NONE:
                break;
            }
        }
        
        // Check for connection timeout
        if (_state == ClientState::Connecting && _peer)
        {
            // Simple timeout check - if peer is in a failed state
            if (_peer->state == ENET_PEER_STATE_DISCONNECTED)
            {
                setState(ClientState::ConnectionFailed);
                disconnect();
            }
        }
    }

	void Client::handleConnect(const ENetEvent& event)
	{
		setState(ClientState::Connected);
	}

	void Client::handleDisconnect(const ENetEvent& event)
	{
		_peer = nullptr;
		setState(ClientState::Disconnected);
	}

	void Client::handleReceive(const ENetEvent& event)
	{
		std::vector<uint8_t> data(event.packet->data, event.packet->data + event.packet->dataLength);
		if (_messageHandler && !data.empty())
		{
			MessageReader reader(data);
			_messageHandler(reader);
		}
		enet_packet_destroy(event.packet);
	}

	void Client::setState(ClientState state)
	{
		if (_state == state)
			return;

		_state = state;
		if (_stateHandler)
			_stateHandler(_state);
	}

	void Client::setStateHandler(ClientStateHandler handler)
	{
		_stateHandler = handler;
	}

	void Client::setMessageHandler(MessageHandler handler)
	{
		_messageHandler = handler;
	}

    int Client::GetPing() const
    {
        if (_state != ClientState::Connected || !_peer)
            return -1;

        return _peer->roundTripTime;
    }
}
