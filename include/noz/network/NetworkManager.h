#pragma once

namespace noz::network
{
    class Client;
    class Server;

    enum class NetworkEventType
    {
        Connect,
        Disconnect,
        Receive,
        Timeout
    };

    struct NetworkEvent
    {
        NetworkEventType type;
        //ENetPeer* peer;
        std::vector<uint8_t> data;
        uint32_t channel;
    };

    using NetworkEventHandler = std::function<void(const NetworkEvent&)>;

    class NetworkManager : public noz::ISingleton<NetworkManager>
    {
    public:

		NetworkManager();
        ~NetworkManager();

        void update();

		static void load();
		static void unload();

		std::unique_ptr<Client> createClient();
		std::unique_ptr<Server> createServer();

	private:

		void loadInternal();
		void unloadInternal();
    };
}