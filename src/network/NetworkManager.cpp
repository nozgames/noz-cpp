/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/network/NetworkManager.h>
#include <noz/network/Client.h>
#include <noz/network/Server.h>

namespace noz::network
{
    NetworkManager::NetworkManager()
    {
    }

	NetworkManager::~NetworkManager()
	{
	}

    void NetworkManager::update()
    {
    }

	void NetworkManager::load()
	{
		ISingleton<NetworkManager>::load();
		instance()->loadInternal();
	}

	void NetworkManager::loadInternal()
	{
		if (enet_initialize() != 0)
			return;
	}

	void NetworkManager::unload()
	{
		instance()->unloadInternal();
		ISingleton<NetworkManager>::unload();
	}

    void NetworkManager::unloadInternal() 
    {
		enet_deinitialize();
    }

	std::unique_ptr<Client> NetworkManager::createClient()
	{
		return std::make_unique<Client>();
	}

	std::unique_ptr<Server> NetworkManager::createServer()
	{
		return std::make_unique<Server>();
	}
}

