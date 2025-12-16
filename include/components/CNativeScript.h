#ifndef CNATIVESCRIPT_H
#define CNATIVESCRIPT_H

#include <Entity.h>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

class World;

namespace Components
{

namespace Detail
{

template <typename T, typename = void>
struct HasScriptName : std::false_type
{
};

template <typename T>
struct HasScriptName<T, std::void_t<decltype(T::kScriptName)>> : std::true_type
{
};

}  // namespace Detail

class INativeScript
{
public:
    virtual ~INativeScript() = default;

    virtual void onCreate(Entity self, World& world)
    {
        (void)self;
        (void)world;
    }

    virtual void onUpdate(float deltaTime, Entity self, World& world) = 0;
};

struct CNativeScript
{
    std::unique_ptr<INativeScript> instance;
    std::string                    scriptTypeName;
    bool                           created = false;

    template <typename T, typename... Args>
    void bind(Args&&... args)
    {
        static_assert(std::is_base_of<INativeScript, T>::value,
                      "CNativeScript::bind requires T to derive from INativeScript");
        instance = std::make_unique<T>(std::forward<Args>(args)...);
        scriptTypeName.clear();

        // If the script type declares a stable name, capture it for serialization.
        // This keeps existing scripts working even if they don't define kScriptName.
        if constexpr (Detail::HasScriptName<T>::value)
        {
            scriptTypeName = std::string(T::kScriptName);
        }
        created  = false;
    }

    bool isBound() const
    {
        return static_cast<bool>(instance);
    }
};

}  // namespace Components

#endif  // CNATIVESCRIPT_H
