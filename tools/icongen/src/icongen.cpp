/*

	HeroZ Icon Generator

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "icongen_pch.h"
#include <noz/renderer/effects/GammaCorrection.h>
#include <noz/renderer/effects/BorderEffect.h>
#include <noz/MetaFile.h>

using namespace noz;
using namespace noz::renderer;
using namespace noz::node;
using noz::renderer::effects::GammaCorrection;
using noz::renderer::effects::BorderEffect;

namespace icongen
{
	namespace config
	{
		vec3 cameraEulerAngles = vec3(-90.0f, 0.0f, 0.0f);
		float cameraDistance = 2.5f;
		float lightPitch = 10.0f;
		float lightYaw = 10.0f;
		std::string shader = "shaders/default";
		std::string texture = "textures/palette";
		
		// Border settings
		float borderWidth = 4.0f;
		vec3 borderColor = vec3(0.0f, 0.0f, 0.0f); // Black
		float borderAlpha = 1.0f;
		
		// Output defaults
		int defaultSize = 256;
		std::string outputDir = "assets/textures/icons";
		
		void loadFromFile(const std::string& configPath)
		{
			if (!std::filesystem::exists(configPath))
			{
				std::cout << "No config file found at: " << configPath << " - using defaults" << std::endl;
				return;
			}
			
			try
			{
				auto meta = MetaFile::parse(configPath);
				
				// Camera settings
				cameraDistance = meta.getFloat("Camera", "distance", cameraDistance);
				cameraEulerAngles = meta.getVec3("Camera", "eulerAngles", cameraEulerAngles);
				
				// Model defaults
				shader = meta.getString("Mesh", "shader", icongen::config::shader);
				texture = meta.getString("Mesh", "texture", icongen::config::texture);

				// Light settings
				lightPitch = meta.getFloat("Light", "pitch", lightPitch);
				lightYaw = meta.getFloat("Light", "yaw", lightYaw);
				
				// Border settings
				borderWidth = meta.getFloat("Border", "width", borderWidth);
				borderColor = meta.getVec3("Border", "color", borderColor);
				borderAlpha = meta.getFloat("Border", "alpha", borderAlpha);
				
				// Output settings
				defaultSize = meta.getInt("Output", "size", defaultSize);
				outputDir = meta.getString("Output", "path", outputDir);
				
				std::cout << "Loaded config from: " << configPath << std::endl;
			}
			catch (const std::exception& e)
			{
				std::cerr << "Failed to load config file: " << e.what() << std::endl;
			}
		}
	}
}

class IconGenerator
{
public:
	IconGenerator()
		: _rotationX(0.0f)
		, _rotationY(0.0f)
		, _rotationZ(0.0f)
		, _distance(5.0f)
		, _lightPitch(icongen::config::lightPitch)
		, _lightYaw(icongen::config::lightYaw)
		, _translation(0.0f, 0.0f, 0.0f)
		, _captureRequested(false)
		, _outputSize(icongen::config::defaultSize)
		, _autoCapture(false)
		, _shouldExit(false)
		, _showBorderedPreview(true)
		, _lastInputTime(0.0f)
		, _sceneNeedsRender(true)  // Initial render needed
		, _borderNeedsUpdate(true)
	{
	}
	
	void requestCapture() { _captureRequested = true; }
	bool shouldExit() const { return _shouldExit; }

	bool initialize(
		const std::string& meshPath,
		const std::string& outputPath,
		bool autoCapture = false)
	{
		Application::load(_outputSize, _outputSize, "Icon Generator - " + meshPath);

		createRenderTarget();
		
		// Camera
		auto halfSize = icongen::config::cameraDistance * 0.5f;
		_camera = Object::create<Camera>();
		_camera->setOrthographic(-halfSize, halfSize, -halfSize, halfSize, 0.1f, 100.0f);
		_camera->setLocalEulerAngles(icongen::config::cameraEulerAngles);
		_camera->setLocalPosition(_camera->forward() * -icongen::config::cameraDistance);
		
		// Light
		_light = std::make_shared<DirectionalLight>();
		_light->setName("Light");
		_light->setEulerAngles(vec3(icongen::config::lightPitch, icongen::config::lightYaw, 0));

		// Scene
		auto root = Object::create<Node3d>();
		root->add(_camera);
		root->add(_light);
		_scene = Object::create<Scene>();
		_scene->setRoot(root);

		// Create border effect scene
		_borderScene = Object::create<Scene>();
		_borderScene->setRoot(Object::create<Node>());
		_borderEffect = Object::create<BorderEffect>();
		_borderEffect->setInputTexture(_iconRenderTarget);
		_borderScene->root()->add(_borderEffect);
		
		noz::renderer::effects::BorderParams params;
		params.width = icongen::config::borderWidth;
		params.color = noz::Color(icongen::config::borderColor.x, icongen::config::borderColor.y,
			icongen::config::borderColor.z, icongen::config::borderAlpha);
		params.textureWidth = static_cast<float>(_outputSize);
		params.textureHeight = static_cast<float>(_outputSize);
		_borderEffect->setBorderParams(params);

		// Create intermediate render target for border effect
		_borderRenderTarget = std::shared_ptr<Texture>(
			Texture::createRenderTarget(
				Renderer::instance()->GetGPUDevice(),
				_outputSize,
				_outputSize,
				SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
				"BorderRenderTarget"));
		
		// Create final render target for gamma correction output (8-bit for readTexturePixels compatibility)
		_finalRenderTarget = std::shared_ptr<Texture>(
			Texture::createRenderTarget(
				Renderer::instance()->GetGPUDevice(),
				_outputSize,
				_outputSize,
				SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
				"FinalRenderTarget"));
		
		// Create gamma correction scene
		_gammaScene = Object::create<Scene>();
		_gammaScene->setRoot(Object::create<Node>());
		_gammaCorrectionEffect = Object::create<GammaCorrection>();
		_gammaCorrectionEffect->setInputTexture(_borderRenderTarget);
		_gammaScene->root()->add(_gammaCorrectionEffect);

		// Load mesh
		_meshPath = meshPath;
		_outputPath = outputPath;
		_autoCapture = autoCapture;

		if (!loadMesh())
			return false;
	
		// Create UI system
		createUI();

		// Initialize border system
		if (!autoCapture)
		{
			// Start with bordered preview disabled
			_showBorderedPreview = false;
			// Trigger initial border application after a brief delay
			_lastInputTime = Time::totalTime() - 0.05f; // Will trigger border in 0.1 seconds
		}

		return true;
	}

	bool loadMesh()
	{
		if (_meshPath.empty())
		{
			std::cerr << "No mesh path specified" << std::endl;
			return false;
		}

		// Load the mesh resource first
		auto mesh = Asset::load<Mesh>(_meshPath);
		if (!mesh)
		{
			std::cerr << "Failed to load mesh: " << _meshPath << std::endl;
			return false;
		}

		// Try to load existing meta file for this icon
		bool hasMetaFile = loadMetaFile();

		// Create mesh object node (Node3d for transform)
		_meshObject = Object::create<Node3d>();
		_scene->root()->add(_meshObject);
		
		// Create mesh renderer as child
		_meshRenderer = Object::create<MeshRenderer>();
		_meshRenderer->setMesh(mesh);
		_meshRenderer->setShader(Asset::load<Shader>(icongen::config::shader));
		_meshRenderer->setTexture(Asset::load<Texture>(icongen::config::texture));
		_meshObject->add(_meshRenderer);

		// Center the mesh and apply any loaded translation
		auto bounds = mesh->bounds();
		vec3 center = (bounds.min() + bounds.max()) * 0.5f;
		_meshObject->setPosition(-center + _translation);

		// If we didn't load from meta file, auto-calculate initial values
		if (!hasMetaFile)
		{
			// Calculate appropriate camera distance based on model bounds
			float maxExtent = noz::math::max(bounds.max().x - bounds.min().x, 
			                   noz::math::max(bounds.max().y - bounds.min().y, 
			                           bounds.max().z - bounds.min().z));
			_distance = maxExtent * icongen::config::cameraDistance;
			
			// Reset translation for new models
			_translation = vec3(0.0f, 0.0f, 0.0f);
		}
		else
		{
			// Apply loaded rotation if we have one
			if (_hasLoadedRotation)
			{
				_meshObject->setLocalRotation(_loadedRotation);
			}
		}

		// Mark for initial render
		_sceneNeedsRender = true;
		_borderNeedsUpdate = true;

		return true;
	}

	void update()
	{
		auto deltaTime = Time::deltaTime();
		float currentTime = Time::totalTime();

		// Handle input using direct SDL polling
		bool hadInput = handleInput(deltaTime);
		
		if (hadInput)
		{
			_lastInputTime = currentTime;
			if (_showBorderedPreview)
			{
				_showBorderedPreview = false;  // Hide bordered preview during interaction
				// Switch back to showing processed texture
				_iconImage->setTexture(_finalRenderTarget);
			}
		}
		else if (!_showBorderedPreview && currentTime - _lastInputTime > 0.5f)  // 0.5 second delay
		{
			_showBorderedPreview = true;
			_borderNeedsUpdate = true;  // Mark border for update
			std::cout << "Switching to bordered preview..." << std::endl;
		}

		// Rotation is now handled directly in handleInput() for camera-relative control

		// Update scenes
		_scene->update();
		_borderScene->update();
		_gammaScene->update();
		_ui->update();
	}

	bool handleInput(float deltaTime)  // Returns true if any input was detected
	{
		float rotSpeed = noz::math::radians(90.0f * deltaTime); // Convert to radians immediately
		float translateSpeed = 2.0f * deltaTime; // Translation speed
		
		bool hadInput = false;
		bool shiftPressed = InputSystem::instance()->IsKeyPressed(SDL_SCANCODE_LSHIFT) || 
		                    InputSystem::instance()->IsKeyPressed(SDL_SCANCODE_RSHIFT);
		
		// Handle camera-relative rotation and translation - WASD
		if (_meshObject)
		{
			if (shiftPressed)
			{
				// Translation mode with Shift+WASD
				vec3 deltaTranslation(0.0f, 0.0f, 0.0f);
				
				if (InputSystem::instance()->IsKeyPressed(SDL_SCANCODE_A)) // Move left
				{
					deltaTranslation.x -= translateSpeed;
					hadInput = true;
				}
				if (InputSystem::instance()->IsKeyPressed(SDL_SCANCODE_D)) // Move right
				{
					deltaTranslation.x += translateSpeed;
					hadInput = true;
				}
				if (InputSystem::instance()->IsKeyPressed(SDL_SCANCODE_W)) // Move up
				{
					deltaTranslation.y += translateSpeed;
					hadInput = true;
				}
				if (InputSystem::instance()->IsKeyPressed(SDL_SCANCODE_S)) // Move down
				{
					deltaTranslation.y -= translateSpeed;
					hadInput = true;
				}
				
				if (hadInput)
				{
					_translation += deltaTranslation;
					// Update mesh position (center + translation)
					if (_meshRenderer && _meshRenderer->mesh())
					{
						auto bounds = _meshRenderer->mesh()->bounds();
						vec3 center = (bounds.min() + bounds.max()) * 0.5f;
						_meshObject->setPosition(-center + _translation);
					}
					_sceneNeedsRender = true;
				}
			}
			else
			{
				// Rotation mode (default WASD behavior)
				quat currentRotation = _meshObject->localRotation();
				quat deltaRotation = quat(1, 0, 0, 0); // Identity quaternion
				
				// Camera-relative rotations (always relative to camera view)
				if (InputSystem::instance()->IsKeyPressed(SDL_SCANCODE_A)) // Rotate left around camera's up axis
				{
					deltaRotation = noz::math::angleAxis(-rotSpeed, vec3(0, 1, 0)) * deltaRotation;
					hadInput = true;
				}
				if (InputSystem::instance()->IsKeyPressed(SDL_SCANCODE_D)) // Rotate right around camera's up axis
				{
					deltaRotation = noz::math::angleAxis(rotSpeed, vec3(0, 1, 0)) * deltaRotation;
					hadInput = true;
				}
				if (InputSystem::instance()->IsKeyPressed(SDL_SCANCODE_W)) // Rotate up around camera's right axis
				{
					deltaRotation = noz::math::angleAxis(-rotSpeed, vec3(1, 0, 0)) * deltaRotation;
					hadInput = true;
				}
				if (InputSystem::instance()->IsKeyPressed(SDL_SCANCODE_S)) // Rotate down around camera's right axis
				{
					deltaRotation = noz::math::angleAxis(rotSpeed, vec3(1, 0, 0)) * deltaRotation;
					hadInput = true;
				}
				
				if (hadInput)
				{
					// Apply the camera-space rotation by multiplying in the correct order
					// This ensures rotations are always relative to the camera's orientation
					_meshObject->setLocalRotation(deltaRotation * currentRotation);
					_sceneNeedsRender = true;
				}
			}
			// Q/E now control zoom instead of rotation
			// Zoom is handled below with +/-
		}
			
		// Handle zoom with +/- and Q/E
		if (InputSystem::instance()->IsKeyPressed(SDL_SCANCODE_EQUALS) || 
		    InputSystem::instance()->IsKeyPressed(SDL_SCANCODE_E))
		{
			_distance = noz::math::max(1.0f, _distance - 5.0f * deltaTime);
			hadInput = true;
			_sceneNeedsRender = true;
		}
		if (InputSystem::instance()->IsKeyPressed(SDL_SCANCODE_MINUS) || 
		    InputSystem::instance()->IsKeyPressed(SDL_SCANCODE_Q))
		{
			_distance = noz::math::min(20.0f, _distance + 5.0f * deltaTime);
			hadInput = true;
			_sceneNeedsRender = true;
		}
			
		// Handle capture (one-shot, so check for key press event)
		static bool spaceWasPressed = false;
		bool spaceIsPressed = InputSystem::instance()->IsKeyPressed(SDL_SCANCODE_SPACE);
		if (spaceIsPressed && !spaceWasPressed)
		{
			_captureRequested = true;
		}
		spaceWasPressed = spaceIsPressed;
		
		return hadInput;
	}

	void render()
	{
		auto* cb = Renderer::instance()->beginFrame();
		if (!cb)
			return;

		assert(cb);
		assert(_scene);
		assert(_camera);
		assert(_iconRenderTarget);

		// Render scene to icon render target
		cb->beginOpaquePass(_iconRenderTarget, true, Color::Transparent, false);
		_scene->render(cb);
		cb->endOpaquePass();

		// Render border effect to intermediate target
		cb->beginOpaquePass(_borderRenderTarget, true, Color::Transparent, false);
		_borderScene->render(cb);
		cb->endOpaquePass();

		// Render gamma correction to final target
		cb->beginOpaquePass(_finalRenderTarget, true, Color::Transparent, false);
		_gammaScene->render(cb);
		cb->endOpaquePass();

		// Render UI to screen (displaying final texture)
		cb->beginOpaquePass(true, Color::Black, false);
		_ui->render(cb);
		cb->endOpaquePass();

		// End frame to execute the commands
		Renderer::instance()->endFrame();

		// Handle capture after rendering
		if (_captureRequested)
		{
			if (captureIcon())
			{
				if (_autoCapture)
				{
					// Exit after successful capture in auto mode
					_shouldExit = true;
				}
			}
			_captureRequested = false;
		}
	}

	bool captureIcon()
	{
		std::cout << "Capturing icon to: " << _outputPath << std::endl;
		
		// Create output directory if it doesn't exist
		std::filesystem::path outputDir = std::filesystem::path(_outputPath).parent_path();
		if (!outputDir.empty())
			std::filesystem::create_directories(outputDir);

		// Read pixels from the appropriate texture (bordered if available and visible)
		auto* renderer = Renderer::instance();
		if (!renderer || !_iconRenderTarget)
		{
			std::cerr << "Renderer or icon render target not available" << std::endl;
			return false;
		}

		noz::Image capturedImage;
		capturedImage = renderer->readTexturePixels(_finalRenderTarget);
		if (capturedImage.empty())
		{
			std::cerr << "Failed to read pixels from render target" << std::endl;
			return false;
		}

		// Save the image as PNG
		if (capturedImage.saveToFile(_outputPath))
		{
			std::cout << "Icon saved successfully as PNG!" << std::endl;
			
			// Save meta file with current settings
			if (saveMetaFile())
			{
				std::cout << "Meta file saved successfully!" << std::endl;
			}
			
			return true;
		}
		else
		{
			std::cerr << "Failed to save PNG file" << std::endl;
			return false;
		}
	}

	void shutdown()
	{
		_scene.reset();
		_meshRenderer.reset();
		_meshObject.reset();
		_camera.reset();
		_light.reset();
		_borderScene.reset();
		_gammaScene.reset();
		_borderEffect.reset();
		_gammaCorrectionEffect.reset();
		_ui.reset();
		_iconImage.reset();
		_iconRenderTarget.reset();
		_borderRenderTarget.reset();
		_finalRenderTarget.reset();
	}

private:

	void createRenderTarget()
	{
		auto* renderer = Renderer::instance();
		assert(renderer);

		auto* gpu = renderer->GetGPUDevice();
		assert(gpu);
		
		_iconRenderTarget = std::shared_ptr<Texture>(
			Texture::createRenderTarget(
				gpu,
				_outputSize,
				_outputSize,
				SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
				"IconRenderTarget"));
	}

	void createUI()
	{
		_ui = Object::create<noz::node::Scene>();
		_canvas = Object::create<noz::ui::Canvas>();
		_canvas->setReferenceSize(vec2(static_cast<float>(_outputSize), static_cast<float>(_outputSize)));

		noz::ui::Style canvasStyle = noz::ui::Style::defaultStyle();
		canvasStyle.backgroundColor = noz::Color(0.2f, 0.2f, 0.2f, 1.0f);
		canvasStyle.width = noz::ui::StyleLength::percent(1);
		canvasStyle.height = noz::ui::StyleLength::percent(1);
		_canvas->setStyle(canvasStyle);

		_iconImage = std::make_shared<noz::ui::Image>();
		_iconImage->setTexture(_finalRenderTarget);

		noz::ui::Style iconStyle = noz::ui::Style::defaultStyle();
		iconStyle.width = noz::ui::StyleLength::fixed(static_cast<float>(_outputSize));
		iconStyle.height = noz::ui::StyleLength::fixed(static_cast<float>(_outputSize));
		iconStyle.marginLeft = noz::ui::StyleLength::Auto;
		iconStyle.marginTop = noz::ui::StyleLength::Auto;
		iconStyle.marginRight = noz::ui::StyleLength::Auto;
		iconStyle.marginBottom = noz::ui::StyleLength::Auto;
		_iconImage->setStyle(iconStyle);

		_canvas->add(_iconImage);
		
		_ui->setRoot(_canvas);
	}
	
	void renderSceneToTexture(noz::renderer::CommandBuffer* cb)
	{
		assert(cb);
		assert(_scene);
		assert(_camera);
		assert(_iconRenderTarget);
				
		cb->beginOpaquePass(_iconRenderTarget, true, Color::Transparent, false);
		_scene->render(cb);
		cb->endOpaquePass();
	}

	std::shared_ptr<Scene> _scene;
	std::shared_ptr<Node3d> _meshObject;
	std::shared_ptr<MeshRenderer> _meshRenderer;
	std::shared_ptr<Camera> _camera;
	std::shared_ptr<DirectionalLight> _light;
	
	// Effects
	std::shared_ptr<Scene> _borderScene;
	std::shared_ptr<Scene> _gammaScene;
	std::shared_ptr<BorderEffect> _borderEffect;
	std::shared_ptr<GammaCorrection> _gammaCorrectionEffect;

	std::shared_ptr<Scene> _ui;
	std::shared_ptr<noz::ui::Canvas> _canvas;
	std::shared_ptr<noz::ui::Image> _iconImage;

	// Render targets
	std::shared_ptr<noz::renderer::Texture> _iconRenderTarget;
	std::shared_ptr<noz::renderer::Texture> _borderRenderTarget;
	std::shared_ptr<noz::renderer::Texture> _finalRenderTarget;
	std::string _meshPath;
	std::string _outputPath;
	
	float _rotationX; // Pitch (around global X-axis)
	float _rotationY; // Yaw (around global Y-axis) 
	float _rotationZ; // Roll (around global Z-axis)
	float _distance;
	float _lightPitch;
	float _lightYaw;
	vec3 _translation; // Translation offset for the mesh
	bool _captureRequested;
	bool _autoCapture;
	bool _shouldExit;
	int _outputSize;
	
	// Preview system
	bool _showBorderedPreview;
	float _lastInputTime;
	
	// Dirty flags for efficient rendering
	bool _sceneNeedsRender;
	bool _borderNeedsUpdate;
	
	// Meta file support
	bool _hasLoadedRotation = false;
	quat _loadedRotation = quat(1, 0, 0, 0);
	
	// Meta file functions
	std::string getMetaFilePath() const
	{
		std::filesystem::path iconPath(_outputPath);
		return iconPath.replace_extension(".meta").string();
	}
	
	bool saveMetaFile() const
	{
		try
		{
			std::string metaPath = getMetaFilePath();
			
			// Read existing meta file first to preserve other fields
			std::unordered_map<std::string, std::string> metaData;
			if (std::filesystem::exists(metaPath))
			{
				auto existingMeta = noz::MetaFile::parse(metaPath);
				metaData = existingMeta.data();
				std::cout << "Updating existing meta file" << std::endl;
			}
			else
			{
				std::cout << "Creating new meta file" << std::endl;
			}
			
			// Update icongen-specific fields
			metaData["icongen.meshPath"] = _meshPath;
			metaData["icongen.iconSize"] = std::to_string(_outputSize);
			metaData["icongen.distance"] = std::to_string(_distance);
			
			// Save translation
			metaData["icongen.translationX"] = std::to_string(_translation.x);
			metaData["icongen.translationY"] = std::to_string(_translation.y);
			metaData["icongen.translationZ"] = std::to_string(_translation.z);
			
			// Save current rotation as quaternion components
			if (_meshObject)
			{
				quat rot = _meshObject->localRotation();
				metaData["icongen.rotationW"] = std::to_string(rot.w);
				metaData["icongen.rotationX"] = std::to_string(rot.x);
				metaData["icongen.rotationY"] = std::to_string(rot.y);
				metaData["icongen.rotationZ"] = std::to_string(rot.z);
			}
			
			// Write meta file in the correct format
			std::ofstream file(metaPath);
			if (file.is_open())
			{
				file << "# Icon generation settings\n";
				
				// Write all entries, sorted by key for consistency
				std::vector<std::pair<std::string, std::string>> sortedData(metaData.begin(), metaData.end());
				std::sort(sortedData.begin(), sortedData.end());
				
				for (const auto& [key, value] : sortedData)
				{
					file << key << ": " << value << "\n";
				}
				
				return true;
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Failed to save meta file: " << e.what() << std::endl;
		}
		return false;
	}
	
	bool loadMetaFile()
	{
		std::string metaPath = getMetaFilePath();
		if (!std::filesystem::exists(metaPath))
		{
			std::cout << "No existing meta file found for this icon" << std::endl;
			return false;
		}
		
		try
		{
			auto meta = noz::MetaFile::parse(metaPath);
			
			// Load icongen settings
			_distance = meta.getFloat("Icon", "distance", _distance);
			
			// Load translation
			_translation.x = meta.getFloat("Icon", "translationX", 0.0f);
			_translation.y = meta.getFloat("Icon", "translationY", 0.0f);
			_translation.z = meta.getFloat("Icon", "translationZ", 0.0f);
					
			// Load rotation if all components are present
			if (meta.hasKey("Icon", "rotationW") && 
			    meta.hasKey("Icon", "rotationX") && 
			    meta.hasKey("Icon", "rotationY") && 
			    meta.hasKey("Icon", "rotationZ"))
			{
				float w = meta.getFloat("Icon", "rotationW");
				float x = meta.getFloat("Icon", "rotationX");
				float y = meta.getFloat("Icon", "rotationY");
				float z = meta.getFloat("Icon", "rotationZ");
				_loadedRotation = quat(w, x, y, z);
				_hasLoadedRotation = true;
			}
			
			std::cout << "Loaded settings from meta file: " << metaPath << std::endl;
			return true;
		}
		catch (const std::exception& e)
		{
			std::cerr << "Failed to load meta file: " << e.what() << std::endl;
		}
		return false;
	}
};

std::string generateOutputPath(const std::string& meshPath, const std::string& outputDir)
{
	// Extract relative path from mesh path (remove resources/models/ prefix)
	std::filesystem::path meshFilePath(meshPath);
	std::string relativePath = meshFilePath.string();
	
	// Remove common prefixes
	if (relativePath.find("assets/models/") == 0)
	{
		relativePath = relativePath.substr(17); // length of "assets/models/"
	}
	else if (relativePath.find("models/") == 0)
	{
		relativePath = relativePath.substr(7); // length of "models/"
	}
	
	// Change extension to .png
	std::filesystem::path outputPath(relativePath);
	outputPath.replace_extension(".png");
	
	// Combine with output directory
	return (std::filesystem::path(outputDir) / outputPath).string();
}

int main(int argc, char* argv[])
{
	// Parse command line arguments
	cxxopts::Options options("icongen", "Generate icon images from 3D models");
	options.add_options()
		("m,mesh", "Mesh file path", cxxopts::value<std::string>())
		("c,config", "Config file path (default: icongen.cfg)", cxxopts::value<std::string>()->default_value("icongen.cfg"))
		("h,help", "Print usage");
	
	options.parse_positional({"mesh"});
	options.positional_help("<mesh_path>");
	
	auto result = options.parse(argc, argv);
	
	if (result.count("help") || !result.count("mesh"))
	{
		std::cout << options.help() << std::endl;
		return 0;
	}
	
	// Load config file first
	std::string configPath = result["config"].as<std::string>();
	icongen::config::loadFromFile(configPath);
	
	// Get mesh path from command line
	std::string meshPath = result["mesh"].as<std::string>();
	
	// Generate output path based on mesh path and config
	std::string outputPath = generateOutputPath(meshPath, icongen::config::outputDir);
	
	std::cout << "Config: " << configPath << std::endl;
	std::cout << "Mesh: " << meshPath << std::endl;
	std::cout << "Output: " << outputPath << std::endl;
	std::cout << "Size: " << icongen::config::defaultSize << "x" << icongen::config::defaultSize << std::endl;
	
	// Get the binary directory using SDL
	const char* basePath = SDL_GetBasePath();
	if (!basePath)
	{
		std::cerr << "Failed to get base path" << std::endl;
		return 1;
	}
	
	// Initialize icon generator
	IconGenerator generator;
	
	if (!generator.initialize(meshPath, outputPath, false))
	{
		std::cerr << "Failed to initialize icon generator" << std::endl;
		Application::unload();
		return 1;
	}

	std::cout << "\nIcon Generator Controls:" << std::endl;
	std::cout << "  W/S: Pitch up/down" << std::endl;
	std::cout << "  A/D: Yaw left/right" << std::endl;
	std::cout << "  Shift+WASD: Translate mesh" << std::endl;
	std::cout << "  Q/E or +/-: Zoom in/out" << std::endl;
	std::cout << "  Space: Save icon to " << outputPath << std::endl;
	std::cout << "  ESC: Exit" << std::endl;

	// Main loop
	float lastTime = SDL_GetTicks() / 1000.0f;
	while (Application::instance()->update())
	{
		generator.update();
		generator.render();
	}

	generator.shutdown();
	
	// Shutdown systems
	Application::unload();
	
	return 0;
}