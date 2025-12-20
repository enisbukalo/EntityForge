#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

#include <CameraProjection.h>
#include <CameraView.h>

namespace
{
constexpr float kPi = 3.14159265358979323846f;

float computeWorldEpsilonFromPixel(const Components::CCamera& camera, const sf::Vector2u& windowSizePx)
{
    const sf::View view  = Internal::buildViewFromCamera(camera, windowSizePx);
    const auto     vpNdc = view.getViewport();

    const float vpWidthPx  = std::max(1.0f, vpNdc.size.x * static_cast<float>(windowSizePx.x));
    const float vpHeightPx = std::max(1.0f, vpNdc.size.y * static_cast<float>(windowSizePx.y));

    const float worldPerPixelX = std::abs(view.getSize().x) / vpWidthPx;
    const float worldPerPixelY = std::abs(view.getSize().y) / vpHeightPx;

    // A bit more than half a pixel in world units.
    return std::max(worldPerPixelX, worldPerPixelY) * 0.75f + 1e-4f;
}

void expectVecNear(const Vec2& a, const Vec2& b, float eps)
{
    EXPECT_NEAR(a.x, b.x, eps);
    EXPECT_NEAR(a.y, b.y, eps);
}

void expectRoundTrip(const Components::CCamera& camera, const sf::Vector2u& windowSizePx, const Vec2& worldPos)
{
    const float eps = computeWorldEpsilonFromPixel(camera, windowSizePx);

    const Vec2i px   = Internal::worldToScreen(camera, windowSizePx, worldPos);
    const Vec2  back = Internal::screenToWorld(camera, windowSizePx, px);

    expectVecNear(back, worldPos, eps);
}

Vec2i viewportCenterPx(const Components::CCamera& camera, const sf::Vector2u& windowSizePx)
{
    const int cx = static_cast<int>(std::lround((camera.viewport.left + camera.viewport.width * 0.5f) * windowSizePx.x));
    const int cy = static_cast<int>(std::lround((camera.viewport.top + camera.viewport.height * 0.5f) * windowSizePx.y));
    return Vec2i{cx, cy};
}
}

TEST(CameraProjectionTest, ViewportCenterMapsToCameraPosition)
{
    Components::CCamera camera;
    camera.position        = Vec2(3.0f, 4.0f);
    camera.worldHeight     = 10.0f;
    camera.zoom            = 1.0f;
    camera.rotationRadians = 0.0f;

    camera.viewport.left   = 0.25f;
    camera.viewport.top    = 0.25f;
    camera.viewport.width  = 0.50f;
    camera.viewport.height = 0.50f;

    const sf::Vector2u windowSizePx(800, 600);

    const Vec2i centerPx = viewportCenterPx(camera, windowSizePx);

    const Vec2 world = Internal::screenToWorld(camera, windowSizePx, centerPx);
    expectVecNear(world, camera.position, 1e-3f);

    const Vec2i pixel = Internal::worldToScreen(camera, windowSizePx, camera.position);
    EXPECT_EQ(pixel.x, centerPx.x);
    EXPECT_EQ(pixel.y, centerPx.y);
}

TEST(CameraProjectionTest, RoundTripWithTranslation)
{
    Components::CCamera camera;
    camera.position    = Vec2(12.0f, -7.0f);
    camera.worldHeight = 10.0f;

    const sf::Vector2u windowSizePx(800, 600);

    expectRoundTrip(camera, windowSizePx, Vec2(12.0f, -7.0f));
    expectRoundTrip(camera, windowSizePx, Vec2(14.25f, -3.5f));
    expectRoundTrip(camera, windowSizePx, Vec2(6.0f, -9.0f));
}

TEST(CameraProjectionTest, RoundTripWithZoom)
{
    Components::CCamera camera;
    camera.position    = Vec2(-2.0f, 5.0f);
    camera.worldHeight = 10.0f;
    camera.zoom        = 2.5f;

    const sf::Vector2u windowSizePx(1024, 768);

    expectRoundTrip(camera, windowSizePx, Vec2(-2.0f, 5.0f));
    expectRoundTrip(camera, windowSizePx, Vec2(-1.0f, 6.0f));
    expectRoundTrip(camera, windowSizePx, Vec2(-4.25f, 3.0f));
}

TEST(CameraProjectionTest, RoundTripWithRotation)
{
    Components::CCamera camera;
    camera.position        = Vec2(1.0f, 2.0f);
    camera.worldHeight     = 10.0f;
    camera.zoom            = 1.0f;
    camera.rotationRadians = kPi * 0.25f;

    const sf::Vector2u windowSizePx(800, 600);

    expectRoundTrip(camera, windowSizePx, Vec2(1.0f, 2.0f));
    expectRoundTrip(camera, windowSizePx, Vec2(4.0f, 2.0f));
    expectRoundTrip(camera, windowSizePx, Vec2(1.0f, 6.0f));
}

TEST(CameraProjectionTest, RoundTripWithNonFullscreenViewport)
{
    Components::CCamera camera;
    camera.position    = Vec2(0.0f, 0.0f);
    camera.worldHeight = 10.0f;
    camera.zoom        = 1.3f;

    camera.viewport.left   = 0.10f;
    camera.viewport.top    = 0.20f;
    camera.viewport.width  = 0.50f;
    camera.viewport.height = 0.60f;

    const sf::Vector2u windowSizePx(900, 500);

    expectRoundTrip(camera, windowSizePx, Vec2(0.0f, 0.0f));
    expectRoundTrip(camera, windowSizePx, Vec2(2.0f, 3.0f));
    expectRoundTrip(camera, windowSizePx, Vec2(-4.0f, -1.5f));
}
