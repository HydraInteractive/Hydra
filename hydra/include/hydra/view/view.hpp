#pragma once

#include <cstdint>

#include <glm/glm.hpp>
#include <hydra/renderer/renderer.hpp>

namespace Hydra::View {
	class IView : public Hydra::Renderer::IRenderTarget {
	public:
		virtual ~IView() = 0;

		/// Run the event loop stuff
		virtual void update() = 0;

		virtual void show() = 0;
		virtual void hide() = 0;

		virtual glm::ivec2 getSize() = 0;
		virtual void* getHandler() = 0;

		/// Did it get a close event, and hid itself
		virtual bool isClosed() = 0;
		virtual bool didChangeSize() = 0;
	};
	inline IView::~IView() {}
}
