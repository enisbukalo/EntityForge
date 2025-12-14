#include "SystemLocator.h"

#include "S2DPhysics.h"
#include "SAudio.h"
#include "SCamera.h"
#include "SInput.h"
#include "SParticle.h"
#include "SRenderer.h"

namespace Systems
{

namespace
{
SInput*     g_input    = nullptr;
S2DPhysics* g_physics  = nullptr;
SRenderer*  g_renderer = nullptr;
SParticle*  g_particle = nullptr;
SAudio*     g_audio    = nullptr;
SCamera*    g_camera   = nullptr;
}  // namespace

void SystemLocator::provideInput(SInput* input)
{
    g_input = input;
}

void SystemLocator::providePhysics(S2DPhysics* physics)
{
    g_physics = physics;
}

void SystemLocator::provideRenderer(SRenderer* renderer)
{
    g_renderer = renderer;
}

void SystemLocator::provideParticle(SParticle* particle)
{
    g_particle = particle;
}

void SystemLocator::provideAudio(SAudio* audio)
{
    g_audio = audio;
}

void SystemLocator::provideCamera(SCamera* camera)
{
    g_camera = camera;
}

SInput& SystemLocator::input()
{
    assert(g_input && "Input system not set");
    return *g_input;
}

S2DPhysics& SystemLocator::physics()
{
    assert(g_physics && "Physics system not set");
    return *g_physics;
}

SRenderer& SystemLocator::renderer()
{
    assert(g_renderer && "Renderer system not set");
    return *g_renderer;
}

SParticle& SystemLocator::particle()
{
    assert(g_particle && "Particle system not set");
    return *g_particle;
}

SAudio& SystemLocator::audio()
{
    assert(g_audio && "Audio system not set");
    return *g_audio;
}

SCamera& SystemLocator::camera()
{
    assert(g_camera && "Camera system not set");
    return *g_camera;
}

SInput* SystemLocator::tryInput()
{
    return g_input;
}

S2DPhysics* SystemLocator::tryPhysics()
{
    return g_physics;
}

SRenderer* SystemLocator::tryRenderer()
{
    return g_renderer;
}

SParticle* SystemLocator::tryParticle()
{
    return g_particle;
}

SAudio* SystemLocator::tryAudio()
{
    return g_audio;
}

SCamera* SystemLocator::tryCamera()
{
    return g_camera;
}

}  // namespace Systems
