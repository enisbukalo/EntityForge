#include <CameraView.h>

#include <algorithm>
#include <cmath>

namespace
{
constexpr float kPi = 3.14159265358979323846f;

float safePositiveOr(float value, float fallback)
{
    return (value > 0.0f) ? value : fallback;
}

float computeViewportAspect(const Components::CCamera::Viewport& viewport, const sf::Vector2u& windowSizePx)
{
    const float windowWidthPx  = static_cast<float>(windowSizePx.x);
    const float windowHeightPx = static_cast<float>(windowSizePx.y);

    const float viewportWidthPx  = windowWidthPx * viewport.width;
    const float viewportHeightPx = windowHeightPx * viewport.height;

    const float safeWidth  = safePositiveOr(viewportWidthPx, safePositiveOr(windowWidthPx, 1.0f));
    const float safeHeight = safePositiveOr(viewportHeightPx, safePositiveOr(windowHeightPx, 1.0f));

    return safeWidth / safeHeight;
}
}  // namespace

namespace Internal
{

sf::View buildViewFromCamera(const Components::CCamera& camera, const sf::Vector2u& windowSizePx)
{
    sf::View view;

    view.setCenter(camera.position.x, camera.position.y);

    const float zoom        = safePositiveOr(camera.zoom, 1.0f);
    const float worldHeight = camera.worldHeight / zoom;

    const float aspect     = computeViewportAspect(camera.viewport, windowSizePx);
    const float worldWidth = worldHeight * aspect;

    // Y-up mapping: flip Y by negating the view height.
    view.setSize(worldWidth, -worldHeight);

    // SFML uses degrees clockwise (Y-down); with our mapping we currently apply the plan's
    // recommended sign to match expected camera rotation.
    const float degrees = -(camera.rotationRadians * (180.0f / kPi));
    view.setRotation(degrees);

    view.setViewport(sf::FloatRect(camera.viewport.left, camera.viewport.top, camera.viewport.width, camera.viewport.height));

    return view;
}

}  // namespace Internal
