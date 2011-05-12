#include "LobbyState.h"

using namespace Ogre;

LobbyState::LobbyState()
{
}

void LobbyState::enter()
{
    LOG("Entering LobbyState...");
}

bool LobbyState::pause()
{
    LOG("Pausing LobbyState...");

    return true;
}

void LobbyState::resume()
{
    LOG("Resuming LobbyState...");
}

void LobbyState::exit()
{
    LOG("Leaving LobbyState...");
}

void LobbyState::createScene()
{
}

void LobbyState::update(double timeSinceLastFrame)
{
}

void LobbyState::resized(Ogre::RenderWindow *r)
{
}
