//Ewan Chang - 100787343
//Kevin Huang - 100788603

#include <Logging.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <json.hpp>
#include <fstream>
#include <sstream>
#include <typeindex>
#include <optional>
#include <string>

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

// Graphics
#include "Graphics/IndexBuffer.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture2D.h"
#include "Graphics/VertexTypes.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/MovementBehaviour.h"
#include "Gameplay/Components/Player2MovementBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "../../src/Graphics/DebugDraw.h"

//#define LOG_GL_NOTIFICATIONS

/*
	Handles debug messages from OpenGL
	https://www.khronos.org/opengl/wiki/Debug_Output#Message_Components
	@param source    Which part of OpenGL dispatched the message
	@param type      The type of message (ex: error, performance issues, deprecated behavior)
	@param id        The ID of the error or message (to distinguish between different types of errors, like nullref or index out of range)
	@param severity  The severity of the message (from High to Notification)
	@param length    The length of the message
	@param message   The human readable message from OpenGL
	@param userParam The pointer we set with glDebugMessageCallback (should be the game pointer)
*/
void GlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	std::string sourceTxt;
	switch (source) {
		case GL_DEBUG_SOURCE_API: sourceTxt = "DEBUG"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceTxt = "WINDOW"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceTxt = "SHADER"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY: sourceTxt = "THIRD PARTY"; break;
		case GL_DEBUG_SOURCE_APPLICATION: sourceTxt = "APP"; break;
		case GL_DEBUG_SOURCE_OTHER: default: sourceTxt = "OTHER"; break;
	}
	switch (severity) {
		case GL_DEBUG_SEVERITY_LOW:          LOG_INFO("[{}] {}", sourceTxt, message); break;
		case GL_DEBUG_SEVERITY_MEDIUM:       LOG_WARN("[{}] {}", sourceTxt, message); break;
		case GL_DEBUG_SEVERITY_HIGH:         LOG_ERROR("[{}] {}", sourceTxt, message); break;
			#ifdef LOG_GL_NOTIFICATIONS
		case GL_DEBUG_SEVERITY_NOTIFICATION: LOG_INFO("[{}] {}", sourceTxt, message); break;
			#endif
		default: break;
	}
}

// Stores our GLFW window in a global variable for now
GLFWwindow* window;
// The current size of our window in pixels
glm::ivec2 windowSize = glm::ivec2(800, 800);
// The title of our GLFW window
std::string windowTitle = "INFR-1350U";

// using namespace should generally be avoided, and if used, make sure it's ONLY in cpp files
using namespace Gameplay;
using namespace Gameplay::Physics;

// The scene that we will be rendering
// The scene that we will be rendering
Scene::Sptr scene = nullptr;

void GlfwWindowResizedCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	windowSize = glm::ivec2(width, height);
	if (windowSize.x * windowSize.y > 0) {
		scene->MainCamera->ResizeWindow(width, height);
	}
}

/// <summary>
/// Handles intializing GLFW, should be called before initGLAD, but after Logger::Init()
/// Also handles creating the GLFW window
/// </summary>
/// <returns>True if GLFW was initialized, false if otherwise</returns>
bool initGLFW() {
	// Initialize GLFW
	if (glfwInit() == GLFW_FALSE) {
		LOG_ERROR("Failed to initialize GLFW");
		return false;
	}

	//Create a new GLFW window and make it current
	window = glfwCreateWindow(windowSize.x, windowSize.y, windowTitle.c_str(), nullptr, nullptr);
	glfwMakeContextCurrent(window);
	
	// Set our window resized callback
	glfwSetWindowSizeCallback(window, GlfwWindowResizedCallback);

	return true;
}

/// <summary>
/// Handles initializing GLAD and preparing our GLFW window for OpenGL calls
/// </summary>
/// <returns>True if GLAD is loaded, false if there was an error</returns>
bool initGLAD() {
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		LOG_ERROR("Failed to initialize Glad");
		return false;
	}
	return true;
}

/// <summary>
/// Draws a widget for saving or loading our scene
/// </summary>
/// <param name="scene">Reference to scene pointer</param>
/// <param name="path">Reference to path string storage</param>
/// <returns>True if a new scene has been loaded</returns>
bool DrawSaveLoadImGui(Scene::Sptr& scene, std::string& path) {
	// Since we can change the internal capacity of an std::string,
	// we can do cool things like this!
	ImGui::InputText("Path", path.data(), path.capacity());

	// Draw a save button, and save when pressed
	if (ImGui::Button("Save")) {
		scene->Save(path);
		std::string newFilename = std::filesystem::path(path).stem().string() + "-manifest.json";
		ResourceManager::SaveManifest(newFilename);
	}
	ImGui::SameLine();
	// Load scene from file button
	if (ImGui::Button("Load")) {
		// Since it's a reference to a ptr, this will
		// overwrite the existing scene!
		//scene = nullptr;
		std::string newFilename = std::filesystem::path(path).stem().string() + "-manifest.json";
		ResourceManager::LoadManifest(newFilename);
		scene = Scene::Load(path);

		return true;
	}
	return false;
}

/// <summary>
/// Draws some ImGui controls for the given light
/// </summary>
/// <param name="title">The title for the light's header</param>
/// <param name="light">The light to modify</param>
/// <returns>True if the parameters have changed, false if otherwise</returns>
bool DrawLightImGui(const Scene::Sptr& scene, const char* title, int ix) {
	bool isEdited = false;
	bool result = false;
	Light& light = scene->Lights[ix];
	ImGui::PushID(&light); // We can also use pointers as numbers for unique IDs
	if (ImGui::CollapsingHeader(title)) {
		isEdited |= ImGui::DragFloat3("Pos", &light.Position.x, 0.01f);
		isEdited |= ImGui::ColorEdit3("Col", &light.Color.r);
		isEdited |= ImGui::DragFloat("Range", &light.Range, 0.1f);

		result = ImGui::Button("Delete");
	}
	if (isEdited) {
		scene->SetShaderLight(ix);
	}

	ImGui::PopID();
	return result;
}

int main() {
	Logger::Init(); // We'll borrow the logger from the toolkit, but we need to initialize it

	//Initialize GLFW
	if (!initGLFW())
		return 1;

	//Initialize GLAD
	if (!initGLAD())
		return 1;

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(GlDebugMessage, nullptr);

	// Initialize our ImGui helper
	ImGuiHelper::Init(window);

	// Initialize our resource manager
	ResourceManager::Init();

	// Register all our resource types so we can load them from manifest files
	ResourceManager::RegisterType<Texture2D>();
	ResourceManager::RegisterType<Material>();
	ResourceManager::RegisterType<MeshResource>();
	ResourceManager::RegisterType<Shader>();

	// Register all of our component types so we can load them from files
	ComponentManager::RegisterType<Camera>();
	ComponentManager::RegisterType<RenderComponent>();
	ComponentManager::RegisterType<RigidBody>();
	ComponentManager::RegisterType<TriggerVolume>();
	ComponentManager::RegisterType<RotatingBehaviour>();
	ComponentManager::RegisterType<JumpBehaviour>();
	ComponentManager::RegisterType<MovementBehaviour>();
	ComponentManager::RegisterType<Player2MovementBehaviour>();
	ComponentManager::RegisterType<MaterialSwapBehaviour>();

	// GL states, we'll enable depth testing and backface fulling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene) {
		ResourceManager::LoadManifest("manifest.json");
		scene = Scene::Load("scene.json");
	} 
	else {
		// Create our OpenGL resources
		Shader::Sptr uboShader = ResourceManager::CreateAsset<Shader>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shader.glsl" }, 
			{ ShaderPartType::Fragment, "shaders/frag_blinn_phong_textured.glsl" }
		}); 

		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
		MeshResource::Sptr tableMesh = ResourceManager::CreateAsset<MeshResource>("AirHockeyTable.obj");
		MeshResource::Sptr puckMesh = ResourceManager::CreateAsset<MeshResource>("AirHockeyPuck.obj");
		MeshResource::Sptr paddleMesh = ResourceManager::CreateAsset<MeshResource>("AirHockeyPaddle.obj");

		Texture2D::Sptr boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
		Texture2D::Sptr tableTexture = ResourceManager::CreateAsset<Texture2D>("textures/Table-Texture.png");
		Texture2D::Sptr puckTexture  = ResourceManager::CreateAsset<Texture2D>("textures/Puck-texture.png");  
		Texture2D::Sptr paddle2Texture  = ResourceManager::CreateAsset<Texture2D>("textures/Paddle2-texture.png");  
		Texture2D::Sptr monkeyTex  = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");  
		
		// Create an empty scene
		scene = std::make_shared<Scene>();

		// I hate this
		scene->BaseShader = uboShader;

		// Create our materials
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>();
		{
			boxMaterial->Name = "Box";
			boxMaterial->MatShader = scene->BaseShader;
			boxMaterial->Texture = boxTexture;
			boxMaterial->Shininess = 2.0f;
		}	

		Material::Sptr monkeyMaterial = ResourceManager::CreateAsset<Material>();
		{
			monkeyMaterial->Name = "Monkey";
			monkeyMaterial->MatShader = scene->BaseShader;
			monkeyMaterial->Texture = monkeyTex;
			monkeyMaterial->Shininess = 256.0f;

		}

		Material::Sptr tableMaterial = ResourceManager::CreateAsset<Material>();
		{
			tableMaterial->Name = "Table";
			tableMaterial->MatShader = scene->BaseShader;
			tableMaterial->Texture = tableTexture;
			tableMaterial->Shininess = 256.0f;

		}

		Material::Sptr puckMaterial = ResourceManager::CreateAsset<Material>();
		{
			puckMaterial->Name = "Puck";
			puckMaterial->MatShader = scene->BaseShader;
			puckMaterial->Texture = puckTexture;
			puckMaterial->Shininess = 256.0f;
		}

		Material::Sptr paddleMaterial = ResourceManager::CreateAsset<Material>();
		{
			paddleMaterial->Name = "Paddle";
			paddleMaterial->MatShader = scene->BaseShader;
			paddleMaterial->Texture = puckTexture;
			paddleMaterial->Shininess = 256.0f;
		}

		Material::Sptr paddle2Material = ResourceManager::CreateAsset<Material>();
		{
			paddle2Material->Name = "Paddle 2";
			paddle2Material->MatShader = scene->BaseShader;
			paddle2Material->Texture = paddle2Texture;
			paddle2Material->Shininess = 256.0f;
		}

		// Create some lights for our scene
		scene->Lights.resize(3);
		scene->Lights[0].Position = glm::vec3(-8.0f, 0.0f, 6.0f);
		scene->Lights[0].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[0].Range = 30.0f;

		scene->Lights[1].Position = glm::vec3(8.0f, 0.0f, 6.0f);
		scene->Lights[1].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[1].Range = 30.0f;

		scene->Lights[2].Position = glm::vec3(0.0f, 0.0f, 6.0f);
		scene->Lights[2].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[2].Range = 30.0f;

		// We'll create a mesh that is a simple plane that we can resize later
		/*MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();*/

		// Set up the scene's camera
		GameObject::Sptr camera = scene->CreateGameObject("Main Camera");
		{
			camera->SetPostion(glm::vec3(0, 0, 12));
			camera->LookAt(glm::vec3(0.0f));
			camera->SetRotation(glm::vec3(0.0f, 0.0f, -180.0f));

			Camera::Sptr cam = camera->Add<Camera>();

			// Make sure that the camera is set as the scene's main camera!
			scene->MainCamera = cam;
		}

		GameObject::Sptr table = scene->CreateGameObject("Table");
		{

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = table->Add<RenderComponent>();
			renderer->SetMesh(tableMesh);
			renderer->SetMaterial(tableMaterial);

			table->SetScale(glm::vec3(2.5));
			table->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));
			

			// Add a static rigid body
			RigidBody::Sptr tablePhysics = table->Add<RigidBody>(RigidBodyType::Static);
			tablePhysics->SetMass(10.0f);
			//Table Surface
			BoxCollider::Sptr tableCollider = BoxCollider::Create(glm::vec3(1.0f, 1.0f, 1.0f));
			tablePhysics->AddCollider(tableCollider);
			tableCollider->SetScale(glm::vec3(11.0f, 2.5f, 9.0f));
			tableCollider->SetPosition(glm::vec3(0.0f, -0.8f, 0.0f));

			//Top Wall
			BoxCollider::Sptr topWallCollider = BoxCollider::Create(glm::vec3(1.0f, 1.0f, 1.0f));
			tablePhysics->AddCollider(topWallCollider);
			topWallCollider->SetPosition(glm::vec3(-0.23f, 2.0f, 6.05f));
			topWallCollider->SetScale(glm::vec3(11.2f, 1.5f, 1.0f));

			//Right Wall
			BoxCollider::Sptr rightWallCollider = BoxCollider::Create(glm::vec3(1.0f, 1.0f, 1.0f));
			tablePhysics->AddCollider(rightWallCollider);
			rightWallCollider->SetPosition(glm::vec3(-11.4f, 2.0f, 0.0f));
			rightWallCollider->SetScale(glm::vec3(1.0f, 1.5f, 5.5f));

			//Bottom Wall
			BoxCollider::Sptr bottomWallCollider = BoxCollider::Create(glm::vec3(1.0f, 1.0f, 1.0f));
			tablePhysics->AddCollider(bottomWallCollider);
			bottomWallCollider->SetPosition(glm::vec3(0.0f, 2.0f, -6.2f));
			bottomWallCollider->SetScale(glm::vec3(11.2f, 1.5f, 1.0f));

			//Left Wall
			BoxCollider::Sptr leftWallCollider = BoxCollider::Create(glm::vec3(1.0f, 1.0f, 1.0f));
			tablePhysics->AddCollider(leftWallCollider);
			leftWallCollider->SetPosition(glm::vec3(11.4f, 2.0f, 0.0f));
			leftWallCollider->SetScale(glm::vec3(1.0f, 1.2f, 5.5f));

			//Table Roof
			BoxCollider::Sptr roofCollider = BoxCollider::Create(glm::vec3(2.5f, 2.5f, 2.5f));
			tablePhysics->AddCollider(roofCollider);
			roofCollider->SetPosition(glm::vec3(0.0f, 4.65f, 0.0f));
			roofCollider->SetScale(glm::vec3(4.4f, 0.5f, 2.2f));
		}

		GameObject::Sptr puck = scene->CreateGameObject("Puck");
		{
			// Set position in the scene
			puck->SetPostion(glm::vec3(0.0f, 0.0f, 2.0f));
			// Scale down the plane
			puck->SetScale(glm::vec3(0.5f));
			puck->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));

			puck->Add<JumpBehaviour>(); // To set angular factor to 1 on Z axis

			// Create and attach a render component
			RenderComponent::Sptr renderer = puck->Add<RenderComponent>();
			renderer->SetMesh(puckMesh);
			renderer->SetMaterial(puckMaterial);

			RigidBody::Sptr puckPhysics = puck->Add<RigidBody>(RigidBodyType::Dynamic);
			ICollider::Sptr puckCollider = puckPhysics->AddCollider(ConvexMeshCollider::Create());
			puckCollider->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

			// We'll add a behaviour that will interact with our trigger volumes
			MaterialSwapBehaviour::Sptr triggerInteraction = puck->Add<MaterialSwapBehaviour>();
			triggerInteraction->EnterMaterial = boxMaterial;
			triggerInteraction->ExitMaterial = puckMaterial;
		}

		GameObject::Sptr paddle = scene->CreateGameObject("Paddle");
		{
			// Set position in the scene
			paddle->SetPostion(glm::vec3(3.0f, 0.0f, 1.1f));
			paddle->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));
			paddle->SetScale(glm::vec3(0.6f));

			// Add some behaviour that relies on the physics body
			paddle->Add<MovementBehaviour>();

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = paddle->Add<RenderComponent>();
			renderer->SetMesh(paddleMesh);
			renderer->SetMaterial(puckMaterial);

			// Add a dynamic rigid body to this monkey
			RigidBody::Sptr playerPhysics = paddle->Add<RigidBody>(RigidBodyType::Dynamic);
			playerPhysics->SetMass(1.0f);
			//playerPhysics->AddCollider(ConvexMeshCollider::Create());
			BoxCollider::Sptr playerCollider = BoxCollider::Create(glm::vec3(0.6f, 0.6f, 0.6f));
			playerPhysics->AddCollider(playerCollider);
			//playerPhysics->SetAngularFactor(glm::vec3(0.0f, 0.0f, 1.0f));
			playerCollider->SetPosition(glm::vec3(0.0f, 0.8f, 0.0f));
		}

		GameObject::Sptr paddle2 = scene->CreateGameObject("Paddle 2");
		{
			// Set and rotation position in the scene
			paddle2->SetPostion(glm::vec3(-3.0f, 0.0f, 1.1f));
			paddle2->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));
			paddle2->SetScale(glm::vec3(0.6f));

			paddle2->Add<Player2MovementBehaviour>();

			// Add a render component
			RenderComponent::Sptr renderer = paddle2->Add<RenderComponent>();
			renderer->SetMesh(paddleMesh);
			renderer->SetMaterial(paddle2Material);

			// Add a dynamic rigid body to this monkey
			RigidBody::Sptr player2Physics = paddle2->Add<RigidBody>(RigidBodyType::Dynamic);
			player2Physics->SetMass(1.0f);
			//player2Physics->AddCollider(ConvexMeshCollider::Create());
			BoxCollider::Sptr player2Collider = BoxCollider::Create(glm::vec3(0.6f, 0.6f, 0.6f));
			player2Physics->AddCollider(player2Collider);
			player2Collider->SetPosition(glm::vec3(0.0f, 0.8f, 0.0f));

			// This is an example of attaching a component and setting some parameters
			//RotatingBehaviour::Sptr behaviour = monkey2->Add<RotatingBehaviour>();
			//behaviour->RotationSpeed = glm::vec3(0.0f, 0.0f, -90.0f);
		}

		// Kinematic rigid bodies are those controlled by some outside controller
		// and ONLY collide with dynamic objects
		//RigidBody::Sptr physics = monkey2->Add<RigidBody>(RigidBodyType::Kinematic);
		//physics->AddCollider(ConvexMeshCollider::Create());

		// Create a trigger volume for testing how we can detect collisions with objects!
		GameObject::Sptr leftGoalTrigger = scene->CreateGameObject("Left Goal Trigger"); 
		{
			TriggerVolume::Sptr leftVolume = leftGoalTrigger->Add<TriggerVolume>();
			BoxCollider::Sptr goal1Collider = BoxCollider::Create(glm::vec3(3.0f, 3.0f, 1.0f));
			goal1Collider->SetPosition(glm::vec3(10.9f, 0.0f, 0.5f));
			goal1Collider->SetScale(glm::vec3(0.17f, 0.63f, 1.0f));
			leftVolume->AddCollider(goal1Collider);
		}

		// Create a trigger volume for testing how we can detect collisions with objects!
		GameObject::Sptr rightGoalTrigger = scene->CreateGameObject("Right Goal Trigger");
		{
			TriggerVolume::Sptr rightVolume = rightGoalTrigger->Add<TriggerVolume>();
			BoxCollider::Sptr goal2Collider = BoxCollider::Create(glm::vec3(1.0f, 3.0f, 1.0f));
			goal2Collider->SetPosition(glm::vec3(-10.83f, 0.0f, 0.5f));
			goal2Collider->SetScale(glm::vec3(0.42f, 0.66f, 1.0f));
			rightVolume->AddCollider(goal2Collider);
		}

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");
	}

	// Call scene awake to start up all of our components
	scene->Window = window;
	scene->Awake();

	// We'll use this to allow editing the save/load path
	// via ImGui, note the reserve to allocate extra space
	// for input!
	std::string scenePath = "scene.json"; 
	scenePath.reserve(256); 

	bool isRotating = true;
	float rotateSpeed = 90.0f;

	// Our high-precision timer
	double lastFrame = glfwGetTime();


	BulletDebugMode physicsDebugMode = BulletDebugMode::None;
	float playbackSpeed = 1.0f;

	nlohmann::json editorSceneState;

	///// Game loop /////
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		ImGuiHelper::StartFrame();

		// Calculate the time since our last frame (dt)
		double thisFrame = glfwGetTime();
		float dt = static_cast<float>(thisFrame - lastFrame);

		// Showcasing how to use the imGui library!
		bool isDebugWindowOpen = ImGui::Begin("Debugging");
		if (isDebugWindowOpen) {
			// Draws a button to control whether or not the game is currently playing
			static char buttonLabel[64];
			sprintf_s(buttonLabel, "%s###playmode", scene->IsPlaying ? "Exit Play Mode" : "Enter Play Mode");
			if (ImGui::Button(buttonLabel)) {
				// Save scene so it can be restored when exiting play mode
				if (!scene->IsPlaying) {
					editorSceneState = scene->ToJson();
				}

				// Toggle state
				scene->IsPlaying = !scene->IsPlaying;

				// If we've gone from playing to not playing, restore the state from before we started playing
				if (!scene->IsPlaying) {
					scene = nullptr;
					// We reload to scene from our cached state
					scene = Scene::FromJson(editorSceneState);
					// Don't forget to reset the scene's window and wake all the objects!
					scene->Window = window;
					scene->Awake();
				}
			}

			// Make a new area for the scene saving/loading
			ImGui::Separator();
			if (DrawSaveLoadImGui(scene, scenePath)) {
				// C++ strings keep internal lengths which can get annoying
				// when we edit it's underlying datastore, so recalcualte size
				scenePath.resize(strlen(scenePath.c_str()));

				// We have loaded a new scene, call awake to set
				// up all our components
				scene->Window = window;
				scene->Awake();
			}
			ImGui::Separator();
			// Draw a dropdown to select our physics debug draw mode
			if (BulletDebugDraw::DrawModeGui("Physics Debug Mode:", physicsDebugMode)) {
				scene->SetPhysicsDebugDrawMode(physicsDebugMode);
			}
			LABEL_LEFT(ImGui::SliderFloat, "Playback Speed:    ", &playbackSpeed, 0.0f, 10.0f);
			ImGui::Separator();
		}

		// Clear the color and depth buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Update our application level uniforms every frame

		// Draw some ImGui stuff for the lights
		if (isDebugWindowOpen) {
			for (int ix = 0; ix < scene->Lights.size(); ix++) {
				char buff[256];
				sprintf_s(buff, "Light %d##%d", ix, ix);
				// DrawLightImGui will return true if the light was deleted
				if (DrawLightImGui(scene, buff, ix)) {
					// Remove light from scene, restore all lighting data
					scene->Lights.erase(scene->Lights.begin() + ix);
					scene->SetupShaderAndLights();
					// Move back one element so we don't skip anything!
					ix--;
				}
			}
			// As long as we don't have max lights, draw a button
			// to add another one
			if (scene->Lights.size() < scene->MAX_LIGHTS) {
				if (ImGui::Button("Add Light")) {
					scene->Lights.push_back(Light());
					scene->SetupShaderAndLights();
				}
			}
			// Split lights from the objects in ImGui
			ImGui::Separator();
		}

		dt *= playbackSpeed;

		// Perform updates for all components
		scene->Update(dt);

		// Grab shorthands to the camera and shader from the scene
		Camera::Sptr camera = scene->MainCamera;

		// Cache the camera's viewprojection
		glm::mat4 viewProj = camera->GetViewProjection();
		DebugDrawer::Get().SetViewProjection(viewProj);

		// Update our worlds physics!
		scene->DoPhysics(dt);

		// Draw object GUIs
		if (isDebugWindowOpen) {
			scene->DrawAllGameObjectGUIs();
		}
		
		// The current material that is bound for rendering
		Material::Sptr currentMat = nullptr;
		Shader::Sptr shader = nullptr;

		// Render all our objects
		ComponentManager::Each<RenderComponent>([&](const RenderComponent::Sptr& renderable) {

			// If the material has changed, we need to bind the new shader and set up our material and frame data
			// Note: This is a good reason why we should be sorting the render components in ComponentManager
			if (renderable->GetMaterial() != currentMat) {
				currentMat = renderable->GetMaterial();
				shader = currentMat->MatShader;

				shader->Bind();
				shader->SetUniform("u_CamPos", scene->MainCamera->GetGameObject()->GetPosition());
				currentMat->Apply();
			}

			// Grab the game object so we can do some stuff with it
			GameObject* object = renderable->GetGameObject();

			// Set vertex shader parameters
			shader->SetUniformMatrix("u_ModelViewProjection", viewProj * object->GetTransform());
			shader->SetUniformMatrix("u_Model", object->GetTransform());
			shader->SetUniformMatrix("u_NormalMatrix", glm::mat3(glm::transpose(glm::inverse(object->GetTransform()))));

			// Draw the object
			renderable->GetMesh()->Draw();
		});


		// End our ImGui window
		ImGui::End();

		VertexArrayObject::Unbind();

		lastFrame = thisFrame;
		ImGuiHelper::EndFrame();
		glfwSwapBuffers(window);
	}

	// Clean up the ImGui library
	ImGuiHelper::Cleanup();

	// Clean up the resource manager
	ResourceManager::Cleanup();

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}