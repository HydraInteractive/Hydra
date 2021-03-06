/**
 * A wrapper around Dear ImGui to be able to create debug UIs.
 *
 * License: Mozilla Public License Version 2.0 (https://www.mozilla.org/en-US/MPL/2.0/ OR See accompanying file LICENSE)
 * Authors:
 *  - Dan Printzell
 */
#pragma once
#include <hydra/ext/api.hpp>

#include <hydra/view/view.hpp>
#include <memory>
#include <cstdarg>

#include <glm/gtc/type_ptr.hpp>

// TODO: Abstact away?
union SDL_Event;

#ifndef PRINTFARGS
#if defined(__clang__) || defined(__GNUC__)
#define PRINTFARGS(FMT) __attribute__((format(printf, FMT, (FMT+1))))
#else
#define PRINTFARGS(FMT)
#endif
#endif

namespace Hydra { enum class HYDRA_BASE_API LogLevel; }
namespace Hydra::World { class ISystem; }

namespace Hydra::Renderer {
	class HYDRA_BASE_API IUILog;
	class HYDRA_BASE_API ITexture;

	enum class HYDRA_BASE_API UIFont {
		normal,
		normalBold,
		medium,
		big,
		monospace
	};

	class HYDRA_BASE_API IUIRenderer {
	public:
		virtual ~IUIRenderer() = 0;

		virtual void handleEvent(SDL_Event& event) = 0;

		virtual void newFrame() = 0;

		virtual void reset() = 0;
		virtual void render(float delta) = 0; // TODO: Move to IRenderer(?)

		virtual void registerSystems(const std::vector<Hydra::World::ISystem*>& systems) = 0;

		virtual void pushFont(UIFont font) = 0;
		virtual void popFont() = 0;

		virtual IUILog* getLog() = 0;

		virtual bool usingKeyboard() = 0;
		virtual bool isDraging() = 0;
	};
	inline IUIRenderer::~IUIRenderer() {}

	class HYDRA_BASE_API IUILog {
	public:
		virtual ~IUILog() = 0;

		void log(Hydra::LogLevel level, const char* fmt, ...) PRINTFARGS(3) {
			va_list va;
			va_start(va, fmt);
			log(level, fmt, va);
			va_end(va);
		}

		virtual void log(Hydra::LogLevel level, const char* fmt, va_list va) = 0;

		virtual void clear() = 0;

		virtual void render(bool* pOpen) = 0;
	};
	inline IUILog::~IUILog() {}

	namespace UIRenderer {
		HYDRA_BASE_API std::unique_ptr<IUIRenderer> create(Hydra::View::IView& view);
	};
};
