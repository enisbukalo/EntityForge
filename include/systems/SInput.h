#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include "ActionBinding.h"
#include "IInputListener.h"
#include "InputEvents.h"
#include "MouseButton.h"
#include "System.h"

using ListenerId = size_t;
using BindingId  = size_t;

namespace Systems
{

class SInput : public System
{
private:
    sf::RenderWindow* m_window      = nullptr;
    bool              m_passToImGui = true;

    // Key state maps: current down state and previous frame
    std::unordered_map<KeyCode, bool>     m_keyDown;
    std::unordered_map<KeyCode, bool>     m_keyPressed;
    std::unordered_map<KeyCode, bool>     m_keyReleased;
    std::unordered_map<KeyCode, bool>     m_keyRepeat;
    std::unordered_map<MouseButton, bool> m_mouseDown;
    std::unordered_map<MouseButton, bool> m_mousePressed;
    std::unordered_map<MouseButton, bool> m_mouseReleased;
    Vec2i                                 m_mousePosition;

    // Action bindings (per-action -> list of pairs(bindingId, binding))
    std::unordered_map<std::string, std::vector<std::pair<BindingId, ActionBinding>>> m_actionBindings;
    std::unordered_map<std::string, ActionState>                                      m_actionStates;
    BindingId                                                                         m_nextBindingId = 1;

    // Listener collections
    std::unordered_map<ListenerId, std::function<void(const InputEvent&)>> m_subscribers;
    std::vector<IInputListener*>                                           m_listenerPointers;
    ListenerId                                                             m_nextListenerId = 1;

    // Helper to namespace actions per-entity so identical action names across entities don't collide
    std::string scopeAction(Entity entity, const std::string& actionName) const;
    void        registerControllerBindings(World& world);
    void        updateControllerStates(World& world);

    void dispatch(World& world, const InputEvent& inputEvent);

public:
    SInput();
    ~SInput();

    // Delete copy and move constructors/assignment operators
    SInput(const SInput&)            = delete;
    SInput(SInput&&)                 = delete;
    SInput& operator=(const SInput&) = delete;
    SInput& operator=(SInput&&)      = delete;

    void initialize(sf::RenderWindow* window, bool passToImGui = true);
    void shutdown();

    void update(float deltaTime, World& world) override;

    ListenerId subscribe(std::function<void(const InputEvent&)> cb);
    void       unsubscribe(ListenerId id);

    void addListener(IInputListener* listener);
    void removeListener(IInputListener* listener);

    // Query APIs
    bool  isKeyDown(KeyCode key) const;
    bool  wasKeyPressed(KeyCode key) const;
    bool  wasKeyReleased(KeyCode key) const;
    bool  isMouseDown(MouseButton button) const;
    bool  wasMousePressed(MouseButton button) const;
    bool  wasMouseReleased(MouseButton button) const;
    Vec2i getMousePositionWindow() const;

    // Action binding
    BindingId   bindAction(const std::string& actionName, const ActionBinding& binding);
    void        unbindAction(const std::string& actionName, BindingId id);  // remove specific binding
    void        unbindAction(const std::string& actionName);                // remove all bindings
    ActionState getActionState(const std::string& actionName) const;

    // ImGui handling
    void setPassToImGui(bool pass)
    {
        m_passToImGui = pass;
    }

    std::string_view name() const override
    {
        return "SInput";
    }
};

}  // namespace Systems
