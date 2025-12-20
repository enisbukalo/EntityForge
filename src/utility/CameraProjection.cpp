#include <CameraProjection.h>

#include <CameraView.h>

#include <algorithm>
#include <cmath>

namespace
{
float safePositiveOr(float value, float fallback)
{
    return (value > 0.0f) ? value : fallback;
}

struct ViewportPx
{
    float left   = 0.0f;
    float top    = 0.0f;
    float width  = 1.0f;
    float height = 1.0f;
};

ViewportPx computeViewportPx(const sf::View& view, const sf::Vector2u& windowSizePx)
{
    const sf::FloatRect vp = view.getViewport();

    const float windowWidth  = static_cast<float>(windowSizePx.x);
    const float windowHeight = static_cast<float>(windowSizePx.y);

    ViewportPx viewport;
    viewport.left   = vp.position.x * windowWidth;
    viewport.top    = vp.position.y * windowHeight;
    viewport.width  = vp.size.x * windowWidth;
    viewport.height = vp.size.y * windowHeight;

    // Avoid division by zero; keep behavior stable for unit tests that pass (0,0).
    viewport.width  = safePositiveOr(viewport.width, safePositiveOr(windowWidth, 1.0f));
    viewport.height = safePositiveOr(viewport.height, safePositiveOr(windowHeight, 1.0f));

    return viewport;
}

sf::Vector2f clampToFinite(sf::Vector2f v)
{
    if (!std::isfinite(v.x))
    {
        v.x = 0.0f;
    }
    if (!std::isfinite(v.y))
    {
        v.y = 0.0f;
    }
    return v;
}
}  // namespace

namespace Internal
{

Vec2 screenToWorld(const Components::CCamera& camera, const sf::Vector2u& windowSizePx, const Vec2i& windowPx)
{
    const sf::View   view = Internal::buildViewFromCamera(camera, windowSizePx);
    const ViewportPx vp   = computeViewportPx(view, windowSizePx);

    const float px = static_cast<float>(windowPx.x);
    const float py = static_cast<float>(windowPx.y);

    const float normalizedX = (px - vp.left) / vp.width;
    const float normalizedY = (py - vp.top) / vp.height;

    // Convert viewport-normalized [0,1] to NDC [-1,1]. SFML's NDC has Y-up.
    const float ndcX = normalizedX * 2.0f - 1.0f;
    const float ndcY = 1.0f - normalizedY * 2.0f;

    sf::Vector2f world = view.getInverseTransform().transformPoint(sf::Vector2f{ndcX, ndcY});
    world              = clampToFinite(world);

    return Vec2(world.x, world.y);
}

Vec2i worldToScreen(const Components::CCamera& camera, const sf::Vector2u& windowSizePx, const Vec2& worldPos)
{
    const sf::View   view = Internal::buildViewFromCamera(camera, windowSizePx);
    const ViewportPx vp   = computeViewportPx(view, windowSizePx);

    sf::Vector2f ndc = view.getTransform().transformPoint(sf::Vector2f{worldPos.x, worldPos.y});
    ndc              = clampToFinite(ndc);

    const float normalizedX = (ndc.x + 1.0f) * 0.5f;
    const float normalizedY = (1.0f - ndc.y) * 0.5f;

    const float px = vp.left + normalizedX * vp.width;
    const float py = vp.top + normalizedY * vp.height;

    return Vec2i{static_cast<int>(std::lround(px)), static_cast<int>(std::lround(py))};
}

}  // namespace Internal
