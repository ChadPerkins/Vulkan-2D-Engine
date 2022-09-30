#pragma once
#include "VE_Model.h"

#include <memory>

namespace VulkanEngine {

	struct RigidBody2DComponent {
		glm::vec2 Velocity;
		float Mass{1.0f};
	 };

	struct Transform2DComponent
	{
		glm::vec2 Translation{}; // Position offset
		glm::vec2 Scale{ 1.0f, 1.0f };
		float Rotation;

		glm::mat2 Mat2()
		{
			const float sin = glm::sin(Rotation);
			const float cos = glm::cos(Rotation);
			glm::mat2 RotationMat({ cos, sin }, { -sin, cos });
			
			glm::mat2 ScaleMat({ Scale.x, 0.0f }, { 0.0f, Scale.y });
			return RotationMat * ScaleMat;
		}
	};

	class VEGameObject
	{
	public:
		using id_t = unsigned int;

		static VEGameObject CreateGameObject()
		{
			static id_t currentId = 0;
			return VEGameObject(currentId++);
		}

		VEGameObject(const VEGameObject&) = delete;
		VEGameObject& operator=(const VEGameObject&) = delete;
		VEGameObject(VEGameObject&&) = default;
		VEGameObject& operator=(VEGameObject&&) = default;

		id_t GetId() { return m_Id; }

		std::shared_ptr<VEModel> m_Model;
		glm::vec3 m_Color{};
		Transform2DComponent m_Transform2D;
		RigidBody2DComponent m_RigidBody2D;

	private:
		VEGameObject(id_t objId) : m_Id{ objId } {}

	private:
		id_t m_Id;
	};
}