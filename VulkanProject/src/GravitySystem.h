#pragma once
#include "VE_GameObject.h"

#include <vector>

namespace VulkanEngine {

    class GravityPhysicsSystem {
    public:
        GravityPhysicsSystem(float strength);

        const float m_StrengthGravity;

        void Update(std::vector<VEGameObject>& objs, float dt, unsigned int substeps = 1);
        glm::vec2 ComputeForce(VEGameObject& fromObj, VEGameObject& toObj) const;


    private:
        void StepSimulation(std::vector<VEGameObject>& physicsObjs, float dt);
    };

    class Vec2FieldSystem
    {
    public:
        void Update(const GravityPhysicsSystem& physicsSystem,
            std::vector<VEGameObject>& physicsObjs,
            std::vector<VEGameObject>& vectorField);
        
    };

 }