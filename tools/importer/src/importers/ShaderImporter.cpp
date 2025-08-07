/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "ShaderImporter.h"
#include <SDL3_shadercross/SDL_shadercross.h>

namespace noz
{
    namespace import
    {
        ShaderImporter::ShaderImporter(const ImportConfig::ShaderConfig& config)
            : _config(config)
        {
        }
        
        bool ShaderImporter::canImport(const std::string& filePath) const
        {
            std::filesystem::path path(filePath);
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });            
            return extension == ".hlsl";
        }
        
        bool ShaderImporter::import(const std::string& sourcePath, const std::string& outputDir)
        {
            try
            {
                std::filesystem::path source(sourcePath);
                std::filesystem::path output(outputDir);
                
                // Create output filename
                std::string outputName = source.stem().string() + ".shader";
                std::filesystem::path outputPath = output / outputName;
                
                return processShader(sourcePath, outputPath.string());
            }
            catch (const std::exception& e)
            {
                std::cerr << "Shader import error: " << e.what() << std::endl;
            return false;
            }
        }
        
        std::vector<std::string> ShaderImporter::getSupportedExtensions() const
        {
            return {".hlsl" };
        }
        
        std::string ShaderImporter::getName() const
        {
            return "ShaderImporter";
        }
        
        bool ShaderImporter::processShader(const std::string& sourcePath, const std::string& outputPath)
        {
            // Read source shader file
            std::ifstream sourceFile(sourcePath);
            if (!sourceFile.is_open())
                return false;
            
            std::stringstream sourceStream;
            sourceStream << sourceFile.rdbuf();
            std::string shaderSource = sourceStream.str();
            sourceFile.close();
            
            // Preprocess shader for vertex stage (with VERTEX_SHADER define)
            std::string processedVertexShader = preprocessShader(shaderSource, sourcePath, "VERTEX_SHADER");
            
            // Preprocess shader for fragment stage (with FRAGMENT_SHADER define)
            std::string processedFragmentShader = preprocessShader(shaderSource, sourcePath, "FRAGMENT_SHADER");
            
            // Validate shader if enabled
            if (_config.validateShaders)
            {
                if (!validateShader(processedVertexShader) || !validateShader(processedFragmentShader))
                {
                    std::cerr << "Shader validation failed: " << sourcePath << std::endl;
                    return false;
                }
            }
            
            return writeShader(outputPath, processedVertexShader, processedFragmentShader, sourcePath);
        }
        
        std::string ShaderImporter::preprocessShader(const std::string& source, const std::string& sourcePath, const std::string& stage)
        {
            std::string processed = source;
            
            // Handle #include directives
            processed = processIncludes(processed, sourcePath);
            
            // Handle custom stage directives and convert to #ifdef blocks
            processed = processStageDirectives(processed, stage);
            
            // Handle #define directives (add stage define if specified)
            std::string stageDefine = stage.empty() ? "" : "#define " + stage + "\n";
            processed = processDefines(processed, stageDefine);
            
            // Remove comments (optional, for cleaner output)
            processed = removeComments(processed);
            
            return processed;
        }
        
        std::string ShaderImporter::processIncludes(const std::string& source, const std::string& sourcePath)
        {
            std::string result = source;
            std::regex includePattern("#include\\s*[\"<]([^\">]+)[\">]");
            
            std::sregex_iterator iter(result.begin(), result.end(), includePattern);
            std::sregex_iterator end;
            
            // Process includes in reverse order to maintain line numbers
            std::vector<std::pair<std::string, std::string>> includes;
            for (; iter != end; ++iter)
            {
                std::string includePath = (*iter)[1].str();
                std::string fullIncludePath = resolveIncludePath(includePath, sourcePath);
                includes.push_back({(*iter)[0].str(), fullIncludePath});
            }
            
            // Replace includes with their content
            for (const auto& include : includes)
            {
                std::string includeContent = loadIncludeFile(include.second);
                if (!includeContent.empty())
                {
                    // Replace the include directive with the content
                    size_t pos = result.find(include.first);
                    if (pos != std::string::npos)
                    {
                        result.replace(pos, include.first.length(), includeContent);
                    }
                }
            }
            
            return result;
        }
        
        std::string ShaderImporter::processStageDirectives(const std::string& source, const std::string& stage)
        {
            std::string result = source;
            
            // Convert custom stage directives to #ifdef blocks based on current stage
            // Format: //@ VERTEX ... //@ END and //@ FRAGMENT ... //@ END
            
            // Use [\s\S] instead of . to match newlines (equivalent to dotall)
            std::regex vertexPattern(R"(//@ VERTEX\s*\n([\s\S]*?)//@ END)");
            std::regex fragmentPattern(R"(//@ FRAGMENT\s*\n([\s\S]*?)//@ END)");
            
            if (stage == "VERTEX_SHADER")
            {
                // Keep vertex shader blocks, remove fragment shader blocks
                result = std::regex_replace(result, vertexPattern, "$1");
                result = std::regex_replace(result, fragmentPattern, "");
            }
            else if (stage == "FRAGMENT_SHADER")
            {
                // Keep fragment shader blocks, remove vertex shader blocks
                result = std::regex_replace(result, fragmentPattern, "$1");
                result = std::regex_replace(result, vertexPattern, "");
            }
            
            return result;
        }
        
        std::string ShaderImporter::resolveIncludePath(const std::string& includePath, const std::string& sourcePath)
        {
            // First, try relative to source file
            std::filesystem::path sourceDir = std::filesystem::path(sourcePath).parent_path();
            std::filesystem::path relativePath = sourceDir / includePath;
            
            if (std::filesystem::exists(relativePath))
            {
                return relativePath.string();
            }
            
            // Then try include paths from config
            for (const auto& includeDir : _config.includePaths)
            {
                std::filesystem::path configPath = std::filesystem::path(includeDir) / includePath;
                if (std::filesystem::exists(configPath))
                {
                    return configPath.string();
                }
            }
            
            // Fallback to include path as-is
            return includePath;
        }
        
        std::string ShaderImporter::loadIncludeFile(const std::string& includePath)
        {
            std::ifstream includeFile(includePath);
            if (!includeFile.is_open())
            {
                std::cerr << "Failed to load include file: " << includePath << std::endl;
                return "";
            }
            
            std::stringstream content;
            content << includeFile.rdbuf();
            includeFile.close();
            
            return content.str();
        }
        
        std::string ShaderImporter::processDefines(const std::string& source, const std::string& additionalDefines)
        {
            // Prepend any additional defines at the top of the file
            std::string result = additionalDefines + source;
            
            // For now, just return the source with additional defines
            // In the future, this could handle #define directives processing
            return result;
        }
        
        std::string ShaderImporter::removeComments(const std::string& source)
        {
            std::string result = source;
            
            // Remove single-line comments (//)
            std::regex singleLineComment("//.*");
            result = std::regex_replace(result, singleLineComment, "");
            
            // Remove multi-line comments (/* */)
            std::regex multiLineComment("/\\*.*?\\*/");
            result = std::regex_replace(result, multiLineComment, "");
            
            return result;
        }
        
        bool ShaderImporter::validateShader(const std::string& shaderSource)
        {
            // Basic validation - check for common HLSL syntax
            std::string lowerSource = shaderSource;
            std::transform(lowerSource.begin(), lowerSource.end(), lowerSource.begin(), [](unsigned char c) { return std::tolower(c); });
            
            // Check for basic HLSL structure
            bool hasVertexShader = lowerSource.find("vs(") != std::string::npos || 
                                  lowerSource.find("vertexshader") != std::string::npos;
            bool hasPixelShader = lowerSource.find("ps(") != std::string::npos || 
                                 lowerSource.find("pixelshader") != std::string::npos;
            
            // For now, just check if it looks like a valid HLSL file
            bool hasValidStructure = hasVertexShader || hasPixelShader;
            
            if (!hasValidStructure)
            {
                std::cerr << "Shader validation failed: No valid vertex or pixel shader found" << std::endl;
            return false;
            }
            
            return true;
        }
        
        // Helper function to count shader resources in HLSL code
        struct ShaderResourceCounts
        {
            int uniformBuffers = 0;
            int samplers = 0;
            int storageTextures = 0;
            int storageBuffers = 0;
            int vertexUniformBuffers = 0;
            int fragmentUniformBuffers = 0;
        };

        ShaderResourceCounts calcResourceCounts(const std::string& hlslCode)
        {
            ShaderResourceCounts counts;
            std::string code = hlslCode;
            
            // Convert to lowercase for case-insensitive matching
            std::transform(code.begin(), code.end(), code.begin(), [](unsigned char c) { return std::tolower(c); });
            
            // Regex patterns for different resource types
            static std::regex cbufferPattern(R"(cbuffer.*?register\s*\(\s*(vs_b|ps_b|b)(\d+))");
            static std::regex samplerPattern(R"(sampler.*?register\s*\(\s*(ps_s|s)(\d+))");
            static std::regex texturePattern(R"(texture2d.*?register\s*\(\s*(ps_t|t)(\d+))");
            static std::regex storageTexturePattern(R"(rwtexture2d.*?register\s*\(\s*(ps_u|u)(\d+))");
            static std::regex structuredBufferPattern(R"(structuredbuffer.*?register\s*\(\s*(ps_u|u)(\d+))");
            static std::regex rwStructuredBufferPattern(R"(rwstructuredbuffer.*?register\s*\(\s*(ps_u|u)(\d+))");
            
            // Count cbuffers and categorize by binding type
            std::sregex_iterator cbufferIter(code.begin(), code.end(), cbufferPattern);
            std::sregex_iterator cbufferEnd;
            for (; cbufferIter != cbufferEnd; ++cbufferIter)
            {
                counts.uniformBuffers++;
                std::string bindingType = (*cbufferIter)[1].str();
                
                if (bindingType == "vs_b")
                {
                    counts.vertexUniformBuffers++;
                }
                else if (bindingType == "ps_b")
                {
                    counts.fragmentUniformBuffers++;
                }
                else if (bindingType == "b")
                {
                    // Generic binding - count for both vertex and fragment
                    counts.vertexUniformBuffers++;
                    counts.fragmentUniformBuffers++;
                }
            }
            
            // Count samplers
            std::sregex_iterator samplerIter(code.begin(), code.end(), samplerPattern);
            std::sregex_iterator samplerEnd;
            for (; samplerIter != samplerEnd; ++samplerIter)
                counts.samplers++;
            
            // Count textures (non-storage)
            std::sregex_iterator textureIter(code.begin(), code.end(), texturePattern);
            std::sregex_iterator textureEnd;
            for (; textureIter != textureEnd; ++textureIter)
            {
                // Don't count as storage texture
            }
            
            // Count storage textures
            std::sregex_iterator storageTextureIter(code.begin(), code.end(), storageTexturePattern);
            std::sregex_iterator storageTextureEnd;
            for (; storageTextureIter != storageTextureEnd; ++storageTextureIter)
            {
                counts.storageTextures++;
            }
            
            // Count structured buffers
            std::sregex_iterator structuredBufferIter(code.begin(), code.end(), structuredBufferPattern);
            std::sregex_iterator structuredBufferEnd;
            for (; structuredBufferIter != structuredBufferEnd; ++structuredBufferIter)
            {
                counts.storageBuffers++;
            }
            
            // Count RW structured buffers
            std::sregex_iterator rwStructuredBufferIter(code.begin(), code.end(), rwStructuredBufferPattern);
            std::sregex_iterator rwStructuredBufferEnd;
            for (; rwStructuredBufferIter != rwStructuredBufferEnd; ++rwStructuredBufferIter)
            {
                counts.storageBuffers++;
            }
            
            return counts;
        }

        bool ShaderImporter::writeShader(const std::string& outputPath, const std::string& vertexShader, const std::string& fragmentShader, const std::string& sourcePath)
        {
            // Calculate resource counts from both shaders
            auto vertexResourceCounts = calcResourceCounts(vertexShader);
            auto fragmentResourceCounts = calcResourceCounts(fragmentShader);            
            
            // Combine resource counts
            ShaderResourceCounts resourceCounts;
            resourceCounts.uniformBuffers = vertexResourceCounts.uniformBuffers + fragmentResourceCounts.uniformBuffers;
            resourceCounts.samplers = fragmentResourceCounts.samplers; // Usually only in fragment
            resourceCounts.storageTextures = fragmentResourceCounts.storageTextures;
            resourceCounts.storageBuffers = fragmentResourceCounts.storageBuffers;
            resourceCounts.vertexUniformBuffers = vertexResourceCounts.vertexUniformBuffers;
            resourceCounts.fragmentUniformBuffers = fragmentResourceCounts.fragmentUniformBuffers;

			auto meta = noz::MetaFile::parse(sourcePath + ".meta");
            auto depthTest = meta.getBool("Pipeline", "depth_test", true);
			auto depthWrite = meta.getBool("Pipeline", "depth_write", true);
			auto blendEnabled = meta.getBool("Pipeline", "blend_enabled", false);            
            
			auto srcBlend = meta.getString("Pipeline", "src_blend_factor", "one");
			SDL_GPUBlendFactor srcBlendFactor = SDL_GPU_BLENDFACTOR_ONE;
			if (srcBlend == "one") srcBlendFactor = SDL_GPU_BLENDFACTOR_ONE;
			else if (srcBlend == "zero") srcBlendFactor = SDL_GPU_BLENDFACTOR_ZERO;
			else if (srcBlend == "src_alpha") srcBlendFactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
			else if (srcBlend == "one_minus_src_alpha") srcBlendFactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            
			auto dstBlend = meta.getString("Pipeline", "dst_blend_factor", "zero");
			SDL_GPUBlendFactor dstBlendFactor = SDL_GPU_BLENDFACTOR_ZERO;
			if (dstBlend == "one") dstBlendFactor = SDL_GPU_BLENDFACTOR_ONE;
			else if (dstBlend == "zero") dstBlendFactor = SDL_GPU_BLENDFACTOR_ZERO;
			else if (dstBlend == "src_alpha") dstBlendFactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
			else if (dstBlend == "one_minus_src_alpha") dstBlendFactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            
            auto cullMode = meta.getString("Pipeline", "cull_mode", "none");
			SDL_GPUCullMode cullModeValue = SDL_GPU_CULLMODE_NONE;
			if (cullMode == "none") cullModeValue = SDL_GPU_CULLMODE_NONE;
			else if (cullMode == "front") cullModeValue = SDL_GPU_CULLMODE_FRONT;
			else if (cullMode == "back") cullModeValue = SDL_GPU_CULLMODE_BACK;

            // Compile shaders to SPIR-V
            void* vertexBytecode = nullptr;
            size_t vertexBytecodeSize = 0;
            void* fragmentBytecode = nullptr;
            size_t fragmentBytecodeSize = 0;

            // Compile vertex shader
            SDL_ShaderCross_HLSL_Info vertexInfo = {};
            vertexInfo.source = vertexShader.c_str();
            vertexInfo.entrypoint = "vs";
            vertexInfo.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;

            vertexBytecode = SDL_ShaderCross_CompileSPIRVFromHLSL(&vertexInfo, &vertexBytecodeSize);
            if (!vertexBytecode)
            {
                std::cerr << "Failed to compile vertex shader: " << SDL_GetError() << std::endl;
                return false;
            }

            // Compile fragment shader
            SDL_ShaderCross_HLSL_Info fragmentInfo = {};
            fragmentInfo.source = fragmentShader.c_str();
            fragmentInfo.entrypoint = "ps";
            fragmentInfo.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;

            fragmentBytecode = SDL_ShaderCross_CompileSPIRVFromHLSL(&fragmentInfo, &fragmentBytecodeSize);
            if (!fragmentBytecode)
            {
                std::cerr << "Failed to compile fragment shader: " << SDL_GetError() << std::endl;
                SDL_free(vertexBytecode);
                return false;
            }

            noz::StreamWriter writer;
            writer.writeFileSignature("SHDR");
            writer.writeUInt32(1);
            writer.writeBytes((uint8_t*)vertexBytecode, (uint16_t)vertexBytecodeSize);
			writer.writeBytes((uint8_t*)fragmentBytecode, (uint16_t)fragmentBytecodeSize);
            
            // Write resource counts
            writer.writeInt32(resourceCounts.vertexUniformBuffers);
            writer.writeInt32(resourceCounts.fragmentUniformBuffers);
            writer.writeInt32(resourceCounts.samplers);
            writer.writeInt32(resourceCounts.storageTextures);
            writer.writeInt32(resourceCounts.storageBuffers);
            
            // Write pipeline properties
            writer.writeBool(depthTest);
            writer.writeBool(depthWrite);
            writer.writeBool(blendEnabled);
            writer.writeUInt32(srcBlendFactor);
            writer.writeUInt32(dstBlendFactor);
            writer.writeUInt32(cullModeValue);

			SDL_free(vertexBytecode);
			SDL_free(fragmentBytecode);

            if (!writer.writeToFile(outputPath))
            {
                std::cerr << "Failed to write shader file: " << outputPath << std::endl;
				return false;
            }
            
            return true;
        }
    }
} 