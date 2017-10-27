#include <barcode/gamestate.hpp>

#include <hydra/renderer/glrenderer.hpp>
#include <hydra/renderer/glshader.hpp>
#include <hydra/io/gltextureloader.hpp>
#include <hydra/io/glmeshloader.hpp>

#include <hydra/world/blueprintloader.hpp>
#include <imgui/imgui.h>
#include <hydra/component/aicomponent.hpp>
#include <hydra/component/rigidbodycomponent.hpp>
#include <hydra/component/lightcomponent.hpp>
#include <hydra/component/pointlightcomponent.hpp>

using world = Hydra::World::World;

namespace Barcode {
	GameState::GameState() : _engine(Hydra::IEngine::getInstance()) {}

	void GameState::load() {
		_textureLoader = Hydra::IO::GLTextureLoader::create();
		_meshLoader = Hydra::IO::GLMeshLoader::create(_engine->getRenderer());

		auto windowSize = _engine->getView()->getSize();
		{
			auto& batch = _geometryBatch;
			batch.vertexShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::vertex, "assets/shaders/geometry.vert");
			batch.geometryShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::geometry, "assets/shaders/geometry.geom");
			batch.fragmentShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::fragment, "assets/shaders/geometry.frag");

			batch.pipeline = Hydra::Renderer::GLPipeline::create();
			batch.pipeline->attachStage(*batch.vertexShader);
			batch.pipeline->attachStage(*batch.geometryShader);
			batch.pipeline->attachStage(*batch.fragmentShader);
			batch.pipeline->finalize();

			batch.output = Hydra::Renderer::GLFramebuffer::create(windowSize, 0);
			batch.output
				->addTexture(0, Hydra::Renderer::TextureType::f16RGB) // Position
				.addTexture(1, Hydra::Renderer::TextureType::u8RGBA) // Diffuse
				.addTexture(2, Hydra::Renderer::TextureType::f16RGB) // Normal
				.addTexture(3, Hydra::Renderer::TextureType::f16RGBA) // Light pos
				.addTexture(4, Hydra::Renderer::TextureType::u8RGB) // Position in view-space
				.addTexture(5, Hydra::Renderer::TextureType::u8R) // Glow.
				.addTexture(6, Hydra::Renderer::TextureType::f16Depth) // real depth
				.finalize();

			batch.batch.clearColor = glm::vec4(0, 0, 0, 1);
			batch.batch.clearFlags = Hydra::Renderer::ClearFlags::color | Hydra::Renderer::ClearFlags::depth;
			batch.batch.renderTarget = batch.output.get();
			batch.batch.pipeline = batch.pipeline.get();
		}

		{
			auto& batch = _animationBatch;
			batch.vertexShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::vertex, "assets/shaders/animationGeometry.vert");
			batch.geometryShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::geometry, "assets/shaders/animationGeometry.geom");
			batch.fragmentShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::fragment, "assets/shaders/animationGeometry.frag");

			batch.pipeline = Hydra::Renderer::GLPipeline::create();
			batch.pipeline->attachStage(*batch.vertexShader);
			batch.pipeline->attachStage(*batch.geometryShader);
			batch.pipeline->attachStage(*batch.fragmentShader);
			batch.pipeline->finalize();

			batch.batch.clearColor = glm::vec4(0, 0, 0, 1);
			batch.batch.clearFlags = ClearFlags::none;
			batch.batch.renderTarget = _geometryBatch.output.get();
			batch.batch.pipeline = batch.pipeline.get();
		}

		{ // Lighting pass batch
			auto& batch = _lightingBatch;
			batch.vertexShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::vertex, "assets/shaders/lighting.vert");
			batch.fragmentShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::fragment, "assets/shaders/lighting.frag");

			batch.pipeline = Hydra::Renderer::GLPipeline::create();
			batch.pipeline->attachStage(*batch.vertexShader);
			batch.pipeline->attachStage(*batch.fragmentShader);
			batch.pipeline->finalize();

			batch.output = Hydra::Renderer::GLFramebuffer::create(windowSize, 0);
			batch.output
				->addTexture(0, Hydra::Renderer::TextureType::u8RGB)
				.addTexture(1, Hydra::Renderer::TextureType::u8RGB)
				.finalize();

			batch.batch.clearColor = glm::vec4(0, 0, 0, 1);
			batch.batch.clearFlags = Hydra::Renderer::ClearFlags::color | Hydra::Renderer::ClearFlags::depth;
			batch.batch.renderTarget = batch.output.get();
			batch.batch.pipeline = batch.pipeline.get(); // TODO: Change to "null" pipeline
		}


		{
			auto& batch = _glowBatch;
			batch.vertexShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::vertex, "assets/shaders/blur.vert");
			batch.fragmentShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::fragment, "assets/shaders/blur.frag");

			_glowVertexShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::vertex, "assets/shaders/glow.vert");
			_glowFragmentShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::fragment, "assets/shaders/glow.frag");

			batch.pipeline = Hydra::Renderer::GLPipeline::create();
			batch.pipeline->attachStage(*batch.vertexShader);
			batch.pipeline->attachStage(*batch.fragmentShader);
			batch.pipeline->finalize();

			_glowPipeline = Hydra::Renderer::GLPipeline::create();
			_glowPipeline->attachStage(*_glowVertexShader);
			_glowPipeline->attachStage(*_glowFragmentShader);
			_glowPipeline->finalize();

			batch.output = Hydra::Renderer::GLFramebuffer::create(windowSize, 0);
			batch.output
				->addTexture(0, Hydra::Renderer::TextureType::u8RGB)
				.finalize();

			// Extra buffer for ping-ponging the texture for two-pass gaussian blur.
			_blurrExtraFBO1 = Hydra::Renderer::GLFramebuffer::create(windowSize, 0);
			_blurrExtraFBO1
				->addTexture(0, Hydra::Renderer::TextureType::u8RGB)
				.finalize();
			_blurrExtraFBO2 = Hydra::Renderer::GLFramebuffer::create(windowSize, 0);
			_blurrExtraFBO2
				->addTexture(0, Hydra::Renderer::TextureType::u8RGB)
				.finalize();

			_fiveGaussianKernel1 = { 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };
			_fiveGaussianKernel2 = { 0.102637f, 0.238998f, 0.31673f, 0.238998f, 0.102637f };

			// 3 Blurred Textures and one original.
			_blurredOriginal = Hydra::Renderer::GLTexture::createEmpty(windowSize.x, windowSize.y, TextureType::u8RGB);
			_blurredIMG1 = Hydra::Renderer::GLTexture::createEmpty(windowSize.x, windowSize.y, TextureType::u8RGB);
			_blurredIMG2 = Hydra::Renderer::GLTexture::createEmpty(windowSize.x, windowSize.y, TextureType::u8RGB);
			_blurredIMG3 = Hydra::Renderer::GLTexture::createEmpty(windowSize.x, windowSize.y, TextureType::u8RGB);

			batch.batch.clearColor = glm::vec4(0, 0, 0, 1);
			batch.batch.clearFlags = ClearFlags::color | ClearFlags::depth;
			batch.batch.renderTarget = batch.output.get();
			batch.batch.pipeline = batch.pipeline.get();
		}

		{ // PARTICLES
			auto& batch = _particleBatch;
			batch.vertexShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::vertex, "assets/shaders/particles.vert");
			batch.fragmentShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::fragment, "assets/shaders/particles.frag");

			batch.pipeline = Hydra::Renderer::GLPipeline::create();
			batch.pipeline->attachStage(*batch.vertexShader);
			batch.pipeline->attachStage(*batch.fragmentShader);
			batch.pipeline->finalize();

			_particleAtlases = Hydra::Renderer::GLTexture::createFromFile("assets/textures/TempAtlas.png");

			batch.batch.clearColor = glm::vec4(0, 0, 0, 1);
			batch.batch.clearFlags = ClearFlags::depth;
			batch.batch.renderTarget = _engine->getView();
			batch.batch.pipeline = batch.pipeline.get();
		}

		{ // Shadow pass
			auto& batch = _shadowBatch;
			batch.vertexShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::vertex, "assets/shaders/shadow.vert");
			batch.fragmentShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::fragment, "assets/shaders/shadow.frag");

			batch.pipeline = Hydra::Renderer::GLPipeline::create();
			batch.pipeline->attachStage(*batch.vertexShader);
			batch.pipeline->attachStage(*batch.fragmentShader);
			batch.pipeline->finalize();

			batch.output = Hydra::Renderer::GLFramebuffer::create(glm::vec2(1024), 0);
			batch.output->addTexture(0, Hydra::Renderer::TextureType::f16Depth).finalize();

			batch.batch.clearColor = glm::vec4(0, 0, 0, 1);
			batch.batch.clearFlags = Hydra::Renderer::ClearFlags::depth;
			batch.batch.renderTarget = batch.output.get();
			batch.batch.pipeline = batch.pipeline.get();
		}

		{ // SSAO
			auto& batch = _ssaoBatch;
			batch.vertexShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::vertex, "assets/shaders/ssao.vert");
			batch.fragmentShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::fragment, "assets/shaders/ssao.frag");

			batch.pipeline = Hydra::Renderer::GLPipeline::create();
			batch.pipeline->attachStage(*batch.vertexShader);
			batch.pipeline->attachStage(*batch.fragmentShader);
			batch.pipeline->finalize();

			batch.output = Hydra::Renderer::GLFramebuffer::create(windowSize / 4, 0);
			batch.output->addTexture(0, Hydra::Renderer::TextureType::f16R).finalize();


			batch.batch.clearColor = glm::vec4(0, 0, 0, 1);
			batch.batch.clearFlags = Hydra::Renderer::ClearFlags::color | Hydra::Renderer::ClearFlags::depth;
			batch.batch.renderTarget = batch.output.get();
			batch.batch.pipeline = batch.pipeline.get();

			std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
			std::default_random_engine generator;
			size_t kernelSize = 8;
			std::vector<glm::vec3> ssaoKernel;
			for (size_t i = 0; i < kernelSize; i++) {
				glm::vec3 sample(
					randomFloats(generator) * 2.0 - 1.0,
					randomFloats(generator) * 2.0 - 1.0,
					randomFloats(generator)
				);
				sample = glm::normalize(sample);
				sample *= randomFloats(generator);
				float scale = (float)i / kernelSize;
				scale = 0.1 + (scale * scale) * (1.0 - 0.1);
				sample *= scale;
				ssaoKernel.push_back(sample);
			}

			std::vector<glm::vec3> ssaoNoise;
			for (unsigned int i = 0; i < 16; i++) {
				glm::vec3 noise(
					randomFloats(generator) * 2.0 - 1.0,
					randomFloats(generator) * 2.0 - 1.0,
					0.0f);
				ssaoNoise.push_back(noise);
			}

			_ssaoNoise = Hydra::Renderer::GLTexture::createFromData(4, 4, TextureType::f32RGB, ssaoNoise.data());

			for (size_t i = 0; i < kernelSize; i++)
				_ssaoBatch.pipeline->setValue(4 + i, ssaoKernel[i]);
		}

		{
			auto& batch = _viewBatch;
			batch.vertexShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::vertex, "assets/shaders/view.vert");
			batch.fragmentShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::fragment, "assets/shaders/view.frag");

			batch.pipeline = Hydra::Renderer::GLPipeline::create();
			batch.pipeline->attachStage(*batch.vertexShader);
			batch.pipeline->attachStage(*batch.fragmentShader);
			batch.pipeline->finalize();

			batch.batch.clearColor = glm::vec4(0, 0.0, 0.0, 1);
			batch.batch.clearFlags = Hydra::Renderer::ClearFlags::color | Hydra::Renderer::ClearFlags::depth;
			batch.batch.renderTarget = _engine->getView();
			batch.batch.pipeline = batch.pipeline.get(); // TODO: Change to "null" pipeline
		}

		_initWorld();
	}

	GameState::~GameState() { }

	void GameState::onMainMenu() { }

	void GameState::runFrame(float delta) {
		auto windowSize = _engine->getView()->getSize();

		_physicsSystem.tick(delta);
		_cameraSystem.tick(delta);
		_aiSystem.tick(delta);
		_bulletSystem.tick(delta);
		_playerSystem.tick(delta);
		_abilitySystem.tick(delta);
		_lightSystem.tick(delta);
		_particleSystem.tick(delta);
		_rendererSystem.tick(delta);

		const glm::vec3 cameraPos = _cc->position;

		{ // Render objects (Deferred rendering)
		  //_world->tick(TickAction::render, delta);

		  // Render to geometryFBO

		  // FIXME: Fix this shit code
			for (auto& light : Hydra::Component::LightComponent::componentHandler->getActiveComponents())
				_light = static_cast<Hydra::Component::LightComponent*>(light.get());

			auto lightViewMX = _light->getViewMatrix();
			auto lightPMX = _light->getProjectionMatrix();
			glm::mat4 biasMatrix(
				0.5, 0.0, 0.0, 0.0,
				0.0, 0.5, 0.0, 0.0,
				0.0, 0.0, 0.5, 0.0,
				0.5, 0.5, 0.5, 1.0
			);
			glm::mat4 lightS = biasMatrix * lightPMX * lightViewMX;

			_geometryBatch.pipeline->setValue(0, _cc->getViewMatrix());
			_geometryBatch.pipeline->setValue(1, _cc->getProjectionMatrix());
			_geometryBatch.pipeline->setValue(2, cameraPos);
			_geometryBatch.pipeline->setValue(4, lightS);

			_animationBatch.pipeline->setValue(0, _cc->getViewMatrix());
			_animationBatch.pipeline->setValue(1, _cc->getProjectionMatrix());
			_animationBatch.pipeline->setValue(2, cameraPos);
			_animationBatch.pipeline->setValue(4, lightS);

			for (auto& kv : _geometryBatch.batch.objects)
				kv.second.clear();

			for (auto& kv : _animationBatch.batch.objects)
				kv.second.clear();

			for (auto& drawObj : _engine->getRenderer()->activeDrawObjects()) {
				if (!drawObj->disable && drawObj->mesh && drawObj->mesh->hasAnimation() == false)
					_geometryBatch.batch.objects[drawObj->mesh].push_back(drawObj->modelMatrix);

				else if (!drawObj->disable && drawObj->mesh && drawObj->mesh->hasAnimation() == true) {
					_animationBatch.batch.objects[drawObj->mesh].push_back(drawObj->modelMatrix);

					//int currentFrame = drawObj->mesh->getCurrentKeyframe();

					//if (currentFrame < drawObj->mesh->getMaxFramesForAnimation()) {
					//	drawObj->mesh->setCurrentKeyframe(currentFrame + 1);
					//}
					//else {
					//	drawObj->mesh->setCurrentKeyframe(1);
					//}

					//glm::mat4 tempMat;
					//for (int i = 0; i < drawObj->mesh->getNrOfJoints(); i++) {
					//	tempMat = drawObj->mesh->getTransformationMatrices(i);
					//	_animationBatch.pipeline->setValue(11 + i, tempMat);
					//}
				}
			}

			// Sort Front to back
			for (auto& kv : _geometryBatch.batch.objects) {
				std::vector<glm::mat4>& list = kv.second;

				std::sort(list.begin(), list.end(), [cameraPos](const glm::mat4& a, const glm::mat4& b) {
					return glm::distance(glm::vec3(a[3]), cameraPos) < glm::distance(glm::vec3(b[3]), cameraPos);
				});
			}
			// Sort Front to back for animation
			for (auto& kv : _animationBatch.batch.objects) {
				std::vector<glm::mat4>& list = kv.second;

				std::sort(list.begin(), list.end(), [cameraPos](const glm::mat4& a, const glm::mat4& b) {
					return glm::distance(glm::vec3(a[3]), cameraPos) < glm::distance(glm::vec3(b[3]), cameraPos);
				});
			}

			_engine->getRenderer()->render(_geometryBatch.batch);
			_engine->getRenderer()->renderAnimation(_animationBatch.batch);
			//_engine->getRenderer()->render(_shadowBatch.batch);
			//_engine->getRenderer()->renderAnimation(_animationBatch.batch);
		}

		{
			for (auto& kv : _shadowBatch.batch.objects)
				kv.second.clear();

			for (auto& drawObj : _engine->getRenderer()->activeDrawObjects())
				if (!drawObj->disable && drawObj->mesh)
					_shadowBatch.batch.objects[drawObj->mesh].push_back(drawObj->modelMatrix);

			_shadowBatch.pipeline->setValue(0, _light->getViewMatrix());
			_shadowBatch.pipeline->setValue(1, _light->getProjectionMatrix());

			//_engine->getRenderer()->render(_shadowBatch.batch);
			_engine->getRenderer()->renderShadows(_shadowBatch.batch);
		}

		static bool enableSSAO = true;
		ImGui::Checkbox("Enable SSAO", &enableSSAO);
		static bool enableBlur = true;
		ImGui::Checkbox("Enable blur", &enableBlur);

		{

			_ssaoBatch.pipeline->setValue(0, 0);
			_ssaoBatch.pipeline->setValue(1, 1);
			_ssaoBatch.pipeline->setValue(2, 2);


			_ssaoBatch.pipeline->setValue(3, _cc->getProjectionMatrix());

			(*_geometryBatch.output)[4]->bind(0);
			(*_geometryBatch.output)[2]->bind(1);
			_ssaoNoise->bind(2);

			_engine->getRenderer()->postProcessing(_ssaoBatch.batch);
			int nrOfTImes = 1;
			_blurGlowTexture((*_ssaoBatch.output)[0], nrOfTImes, (*_ssaoBatch.output)[0]->getSize(), _fiveGaussianKernel1, enableBlur)
				->resolve(0, (*_ssaoBatch.output)[0]);
		}

		{ // Lighting pass
			_lightingBatch.pipeline->setValue(0, 0);
			_lightingBatch.pipeline->setValue(1, 1);
			_lightingBatch.pipeline->setValue(2, 2);
			_lightingBatch.pipeline->setValue(3, 3);
			_lightingBatch.pipeline->setValue(4, 4);
			_lightingBatch.pipeline->setValue(5, 5);
			_lightingBatch.pipeline->setValue(6, 6);

			_lightingBatch.pipeline->setValue(7, _cc->position);
			_lightingBatch.pipeline->setValue(8, enableSSAO);
			auto& lights = Hydra::Component::PointLightComponent::componentHandler->getActiveComponents();

			_lightingBatch.pipeline->setValue(9, (int)(lights.size()));
			_lightingBatch.pipeline->setValue(10, _light->direction);
			_lightingBatch.pipeline->setValue(11, _light->color);

			// good code lmao XD
			int i = 12;
			for (auto& p : lights) {
				auto pc = static_cast<Hydra::Component::PointLightComponent*>(p.get());
				_lightingBatch.pipeline->setValue(i++, pc->position);
				_lightingBatch.pipeline->setValue(i++, pc->color);
				_lightingBatch.pipeline->setValue(i++, pc->constant);
				_lightingBatch.pipeline->setValue(i++, pc->linear);
				_lightingBatch.pipeline->setValue(i++, pc->quadratic);
			}

			(*_geometryBatch.output)[0]->bind(0);
			(*_geometryBatch.output)[1]->bind(1);
			(*_geometryBatch.output)[2]->bind(2);
			(*_geometryBatch.output)[3]->bind(3);
			_shadowBatch.output->getDepth()->bind(4);
			(*_ssaoBatch.output)[0]->bind(5);
			(*_geometryBatch.output)[5]->bind(6);

			_engine->getRenderer()->postProcessing(_lightingBatch.batch);
		}


		{ // Glow
			if (enableBlur) {
				int nrOfTimes;
				nrOfTimes = 1;

				glm::vec2 size = windowSize;

				_lightingBatch.output->resolve(0, _blurredOriginal);
				_lightingBatch.output->resolve(1, (*_glowBatch.output)[0]);

				_blurGlowTexture((*_glowBatch.output)[0], nrOfTimes + 1, size * 0.5f, _fiveGaussianKernel2, enableBlur)->resolve(0, _blurredIMG1);

				_glowBatch.batch.pipeline = _glowPipeline.get();

				_glowBatch.batch.pipeline->setValue(1, 1);
				_glowBatch.batch.pipeline->setValue(2, 2);
				_glowBatch.batch.pipeline->setValue(3, enableBlur);

				_blurredOriginal->bind(1);
				_blurredIMG1->bind(2);

				_glowBatch.batch.renderTarget = _engine->getView();
				_engine->getRenderer()->postProcessing(_glowBatch.batch);
				_glowBatch.batch.renderTarget = _glowBatch.output.get();
				_glowBatch.batch.pipeline = _glowBatch.pipeline.get();
			}
			else
				_engine->getView()->blit(_lightingBatch.output.get(), 0);
		}

		{ // Render transparent objects	(Forward rendering)
		  //_world->tick(TickAction::renderTransparent, delta);
		}

		{ // Particle batch
			for (auto& kv : _particleBatch.batch.objects) {
				kv.second.clear();
				_particleBatch.batch.textureInfo.clear();
			}

			for (auto& pc : Hydra::Component::ParticleComponent::componentHandler->getActiveComponents()) {
				auto p = static_cast<Hydra::Component::ParticleComponent*>(pc.get());
				auto e = world::getEntity(p->entityID);
				auto drawObj = e->getComponent<Hydra::Component::DrawObjectComponent>();
				auto t = e->getComponent<Hydra::Component::TransformComponent>();
				auto& particles = p->particles;
				for (auto& particle : particles) {
					if (particle.life <= 0)
						continue;
					_particleBatch.batch.objects[drawObj->drawObject->mesh].push_back(/*t->getMatrix() */ particle.getMatrix());
					_particleBatch.batch.textureInfo.push_back(particle.texOffset1);
					_particleBatch.batch.textureInfo.push_back(particle.texOffset2);
					_particleBatch.batch.textureInfo.push_back(particle.texCoordInfo);
				}
			}
			{
				auto viewMatrix = _cc->getViewMatrix();
				glm::vec3 rightVector = { viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0] };
				glm::vec3 upVector = { viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1] };
				_particleBatch.pipeline->setValue(0, viewMatrix);
				_particleBatch.pipeline->setValue(1, _cc->getProjectionMatrix());
				_particleBatch.pipeline->setValue(2, rightVector);
				_particleBatch.pipeline->setValue(3, upVector);
				_particleBatch.pipeline->setValue(4, 0);
				_particleAtlases->bind(0);

				for (auto& kv : _particleBatch.batch.objects) {
					std::vector<glm::mat4>& list = kv.second;

					std::sort(list.begin(), list.end(), [cameraPos](const glm::mat4& a, const glm::mat4& b) {
						return glm::distance(glm::vec3(a[3]), cameraPos) < glm::distance(glm::vec3(b[3]), cameraPos);
					});
				}

				_engine->getRenderer()->render(_particleBatch.batch);
			}
		}

		{ // Hud windows
		  //static float f = 0.0f;
		  //static bool b = false;
		  //static float invisF[3] = { 0, 0, 0 };
			float hpP = 100;
			float ammoP = 100;
			float degrees = 0;
			std::vector<Buffs> perksList;
			for (auto& p : Hydra::Component::PlayerComponent::componentHandler->getActiveComponents()) {
				auto player = static_cast<Hydra::Component::PlayerComponent*>(p.get());
				perksList = player->activeBuffs.getActiveBuffs();
			}
			for (auto& camera : Hydra::Component::CameraComponent::componentHandler->getActiveComponents())
				degrees = glm::degrees(static_cast<Hydra::Component::CameraComponent*>(camera.get())->cameraYaw);

			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, float(0.0f));

			const int x = _engine->getView()->getSize().x / 2;
			const ImVec2 pos = ImVec2(x, _engine->getView()->getSize().y / 2);

			//Crosshair
			ImGui::SetNextWindowPos(pos + ImVec2(-10, 1));
			ImGui::SetNextWindowSize(ImVec2(20, 20));
			ImGui::Begin("Crosshair", NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/Crosshair.png")->getID()), ImVec2(20, 20));
			ImGui::End();

			//AimRing
			ImGui::SetNextWindowPos(pos + ImVec2(-51, -42));
			ImGui::SetNextWindowSize(ImVec2(120, 120));
			ImGui::Begin("AimRing", NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/AimRing.png")->getID()), ImVec2(100, 100));
			ImGui::End();

			//Hp bar on ring
			float offsetHpF = 72 * hpP * 0.01;
			int offsetHp = offsetHpF;
			ImGui::SetNextWindowPos(pos + ImVec2(-47, -26 + 72 - offsetHp));
			ImGui::SetNextWindowSize(ImVec2(100, 100));
			ImGui::Begin("HpOnRing", NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/HpOnRing.png")->getID()), ImVec2(22, offsetHp), ImVec2(0, 1 - hpP * 0.01), ImVec2(1, 1));
			ImGui::End();

			//Ammo on bar
			float offsetAmmoF = 72 * ammoP * 0.01;
			int offsetAmmo = offsetAmmoF;
			ImGui::SetNextWindowPos(pos + ImVec2(+25, -26 + 72 - offsetAmmo));
			ImGui::SetNextWindowSize(ImVec2(100, 100));
			ImGui::Begin("AmmoOnRing", NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/AmmoOnRing.png")->getID()), ImVec2(22, offsetAmmo), ImVec2(0, 1 - ammoP * 0.01), ImVec2(1, 1));
			ImGui::End();

			//compas that turns with player
			float degreesP = ((float(100) / float(360) * degrees) / 100);
			float degreesO = float(1000) * degreesP;
			ImGui::SetNextWindowPos(ImVec2(pos.x - 275, +70));
			ImGui::SetNextWindowSize(ImVec2(600, 20));
			ImGui::Begin("Compass", NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/CompassCut.png")->getID()), ImVec2(550, 20), ImVec2(degreesO / float(1000), 0), ImVec2((float(1) - ((float(450) - degreesO) / float(1000))), 1));
			_textureLoader->getTexture("assets/hud/CompassCut.png")->setRepeat();
			ImGui::End();

			//Enemys on compas
			int i = 0;
			glm::mat4 viewMat = static_cast<Hydra::Component::CameraComponent*>(Hydra::Component::CameraComponent::componentHandler->getActiveComponents()[0].get())->getViewMatrix();
			for (auto& enemy : Hydra::Component::AIComponent::componentHandler->getActiveComponents()) {
				char buf[128];
				snprintf(buf, sizeof(buf), "AI is a scrub here is it's scrubID: %d", i);
				auto playerP = _cc->position;
				auto enemyP = world::getEntity(enemy->entityID)->getComponent<Hydra::Component::TransformComponent>()->position;
				auto enemyDir = normalize(enemyP - playerP);

				glm::vec3 forward(-viewMat[0][2], -viewMat[1][2], -viewMat[2][2]);
				glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), forward));

				glm::vec2 forward2D = glm::normalize(glm::vec2(forward.x, forward.z));
				glm::vec2 right2D = glm::normalize(glm::vec2(right.x, right.z));
				glm::vec2 enemy2D = glm::normalize(glm::vec2(enemyDir.x, enemyDir.z));

				float dotPlacment = glm::dot(forward2D, enemy2D); // -1 - +1
				float leftRight = glm::dot(right2D, enemy2D);
				if (leftRight < 0)
				{
					leftRight = 1;
				}
				else
				{
					leftRight = -1;
				}
				if (dotPlacment < 0)
				{
					dotPlacment = 0;
				}
				dotPlacment = dotPlacment;
				dotPlacment = leftRight * (1 - dotPlacment) * 275;
				ImGui::SetNextWindowPos(ImVec2(x + dotPlacment, 75)); //- 275
				ImGui::SetNextWindowSize(ImVec2(20, 20));
				ImGui::Begin(buf, NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
				ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/Red.png")->getID()), ImVec2(10, 10));
				ImGui::End();
				i++;
			}

			//Dynamic cooldown dots
			int amountOfActives = 3;
			int coolDownList[64] = { 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };
			float pForEatchDot = float(1) / float(amountOfActives);
			float stepSize = float(70) * pForEatchDot;
			for (int i = 0; i < amountOfActives; i++)
			{
				char buf[128];
				snprintf(buf, sizeof(buf), "Cooldown%d", i);
				float yOffset = float(stepSize * float(i + 1));
				float xOffset = pow(abs((yOffset - (stepSize / float(2)) - float(35))) * 0.1069, 2);
				ImGui::SetNextWindowPos(pos + ImVec2(-64 + xOffset, -24 + yOffset - ((stepSize + 10.0) / 2.0)));
				ImGui::SetNextWindowSize(ImVec2(15, 15));
				ImGui::Begin(buf, NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
				if (coolDownList[i] >= 7)
				{
					ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/Red.png")->getID()), ImVec2(10, 10));
				}
				else if (coolDownList[i] <= 0)
				{
					ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/Green.png")->getID()), ImVec2(10, 10));
				}
				else
				{
					ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/Yellow.png")->getID()), ImVec2(10, 10));
				}

				ImGui::End();
			}

			//Perk Icons
			int amountOfPerks = perksList.size();
			for (int i = 0; i < amountOfPerks; i++)
			{
				char buf[128];
				snprintf(buf, sizeof(buf), "Perk%d", i);
				float xOffset = float((-10 * amountOfPerks) + (20 * i));
				ImGui::SetNextWindowPos(pos + ImVec2(xOffset, +480));
				ImGui::SetNextWindowSize(ImVec2(20, 20));
				ImGui::Begin(buf, NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
				switch (perksList[i])
				{
				case BUFF_BULLETVELOCITY:
					ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/BulletVelocity.png")->getID()), ImVec2(20, 20));
					break;
				case BUFF_DAMAGEUPGRADE:
					ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/DamageUpgrade.png")->getID()), ImVec2(20, 20));
					break;
				case BUFF_HEALING:
					ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/Healing.png")->getID()), ImVec2(20, 20));
					break;
				case BUFF_HEALTHUPGRADE:
					ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/HealthUpgrade.png")->getID()), ImVec2(20, 20));
					break;
				}

				ImGui::End();
			}

			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();

			//////Debug for pathfinding
			//ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
			//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			//ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, float(0.0f));

			//int k = 0;
			//for (auto& entity : _world->getActiveComponents<Hydra::Component::AIComponent>())
			//{
			//	for (int i = 0; i < 30; i++)
			//	{
			//		for (int j = 0; j < 30; j++)
			//		{
			//			if (entity != nullptr)
			//			{
			//				char buf[128];
			//				snprintf(buf, sizeof(buf), "%d%d", i, j);
			//				if (entity->getComponent<Hydra::Component::AIComponent>()->getWall(i, j) == 1)
			//				{
			//					ImGui::SetNextWindowPos(ImVec2(10 * i, 10 * j));
			//					ImGui::SetNextWindowSize(ImVec2(20, 20));
			//					ImGui::Begin(buf, NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			//					ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/Red.png")->getID()), ImVec2(20, 20));
			//					ImGui::End();
			//				}
			//				if (entity->getComponent<Hydra::Component::AIComponent>()->getWall(i, j) == 2)
			//				{
			//					ImGui::SetNextWindowPos(ImVec2(10 * i, 10 * j));
			//					ImGui::SetNextWindowSize(ImVec2(20, 20));
			//					ImGui::Begin(buf, NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			//					ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/Blue.png")->getID()), ImVec2(20, 20));
			//					ImGui::End();
			//				}
			//				else if (entity->getComponent<Hydra::Component::AIComponent>()->getWall(i, j) == 3)
			//				{
			//					ImGui::SetNextWindowPos(ImVec2(10 * i, 10 * j));
			//					ImGui::SetNextWindowSize(ImVec2(20, 20));
			//					ImGui::Begin(buf, NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			//					ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/Yellow.png")->getID()), ImVec2(20, 20));
			//					ImGui::End();
			//				}
			//				//else if (entity->getComponent<Hydra::Component::AIComponent>()->getWall(i, j) == 0)
			//				//{
			//				//	ImGui::SetNextWindowPos(ImVec2(10 * i, 10 * j));
			//				//	ImGui::SetNextWindowSize(ImVec2(20, 20));
			//				//	ImGui::Begin(buf, NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			//				//	ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/Green.png")->getID()), ImVec2(20, 20));
			//				//	ImGui::End();
			//				//}
			//			}
			//		}
			//	}
			//	k++;
			//}

			//ImGui::PopStyleColor();
			//ImGui::PopStyleVar();
			//ImGui::PopStyleVar();
		}


		{ // Sync with network
		  // _world->tick(TickAction::network, delta);
		}
	}

	void GameState::_initSystem() {
		const std::vector<Hydra::World::ISystem*> systems = { _engine->getDeadSystem(), &_cameraSystem, &_lightSystem, &_particleSystem, &_abilitySystem, &_aiSystem, &_physicsSystem, &_bulletSystem, &_playerSystem, &_rendererSystem };
		_engine->getUIRenderer()->registerSystems(systems);
	}

	void GameState::_initWorld() {
		{
			auto floor = world::newEntity("Floor", world::root());
			auto t = floor->addComponent<Hydra::Component::TransformComponent>();
			t->position = glm::vec3(0, -1, 0);
			floor->addComponent<Hydra::Component::RigidBodyComponent>()->createStaticPlane(glm::vec3(0, 1, 0), 1);
		}
		{
			auto physicsBox = world::newEntity("Physics box", world::root());
			auto t = physicsBox->addComponent<Hydra::Component::TransformComponent>();
			t->position = glm::vec3(2, 25, 2);
			physicsBox->addComponent<Hydra::Component::RigidBodyComponent>()->createBox(glm::vec3(0.5f), 10);
			physicsBox->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/Computer1.ATTIC");
		}
		auto playerEntity = world::newEntity("Player", world::root());
		{
			auto p = playerEntity->addComponent<Hydra::Component::PlayerComponent>();
			auto c = playerEntity->addComponent<Hydra::Component::CameraComponent>();
			auto h = playerEntity->addComponent<Hydra::Component::LifeComponent>();
			auto m = playerEntity->addComponent<Hydra::Component::MovementComponent>();
			h->health = 100;
			h->maxHP = 100;
			m->movementSpeed = 20.0f;
			c->renderTarget = _geometryBatch.output.get();
			//c->position = glm::vec3{ 5, 0, -3 };
			auto t = playerEntity->addComponent<Hydra::Component::TransformComponent>();
			t->position = glm::vec3{ 0, 0, 20 };
			{
				auto weaponEntity = world::newEntity("Weapon", playerEntity);
				weaponEntity->addComponent<Hydra::Component::WeaponComponent>();
				weaponEntity->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/alphaGunModel.ATTIC");
				auto t2 = weaponEntity->addComponent<Hydra::Component::TransformComponent>();
				t2->position = glm::vec3(2, -1.5, -2);
				t2->rotation = glm::quat(0, 0, 1, 0);
				t2->ignoreParent = true;
			}
		}

		{
			auto alienEntity = world::newEntity("Alien", world::root());
			auto a = alienEntity->addComponent<Hydra::Component::AIComponent>();
			//a->_enemyID = Hydra::Component::EnemyTypes::Alien;
			a->_damage = 4;
			a->_originalRange = 4;
			a->behaviour = std::make_shared<AlienBehaviour>(alienEntity, playerEntity);
			auto h = alienEntity->addComponent<Hydra::Component::LifeComponent>();
			h->maxHP = 80;
			h->health = 80;
			auto m = alienEntity->addComponent<Hydra::Component::MovementComponent>();
			m->movementSpeed = 8.0f;
			auto t = alienEntity->addComponent<Hydra::Component::TransformComponent>();
			t->position = glm::vec3{ 10, 0, 20 };
			t->scale = glm::vec3{ 2,2,2 };
			a->_scale = glm::vec3{ 2,2,2 };
			alienEntity->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/characters/AlienModel1.mATTIC");
			a->behaviour->refreshComponents();
		}

		{
			auto pointLight1 = world::newEntity("Pointlight1", world::root());
			pointLight1->addComponent<Hydra::Component::TransformComponent>();
			pointLight1->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/EscapePodDoor.mATTIC");
			auto p1LC = pointLight1->addComponent<Hydra::Component::PointLightComponent>();
			p1LC->color = glm::vec3(0, 1, 0);
		} {
			auto pointLight2 = world::newEntity("Pointlight2", world::root());
			auto t = pointLight2->addComponent<Hydra::Component::TransformComponent>();
			t->position = glm::vec3(45, 0, 0);
			pointLight2->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/CylinderContainer.ATTIC");
			auto p2LC = pointLight2->addComponent<Hydra::Component::PointLightComponent>();
			p2LC->position = glm::vec3(45, 0, 0);
			p2LC->color = glm::vec3(1, 0, 0);
		} {
			auto pointLight3 = world::newEntity("Pointlight3", world::root());
			auto t = pointLight3->addComponent<Hydra::Component::TransformComponent>();
			t->position = glm::vec3(45, 0, 0);
			pointLight3->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/CylinderContainer.ATTIC");
			auto p3LC = pointLight3->addComponent<Hydra::Component::PointLightComponent>();
			p3LC->position = glm::vec3(0, 0, 45);
			p3LC->color = glm::vec3(1, 0, 0);
		} {
			auto pointLight4 = world::newEntity("Pointlight4", world::root());
			auto t = pointLight4->addComponent<Hydra::Component::TransformComponent>();
			t->position = glm::vec3(45, 0, 0);
			pointLight4->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/CylinderContainer.ATTIC");
			auto p4LC = pointLight4->addComponent<Hydra::Component::PointLightComponent>();
			p4LC->position = glm::vec3(45, 0, 45);
			p4LC->color = glm::vec3(1, 0, 0);
		}

		{
			auto test = world::newEntity("test", world::root());
			test->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/CylinderContainer.ATTIC");
			auto t = test->addComponent<Hydra::Component::TransformComponent>();
			t->position = glm::vec3(-7, 0, 0);
		} {
			auto wall1 = world::newEntity("Wall1", world::root());
			wall1->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/Wall_V4.mATTIC");
			auto t = wall1->addComponent<Hydra::Component::TransformComponent>();
			t->position = glm::vec3(8, 0, 33);
		} {
			auto wall2 = world::newEntity("Wall2", world::root());
			wall2->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/Wall_V4.mATTIC");
			auto t = wall2->addComponent<Hydra::Component::TransformComponent>();
			t->position = glm::vec3(55, 0, 15);
		} {
			auto wall3 = world::newEntity("Wall3", world::root());
			wall3->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/Wall_V4.mATTIC");
			auto t = wall3->addComponent<Hydra::Component::TransformComponent>();
			t->position = glm::vec3(19.5, 0, -13);
		} {
			auto roof = world::newEntity("Roof", world::root());
			roof->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/Roof_V2.mATTIC");
			auto t = roof->addComponent<Hydra::Component::TransformComponent>();
			t->position = glm::vec3(14, 8, 9);
			t->scale = glm::vec3(40, 40, 40);
			t->rotation = glm::quat(0, 0, 0, 1);
		} {
			auto floor = world::newEntity("Floor", world::root());
			floor->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/Floor_v2.mATTIC");
			auto t = floor->addComponent<Hydra::Component::TransformComponent>();
			t->position = glm::vec3(14, -8, 9);
		}

		{
			auto particleEmitter = world::newEntity("ParticleEmitter", world::root());
			particleEmitter->addComponent<Hydra::Component::MeshComponent>()->loadMesh("assets/objects/R2D3.mATTIC");
			auto p = particleEmitter->addComponent<Hydra::Component::ParticleComponent>();
			p->delay = 1.0f / 15.0f;
			auto t = particleEmitter->addComponent<Hydra::Component::TransformComponent>();
			t->position = glm::vec3{ 4, 0, 4 };
		}

		{
			auto lightEntity = world::newEntity("Light", world::root());
			auto l = lightEntity->addComponent<Hydra::Component::LightComponent>();
			l->position = glm::vec3(-5, 0.75, 4.3);
			l->direction = glm::vec3(-1, 0, 0);
			auto t = lightEntity->addComponent<Hydra::Component::TransformComponent>();
			t->position = glm::vec3(8.0, 0, 3.5);
		}
		//TODO: Fix AI Serialization
		//{
		//	BlueprintLoader::save("world.blueprint", "World Blueprint", world::root());
		//	Hydra::World::World::reset();
		//	auto bp = BlueprintLoader::load("world.blueprint");
		//	bp->spawn(world::root());
		//}

		{
			_cc = static_cast<Hydra::Component::CameraComponent*>(Hydra::Component::CameraComponent::componentHandler->getActiveComponents()[0].get());
			_cc->renderTarget = _geometryBatch.output.get();

			for (auto& rb : Hydra::Component::RigidBodyComponent::componentHandler->getActiveComponents()) {
				_engine->log(Hydra::LogLevel::normal, "Enabling bullet for %s", world::getEntity(rb->entityID)->name.c_str());
				_physicsSystem.enable(static_cast<Hydra::Component::RigidBodyComponent*>(rb.get()));
			}
		}
	}

	std::shared_ptr<Hydra::Renderer::IFramebuffer> GameState::_blurGlowTexture(std::shared_ptr<Hydra::Renderer::ITexture>& texture, int nrOfTimes, glm::vec2 size, const std::vector<float>& kernel, bool blurEnabled) {
		// TO-DO: Make it agile so it can blur any texture
		_glowBatch.pipeline->setValue(1, 1); // This bind will never change
		bool horizontal = true;
		bool firstPass = true;
		_blurrExtraFBO1->resize(size);
		_blurrExtraFBO2->resize(size);
		_glowBatch.pipeline->setValue(3, 5);

		for (int i = 0; i < 5; i++) {
			_glowBatch.pipeline->setValue(4 + i, kernel[i]);
		}
		for (int i = 0; i < nrOfTimes * 2; i++) {
			if (firstPass) {
				_glowBatch.batch.renderTarget = _blurrExtraFBO2.get();
				texture->bind(1);
				firstPass = false;
			}
			else if (horizontal) {
				_glowBatch.batch.renderTarget = _blurrExtraFBO2.get();
				(*_blurrExtraFBO1)[0]->bind(1);
			}
			else {
				_glowBatch.batch.renderTarget = _blurrExtraFBO1.get();
				(*_blurrExtraFBO2)[0]->bind(1);
			}
			_glowBatch.pipeline->setValue(2, horizontal);
			_engine->getRenderer()->postProcessing(_glowBatch.batch);
			horizontal = !horizontal;
		}

		// Change back to normal rendertarget.
		_glowBatch.batch.renderTarget = _glowBatch.output.get();
		return _blurrExtraFBO1;
	}
}