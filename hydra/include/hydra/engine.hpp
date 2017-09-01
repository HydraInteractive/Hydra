#pragma once
#include <hydra/ext/api.hpp>

#include <memory>

#include <hydra/renderer/renderer.hpp>
#include <hydra/world/world.hpp>
#include <hydra/io/textureloader.hpp>
#include <hydra/io/meshloader.hpp>

#ifndef PRINTFARGS
#if defined(__clang__) || defined(__GNUC__)
#define PRINTFARGS(FMT) __attribute__((format(printf, FMT, (FMT+1))))
#else
#define PRINTFARGS(FMT)
#endif
#endif

namespace Hydra {
	enum class HYDRA_API LogLevel {
		verbose,
		normal,
		warning,
		error
	};
	HYDRA_API inline const char* toString(LogLevel level) {
		static const char* name[4] = {"verbose", "normal", "warning", "error"};
		return name[static_cast<int>(level)];
	}

	class HYDRA_API IEngine {
	public:
		virtual ~IEngine() = 0;

		virtual void run() = 0;

		virtual World::IWorld* getWorld() = 0;
		virtual Renderer::IRenderer* getRenderer() = 0;
		virtual IO::ITextureLoader* getTextureLoader() = 0;
		virtual IO::IMeshLoader* getMeshLoader() = 0;

		virtual void log(LogLevel level, const char* fmt, ...) PRINTFARGS(3) = 0;

		static IEngine*& getInstance();

	private:
		static IEngine* _instance;
	};
	inline IEngine::~IEngine() {}
}
