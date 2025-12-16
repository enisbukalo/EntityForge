#ifndef JSON_COMPONENT_SERIALIZERS_H
#define JSON_COMPONENT_SERIALIZERS_H

#include "ComponentSerializationRegistry.h"

namespace Serialization
{

/**
 * @brief Registers built-in engine component serializers for JSON save games.
 */
void registerBuiltInJsonComponentSerializers(ComponentSerializationRegistry& registry);

}  // namespace Serialization

#endif  // JSON_COMPONENT_SERIALIZERS_H
