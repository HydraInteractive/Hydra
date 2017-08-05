#include <hydra/engine.hpp>

#include <memory>

#include <hydra/world/world.hpp>
#include <hydra/view/sdlview.hpp>
#include <hydra/renderer/glrenderer.hpp>
#include <hydra/renderer/glshader.hpp>
#include <hydra/renderer/uirenderer.hpp>

#include <hydra/component/meshcomponent.hpp>
#include <hydra/component/transformcomponent.hpp>
#include <hydra/component/cameracomponent.hpp>

#include <hydra/world/blueprintloader.hpp>

using namespace Hydra;

Hydra::IEngine* engineInstance;

class Engine final : public Hydra::IEngine {
public:
	Engine() {
		engineInstance = this;

		_world = World::World::create();

		_view = View::SDLView::create();
		_renderer = Renderer::GLRenderer::create(*_view);
		_uiRenderer = Renderer::UIRenderer::create(*_view);
		_textureLoader = std::make_unique<IO::TextureLoader>();

		_vertexShader = Renderer::GLShader::createFromSource(Renderer::PipelineStage::vertex, "assets/shaders/base.vert");
		_geometryShader = Renderer::GLShader::createFromSource(Renderer::PipelineStage::geometry, "assets/shaders/base.geom");
		_fragmentShader = Renderer::GLShader::createFromSource(Renderer::PipelineStage::fragment, "assets/shaders/base.frag");

		_pipeline = Renderer::GLPipeline::create();
		_pipeline->attachStage(*_vertexShader);
		_pipeline->attachStage(*_geometryShader);
		_pipeline->attachStage(*_fragmentShader);

		std::shared_ptr<IEntity> cameraEntity = _world->createEntity("Camera");
		_cc = cameraEntity->addComponent<Component::CameraComponent>();

		std::shared_ptr<IEntity> testEntity = _world->createEntity("TestEntity");
		testEntity->addComponent<Component::MeshComponent>("assets/objects/test.fbx");
		testEntity->addComponent<Component::TransformComponent>(glm::vec3(0), glm::vec3(2));

		BlueprintLoader::save("world.blueprint", "World Blueprint", _world);
	}

	~Engine() final {}

	void run() final {
		while (!_view->isClosed()) {
			_world->tick(World::TickAction::checkDead);
			_renderer->cleanup();

			_view->update(_uiRenderer.get());
			_uiRenderer->newFrame();

			_world->tick(World::TickAction::physics);

			_world->tick(World::TickAction::render);
			_batch.clearColor = glm::vec4(0, 0, 0, 1);
			_batch.renderTarget = _view.get();
			_batch.pipeline = _pipeline.get();

			_geometryShader->setValue(0, _cc->getViewMatrix());
			_geometryShader->setValue(1, _cc->getProjectionMatrix());
			_geometryShader->setValue(2, _cc->getPosition());

			for (DrawObject* drawObj : _renderer->activeDrawObjects())
				_batch.objects[drawObj->mesh].push_back(drawObj->modelMatrix);

			_renderer->render(_batch);

			_uiRenderer->render();
			_view->finalize();

			_world->tick(World::TickAction::network);
		}
	}


	World::IWorld* getWorld() final { return _world.get(); }
	Renderer::IRenderer* getRenderer() final { return _renderer.get(); }
	IO::TextureLoader* getTextureLoader() final { return _textureLoader.get(); }

private:
	std::shared_ptr<World::IWorld> _world;

	std::unique_ptr<View::IView> _view;
	std::unique_ptr<Renderer::IRenderer> _renderer;
	std::unique_ptr<Renderer::IUIRenderer> _uiRenderer;
	std::unique_ptr<IO::TextureLoader> _textureLoader;

	std::unique_ptr<Renderer::IShader> _vertexShader;
	std::unique_ptr<Renderer::IShader> _geometryShader;
	std::unique_ptr<Renderer::IShader> _fragmentShader;

	std::unique_ptr<Renderer::IPipeline> _pipeline;

	Component::CameraComponent* _cc = nullptr;
	Renderer::Batch _batch;
};

int main(int argc, const char** argv) {
	Engine().run();
	return 0;
}
