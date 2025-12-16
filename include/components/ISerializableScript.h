#ifndef ISERIALIZABLE_SCRIPT_H
#define ISERIALIZABLE_SCRIPT_H

#include <ScriptFieldArchive.h>

namespace Components
{

/**
 * @brief Optional interface for native scripts to persist selected fields.
 *
 * Scripts that implement this interface can round-trip their chosen data through
 * the save-game system.
 */
class ISerializableScript
{
public:
    virtual ~ISerializableScript() = default;

    virtual void serializeFields(Serialization::ScriptFieldWriter& out) const  = 0;
    virtual void deserializeFields(const Serialization::ScriptFieldReader& in) = 0;
};

}  // namespace Components

#endif  // ISERIALIZABLE_SCRIPT_H
