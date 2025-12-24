#include <SObjectives.h>

#include <Logger.h>
#include <World.h>

#include <cstdint>

#include <CName.h>
#include <CObjectives.h>
#include <ObjectiveDefinition.h>
#include <ObjectiveEvents.h>
#include <ObjectiveRegistry.h>
#include <TriggerEvents.h>

namespace
{
void forEachObjectives(World& world, const std::function<void(Components::CObjectives&)>& fn)
{
    world.components().each<Components::CObjectives>([&](Entity /*e*/, Components::CObjectives& objectives)
                                                     { fn(objectives); });
}

bool hasSignal(const Objectives::ObjectiveDefinition& def, const std::string& signalId)
{
    if (def.progression.mode != Objectives::ProgressionMode::Signals)
    {
        return false;
    }
    for (const auto& s : def.progression.signals)
    {
        if (s == signalId)
        {
            return true;
        }
    }
    return false;
}

bool evalActivationConditions(const Objectives::ObjectiveDefinition& def, const Components::CObjectives& objectives)
{
    for (const auto& c : def.activationConditions)
    {
        switch (c.type)
        {
            case Objectives::ActivationConditionType::FlagSet:
                if (!objectives.isManagerFlagSet(c.flag))
                {
                    return false;
                }
                break;
            case Objectives::ActivationConditionType::CounterAtLeast:
                if (objectives.getManagerCounter(c.key) < c.value)
                {
                    return false;
                }
                break;
        }
    }
    return true;
}

bool prereqsCompleted(const Objectives::ObjectiveDefinition& def, const Components::CObjectives& objectives)
{
    for (const auto& prereq : def.prerequisites)
    {
        if (!objectives.isObjectiveCompleted(prereq))
        {
            return false;
        }
    }
    return true;
}

void completeObjective(Components::CObjectives& objectives, const std::string& objectiveId)
{
    const auto* before = objectives.tryGetObjective(objectiveId);
    const auto  prev   = before ? before->status : Components::ObjectiveStatus::Inactive;

    objectives.setObjectiveStatus(objectiveId, Components::ObjectiveStatus::Completed);
    (void)objectives.tryGrantObjectiveReward(objectiveId);

    if (prev != Components::ObjectiveStatus::Completed)
    {
        const auto* after  = objectives.tryGetObjective(objectiveId);
        const auto  title  = (after != nullptr) ? after->title : std::string{};
        const auto  detail = title.empty() ? std::string{} : (" - " + title);
        LOG_INFO("Objectives: Completed '{}'{}", objectiveId, detail);
    }
}

std::string tryGetEntityName(const World& world, Entity e)
{
    const auto* name = world.get<Components::CName>(e);
    if (!name)
    {
        return {};
    }
    return name->name;
}
}  // namespace

namespace Systems
{

void SObjectives::update(float /*deltaTime*/, World& world)
{
    if (!m_initialized)
    {
        // Register subscriptions once. Note: handlers queue events; we apply them once-per-tick in update().
        m_managerCounterIncrementSub = ScopedSubscription(world.events(),
                                                          world.events().subscribe<Objectives::ObjectiveManagerCounterIncrement>(
                                                              [&](const Objectives::ObjectiveManagerCounterIncrement& ev, World&)
                                                              { m_pendingManagerCounterIncrements.push_back(ev); }));

        m_managerFlagSetSub = ScopedSubscription(world.events(),
                                                 world.events().subscribe<Objectives::ObjectiveManagerFlagSet>(
                                                     [&](const Objectives::ObjectiveManagerFlagSet& ev, World&)
                                                     { m_pendingManagerFlagSets.push_back(ev); }));

        m_activateSub = ScopedSubscription(world.events(),
                                           world.events().subscribe<Objectives::ObjectiveActivate>(
                                               [&](const Objectives::ObjectiveActivate& ev, World&)
                                               { m_pendingActivates.push_back(ev); }));

        m_counterIncrementSub = ScopedSubscription(world.events(),
                                                   world.events().subscribe<Objectives::ObjectiveCounterIncrement>(
                                                       [&](const Objectives::ObjectiveCounterIncrement& ev, World&)
                                                       { m_pendingCounterIncrements.push_back(ev); }));

        m_completeSub = ScopedSubscription(world.events(),
                                           world.events().subscribe<Objectives::ObjectiveComplete>(
                                               [&](const Objectives::ObjectiveComplete& ev, World&)
                                               { m_pendingCompletes.push_back(ev); }));

        m_signalSub = ScopedSubscription(world.events(),
                                         world.events().subscribe<Objectives::ObjectiveSignal>(
                                             [&](const Objectives::ObjectiveSignal& ev, World&)
                                             { m_pendingSignals.push_back(ev); }));

        m_triggerEnterSub = ScopedSubscription(world.events(),
                                               world.events().subscribe<Physics::TriggerEnter>(
                                                   [&](const Physics::TriggerEnter& ev, World&)
                                                   { m_pendingTriggerEnters.push_back(ev); }));

        m_triggerExitSub = ScopedSubscription(world.events(),
                                              world.events().subscribe<Physics::TriggerExit>(
                                                  [&](const Physics::TriggerExit& ev, World&)
                                                  { m_pendingTriggerExits.push_back(ev); }));

        m_initialized = true;
    }

    // Apply queued manager state updates (activationConditions source).
    if (!m_pendingManagerCounterIncrements.empty() || !m_pendingManagerFlagSets.empty())
    {
        forEachObjectives(world,
                          [&](Components::CObjectives& objectives)
                          {
                              for (const auto& ev : m_pendingManagerCounterIncrements)
                              {
                                  objectives.incrementManagerCounter(ev.key, ev.delta);
                              }
                              for (const auto& ev : m_pendingManagerFlagSets)
                              {
                                  objectives.setManagerFlag(ev.flag, ev.value);
                              }
                          });
    }

    // Process activation requests.
    if (!m_pendingActivates.empty())
    {
        forEachObjectives(world,
                          [&](Components::CObjectives& objectives)
                          {
                              for (const auto& ev : m_pendingActivates)
                              {
                                  const auto* def = (m_registry != nullptr) ? m_registry->find(ev.objectiveId) : nullptr;
                                  if (def != nullptr)
                                  {
                                      if (!prereqsCompleted(*def, objectives) || !evalActivationConditions(*def, objectives))
                                      {
                                          continue;
                                      }

                                      auto& inst         = objectives.getOrAddObjective(def->id);
                                      inst.title         = def->title;
                                      inst.description   = def->description;
                                      inst.prerequisites = def->prerequisites;
                                      inst.onComplete    = def->onComplete;
                                      if (inst.status == Components::ObjectiveStatus::Inactive)
                                      {
                                          inst.status       = Components::ObjectiveStatus::InProgress;
                                          const auto detail = inst.title.empty() ? std::string{} : (" - " + inst.title);
                                          LOG_INFO("Objectives: Activated '{}'{}", inst.id, detail);
                                      }
                                  }
                                  else
                                  {
                                      // Fallback: allow activation using runtime-only instance prerequisites.
                                      const auto* before = objectives.tryGetObjective(ev.objectiveId);
                                      const auto prev = before ? before->status : Components::ObjectiveStatus::Inactive;
                                      if (objectives.activateObjective(ev.objectiveId) && prev == Components::ObjectiveStatus::Inactive)
                                      {
                                          LOG_INFO("Objectives: Activated '{}'", ev.objectiveId);
                                      }
                                  }
                              }
                          });
    }

    // Targeted objective updates.
    if (!m_pendingCounterIncrements.empty())
    {
        forEachObjectives(world,
                          [&](Components::CObjectives& objectives)
                          {
                              for (const auto& ev : m_pendingCounterIncrements)
                              {
                                  if (objectives.incrementObjectiveCounter(ev.objectiveId, ev.key, ev.delta))
                                  {
                                      if (const auto* inst = objectives.tryGetObjective(ev.objectiveId))
                                      {
                                          const auto it  = inst->counters.find(ev.key);
                                          const auto val = (it != inst->counters.end()) ? it->second : 0;
                                          LOG_INFO("Objectives: '{}' counter '{}' -> {}", ev.objectiveId, ev.key, val);
                                      }
                                  }
                              }
                          });
    }

    if (!m_pendingCompletes.empty())
    {
        forEachObjectives(world,
                          [&](Components::CObjectives& objectives)
                          {
                              for (const auto& ev : m_pendingCompletes)
                              {
                                  completeObjective(objectives, ev.objectiveId);
                              }
                          });
    }

    // Data-driven progression: signals
    if (!m_pendingSignals.empty() && m_registry != nullptr)
    {
        forEachObjectives(world,
                          [&](Components::CObjectives& objectives)
                          {
                              for (const auto& ev : m_pendingSignals)
                              {
                                  for (auto& inst : objectives.objectives)
                                  {
                                      if (inst.status != Components::ObjectiveStatus::InProgress)
                                      {
                                          continue;
                                      }

                                      const auto* def = m_registry->find(inst.id);
                                      if (def == nullptr)
                                      {
                                          continue;
                                      }
                                      if (hasSignal(*def, ev.signalId))
                                      {
                                          if (def->progression.signalCount <= 1)
                                          {
                                              completeObjective(objectives, inst.id);
                                              continue;
                                          }

                                          // Count matching signals towards completion.
                                          inst.counters[ev.signalId] += 1;

                                          std::int64_t total = 0;
                                          for (const auto& sid : def->progression.signals)
                                          {
                                              const auto it = inst.counters.find(sid);
                                              if (it != inst.counters.end())
                                              {
                                                  total += it->second;
                                              }
                                          }

                                          if (total >= def->progression.signalCount)
                                          {
                                              completeObjective(objectives, inst.id);
                                          }
                                      }
                                  }
                              }
                          });
    }

    // Data-driven progression: triggers
    if ((!m_pendingTriggerEnters.empty() || !m_pendingTriggerExits.empty()) && m_registry != nullptr)
    {
        forEachObjectives(world,
                          [&](Components::CObjectives& objectives)
                          {
                              auto applyTrigger = [&](Objectives::TriggerEventType eventType, const std::string& triggerName)
                              {
                                  for (auto& inst : objectives.objectives)
                                  {
                                      if (inst.status != Components::ObjectiveStatus::InProgress)
                                      {
                                          continue;
                                      }

                                      const auto* def = m_registry->find(inst.id);
                                      if (def == nullptr || def->progression.mode != Objectives::ProgressionMode::Triggers)
                                      {
                                          continue;
                                      }

                                      for (const auto& rule : def->progression.triggers)
                                      {
                                          if (rule.type != eventType)
                                          {
                                              continue;
                                          }
                                          if (rule.triggerName != triggerName)
                                          {
                                              continue;
                                          }

                                          if (rule.action == Objectives::TriggerAction::Complete)
                                          {
                                              completeObjective(objectives, inst.id);
                                          }
                                          else if (rule.action == Objectives::TriggerAction::IncrementCounter)
                                          {
                                              inst.counters[rule.key] += rule.delta;
                                              const auto it  = inst.counters.find(rule.key);
                                              const auto val = (it != inst.counters.end()) ? it->second : 0;
                                              LOG_INFO("Objectives: '{}' counter '{}' -> {}", inst.id, rule.key, val);
                                          }
                                      }
                                  }
                              };

                              for (const auto& ev : m_pendingTriggerEnters)
                              {
                                  const std::string triggerName = tryGetEntityName(world, ev.triggerEntity);
                                  if (!triggerName.empty())
                                  {
                                      applyTrigger(Objectives::TriggerEventType::Enter, triggerName);
                                  }
                              }
                              for (const auto& ev : m_pendingTriggerExits)
                              {
                                  const std::string triggerName = tryGetEntityName(world, ev.triggerEntity);
                                  if (!triggerName.empty())
                                  {
                                      applyTrigger(Objectives::TriggerEventType::Exit, triggerName);
                                  }
                              }
                          });
    }

    // Clear queues for next tick.
    m_pendingManagerCounterIncrements.clear();
    m_pendingManagerFlagSets.clear();
    m_pendingActivates.clear();
    m_pendingCounterIncrements.clear();
    m_pendingCompletes.clear();
    m_pendingSignals.clear();
    m_pendingTriggerEnters.clear();
    m_pendingTriggerExits.clear();
}

}  // namespace Systems
