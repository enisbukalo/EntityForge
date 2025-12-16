#ifndef CAUDIOSETTINGS_H
#define CAUDIOSETTINGS_H

#include "AudioTypes.h"

namespace Components
{

/**
 * @brief Persistent audio configuration intended for save/load.
 *
 * This is world state (ECS data), not runtime-only system state.
 */
struct CAudioSettings
{
    float masterVolume = AudioConstants::DEFAULT_MASTER_VOLUME;
    float musicVolume  = AudioConstants::DEFAULT_MUSIC_VOLUME;
    float sfxVolume    = AudioConstants::DEFAULT_SFX_VOLUME;
};

}  // namespace Components

#endif  // CAUDIOSETTINGS_H
