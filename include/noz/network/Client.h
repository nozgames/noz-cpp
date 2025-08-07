#pragma once

#include "NetworkTypes.h"
#include "MessageReader.h"

namespace noz::network
{
    enum class ClientState
    {
        Disconnected,
        Connecting,
        Connected,
        ConnectionFailed
    };

    using ClientStateHandler = std::function<void(ClientState)>;
    using MessageHandler = std::function<void(MessageReader& message)>;

    class Client
    {
    public:

		Client();
        ~Client();

        bool connect(const std::string& host, uint16_t port, uint32_t timeout = 5000);
        
        void disconnect();

        bool send(const std::vector<uint8_t>& data, uint32_t channel = 0, bool reliable = false);
        
        void update();

		ClientState state() const;

		void setStateHandler(ClientStateHandler handler);

		void setMessageHandler(MessageHandler handler);

        // Check if connected
        bool IsConnected() const { return _state == ClientState::Connected; }

        // Get connection info
        std::string host() const { return _serverHost; }
        uint16_t port() const { return _serverPort; }

        // Ping server (returns ping time in milliseconds, -1 if not connected)
        int GetPing() const;

    private:

		void setState(ClientState state);
		void handleConnect(const ENetEvent& event);
		void handleDisconnect(const ENetEvent& event);
		void handleReceive(const ENetEvent& event);

        ENetHost* _host;
        ENetPeer* _peer;
        ClientState _state;
        std::string _serverHost;
        uint16_t _serverPort;
		ClientStateHandler _stateHandler;
        MessageHandler _messageHandler;
    };

	inline ClientState Client::state() const
	{
		return _state;
	}
}
