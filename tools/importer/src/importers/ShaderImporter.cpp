/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "ShaderImporter.h"
#include <SDL3_shadercross/SDL_shadercross.h>

namespace noz::import
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
        
    void ShaderImporter::import(const std::string& sourcePath, const std::string& outputDir)
    {
        std::filesystem::path source(sourcePath);
        std::filesystem::path output(outputDir);
        auto outputName = source.stem().string() + ".shader";
        auto outputPath = output / outputName;

        auto meta = MetaFile::parse(sourcePath + ".meta");
        
        // Read source shader file
        std::ifstream sourceFile(source);
        if (!sourceFile.is_open())
			throw std::runtime_error("failed to open source file");
            
        std::stringstream sourceStream;
        sourceStream << sourceFile.rdbuf();
        std::string shaderSource = sourceStream.str();
        sourceFile.close();
            
        // Parse vertex shader
        auto vertexShader = parseShader(shaderSource, sourcePath, "VERTEX_SHADER");
		validateVertexShader(vertexShader);

		// Parse fragment shader
        auto fragmentShader = parseShader(shaderSource, sourcePath, "FRAGMENT_SHADER");
		validateFragmentShader(fragmentShader);
            
        // Write the shader
        auto includeDir = std::filesystem::path(sourcePath).parent_path().string();
        noz::StreamWriter writer;
        writeShader(vertexShader, fragmentShader, meta, includeDir, writer);
        writer.writeToFile(outputPath.string());
    }
        
    ShaderImporter::ShaderInfo ShaderImporter::parseShader(const std::string& source, const std::string& sourcePath, const std::string& stage)
    {
		ShaderInfo info = {};
        info.source = preprocessIncludes(preprocessStageDirectives(source, stage), sourcePath);

        // Convert to lowercase for case-insensitive matching
		std::string code = info.source;
        std::transform(code.begin(), code.end(), code.begin(), [](unsigned char c) { return std::tolower(c); });

        // Regex patterns for different resource types
        static std::regex cbufferPattern(R"(cbuffer.*?register\s*\(\s*b(\d+))");
        static std::regex samplerPattern(R"(sampler.*?register\s*\(\s*s(\d+))");

        // Count cbuffers and categorize by binding type
        std::sregex_iterator cbufferIter(code.begin(), code.end(), cbufferPattern);
        std::sregex_iterator cbufferEnd;
        for (; cbufferIter != cbufferEnd; ++cbufferIter)
        {
            auto index = std::stoi((*cbufferIter)[1].str());
            if (index > 15)
                throw std::runtime_error("Invalid vertex uniform buffer index: " + std::to_string(index));

            info.uniformBuffers[index] = true;
        }

        // Count samplers
        std::sregex_iterator samplerIter(code.begin(), code.end(), samplerPattern);
        std::sregex_iterator samplerEnd;
        for (; samplerIter != samplerEnd; ++samplerIter)
        {
            auto index = std::stoi((*samplerIter)[1].str());
            if (index > 15)
                throw std::runtime_error("Invalid sampler buffer index: " + std::to_string(index));

            info.samplers[index] = true;
        }

        return info;
    }
        
    std::string ShaderImporter::preprocessIncludes(const std::string& source, const std::string& sourcePath)
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
        
    std::string ShaderImporter::preprocessStageDirectives(const std::string& source, const std::string& stage)
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
        
    void ShaderImporter::validateVertexShader(const ShaderInfo& shader) const
    {
		auto code = shader.source;
        std::transform(code.begin(), code.end(), code.begin(), [](unsigned char c) { return std::tolower(c); });

        if (code.find("vs(") == std::string::npos)
			throw std::runtime_error("Vertex shader must contain a 'vs' entry point");
    }

    void ShaderImporter::validateFragmentShader(const ShaderInfo& shader) const
    {
        auto code = shader.source;
        std::transform(code.begin(), code.end(), code.begin(), [](unsigned char c) { return std::tolower(c); });

        if (code.find("ps(") == std::string::npos)
            throw std::runtime_error("Fragment shader must contain a 'ps' entry point");
    }

    void ShaderImporter::writeShader(
        const ShaderInfo& vs,
        const ShaderInfo& fs,
        const MetaFile& meta,
        const std::string& includeDir,
        StreamWriter& writer)
    {
        auto flags = noz::renderer::Shader::Flags::None;

        if (meta.getBool("Pipeline", "depth_test", true))
			flags = static_cast<noz::renderer::Shader::Flags>(static_cast<uint8_t>(flags) | static_cast<uint8_t>(noz::renderer::Shader::Flags::DepthTest));

        if (meta.getBool("Pipeline", "depth_write", true))
            flags = static_cast<noz::renderer::Shader::Flags>(static_cast<uint8_t>(flags) | static_cast<uint8_t>(noz::renderer::Shader::Flags::DepthWrite));

        if (meta.getBool("Pipeline", "blend_enabled", true))
            flags = static_cast<noz::renderer::Shader::Flags>(static_cast<uint8_t>(flags) | static_cast<uint8_t>(noz::renderer::Shader::Flags::Blend));
            
        // src_blend_factor
		auto srcBlend = meta.getString("Pipeline", "src_blend_factor", "one");
		SDL_GPUBlendFactor srcBlendFactor = SDL_GPU_BLENDFACTOR_ONE;
		if (srcBlend == "one") srcBlendFactor = SDL_GPU_BLENDFACTOR_ONE;
		else if (srcBlend == "zero") srcBlendFactor = SDL_GPU_BLENDFACTOR_ZERO;
		else if (srcBlend == "src_alpha") srcBlendFactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
		else if (srcBlend == "one_minus_src_alpha") srcBlendFactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            
        // dst_blend_factor
		auto dstBlend = meta.getString("Pipeline", "dst_blend_factor", "zero");
		SDL_GPUBlendFactor dstBlendFactor = SDL_GPU_BLENDFACTOR_ZERO;
		if (dstBlend == "one") dstBlendFactor = SDL_GPU_BLENDFACTOR_ONE;
		else if (dstBlend == "zero") dstBlendFactor = SDL_GPU_BLENDFACTOR_ZERO;
		else if (dstBlend == "src_alpha") dstBlendFactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
		else if (dstBlend == "one_minus_src_alpha") dstBlendFactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            
        // cull_mode
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
        vertexInfo.source = vs.source.c_str();
        vertexInfo.entrypoint = "vs";
		vertexInfo.include_dir = includeDir.c_str();
        vertexInfo.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;

        vertexBytecode = SDL_ShaderCross_CompileSPIRVFromHLSL(&vertexInfo, &vertexBytecodeSize);
        if (!vertexBytecode)
            throw std::runtime_error(SDL_GetError());

        // Compile fragment shader
        SDL_ShaderCross_HLSL_Info fragmentInfo = {};
        fragmentInfo.source = fs.source.c_str();
        fragmentInfo.entrypoint = "ps";
        fragmentInfo.include_dir = includeDir.c_str();
        fragmentInfo.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;

        fragmentBytecode = SDL_ShaderCross_CompileSPIRVFromHLSL(&fragmentInfo, &fragmentBytecodeSize);
        if (!fragmentBytecode)
            throw std::runtime_error(SDL_GetError());

        writer.writeFileSignature("SHDR");
        writer.writeUInt32(1);
        writer.writeBytes((uint8_t*)vertexBytecode, (uint16_t)vertexBytecodeSize);
		writer.writeBytes((uint8_t*)fragmentBytecode, (uint16_t)fragmentBytecodeSize);
            
        // Write resource counts
        writer.writeInt32(
            vs.uniformBuffers[static_cast<int>(renderer::registers::Vertex::User0)] +
            vs.uniformBuffers[static_cast<int>(renderer::registers::Vertex::User1)] +
            vs.uniformBuffers[static_cast<int>(renderer::registers::Vertex::User2)]);
        writer.writeInt32(
            fs.uniformBuffers[static_cast<int>(renderer::registers::Fragment::User0)] +
            fs.uniformBuffers[static_cast<int>(renderer::registers::Fragment::User1)] +
            fs.uniformBuffers[static_cast<int>(renderer::registers::Fragment::User2)]);
        writer.writeInt32(
            fs.samplers[static_cast<int>(renderer::registers::Sampler::User0)] +
            fs.samplers[static_cast<int>(renderer::registers::Sampler::User1)] +
            fs.samplers[static_cast<int>(renderer::registers::Sampler::User2)]);

        // Write pipeline properties
        writer.writeUInt8(static_cast<uint8_t>(flags));
        writer.writeUInt32(srcBlendFactor);
        writer.writeUInt32(dstBlendFactor);
        writer.writeUInt32(cullModeValue);

		SDL_free(vertexBytecode);
		SDL_free(fragmentBytecode);
    }

    std::vector<std::string> ShaderImporter::getSupportedExtensions() const
    {
        return { ".hlsl" };
    }

    std::string ShaderImporter::getName() const
    {
        return "ShaderImporter";
    }
}
