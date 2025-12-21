#include "AudioManager.h"

#include "AudioManagerBehaviour.h"

#include <Components.h>
#include <World.h>

namespace Example
{

Entity spawnAudioManager(World& world)
{
    Entity audioManager = world.createEntity();
    Components::CNativeScript* script = world.components().add<Components::CNativeScript>(audioManager);
    script->bind<AudioManagerBehaviour>();
    return audioManager;
}

}  // namespace Example
