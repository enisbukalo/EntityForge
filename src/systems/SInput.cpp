#include "SInput.h"

#include <imgui-SFML.h>
#include <imgui.h>
#include <algorithm>
#include <string>

#include "CInputController.h"
#include "Logger.h"
#include "World.h"

namespace Systems
{

SInput::SInput() = default;

SInput::~SInput()
{
    shutdown();
}

void SInput::initialize(sf::RenderWindow* window, bool passToImGui)
{
    m_window      = window;
    m_passToImGui = passToImGui;
    LOG_INFO("SInput: Initialized (ImGui forwarding: {})", passToImGui);
}

void SInput::shutdown()
{
    LOG_INFO("SInput: Shutting down, clearing {} action bindings", m_actionBindings.size());
    m_window = nullptr;
    m_keyDown.clear();
    m_keyPressed.clear();
    m_keyReleased.clear();
    m_keyRepeat.clear();
    m_mouseDown.clear();
    m_mousePressed.clear();
    m_mouseReleased.clear();
    m_mousePosition = Vec2i{0, 0};
    m_actionBindings.clear();
    m_actionStates.clear();
    m_subscribers.clear();
    m_listenerPointers.clear();
    m_nextBindingId = 1;
}

static KeyCode keyCodeFromSFML(sf::Keyboard::Scancode k)
{
    using K = sf::Keyboard::Scancode;
    switch (k)
    {
        case K::A:
            return KeyCode::A;
        case K::B:
            return KeyCode::B;
        case K::C:
            return KeyCode::C;
        case K::D:
            return KeyCode::D;
        case K::E:
            return KeyCode::E;
        case K::F:
            return KeyCode::F;
        case K::G:
            return KeyCode::G;
        case K::H:
            return KeyCode::H;
        case K::I:
            return KeyCode::I;
        case K::J:
            return KeyCode::J;
        case K::K:
            return KeyCode::K;
        case K::L:
            return KeyCode::L;
        case K::M:
            return KeyCode::M;
        case K::N:
            return KeyCode::N;
        case K::O:
            return KeyCode::O;
        case K::P:
            return KeyCode::P;
        case K::Q:
            return KeyCode::Q;
        case K::R:
            return KeyCode::R;
        case K::S:
            return KeyCode::S;
        case K::T:
            return KeyCode::T;
        case K::U:
            return KeyCode::U;
        case K::V:
            return KeyCode::V;
        case K::W:
            return KeyCode::W;
        case K::X:
            return KeyCode::X;
        case K::Y:
            return KeyCode::Y;
        case K::Z:
            return KeyCode::Z;
        case K::Num0:
            return KeyCode::Num0;
        case K::Num1:
            return KeyCode::Num1;
        case K::Num2:
            return KeyCode::Num2;
        case K::Num3:
            return KeyCode::Num3;
        case K::Num4:
            return KeyCode::Num4;
        case K::Num5:
            return KeyCode::Num5;
        case K::Num6:
            return KeyCode::Num6;
        case K::Num7:
            return KeyCode::Num7;
        case K::Num8:
            return KeyCode::Num8;
        case K::Num9:
            return KeyCode::Num9;
        case K::Escape:
            return KeyCode::Escape;
        case K::Space:
            return KeyCode::Space;
        case K::Enter:
            return KeyCode::Enter;
        case K::Backspace:
            return KeyCode::Backspace;
        case K::Tab:
            return KeyCode::Tab;
        case K::Left:
            return KeyCode::Left;
        case K::Right:
            return KeyCode::Right;
        case K::Up:
            return KeyCode::Up;
        case K::Down:
            return KeyCode::Down;
        case K::F1:
            return KeyCode::F1;
        case K::F2:
            return KeyCode::F2;
        case K::F3:
            return KeyCode::F3;
        case K::F4:
            return KeyCode::F4;
        case K::F5:
            return KeyCode::F5;
        case K::F6:
            return KeyCode::F6;
        case K::F7:
            return KeyCode::F7;
        case K::F8:
            return KeyCode::F8;
        case K::F9:
            return KeyCode::F9;
        case K::F10:
            return KeyCode::F10;
        case K::F11:
            return KeyCode::F11;
        case K::F12:
            return KeyCode::F12;
        case K::LControl:
            return KeyCode::LControl;
        case K::RControl:
            return KeyCode::RControl;
        case K::LShift:
            return KeyCode::LShift;
        case K::RShift:
            return KeyCode::RShift;
        case K::LAlt:
            return KeyCode::LAlt;
        case K::RAlt:
            return KeyCode::RAlt;
        default:
            return KeyCode::Unknown;
    }
}

static MouseButton mouseButtonFromSFML(sf::Mouse::Button mb)
{
    using B = sf::Mouse::Button;
    switch (mb)
    {
        case B::Left:
            return MouseButton::Left;
        case B::Right:
            return MouseButton::Right;
        case B::Middle:
            return MouseButton::Middle;
        case B::Extra1:
            return MouseButton::XButton1;
        case B::Extra2:
            return MouseButton::XButton2;
        default:
            return MouseButton::Unknown;
    }
}

BindingId SInput::bindAction(const std::string& actionName, const ActionBinding& binding)
{
    BindingId id = m_nextBindingId++;
    m_actionBindings[actionName].push_back({id, binding});
    m_actionStates.try_emplace(actionName, ActionState::None);
    return id;
}

void SInput::unbindAction(const std::string& actionName, BindingId id)
{
    auto it = m_actionBindings.find(actionName);
    if (it == m_actionBindings.end())
        return;
    auto& vec = it->second;
    vec.erase(std::remove_if(vec.begin(), vec.end(), [id](const auto& p) { return p.first == id; }), vec.end());
    if (vec.empty())
    {
        m_actionBindings.erase(it);
        m_actionStates.erase(actionName);
    }
}

void SInput::unbindAction(const std::string& actionName)
{
    m_actionBindings.erase(actionName);
    m_actionStates.erase(actionName);
}

void SInput::update(float /*deltaTime*/, World& world)
{
    // Clear transient states (pressed/released) at start of update
    for (auto& kv : m_actionStates)
        if (kv.second == ActionState::Released)
            kv.second = ActionState::None;
    m_keyPressed.clear();
    m_keyReleased.clear();
    m_keyRepeat.clear();
    m_mousePressed.clear();
    m_mouseReleased.clear();
    if (!m_window)
        return;

    auto forwardToImGuiAndCheckCapture = [&](const auto& subEvent) -> bool
    {
        if (!m_passToImGui)
        {
            return false;
        }

        sf::Event event{subEvent};
        ImGui::SFML::ProcessEvent(*m_window, event);
        return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
    };

    auto dispatch = [&](const InputEvent& inputEvent)
    {
        for (const auto& kv : m_subscribers)
        {
            try
            {
                kv.second(inputEvent);
            }
            catch (...)
            {
            }
        }

        for (auto* l : m_listenerPointers)
        {
            if (!l)
                continue;
            switch (inputEvent.type)
            {
                case InputEventType::KeyPressed:
                    l->onKeyPressed(inputEvent.key);
                    break;
                case InputEventType::KeyReleased:
                    l->onKeyReleased(inputEvent.key);
                    break;
                case InputEventType::MouseButtonPressed:
                    l->onMousePressed(inputEvent.mouse);
                    break;
                case InputEventType::MouseButtonReleased:
                    l->onMouseReleased(inputEvent.mouse);
                    break;
                case InputEventType::MouseMoved:
                    l->onMouseMoved(inputEvent.mouseMove);
                    break;
                case InputEventType::TextEntered:
                    l->onTextEntered(inputEvent.text);
                    break;
                case InputEventType::WindowClosed:
                case InputEventType::WindowResized:
                    l->onWindowEvent(inputEvent.window);
                    break;
                default:
                    break;
            }
        }
    };

    // Pump events (SFML 3 typed handlers)
    m_window->handleEvents(
        [&](const sf::Event::Closed& e)
        {
            if (forwardToImGuiAndCheckCapture(e))
                return;

            InputEvent inputEvent{};
            inputEvent.type   = InputEventType::WindowClosed;
            inputEvent.window = WindowEvent{};
            dispatch(inputEvent);
        },
        [&](const sf::Event::Resized& e)
        {
            if (forwardToImGuiAndCheckCapture(e))
                return;

            InputEvent  inputEvent{};
            WindowEvent we{};
            we.width          = e.size.x;
            we.height         = e.size.y;
            inputEvent.type   = InputEventType::WindowResized;
            inputEvent.window = we;
            dispatch(inputEvent);
        },
        [&](const sf::Event::KeyPressed& e)
        {
            if (forwardToImGuiAndCheckCapture(e))
                return;

            InputEvent inputEvent{};
            KeyEvent   ke{};
            ke.key               = keyCodeFromSFML(e.scancode);
            ke.alt               = e.alt;
            ke.ctrl              = e.control;
            ke.shift             = e.shift;
            ke.system            = e.system;
            const auto wasDownIt = m_keyDown.find(ke.key);
            const bool wasDown   = (wasDownIt != m_keyDown.end()) ? wasDownIt->second : false;
            ke.repeat            = wasDown;
            inputEvent.type      = InputEventType::KeyPressed;
            inputEvent.key       = ke;

            m_keyDown[ke.key] = true;
            if (ke.repeat)
            {
                m_keyRepeat[ke.key] = true;
            }
            else
            {
                m_keyPressed[ke.key] = true;
            }

            dispatch(inputEvent);
        },
        [&](const sf::Event::KeyReleased& e)
        {
            if (forwardToImGuiAndCheckCapture(e))
                return;

            InputEvent inputEvent{};
            KeyEvent   ke{};
            ke.key          = keyCodeFromSFML(e.scancode);
            ke.alt          = e.alt;
            ke.ctrl         = e.control;
            ke.shift        = e.shift;
            ke.system       = e.system;
            ke.repeat       = false;
            inputEvent.type = InputEventType::KeyReleased;
            inputEvent.key  = ke;

            m_keyDown[ke.key]     = false;
            m_keyReleased[ke.key] = true;
            m_keyRepeat.erase(ke.key);

            dispatch(inputEvent);
        },
        [&](const sf::Event::MouseMoved& e)
        {
            if (forwardToImGuiAndCheckCapture(e))
                return;

            InputEvent     inputEvent{};
            MouseMoveEvent mm{};
            mm.position          = Vec2i{e.position.x, e.position.y};
            inputEvent.type      = InputEventType::MouseMoved;
            inputEvent.mouseMove = mm;
            m_mousePosition      = mm.position;
            dispatch(inputEvent);
        },
        [&](const sf::Event::MouseButtonPressed& e)
        {
            if (forwardToImGuiAndCheckCapture(e))
                return;

            InputEvent inputEvent{};
            MouseEvent me{};
            me.button        = mouseButtonFromSFML(e.button);
            me.position      = Vec2i{e.position.x, e.position.y};
            inputEvent.type  = InputEventType::MouseButtonPressed;
            inputEvent.mouse = me;

            m_mouseDown[me.button]    = true;
            m_mousePressed[me.button] = true;
            m_mousePosition           = me.position;
            dispatch(inputEvent);
        },
        [&](const sf::Event::MouseButtonReleased& e)
        {
            if (forwardToImGuiAndCheckCapture(e))
                return;

            InputEvent inputEvent{};
            MouseEvent me{};
            me.button        = mouseButtonFromSFML(e.button);
            me.position      = Vec2i{e.position.x, e.position.y};
            inputEvent.type  = InputEventType::MouseButtonReleased;
            inputEvent.mouse = me;

            m_mouseDown[me.button]     = false;
            m_mouseReleased[me.button] = true;
            m_mousePosition            = me.position;
            dispatch(inputEvent);
        },
        [&](const sf::Event::MouseWheelScrolled& e)
        {
            if (forwardToImGuiAndCheckCapture(e))
                return;

            InputEvent inputEvent{};
            WheelEvent we{};
            we.delta         = e.delta;
            we.position      = Vec2i{e.position.x, e.position.y};
            inputEvent.type  = InputEventType::MouseWheel;
            inputEvent.wheel = we;
            dispatch(inputEvent);
        },
        [&](const sf::Event::TextEntered& e)
        {
            if (forwardToImGuiAndCheckCapture(e))
                return;

            InputEvent inputEvent{};
            TextEvent  te{};
            te.unicode      = e.unicode;
            inputEvent.type = InputEventType::TextEntered;
            inputEvent.text = te;
            dispatch(inputEvent);
        });

    // Ensure controller bindings are registered before evaluating action states
    registerControllerBindings(world);

    // Evaluate actions centrally
    for (const auto& actionKv : m_actionBindings)
    {
        const std::string& actionName = actionKv.first;
        const auto&        bindings   = actionKv.second;
        ActionState        newState   = ActionState::None;

        for (const auto& pr : bindings)
        {
            const auto& binding     = pr.second;
            bool        anyDown     = false;
            bool        anyPressed  = false;
            bool        anyReleased = false;
            for (auto k : binding.keys)
            {
                if (m_keyDown.find(k) != m_keyDown.end() && m_keyDown[k])
                    anyDown = true;
                if (m_keyPressed.find(k) != m_keyPressed.end() && m_keyPressed[k])
                    anyPressed = true;
                if (m_keyReleased.find(k) != m_keyReleased.end() && m_keyReleased[k])
                    anyReleased = true;
                if (m_keyRepeat.find(k) != m_keyRepeat.end() && m_keyRepeat[k])
                    anyPressed = true;
            }
            for (auto mb : binding.mouseButtons)
            {
                if (m_mouseDown.find(mb) != m_mouseDown.end() && m_mouseDown[mb])
                    anyDown = true;
                if (m_mousePressed.find(mb) != m_mousePressed.end() && m_mousePressed[mb])
                    anyPressed = true;
                if (m_mouseReleased.find(mb) != m_mouseReleased.end() && m_mouseReleased[mb])
                    anyReleased = true;
            }
            switch (binding.trigger)
            {
                case ActionTrigger::Pressed:
                {
                    bool eff = anyPressed;
                    if (!binding.allowRepeat)
                    {
                        // if any key repeat triggered and repeat not allowed, ignore as pressed
                        bool repeatTriggered = false;
                        for (auto k : binding.keys)
                            if (m_keyRepeat.find(k) != m_keyRepeat.end() && m_keyRepeat[k])
                            {
                                repeatTriggered = true;
                                break;
                            }
                        if (repeatTriggered)
                            eff = false;
                    }
                    if (eff)
                        newState = ActionState::Pressed;
                    else if (anyDown)
                        newState = ActionState::Held;
                }
                break;
                case ActionTrigger::Held:
                    if (anyDown)
                        newState = anyPressed ? ActionState::Pressed : ActionState::Held;
                    break;
                case ActionTrigger::Released:
                    if (anyReleased)
                        newState = ActionState::Released;
                    break;
            }
            if (newState != ActionState::None)
                break;
        }

        ActionState previous = ActionState::None;
        auto        pit      = m_actionStates.find(actionName);
        if (pit != m_actionStates.end())
            previous = pit->second;
        bool wasDown = (previous == ActionState::Pressed || previous == ActionState::Held);
        if (!wasDown && newState == ActionState::Pressed)
            m_actionStates[actionName] = ActionState::Pressed;
        else if (wasDown && newState == ActionState::None)
            m_actionStates[actionName] = ActionState::Released;
        else
            m_actionStates[actionName] = newState;

        if (m_actionStates[actionName] != ActionState::None)
        {
            InputEvent ie{};
            ie.type              = InputEventType::Action;
            ie.action.actionName = actionName;
            ie.action.state      = m_actionStates[actionName];
            for (const auto& kv : m_subscribers)
            {
                try
                {
                    kv.second(ie);
                }
                catch (...)
                {
                }
            }
            for (auto* l : m_listenerPointers)
                if (l)
                    l->onAction(ie.action);
        }
    }

    // Push updated action states back into input controller components
    updateControllerStates(world);
}

std::string SInput::scopeAction(Entity entity, const std::string& actionName) const
{
    return actionName + "@E" + std::to_string(entity.index) + "." + std::to_string(entity.generation);
}

void SInput::registerControllerBindings(World& world)
{
    world.components().each<Components::CInputController>(
        [this](Entity entity, Components::CInputController& controller)
        {
            for (auto& kv : controller.bindings)
            {
                const std::string& actionName = kv.first;
                for (auto& binding : kv.second)
                {
                    if (binding.bindingId == 0)
                    {
                        std::string scoped = scopeAction(entity, actionName);
                        binding.bindingId  = bindAction(scoped, binding.binding);
                    }
                }
            }
        });
}

void SInput::updateControllerStates(World& world)
{
    world.components().each<Components::CInputController>(
        [this](Entity entity, Components::CInputController& controller)
        {
            for (const auto& kv : controller.bindings)
            {
                const std::string& actionName       = kv.first;
                std::string        scoped           = scopeAction(entity, actionName);
                controller.actionStates[actionName] = getActionState(scoped);
            }
        });
}

ListenerId SInput::subscribe(std::function<void(const InputEvent&)> cb)
{
    ListenerId id = m_nextListenerId++;
    m_subscribers.insert({id, cb});
    return id;
}

void SInput::unsubscribe(ListenerId id)
{
    m_subscribers.erase(id);
}

void SInput::addListener(IInputListener* listener)
{
    if (!listener)
        return;
    m_listenerPointers.push_back(listener);
}

void SInput::removeListener(IInputListener* listener)
{
    m_listenerPointers.erase(std::remove(m_listenerPointers.begin(), m_listenerPointers.end(), listener),
                             m_listenerPointers.end());
}

bool SInput::isKeyDown(KeyCode key) const
{
    auto it = m_keyDown.find(key);
    return (it != m_keyDown.end()) && it->second;
}

bool SInput::wasKeyPressed(KeyCode key) const
{
    auto it = m_keyPressed.find(key);
    return (it != m_keyPressed.end()) && it->second;
}

bool SInput::wasKeyReleased(KeyCode key) const
{
    auto it = m_keyReleased.find(key);
    return (it != m_keyReleased.end()) && it->second;
}

bool SInput::isMouseDown(MouseButton button) const
{
    auto it = m_mouseDown.find(button);
    return (it != m_mouseDown.end()) && it->second;
}

bool SInput::wasMousePressed(MouseButton button) const
{
    auto it = m_mousePressed.find(button);
    return (it != m_mousePressed.end()) && it->second;
}

bool SInput::wasMouseReleased(MouseButton button) const
{
    auto it = m_mouseReleased.find(button);
    return (it != m_mouseReleased.end()) && it->second;
}

Vec2i SInput::getMousePositionWindow() const
{
    return m_mousePosition;
}

ActionState SInput::getActionState(const std::string& actionName) const
{
    auto it = m_actionStates.find(actionName);
    if (it == m_actionStates.end())
        return ActionState::None;
    return it->second;
}

}  // namespace Systems
