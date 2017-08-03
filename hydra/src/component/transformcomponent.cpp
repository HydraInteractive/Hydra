#include <hydra/component/transformcomponent.hpp>

#include <hydra/engine.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace Hydra::World;
using namespace Hydra::Component;

extern Hydra::IEngine* engineInstance;

TransformComponent::TransformComponent(IEntity* entity, const glm::vec3& position, const glm::vec3& scale, const glm::quat& rotation) : IComponent(entity), _dirty(true), _position(position), _scale(scale), _rotation(rotation) {}
TransformComponent::~TransformComponent() {}

void TransformComponent::tick(TickAction action) {
	// if (action == TickAction::physics)
}

msgpack::packer<msgpack::sbuffer>& TransformComponent::pack(msgpack::packer<msgpack::sbuffer>& o) const {
	o.pack_map(3);

	o.pack("position");
	o.pack_array(3);
	o.pack_float(_position.x);
	o.pack_float(_position.y);
	o.pack_float(_position.z);

	o.pack("scale");
	o.pack_array(3);
	o.pack_float(_scale.x);
	o.pack_float(_scale.y);
	o.pack_float(_scale.z);

	o.pack("rotation");
	o.pack_array(4);
	o.pack_float(_rotation.x);
	o.pack_float(_rotation.y);
	o.pack_float(_rotation.z);
	o.pack_float(_rotation.z);

	return o;
}

void TransformComponent::setPosition(const glm::vec3& position) {
	_dirty |= _position != position;
	_position = position;
}

void TransformComponent::setScale(const glm::vec3& scale) {
	_dirty |= _scale != scale;
	_scale = scale;
}

void TransformComponent::setRotation(const glm::quat& rotation) {
	_dirty |= _rotation != rotation;
	_rotation = rotation;
}

void TransformComponent::setDirection(const glm::vec3& direction, glm::vec3 up) {
	if (direction == up)
		up.x += 0.0001;

	static const glm::vec3 O = {0, 0, 0};
	glm::mat3 m = glm::lookAt(O, direction, up);
	setRotation(glm::quat_cast(m));
}

void TransformComponent::_recalculateMatrix() {
	auto p = _getParentComponent();
	glm::mat4 parent = p ? p->getMatrix() : glm::mat4(1);
	_dirty = false;

	_matrix = parent * (glm::translate(_position) * glm::mat4_cast(_rotation) * glm::scale(_scale));
}

TransformComponent* TransformComponent::_getParentComponent() {
	return entity->getParent()->getComponent<TransformComponent>();
}
