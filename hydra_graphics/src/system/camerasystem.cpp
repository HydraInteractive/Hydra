#include <hydra/system/camerasystem.hpp>

#include <hydra/ext/openmp.hpp>

#include <hydra/component/cameracomponent.hpp>
#include <hydra/component/freecameracomponent.hpp>
#include <algorithm>

using namespace Hydra::System;

#define ANG2RAD 3.14159265358979323846/180.0

CameraSystem::CameraSystem() {}
CameraSystem::~CameraSystem() {}

void CameraSystem::tick(float delta) {
	using world = Hydra::World::World;

	// Collect data
	int mouseX = 0;
	int mouseY = 0;
	glm::vec3 velocity = glm::vec3{0, 0, 0};
	bool multiplier = false;

	if ((SDL_GetRelativeMouseState(&mouseX, &mouseY) & SDL_BUTTON(3)) == 0)
		mouseX = mouseY = 0;


	{
		const Uint8* keysArray = SDL_GetKeyboardState(nullptr);

		if (keysArray[SDL_SCANCODE_W])
			velocity.z -= 1;
		if (keysArray[SDL_SCANCODE_S])
			velocity.z += 1;
		if (keysArray[SDL_SCANCODE_A])
			velocity.x -= 1;
		if (keysArray[SDL_SCANCODE_D])
			velocity.x += 1;

		multiplier = keysArray[SDL_SCANCODE_LSHIFT];//velocity *= ;
	}

	//Process CameraComponent
	world::getEntitiesWithComponents<Hydra::Component::CameraComponent>(entities);
	for (int_openmp_t i = 0; i < (int_openmp_t)entities.size(); i++) {
		auto cc = entities[i]->getComponent<Hydra::Component::CameraComponent>();

		if (cc->mouseControl) {
			cc->cameraYaw = cc->cameraYaw + mouseX * cc->sensitivity; // std::min(std::max(cc->cameraYaw + mouseX * cc->sensitivity, glm::radians(-89.9999f)), glm::radians(89.9999f));
			cc->cameraPitch = std::min(std::max(cc->cameraPitch + mouseY * cc->sensitivity, glm::radians(-89.9999f)), glm::radians(89.9999f));
		}

		glm::quat qPitch = glm::angleAxis(cc->cameraPitch, glm::vec3(1, 0, 0));
		glm::quat qYaw = glm::angleAxis(cc->cameraYaw, glm::vec3(0, 1, 0));
		glm::quat qRoll = glm::angleAxis(glm::radians(0.f), glm::vec3(0, 0, 1));

		auto t = entities[i]->getComponent<Hydra::Component::TransformComponent>();
		t->rotation = glm::normalize(qPitch * qYaw * qRoll);
	}

	//Process FreeCameraComponent
	world::getEntitiesWithComponents<Hydra::Component::FreeCameraComponent>(entities);
	for (int_openmp_t i = 0; i < (int_openmp_t)entities.size(); i++) {
		auto fc = entities[i]->getComponent<Hydra::Component::FreeCameraComponent>();

		fc->cameraYaw = fc->cameraYaw + mouseX * fc->sensitivity; //std::min(std::max(fc->cameraYaw + mouseX * fc->sensitivity, glm::radians(-89.9999f)), glm::radians(89.9999f));
		fc->cameraPitch = std::min(std::max(fc->cameraPitch + mouseY * fc->sensitivity, glm::radians(-89.9999f)), glm::radians(89.9999f));

		glm::quat qPitch = glm::angleAxis(fc->cameraPitch, glm::vec3(1, 0, 0));
		glm::quat qYaw = glm::angleAxis(fc->cameraYaw, glm::vec3(0, 1, 0));
		glm::quat qRoll = glm::angleAxis(glm::radians(0.f), glm::vec3(0, 0, 1));

		auto t = entities[i]->getComponent<Hydra::Component::TransformComponent>();
		t->rotation = glm::normalize(qPitch * qYaw * qRoll);

		const glm::mat4 viewMat = fc->getViewMatrix();
		t->position += glm::vec3(glm::vec4(velocity * fc->movementSpeed * (multiplier ? fc->shiftMultiplier : 1), 1.0f) * viewMat) * delta;
	}

	entities.clear();
}

void CameraSystem::setCamInternals(Hydra::Component::CameraComponent& cc) {
	cc.tang = (float)tan(ANG2RAD * cc.fov * 0.5);
	cc.nh = cc.zNear * cc.tang;
	cc.nw = cc.nh * cc.aspect;
	cc.fh = cc.zFar * cc.tang;
	cc.fw = cc.fh * cc.aspect;
}

void CameraSystem::setCamDef(const glm::vec3& cPos, const glm::vec3& cDir, const glm::vec3& up, const glm::vec3& right, Hydra::Component::CameraComponent& cc) {
	glm::vec3 dir, nc, fc, X, Y, Z;

	Z = cPos - cDir;
	Z = glm::normalize(Z);

	X = up * Z;
	X = glm::normalize(X);

	Y = Z * X;

	nc = cPos - cDir * cc.zNear;
	fc = cPos - cDir * cc.zFar;

	//near plane
	cc.ntl = nc + up * cc.nh - right * cc.nw;
	cc.ntr = nc + up * cc.nh + right * cc.nw;
	cc.nbl = nc - up * cc.nh - right * cc.nw;
	cc.nbr = nc - up * cc.nh + right * cc.nw;

	//far plane
	cc.ftl = fc + up * cc.fh - right * cc.fw;
	cc.ftr = fc + up * cc.fh + right * cc.fw;
	cc.fbl = fc - up * cc.fh - right * cc.fw;
	cc.fbr = fc - up * cc.fh + right * cc.fw;

	cc.pl[cc.TOP].set3Points(cc.ntr, cc.ntl, cc.ftl);
	cc.pl[cc.BOTTOM].set3Points(cc.nbl, cc.nbr, cc.fbr);
	cc.pl[cc.LEFT].set3Points(cc.ntl, cc.nbl, cc.fbl);
	cc.pl[cc.RIGHT].set3Points(cc.nbr, cc.ntr, cc.fbr);
	cc.pl[cc.NEARP].set3Points(cc.ntl, cc.ntr, cc.nbr);
	cc.pl[cc.FARP].set3Points(cc.ftr, cc.ftl, cc.fbl);

}

CameraSystem::FrustrumCheck CameraSystem::sphereInFrustum(const glm::vec3& p, float radius, Hydra::Component::CameraComponent& cc) {
	float distance = 0;
	CameraSystem::FrustrumCheck result = CameraSystem::FrustrumCheck::inside;
	for (int i = 0; i < 6; i++) {
		distance = cc.pl[i].distance(p);
		if (distance < -radius)
			return CameraSystem::FrustrumCheck::outside;
		else if (distance < radius)
			result = CameraSystem::FrustrumCheck::intersect;
	}
	return result;
}


void CameraSystem::registerUI() {}
