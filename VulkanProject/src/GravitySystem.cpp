#include "GravitySystem.h"


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace VulkanEngine {

    GravityPhysicsSystem::GravityPhysicsSystem(float strength)
        : m_StrengthGravity{ strength }
    {

    }

    // dt stands for delta time, and specifies the amount of time to advance the simulation
    // substeps is how many intervals to divide the forward time step in. More substeps result in a
    // more stable simulation, but takes longer to compute
    void GravityPhysicsSystem::Update(std::vector<VEGameObject>& objs, float dt, unsigned int substeps)
    {
        const float stepDelta = dt / substeps;
        for (int i = 0; i < substeps; i++) {
            StepSimulation(objs, stepDelta);
        }
    }

    glm::vec2 GravityPhysicsSystem::ComputeForce(VEGameObject& fromObj, VEGameObject& toObj) const
    {
        auto offset = fromObj.m_Transform2D.Translation - toObj.m_Transform2D.Translation;
        float distanceSquared = glm::dot(offset, offset);

        // clown town - just going to return 0 if objects are too close together...
        if (glm::abs(distanceSquared) < 1e-10f) {
            return { .0f, .0f };
        }

        float force = m_StrengthGravity * toObj.m_RigidBody2D.Mass * fromObj.m_RigidBody2D.Mass / distanceSquared;
        return force * offset / glm::sqrt(distanceSquared);
    }

    void GravityPhysicsSystem::StepSimulation(std::vector<VEGameObject>& physicsObjs, float dt) {
        // Loops through all pairs of objects and applies attractive force between them
        for (auto iterA = physicsObjs.begin(); iterA != physicsObjs.end(); ++iterA) {
            auto& objA = *iterA;
            for (auto iterB = iterA; iterB != physicsObjs.end(); ++iterB) {
                if (iterA == iterB) continue;
                auto& objB = *iterB;

                auto force = ComputeForce(objA, objB);
                objA.m_RigidBody2D.Velocity += dt * -force / objA.m_RigidBody2D.Mass;
                objB.m_RigidBody2D.Velocity += dt * force / objB.m_RigidBody2D.Mass;
            }
        }

        // update each objects position based on its final velocity
        for (auto& obj : physicsObjs) {
            obj.m_Transform2D.Translation += dt * obj.m_RigidBody2D.Velocity;
        }
    }
 
    void Vec2FieldSystem::Update(const GravityPhysicsSystem& physicsSystem,
        std::vector<VEGameObject>& physicsObjs,
        std::vector<VEGameObject>& vectorField) {
        // For each field line we caluclate the net graviation force for that point in space
        for (auto& vf : vectorField) {
            glm::vec2 direction{};
            for (auto& obj : physicsObjs) {
                direction += physicsSystem.ComputeForce(obj, vf);
            }

            // This scales the length of the field line based on the log of the length
            // values were chosen just through trial and error based on what i liked the look
            // of and then the field line is rotated to point in the direction of the field
            vf.m_Transform2D.Scale.x =
                0.005f + 0.045f * glm::clamp(glm::log(glm::length(direction) + 1) / 3.f, 0.f, 1.f);
            vf.m_Transform2D.Rotation = atan2(direction.y, direction.x);
        }
    }
  
}