#pragma once

#include <EventBus.h>

#include <ObjectiveEvents.h>
#include <System.h>
#include <TriggerEvents.h>

#include <vector>

class World;

namespace Objectives
{
class ObjectiveRegistry;
}

namespace Systems
{

/**
 * @brief quests runtime system.
 *
 * Subscribes to objective-related events and updates Components::CObjectives.
 * Subscriptions are registered lazily on first update() so they exist before the first EventBus pump.
 */
class SObjectives : public System
{
public:
    explicit SObjectives(::Objectives::ObjectiveRegistry* registry = nullptr) : m_registry(registry) {}
    ~SObjectives() = default;

    SObjectives(const SObjectives&)            = delete;
    SObjectives(SObjectives&&)                 = delete;
    SObjectives& operator=(const SObjectives&) = delete;
    SObjectives& operator=(SObjectives&&)      = delete;

    void update(float /*deltaTime*/, World& world) override;

    std::string_view name() const override
    {
        return "SObjectives";
    }

private:
    ::Objectives::ObjectiveRegistry* m_registry{nullptr};

    bool m_initialized{false};

    std::vector<Objectives::ObjectiveManagerCounterIncrement> m_pendingManagerCounterIncrements;
    std::vector<Objectives::ObjectiveManagerFlagSet>          m_pendingManagerFlagSets;
    std::vector<Objectives::ObjectiveActivate>                m_pendingActivates;
    std::vector<Objectives::ObjectiveCounterIncrement>        m_pendingCounterIncrements;
    std::vector<Objectives::ObjectiveComplete>                m_pendingCompletes;
    std::vector<Objectives::ObjectiveSignal>                  m_pendingSignals;
    std::vector<Physics::TriggerEnter>                        m_pendingTriggerEnters;
    std::vector<Physics::TriggerExit>                         m_pendingTriggerExits;

    ScopedSubscription m_managerCounterIncrementSub;
    ScopedSubscription m_managerFlagSetSub;
    ScopedSubscription m_activateSub;
    ScopedSubscription m_counterIncrementSub;
    ScopedSubscription m_completeSub;
    ScopedSubscription m_signalSub;

    ScopedSubscription m_triggerEnterSub;
    ScopedSubscription m_triggerExitSub;
};

}  // namespace Systems
