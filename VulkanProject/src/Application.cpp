#include "Application.h"
#include "SimpleRenderSystem.h"
#include "GravitySystem.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <array>
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace VulkanEngine {

	std::unique_ptr<VEModel> CreateSquareModel(VEDevice& device, glm::vec2 offset)
	{
		std::vector<VEModel::Vertex> vertices = {
			{{ -0.5f, -0.5f }},
			{{  0.5f,  0.5f }},
			{{ -0.5f,  0.5f }},
			{{ -0.5f, -0.5f }},
			{{  0.5f, -0.5f }},
			{{  0.5f,  0.5f }},
		};

		for (auto& v : vertices)
		{
			v.position += offset;
		}

		return std::make_unique<VEModel>(device, vertices);
	}

	std::unique_ptr<VEModel> CreateCircleModel(VEDevice& device, unsigned int numSides) 
	{
		std::vector<VEModel::Vertex> uniqueVertices = {};

		for (int i = 0; i < numSides; i++)
		{
			float angle = i * glm::two_pi<float>() / numSides;
			uniqueVertices.push_back({ {glm::cos(angle), glm::sin(angle)} });
		}

		uniqueVertices.push_back({});  // adds center vertex at 0, 0

		std::vector<VEModel::Vertex> vertices = {};

		for (int i = 0; i < numSides; i++) 
		{
			vertices.push_back(uniqueVertices[i]);
			vertices.push_back(uniqueVertices[(i + 1) % numSides]);
			vertices.push_back(uniqueVertices[numSides]);
		}

		return std::make_unique<VEModel>(device, vertices);
	}

	Application::Application()
	{
		LoadGameObjects();
	}

	Application::~Application()
	{

	}

	void Application::Run()
	{
		// create some models
		// offset model by .5 so rotation occurs at edge rather than center of square
		std::shared_ptr<VEModel> squareModel = CreateSquareModel(device, { .5f, .0f });  
		std::shared_ptr<VEModel> circleModel = CreateCircleModel(device, 64);

		// create physics objects
		std::vector<VEGameObject> physicsObjects = {};

		auto red	= VEGameObject::CreateGameObject();
		red.m_Transform2D.Scale			= glm::vec2{ .05f };
		red.m_Transform2D.Translation	= { .5f, .5f };
		red.m_Color						= { 1.f, 0.f, 0.f };
		red.m_RigidBody2D.Velocity		= { -.5f, .0f };
		red.m_Model						= circleModel;

		physicsObjects.push_back(std::move(red));

		auto blue	= VEGameObject::CreateGameObject();
		blue.m_Transform2D.Scale		= glm::vec2{ .05f };
		blue.m_Transform2D.Translation	= { -.45f, -.25f };
		blue.m_Color					= { 0.f, 0.f, 1.f };
		blue.m_RigidBody2D.Velocity		= { .5f, .0f };
		blue.m_Model					= circleModel;

		physicsObjects.push_back(std::move(blue));

		// create vector field
		std::vector<VEGameObject> vectorField{};
		int gridCount = 40;
		for (int i = 0; i < gridCount; i++) 
		{
			for (int j = 0; j < gridCount; j++) 
			{
				auto vf = VEGameObject::CreateGameObject();

				vf.m_Transform2D.Scale = glm::vec2(0.005f);
				vf.m_Transform2D.Translation = {
					-1.0f + (i + 0.5f) * 2.0f / gridCount,
					-1.0f + (j + 0.5f) * 2.0f / gridCount };
				vf.m_Color = glm::vec3(1.0f);
				vf.m_Model = squareModel;

				vectorField.push_back(std::move(vf));
			}
		}

		GravityPhysicsSystem gravitySystem{ 0.81f };
		Vec2FieldSystem vecFieldSystem{};

		SimpleRenderSystem simpleRenderSystem(device, renderer.GetSwapChainRenderPass());

		while (!window.Close())
		{
			glfwPollEvents();
			
			if (auto commandBuffer = renderer.BeginFrame())
			{
				// update systems
				gravitySystem.Update(physicsObjects, 1.f / 60, 5);
				vecFieldSystem.Update(gravitySystem, physicsObjects, vectorField);

				// render system
				renderer.BeginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.RenderGameObjects(commandBuffer, physicsObjects);

				simpleRenderSystem.RenderGameObjects(commandBuffer, vectorField);

				renderer.EndSwapChainRenderPass(commandBuffer);
				renderer.EndFrame();
			}
		}

		// Block the CPU until all GPU operations are completed
		vkDeviceWaitIdle(device.Device());
	}

	void Application::LoadGameObjects()
	{
		std::vector<VEModel::Vertex> vertices = {
	  { {  0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
	  { {  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f } },
	  { { -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f } }};

		//Sierpinski(vertices, 4, { 0.0f, -0.5f }, { 0.5f,  0.5f }, { -0.5f,  0.5f });
		
		auto model = std::make_shared<VEModel>(device, vertices);

		auto triangle = VEGameObject::CreateGameObject();

		triangle.m_Model							= model;
		triangle.m_Color							= { 0.1f, 0.8f, 0.1f };
		triangle.m_Transform2D.Translation.x		= 0.2f;
		triangle.m_Transform2D.Scale				= { 2.0f, 0.5f };
		triangle.m_Transform2D.Rotation				= 0.25f * glm::two_pi<float>();

		gameObjects.push_back(std::move(triangle));
	}
		
	void Application::Sierpinski(std::vector<VEModel::Vertex>& vertices,
		uint32_t depth,
		glm::vec2 top,
		glm::vec2 right,
		glm::vec2 left)
	{
		if (depth <= 0)
		{
			vertices.push_back({ top });
			vertices.push_back({ right });
			vertices.push_back({ left });
		
		}
		else
		{
			auto leftTop	= 0.5f * (left + top);
			auto rightTop	= 0.5f * (right + top);
			auto leftRight	= 0.5f * (left + right);
			Sierpinski(vertices, depth - 1, leftTop, leftRight, left);
			Sierpinski(vertices, depth - 1, rightTop, right, leftRight);
			Sierpinski(vertices, depth - 1, top, rightTop, leftTop);
			
		}
	}

}