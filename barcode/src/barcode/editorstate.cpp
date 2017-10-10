#include <barcode/editorstate.hpp>

namespace Barcode {
	EditorState::EditorState() : _engine(Hydra::IEngine::getInstance()) {}

	void EditorState::load() {
		_textureLoader = Hydra::IO::GLTextureLoader::create();
		_meshLoader = Hydra::IO::GLMeshLoader::create(_engine->getRenderer());
		_physicsManager = Hydra::Physics::BulletManager::create();

		_finalImage = _engine->getUIRenderer()->addRenderWindow();
		_finalImage->enabled = true;
		_finalImage->title = "Final Image";
		_finalImage->image = Hydra::Renderer::GLTexture::createFromData(_finalImage->size.x, _finalImage->size.y, TextureType::u8RGB, nullptr);

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

			batch.output = Hydra::Renderer::GLFramebuffer::create(windowSize, 4);
			batch.output
				->addTexture(0, Hydra::Renderer::TextureType::f32RGB) // Position
				.addTexture(1, Hydra::Renderer::TextureType::u8RGB) // Diffuse
				.addTexture(2, Hydra::Renderer::TextureType::u8RGB) // Normal
				.addTexture(3, Hydra::Renderer::TextureType::f16RGBA) // Light pos
				.addTexture(4, Hydra::Renderer::TextureType::f16RGB) // Depth
				.addTexture(5, Hydra::Renderer::TextureType::f16Depth) // real depth
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
			batch.vertexShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::vertex, "assets/shaders/editorLighting.vert");
			batch.fragmentShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::fragment, "assets/shaders/editorLighting.frag");

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

		{
			auto& batch = _previewBatch;
			batch.vertexShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::vertex, "assets/shaders/previewWindow.vert");
			batch.fragmentShader = Hydra::Renderer::GLShader::createFromSource(Hydra::Renderer::PipelineStage::fragment, "assets/shaders/previewWindow.frag");

			batch.pipeline = Hydra::Renderer::GLPipeline::create();
			batch.pipeline->attachStage(*batch.vertexShader);
			batch.pipeline->attachStage(*batch.fragmentShader);
			batch.pipeline->finalize();

			batch.output = Hydra::Renderer::GLFramebuffer::create(glm::vec2(720), 0);
			batch.output->addTexture(0, Hydra::Renderer::TextureType::u8RGB).finalize();

			batch.batch.clearColor = glm::vec4(0, 0, 0, 1);
			batch.batch.clearFlags = Hydra::Renderer::ClearFlags::color | Hydra::Renderer::ClearFlags::depth;
			batch.batch.renderTarget = batch.output.get();
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
		this->_importerMenu = new ImporterMenu(_world.get());
		this->_exporterMenu = new ExporterMenu(_world.get());
	}

	EditorState::~EditorState() { }

	void EditorState::onMainMenu() { 
		if (ImGui::BeginMenu("Editor"))
		{
			if (ImGui::MenuItem("Import..."))
			{
				_showImporter = !_showImporter;
				if (_showImporter)
					_importerMenu->refresh();
			}
			if (ImGui::MenuItem("Export..."))
			{
				_showExporter = !_showExporter;
				if (_showExporter)
					_exporterMenu->refresh();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Clear room"))
			{
				ExporterMenu::getRoomEntity(_world.get())->markDead();

				auto roomEntity = _world->createEntity("Room");
				roomEntity->addComponent<Hydra::Component::TransformComponent>(glm::vec3(0, 0, 0));
			}
			ImGui::EndMenu();
		}
	}

	//Global variable to maintain a keyframe for now
	void EditorState::runFrame(float delta) {
		auto windowSize = _engine->getView()->getSize();
		{ // Fetch new events
			_engine->getView()->update(_engine->getUIRenderer());
			_engine->getUIRenderer()->newFrame();
		}

		{ // Update physics
			_world->tick(TickAction::physics, delta);
		}

		{ // Render objects (Deferred rendering)
			glm::vec3 cameraPos;
			_world->tick(TickAction::render, delta);

			// Render to geometryFBO

			for (auto& entity : _world->getActiveComponents<Hydra::Component::LightComponent>()) {
				_light = entity->getComponent<Hydra::Component::LightComponent>();
			}

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
			_geometryBatch.pipeline->setValue(2, cameraPos = _cc->getPosition());
			_geometryBatch.pipeline->setValue(4, lightS);

			_animationBatch.pipeline->setValue(0, _cc->getViewMatrix());
			_animationBatch.pipeline->setValue(1, _cc->getProjectionMatrix());
			_animationBatch.pipeline->setValue(2, cameraPos = _cc->getPosition());

			for (auto& kv : _geometryBatch.batch.objects)
				kv.second.clear();

			for (auto& kv : _animationBatch.batch.objects)
				kv.second.clear();

			for (auto& drawObj : _engine->getRenderer()->activeDrawObjects()) {

				if (!drawObj->disable && drawObj->mesh && drawObj->mesh->hasAnimation() == false && drawObj->mesh->getIndicesCount() != 6)
					_geometryBatch.batch.objects[drawObj->mesh].push_back(drawObj->modelMatrix);
				else if (!drawObj->disable && drawObj->mesh && drawObj->mesh->hasAnimation() == true) {
					_animationBatch.batch.objects[drawObj->mesh].push_back(drawObj->modelMatrix);

					int currentFrame = drawObj->mesh->getCurrentKeyframe();

					if (currentFrame < drawObj->mesh->getMaxFramesForAnimation()) {
						drawObj->mesh->setCurrentKeyframe(currentFrame + 1);
					}
					else
					{
						drawObj->mesh->setCurrentKeyframe(1);
					}
					glm::mat4 tempMat;
					for (int i = 0; i < drawObj->mesh->getNrOfJoints(); i++) {
						tempMat = drawObj->mesh->getTransformationMatrices(i);
						_animationBatch.pipeline->setValue(11 + i, tempMat);
					}
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
			//_engine->getRenderer()->render(_shadowBatch.batch);
			//_engine->getRenderer()->render(_animationBatch.batch);
		}

		{
			for (auto& kv : _shadowBatch.batch.objects)
				kv.second.clear();

			for (auto& drawObj : _engine->getRenderer()->activeDrawObjects()) {
				if (!drawObj->disable && drawObj->mesh && drawObj->mesh->hasAnimation() == false)
					_shadowBatch.batch.objects[drawObj->mesh].push_back(drawObj->modelMatrix);
			}

			_shadowBatch.pipeline->setValue(0, _light->getViewMatrix());
			_shadowBatch.pipeline->setValue(1, _light->getProjectionMatrix());

			_engine->getRenderer()->render(_shadowBatch.batch);
		}

		{ // Lighting pass
			_lightingBatch.pipeline->setValue(0, 0);
			_lightingBatch.pipeline->setValue(1, 1);
			_lightingBatch.pipeline->setValue(2, 2);
			_lightingBatch.pipeline->setValue(3, 3);
			_lightingBatch.pipeline->setValue(4, 4);

			_lightingBatch.pipeline->setValue(5, _cc->getPosition());
			_lightingBatch.pipeline->setValue(6, _light->getDirection());

			(*_geometryBatch.output)[0]->bind(0);
			(*_geometryBatch.output)[1]->bind(1);
			(*_geometryBatch.output)[2]->bind(2);
			(*_geometryBatch.output)[3]->bind(3);
			_shadowBatch.output->getDepth()->bind(4);

			//(*_geometryBatch.output)[4]->bind(4);

			_engine->getRenderer()->postProcessing(_lightingBatch.batch);
		}

		{ // Glow
			int nrOfTimes = 1;
			glm::vec2 size = windowSize;

			_lightingBatch.output->resolve(0, _blurredOriginal);
			_lightingBatch.output->resolve(1, (*_glowBatch.output)[0]);

			_blurGlowTexture((*_glowBatch.output)[0], nrOfTimes, size *= 0.5f)
				->resolve(0, _blurredIMG1);
			_blurGlowTexture(_blurredIMG1, nrOfTimes, size *= 0.5f)
				->resolve(0, _blurredIMG2);
			_blurGlowTexture(_blurredIMG2, nrOfTimes, size *= 0.5f)
				->resolve(0, _blurredIMG3);

			_glowBatch.batch.pipeline = _glowPipeline.get();

			_glowBatch.batch.pipeline->setValue(1, 1);
			_glowBatch.batch.pipeline->setValue(2, 2);
			_glowBatch.batch.pipeline->setValue(3, 3);
			_glowBatch.batch.pipeline->setValue(4, 4);

			_blurredOriginal->bind(1);
			_blurredIMG1->bind(2);
			_blurredIMG2->bind(3);
			_blurredIMG3->bind(4);

			_glowBatch.batch.renderTarget = _engine->getView();
			_engine->getRenderer()->postProcessing(_glowBatch.batch);
			_glowBatch.batch.renderTarget = _glowBatch.output.get();
			_glowBatch.batch.pipeline = _glowBatch.pipeline.get();
		}

		{ // Render transparent objects	(Forward rendering)
			_world->tick(TickAction::renderTransparent, delta);
		}

		{ // Particle batch
			for (auto& kv : _particleBatch.batch.objects) {
				kv.second.clear();
				_particleBatch.batch.textureInfo.clear();
			}

			bool anyParticles = false;
			for (auto& entity : _world->getActiveComponents<Hydra::Component::ParticleComponent>()) {
				auto pc = entity->getComponent<Hydra::Component::ParticleComponent>();
				auto drawObj = entity->getDrawObject();
				auto& particles = pc->getParticles();
				if (particles.size() > 0) {
					for (auto& particle : particles) {
						_particleBatch.batch.objects[drawObj->mesh].push_back(particle->m);
						_particleBatch.batch.textureInfo.push_back(particle->texOffset1);
						_particleBatch.batch.textureInfo.push_back(particle->texOffset2);
						_particleBatch.batch.textureInfo.push_back(particle->texCoordInfo);
					}
					anyParticles = true;
				}
			}
			if (anyParticles) {
				auto viewMatrix = _cc->getViewMatrix();
				glm::vec3 rightVector = { viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0] };
				glm::vec3 upVector = { viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1] };
				_particleBatch.pipeline->setValue(0, viewMatrix);
				_particleBatch.pipeline->setValue(1, _cc->getProjectionMatrix());
				_particleBatch.pipeline->setValue(2, rightVector);
				_particleBatch.pipeline->setValue(3, upVector);
				_particleBatch.pipeline->setValue(4, 0);
				_particleAtlases->bind(0);
				_engine->getRenderer()->render(_particleBatch.batch);
			}
		}

		{ // Hud windows
		  //static float f = 0.0f;
		  //static bool b = false;
		  //static float invisF[3] = { 0, 0, 0 };
			float hpP = 100;
			float ammoP = 100;

			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, float(0.0f));

			const int x = _engine->getView()->getSize().x / 2;
			const ImVec2 pos = ImVec2(x, _engine->getView()->getSize().y / 2);

			ImGui::SetNextWindowPos(pos + ImVec2(-10, 1));
			ImGui::SetNextWindowSize(ImVec2(20, 20));
			ImGui::Begin("Crosshair", NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/Crosshair.png")->getID()), ImVec2(20, 20));
			ImGui::End();

			ImGui::SetNextWindowPos(pos + ImVec2(-51, -42));
			ImGui::SetNextWindowSize(ImVec2(120, 120));
			ImGui::Begin("AimRing", NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/AimRing.png")->getID()), ImVec2(100, 100));
			ImGui::End();

			float offsetHpF = 72 * hpP * 0.01;
			int offsetHp = offsetHpF;
			ImGui::SetNextWindowPos(pos + ImVec2(-47, -26 + 72 - offsetHp));
			ImGui::SetNextWindowSize(ImVec2(100, 100));
			ImGui::Begin("HpOnRing", NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/HpOnRing.png")->getID()), ImVec2(22, offsetHp), ImVec2(0, 1 - hpP * 0.01), ImVec2(1, 1));
			ImGui::End();

			float offsetAmmoF = 72 * ammoP * 0.01;
			int offsetAmmo = offsetAmmoF;
			ImGui::SetNextWindowPos(pos + ImVec2(+25, -26 + 72 - offsetAmmo));
			ImGui::SetNextWindowSize(ImVec2(100, 100));
			ImGui::Begin("AmmoOnRing", NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/AmmoOnRing.png")->getID()), ImVec2(22, offsetAmmo), ImVec2(0, 1 - ammoP * 0.01), ImVec2(1, 1));
			ImGui::End();

			float degres = 0;
			float degresP = ((float(100) / float(360) * degres) / 100);
			float degresO = float(1000) * degresP;
			ImGui::SetNextWindowPos(ImVec2(pos.x - 275, +70));
			ImGui::SetNextWindowSize(ImVec2(600, 20));
			ImGui::Begin("Compass", NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/Compass.png")->getID()), ImVec2(550, 20), ImVec2(degresO / float(1550), 0), ImVec2((float(1) - ((float(1000) - degresO) / float(1550))), 1));
			_textureLoader->getTexture("assets/hud/Compass.png")->setRepeat();
			ImGui::End();


			int i = 0;
			for (auto& entity : _world->getActiveComponents<Hydra::Component::EnemyComponent>())
			{
				char buf[128];
				snprintf(buf, sizeof(buf), "Enemy is a scrub here is it's scrubID: %d", i);
				auto playerP = _cc->getPosition();
				auto enemyP = entity->getComponent<Hydra::Component::EnemyComponent>()->getPosition();
				auto enemyDir = normalize(enemyP - playerP);

				glm::mat4 viewMat = _world->getActiveComponents<Hydra::Component::CameraComponent>()[0]->getComponent<Hydra::Component::CameraComponent>()->getViewMatrix();
				glm::vec3 forward(-viewMat[0][2], -viewMat[1][2], -viewMat[2][2]);
				glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), forward));

				glm::vec2 forward2D = glm::normalize(glm::vec2(forward.x, forward.z));
				glm::vec2 right2D = glm::normalize(glm::vec2(right.x, right.z));
				glm::vec2 enemy2D = glm::normalize(glm::vec2(enemyDir.x, enemyDir.z));

				float dotPlacment = glm::dot(forward2D, enemy2D); // -1 - +1
				float leftRight = glm::dot(right2D, enemy2D);
				if (leftRight < 0)
				{
					leftRight = -1;
				}
				else
				{
					leftRight = 1;
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


			int amountOfActives = 3;
			int coolDownList[20] = { 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };
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
			//70, 24

			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();

			////Debug for pathfinding
			/*ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, float(0.0f));

			int k = 0;
			for (auto& entity : _world->getActiveComponents<Hydra::Component::EnemyComponent>())
			{
			for (int i = 0; i < 30; i++)
			{
			for (int j = 0; j < 30; j++)
			{
			if (entity != nullptr)
			{
			char buf[128];
			snprintf(buf, sizeof(buf), "%d%d", i, j);
			if (_enemy->getWall(i, j) == 1)
			{
			ImGui::SetNextWindowPos(ImVec2(10 * i, 10 * j));
			ImGui::SetNextWindowSize(ImVec2(20, 20));
			ImGui::Begin(buf, NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/Red.png")->getID()), ImVec2(20, 20));
			ImGui::End();
			}
			if (entity->getComponent<Hydra::Component::EnemyComponent>()->getWall(i, j) == 2)
			{
			ImGui::SetNextWindowPos(ImVec2(10 * i, 10 * j));
			ImGui::SetNextWindowSize(ImVec2(20, 20));
			ImGui::Begin(buf, NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/Blue.png")->getID()), ImVec2(20, 20));
			ImGui::End();
			}
			else if (entity->getComponent<Hydra::Component::EnemyComponent>()->getWall(i, j) == 3)
			{
			ImGui::SetNextWindowPos(ImVec2(10 * i, 10 * j));
			ImGui::SetNextWindowSize(ImVec2(20, 20));
			ImGui::Begin(buf, NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/Yellow.png")->getID()), ImVec2(20, 20));
			ImGui::End();
			}
			else if (_enemy->getWall(i, j) == 0)
			{
			ImGui::SetNextWindowPos(ImVec2(10 * i, 10 * j));
			ImGui::SetNextWindowSize(ImVec2(20, 20));
			ImGui::Begin(buf, NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
			ImGui::Image(reinterpret_cast<ImTextureID>(_textureLoader->getTexture("assets/hud/Green.png")->getID()), ImVec2(20, 20));
			ImGui::End();
			}
			}
			}
			}
			k++;
			}

			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();*/
		}

		{
			_engine->getRenderer()->render(_viewBatch.batch);
			if (_showImporter)
				_importerMenu->render(_showImporter, _previewBatch.batch, delta);
			if (_showExporter)
				_exporterMenu->render(_showExporter);

			_lightingBatch.output->resolve(0, _finalImage->image);
		}

		{ // Sync with network
			_world->tick(TickAction::network, delta);
		}
	}

	void EditorState::_initWorld() {
		_world = Hydra::World::World::create();

		auto playerEntity = _world->createEntity("Player");
		_cc = playerEntity->addComponent<Hydra::Component::EditorCameraComponent>(_geometryBatch.output.get(), glm::vec3{ 5, 0, -3 });

		auto roomEntity = _world->createEntity("Room");
		roomEntity->addComponent<Hydra::Component::TransformComponent>(glm::vec3(0,0,0));

		auto lightEntity = _world->createEntity("Light");
		_light = lightEntity->addComponent<Hydra::Component::LightComponent>();
		_light->setPosition(glm::vec3(-5.0, 0.75, 4.3));
		_light->translate(glm::vec3(10, 0, 0));
		_light->setDirection(glm::vec3(-1, 0, 0));
		lightEntity->addComponent<Hydra::Component::TransformComponent>(glm::vec3(8.0, 0, 3.5));


		BlueprintLoader::save("world.blueprint", "World Blueprint", _world->getWorldRoot());
		auto bp = BlueprintLoader::load("world.blueprint");
		_world->setWorldRoot(bp->spawn(_world.get()));

		{
			auto& world = _world->getWorldRoot()->getChildren();
			auto it = std::find_if(world.begin(), world.end(), [](const std::shared_ptr<IEntity>& e) { return e->getName() == "Player"; });
			if (it != world.end()) {
				_cc = (*it)->getComponent<Hydra::Component::EditorCameraComponent>();
				_cc->setRenderTarget(_geometryBatch.output.get());
			}
			else
				_engine->log(Hydra::LogLevel::error, "Camera not found!");
		}

	}

	std::shared_ptr<Hydra::Renderer::IFramebuffer> EditorState::_blurGlowTexture(std::shared_ptr<Hydra::Renderer::ITexture>& texture, int &nrOfTimes, glm::vec2 size) { // TO-DO: Make it agile so it can blur any texture
		_glowBatch.pipeline->setValue(1, 1); // This bind will never change
		bool horizontal = true;
		bool firstPass = true;
		_blurrExtraFBO1->resize(size);
		_blurrExtraFBO2->resize(size);
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
