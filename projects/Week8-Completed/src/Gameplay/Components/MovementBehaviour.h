#pragma once
#include "IComponent.h"
#include "Gameplay/Physics/RigidBody.h"

class MovementBehaviour : public Gameplay::IComponent {
public:
	typedef std::shared_ptr<MovementBehaviour> Sptr;

	MovementBehaviour();
	virtual ~MovementBehaviour();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;

public:
	virtual void RenderImGui() override;
	MAKE_TYPENAME(MovementBehaviour);
	virtual nlohmann::json ToJson() const override;
	static MovementBehaviour::Sptr FromJson(const nlohmann::json& blob);

protected:
	float _move;

	bool _isPressed = false;
	Gameplay::Physics::RigidBody::Sptr _body;
};

