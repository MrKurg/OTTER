#include "Player2MovementBehaviour.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"


void Player2MovementBehaviour::Awake()
{
	_body = GetComponent<Gameplay::Physics::RigidBody>();
	if (_body == nullptr) {
		IsEnabled = false;
	}
}

void Player2MovementBehaviour::RenderImGui() {
	LABEL_LEFT(ImGui::DragFloat, "Move", &_move, 1.0f);
}

nlohmann::json Player2MovementBehaviour::ToJson() const {
	return {
		{ "move", _move }
	};
}

Player2MovementBehaviour::Player2MovementBehaviour() :
	IComponent(),
	_move(10.0f)
{ }

Player2MovementBehaviour::~Player2MovementBehaviour() = default;

Player2MovementBehaviour::Sptr Player2MovementBehaviour::FromJson(const nlohmann::json & blob) {
	Player2MovementBehaviour::Sptr result = std::make_shared<Player2MovementBehaviour>();
	result->_move = blob["move"];
	return result;
}

void Player2MovementBehaviour::Update(float deltaTime) {
	//bool pressed = glfwGetKey(GetGameObject()->GetScene()->Window, GLFW_KEY_SPACE);
	_body->SetAngularFactor(glm::vec3(0.0f, 0.0f, 1.0f));

	if (glfwGetKey(GetGameObject()->GetScene()->Window, GLFW_KEY_RIGHT) == GLFW_PRESS)
	{
		_body->SetLinearVelocity(glm::vec3(-9.0f, 0.0f, 0.0f));
	}
	else if (glfwGetKey(GetGameObject()->GetScene()->Window, GLFW_KEY_LEFT) == GLFW_PRESS)
	{
		_body->SetLinearVelocity(glm::vec3(9.0f, 0.0f, 0.0f));
	}
	else if (glfwGetKey(GetGameObject()->GetScene()->Window, GLFW_KEY_UP) == GLFW_PRESS)
	{
		_body->SetLinearVelocity(glm::vec3(0.0f, -9.0f, 0.0f));
	}
	else if (glfwGetKey(GetGameObject()->GetScene()->Window, GLFW_KEY_DOWN) == GLFW_PRESS)
	{
		_body->SetLinearVelocity(glm::vec3(0.0f, 9.0f, 0.0f));
	}
}