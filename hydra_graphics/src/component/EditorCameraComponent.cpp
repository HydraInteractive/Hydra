#include <hydra/component/EditorCameraComponent.hpp>
Hydra::Component::EditorCameraComponent::EditorCameraComponent(IEntity* entity) : IComponent(entity)
{
	SDL_GetKeyboardState(&keysArrayLength);
	lastKeysArray = new bool[keysArrayLength];
	_renderTarget = nullptr;
}
Hydra::Component::EditorCameraComponent::EditorCameraComponent(IEntity* entity, Hydra::Renderer::IRenderTarget* renderTarget, const glm::vec3& position) : IComponent(entity)
{
	_renderTarget = renderTarget;
	_position = position;
	SDL_GetKeyboardState(&keysArrayLength);
	lastKeysArray = new bool[keysArrayLength];
}
Hydra::Component::EditorCameraComponent::~EditorCameraComponent()
{

}

void Hydra::Component::EditorCameraComponent::tick(TickAction action, float delta)
{
	int mouseX, mouseY;
	if (SDL_GetRelativeMouseState(&mouseX, &mouseY) == SDL_BUTTON(3))
	{
		_cameraYaw += mouseX * _sensitivity;
		_cameraPitch += mouseY * _sensitivity;

		if (_cameraPitch > glm::radians(89.0f))
		{
			_cameraPitch = glm::radians(89.0f);
		}
		else if (_cameraPitch < glm::radians(-89.0f))
		{
			_cameraPitch = glm::radians(-89.0f);
		}
	}

	glm::quat qPitch = glm::angleAxis(_cameraPitch, glm::vec3(1, 0, 0));
	glm::quat qYaw = glm::angleAxis(_cameraYaw, glm::vec3(0, 1, 0));
	glm::quat qRoll = glm::angleAxis(glm::radians(180.f), glm::vec3(0, 0, 1));

	_orientation = qPitch * qYaw * qRoll;
	_orientation = glm::normalize(_orientation);

	Uint8* keysArray;
	keysArray = const_cast<Uint8*>(SDL_GetKeyboardState(&keysArrayLength));

	glm::vec3 velocity = glm::vec3(0, 0, 0);
	if (keysArray[SDL_SCANCODE_W]) {
		velocity.z -= _movementSpeed;
	}
	if (keysArray[SDL_SCANCODE_S]) {
		velocity.z += _movementSpeed;
	}
	if (keysArray[SDL_SCANCODE_A]) {
		velocity.x -= _movementSpeed;
	}
	if (keysArray[SDL_SCANCODE_D]) {
		velocity.x += _movementSpeed;
	}
	if (keysArray[SDL_SCANCODE_LSHIFT])
	{
		velocity *= _shiftMultiplier;
	}
	glm::mat4 viewMat = getViewMatrix();
	_position += glm::vec3(glm::vec4(velocity,1.0f) * viewMat) * delta;
	setPosition(_position);
}

void Hydra::Component::EditorCameraComponent::serialize(nlohmann::json & json) const
{
	json = {
		{ "position",{ _position.x, _position.y, _position.z } },
		{ "orientation",{ _orientation.x, _orientation.y, _orientation.z, _orientation.w } },
		{ "fov", _fov },
		{ "zNear", _zNear },
		{ "zFar", _zFar },
		{ "movementSpeed", _movementSpeed },
		{ "shiftMultiplier", _shiftMultiplier }
	};
}

void Hydra::Component::EditorCameraComponent::deserialize(nlohmann::json & json)
{
	auto& pos = json["position"];
	_position = glm::vec3{ pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>() };

	auto& orientation = json["orientation"];
	_orientation = glm::quat{ orientation[0].get<float>(), orientation[1].get<float>(), orientation[2].get<float>(), orientation[3].get<float>() };

	_fov = json["fov"].get<float>();
	_zNear = json["zNear"].get<float>();
	_zFar = json["zFar"].get<float>();
	_movementSpeed = json["movementSpeed"].get<float>();
	_shiftMultiplier = json["shiftMultiplier"].get<float>();
}

void Hydra::Component::EditorCameraComponent::registerUI()
{
	ImGui::DragFloat3("Position", glm::value_ptr(_position), 0.01f);
	ImGui::InputFloat("Movement Speed", &_movementSpeed);
	ImGui::InputFloat("Shift Multiplier", &_shiftMultiplier);
	ImGui::DragFloat4("Orientation", glm::value_ptr(_orientation), 0.01f);
	ImGui::DragFloat("FOV", &_fov);
	ImGui::DragFloat("Z Near", &_zNear, 0.001f);
	ImGui::DragFloat("Z Far", &_zFar);

	float aspect = (_renderTarget->getSize().x*1.0f) / _renderTarget->getSize().y;
	ImGui::InputFloat("Aspect Ratio", &aspect, 0, 0, -1, ImGuiInputTextFlags_ReadOnly);

}

void Hydra::Component::EditorCameraComponent::setPosition(const glm::vec3 & position)
{
	_position = position;
}