/**
 * Mainmenu state
 *
 * License: Mozilla Public License Version 2.0 (https://www.mozilla.org/en-US/MPL/2.0/ OR See accompanying file LICENSE)
 * Authors:
 *  - Dan Printzell
 */
#pragma once

#include <hydra/engine.hpp>
#include <hydra/world/world.hpp>
#include <hydra/renderer/renderer.hpp>
#include <hydra/renderer/uirenderer.hpp>
#include <hydra/io/meshloader.hpp>
#include <hydra/io/textureloader.hpp>
#include <hydra/system/bulletphysicssystem.hpp>

namespace Barcode {
	class MenuState final : public Hydra::IState {
	public:
		MenuState();
		~MenuState() final;

		void onMainMenu() final;
		void load() final;

		void runFrame(float delta) final;

		inline Hydra::IO::ITextureLoader* getTextureLoader() final { return _textureLoader.get(); }
		inline Hydra::IO::IMeshLoader* getMeshLoader() final { return _meshLoader.get(); }
		inline Hydra::World::ISystem* getPhysicsSystem() final { return &_physicsSystem; }

	private:
		struct RenderBatch final {
			std::unique_ptr<Hydra::Renderer::IShader> vertexShader;
			std::unique_ptr<Hydra::Renderer::IShader> geometryShader;
			std::unique_ptr<Hydra::Renderer::IShader> fragmentShader;
			std::unique_ptr<Hydra::Renderer::IPipeline> pipeline;

			std::shared_ptr<Hydra::Renderer::IFramebuffer> output;
			Hydra::Renderer::Batch batch;
		};

		enum class Menu : uint8_t {
			none = 0,
			play,
			create,
			options
		};

		union SubMenu {
			uint8_t data;
			enum class Play : uint8_t {
				none = 0,
				coop,
				solo
			} play;
			enum class Create : uint8_t {
				none = 0,
				createRoom,
				view
			} create;
			enum class Options : uint8_t {
				none = 0,
				visual,
				gameplay,
				sound
			} options;

			SubMenu(uint8_t d) : data(d) {}
			SubMenu(Play p) : play(p) {}
			SubMenu(Create c) : create(c) {}
			SubMenu(Options o) : options(o) {}
		};

		Hydra::IEngine* _engine;
		std::unique_ptr<Hydra::IO::ITextureLoader> _textureLoader;
		std::unique_ptr<Hydra::IO::IMeshLoader> _meshLoader;
		Hydra::System::BulletPhysicsSystem _physicsSystem;

		RenderBatch _viewBatch;

		Menu _menu = Menu::none;
		SubMenu _submenu = SubMenu{0};

		void _initSystem();
		void _initWorld();

		void _playMenu();
		void _createMenu();
		void _optionsMenu();

		// TODO: FIX!
		int oneTenthX;
		int oneThirdX;
		int oneEightY;
		int oneThirdY;
		int oneHalfY;
		int twoFiftsY;
		int twoThirdY;
	};
	//bool ImageAnimButton(ImTextureID user_texture_id, ImTextureID user_texture_id2, const ImVec2 & size, const ImVec2 & uv0, const ImVec2 & uv1, const ImVec2 & uv2, const ImVec2 & uv3, int frame_padding, const ImVec4 & bg_col, const ImVec4 & tint_col);
}
