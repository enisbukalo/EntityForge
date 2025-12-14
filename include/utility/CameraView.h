#ifndef CAMERA_VIEW_H
#define CAMERA_VIEW_H

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Vector2.hpp>

#include <CCamera.h>

namespace Internal
{

/**
 * @brief Builds an SFML view from a camera component and the current window pixel size.
 *
 * The returned view uses world units (meters) for its coordinate space. To keep the engine's
 * world convention (Y-up) consistent with SFML (Y-down), the view's height is negated.
 */
sf::View buildViewFromCamera(const Components::CCamera& camera, const sf::Vector2u& windowSizePx);

}  // namespace Internal

#endif  // CAMERA_VIEW_H
