#pragma once

#include <memory>

#include <hydra/world/world.hpp>
#include <hydra/renderer/renderer.hpp>

using namespace Hydra::World;

// TODO: Implement LOD

namespace Hydra::Component {
	class MeshComponent final : public IComponent {
	public:
		MeshComponent(IEntity* entity, const std::string& meshFile);
		~MeshComponent() final;

		void tick(TickAction action) final;

		inline const std::string type() const final { return "MeshComponent"; }

		msgpack::packer<msgpack::sbuffer>& pack(msgpack::packer<msgpack::sbuffer>& o) const final;
		void registerUI() final;

	private:
		std::string _meshFile;
		Hydra::Renderer::DrawObject* _drawObject;
		std::shared_ptr<Hydra::Renderer::IMesh> _mesh;
	};
};
