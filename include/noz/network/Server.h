#pragma once

#include "NetworkTypes.h"

namespace noz::network
{
    enum class ServerState
    {
        Stopped,
        Starting,
        Running,
        Stopping
    };

    struct ClientInfo
    {
        uint32_t id;
        ENetPeer* peer;
        std::string address;
        uint16_t port;
        bool connected;
        int ping;
        uint64_t lastSeen;
    };

    using ClientConnectHandler = std::function<void(const ClientInfo&)>;
    using ClientDisconnectHandler = std::function<void(const ClientInfo&)>;
    using ServerMessageHandler = std::function<void(const ClientInfo&, const std::vector<uint8_t>&)>;

    class Server
    {
    public:

        Server();
        ~Server();

        bool start(const std::string& host, uint16_t port, uint32_t maxClients = 32);
        
        void stop();

        bool send(uint32_t clientId, const std::vector<uint8_t>& data, uint32_t channel = 0, bool reliable = false);

        bool broadcast(const std::vector<uint8_t>& data, uint32_t channel = 0, bool reliable = false);

        // Disconnect specific client
        void DisconnectClient(uint32_t clientId, uint32_t reason = 0);

        void update();

        ServerState GetState() const { return _state; }

        uint32_t GetClientCount() const { return static_cast<uint32_t>(_clients.size()); }

        const std::unordered_map<uint32_t, ClientInfo>& GetClients() const { return _clients; }

        const ClientInfo* client(uint32_t clientId) const;

		void setMessageHandler(ServerMessageHandler handler);
		void setClientConnectHandler(ClientConnectHandler handler);
		void setClientDisconnectHandler(ClientDisconnectHandler handler);

		bool isRunning() const;

        // Get server info
        std::string GetHost() const { return _host; }
        uint16_t GetPort() const { return _port; }
        uint32_t GetMaxClients() const { return _maxClients; }

    private:

		bool send(ENetPeer* peer, const std::vector<uint8_t>& data, uint32_t channel = 0, bool reliable = false);

		void handleConnect(const ENetEvent& event);
		void handleDisconnect(const ENetEvent& event);
		void handleReceive(const ENetEvent& event);

        ENetHost* _enetHost;
        ServerState _state;
        std::string _host;
        uint16_t _port;
        uint32_t _maxClients;
        uint32_t _nextClientId;
        std::unordered_map<uint32_t, ClientInfo> _clients;
        ServerMessageHandler _messageHandler;
        ClientConnectHandler _clientConnectHandler;
        ClientDisconnectHandler _clientDisconnectHandler;
    };

	inline bool Server::isRunning() const
	{
		return _state == ServerState::Running;
	}
}