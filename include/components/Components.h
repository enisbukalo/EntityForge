#ifndef COMPONENTS_H
#define COMPONENTS_H

// Umbrella header that exposes all component definitions.
// Including this file makes all Components available to any TU.

// Core components
#include "CCollider2D.h"
#include "CPhysicsBody2D.h"
#include "CRenderable.h"
#include "CTransform.h"

// Audio components
#include "CAudioListener.h"
#include "CAudioSettings.h"
#include "CAudioSource.h"

// Particle and input
#include "CInputController.h"
#include "CParticleEmitter.h"

// Camera
#include "CCamera.h"

// Scripting / behaviours
#include "CNativeScript.h"

// Utility / other components
#include "CMaterial.h"
#include "CName.h"
#include "CShader.h"
#include "CTexture.h"

#endif  // COMPONENTS_H
