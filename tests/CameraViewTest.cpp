#include <gtest/gtest.h>

#include <cmath>

#include <CameraView.h>

namespace
{
constexpr float kEpsilon = 1e-4f;
constexpr float kPi      = 3.14159265358979323846f;

float normalizeDegrees(float degrees)
{
    float normalized = std::fmod(degrees, 360.0f);
    if (normalized < 0.0f)
    {
        normalized += 360.0f;
    }
    return normalized;
}
}

TEST(CameraViewTest, CenterMatchesCameraPosition)
{
    Components::CCamera camera;
    camera.position = Vec2(3.0f, 4.0f);

    const sf::View view = Internal::buildViewFromCamera(camera, sf::Vector2u(800, 600));

    EXPECT_NEAR(view.getCenter().x, 3.0f, kEpsilon);
    EXPECT_NEAR(view.getCenter().y, 4.0f, kEpsilon);
}

TEST(CameraViewTest, ViewportPassthroughMatchesCameraViewport)
{
    Components::CCamera camera;
    camera.viewport.left   = 0.25f;
    camera.viewport.top    = 0.10f;
    camera.viewport.width  = 0.50f;
    camera.viewport.height = 0.75f;

    const sf::View view = Internal::buildViewFromCamera(camera, sf::Vector2u(800, 600));

    const sf::FloatRect vp = view.getViewport();
    EXPECT_NEAR(vp.left, 0.25f, kEpsilon);
    EXPECT_NEAR(vp.top, 0.10f, kEpsilon);
    EXPECT_NEAR(vp.width, 0.50f, kEpsilon);
    EXPECT_NEAR(vp.height, 0.75f, kEpsilon);
}

TEST(CameraViewTest, SizeUsesWorldHeightZoomAndWindowAspect)
{
    Components::CCamera camera;
    camera.worldHeight = 10.0f;
    camera.zoom        = 2.0f;  // => height becomes 5

    const sf::View view = Internal::buildViewFromCamera(camera, sf::Vector2u(800, 600));

    const float expectedHeight = 5.0f;
    const float expectedWidth  = expectedHeight * (800.0f / 600.0f);

    EXPECT_NEAR(view.getSize().x, expectedWidth, 1e-3f);
    EXPECT_NEAR(view.getSize().y, -expectedHeight, 1e-3f);
}

TEST(CameraViewTest, SizeAspectUsesViewportPixelsNotFullWindow)
{
    Components::CCamera camera;
    camera.worldHeight       = 10.0f;
    camera.zoom              = 2.0f;  // => height becomes 5
    camera.viewport.width    = 1.0f;
    camera.viewport.height   = 0.5f;  // half-height => aspect doubles

    const sf::View view = Internal::buildViewFromCamera(camera, sf::Vector2u(800, 600));

    const float expectedHeight = 5.0f;
    const float expectedWidth  = expectedHeight * (800.0f / (600.0f * 0.5f));

    EXPECT_NEAR(view.getSize().x, expectedWidth, 1e-3f);
    EXPECT_NEAR(view.getSize().y, -expectedHeight, 1e-3f);
}

TEST(CameraViewTest, ZoomZeroFallsBackToOne)
{
    Components::CCamera camera;
    camera.worldHeight = 10.0f;
    camera.zoom        = 0.0f;

    const sf::View view = Internal::buildViewFromCamera(camera, sf::Vector2u(800, 600));

    const float expectedHeight = 10.0f;
    const float expectedWidth  = expectedHeight * (800.0f / 600.0f);

    EXPECT_NEAR(view.getSize().x, expectedWidth, 1e-3f);
    EXPECT_NEAR(view.getSize().y, -expectedHeight, 1e-3f);
}

TEST(CameraViewTest, ZeroViewportDimensionsUseWindowAspectForSizing)
{
    Components::CCamera camera;
    camera.worldHeight     = 10.0f;
    camera.zoom            = 1.0f;
    camera.viewport.width  = 0.0f;
    camera.viewport.height = 0.0f;

    const sf::View view = Internal::buildViewFromCamera(camera, sf::Vector2u(800, 600));

    const float expectedHeight = 10.0f;
    const float expectedWidth  = expectedHeight * (800.0f / 600.0f);

    EXPECT_NEAR(view.getSize().x, expectedWidth, 1e-3f);
    EXPECT_NEAR(view.getSize().y, -expectedHeight, 1e-3f);
}

TEST(CameraViewTest, ZeroWindowSizeDoesNotProduceNaNOrInf)
{
    Components::CCamera camera;
    camera.worldHeight = 10.0f;
    camera.zoom        = 1.0f;

    const sf::View view = Internal::buildViewFromCamera(camera, sf::Vector2u(0, 0));

    EXPECT_TRUE(std::isfinite(view.getSize().x));
    EXPECT_TRUE(std::isfinite(view.getSize().y));
}

TEST(CameraViewTest, RotationRadiansConvertedToDegreesWithExpectedSign)
{
    Components::CCamera camera;
    camera.rotationRadians = kPi * 0.5f;

    const sf::View view = Internal::buildViewFromCamera(camera, sf::Vector2u(800, 600));

    // SFML normalizes rotation to [0, 360). Treat angles equivalent mod 360 as equal.
    EXPECT_NEAR(normalizeDegrees(view.getRotation()), normalizeDegrees(-90.0f), 1e-3f);
}
