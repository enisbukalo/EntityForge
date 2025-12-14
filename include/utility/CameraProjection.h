#pragma once

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Vector2.hpp>

#include <CCamera.h>
#include <Vec2.h>
#include <Vec2i.h>

namespace Internal
{

/**
 * @brief Converts a window pixel (screen-space) position to world coordinates for a given camera.
 *
 * This is intentionally independent of a live sf::RenderWindow so it can be unit-tested.
 * The mapping matches rendering by reusing Internal::buildViewFromCamera(...).
 */
Vec2 screenToWorld(const Components::CCamera& camera, const sf::Vector2u& windowSizePx, const Vec2i& windowPx);

/**
 * @brief Converts a world-space position to a window pixel (screen-space) position for a given camera.
 *
 * This is intentionally independent of a live sf::RenderWindow so it can be unit-tested.
 * The mapping matches rendering by reusing Internal::buildViewFromCamera(...).
 */
Vec2i worldToScreen(const Components::CCamera& camera, const sf::Vector2u& windowSizePx, const Vec2& worldPos);

}  // namespace Internal
