//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <SDL3_shadercross/SDL_shadercross.h>
#include "../../../src/internal.h"
#include "../props.h"
#include "../shader_reflect.h"

namespace fs = std::filesystem;

static std::string ExtractStage(const std::string& source, const std::string& stage);

static void WriteCompiledShader(
    const std::string& vertex_shader,
    const std::string& fragment_shader,
    const std::string& original_source,
    const Props& meta,
    Stream* output_stream,
    const fs::path& include_dir,
    const std::string& source_path)
{
    // Make sure include directory is absolute
    fs::path absolute_include_dir = fs::absolute(include_dir);
    std::string include_dir_str = absolute_include_dir.string();
    
    // Setup HLSL info for vertex shader
    SDL_ShaderCross_HLSL_Info vertex_info = {};
    vertex_info.source = vertex_shader.c_str();
    vertex_info.name = source_path.c_str();
    vertex_info.entrypoint = "vs";
    vertex_info.include_dir = include_dir_str.c_str();
    vertex_info.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
    vertex_info.enable_debug = false;
    
    // Compile vertex shader to SPIRV
    size_t vertex_spirv_size = 0;
    void* vertex_spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(&vertex_info, &vertex_spirv_size);
    if (!vertex_spirv)
        throw std::runtime_error(std::string("Failed to compile vertex shader: ") + SDL_GetError());

    // Setup HLSL info for fragment shader
    SDL_ShaderCross_HLSL_Info fragment_info = {};
    fragment_info.source = fragment_shader.c_str();
    fragment_info.name = source_path.c_str();
    fragment_info.entrypoint = "ps";
    fragment_info.include_dir = include_dir_str.c_str();
    fragment_info.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;
    fragment_info.enable_debug = false;
    
    // Compile fragment shader to SPIRV
    size_t fragment_spirv_size = 0;
    void* fragment_spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(&fragment_info, &fragment_spirv_size);
    if (!fragment_spirv)
    {
        SDL_free(vertex_spirv);
        throw std::runtime_error(std::string("Failed to compile fragment shader: ") + SDL_GetError());
    }

    // Use SPIRV reflection to get accurate uniform buffer information
    auto reflection = ReflectShaderUniforms(vertex_spirv, vertex_spirv_size, fragment_spirv, fragment_spirv_size);

    // Use reflection data directly
    int vertex_uniform_count = (int)reflection.vertex_buffers.size();
    int fragment_uniform_count = (int)reflection.fragment_buffers.size();
    int sampler_count = reflection.sampler_count;

    // Write asset header
    AssetHeader header = {};
    header.signature = ASSET_SIGNATURE_SHADER;
    header.version = 1;
    header.flags = 0;
    WriteAssetHeader(output_stream, &header);

    WriteI32(output_stream, vertex_uniform_count);
    WriteI32(output_stream, fragment_uniform_count);
    WriteI32(output_stream, sampler_count);

    // Write bytecode sizes and data
    WriteU32(output_stream, (uint32_t)vertex_spirv_size);
    WriteBytes(output_stream, vertex_spirv, vertex_spirv_size);
    WriteU32(output_stream, (uint32_t)fragment_spirv_size);
    WriteBytes(output_stream, fragment_spirv, fragment_spirv_size);

    // Parse shader flags from meta file
    shader_flags_t flags = shader_flags_none;
    if (meta.GetBool("shader", "depth_test", true))
        flags = (shader_flags_t)(flags | shader_flags_depth_test);
    if (meta.GetBool("shader", "depth_write", true))
        flags = (shader_flags_t)(flags | shader_flags_depth_write);
    if (meta.GetBool("shader", "blend_enabled", false))
        flags = (shader_flags_t)(flags | shader_flags_blend);
    
    // Parse blend factors from meta file
    std::string src_blend = meta.GetString("shader", "src_blend_factor", "one");
    SDL_GPUBlendFactor src_blend_factor = SDL_GPU_BLENDFACTOR_ONE;
    if (src_blend == "zero") src_blend_factor = SDL_GPU_BLENDFACTOR_ZERO;
    else if (src_blend == "one") src_blend_factor = SDL_GPU_BLENDFACTOR_ONE;
    else if (src_blend == "src_alpha") src_blend_factor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    else if (src_blend == "one_minus_src_alpha") src_blend_factor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    
    std::string dst_blend = meta.GetString("shader", "dst_blend_factor", "zero");
    SDL_GPUBlendFactor dst_blend_factor = SDL_GPU_BLENDFACTOR_ZERO;
    if (dst_blend == "zero") dst_blend_factor = SDL_GPU_BLENDFACTOR_ZERO;
    else if (dst_blend == "one") dst_blend_factor = SDL_GPU_BLENDFACTOR_ONE;
    else if (dst_blend == "src_alpha") dst_blend_factor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    else if (dst_blend == "one_minus_src_alpha") dst_blend_factor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    
    // Parse cull mode from meta file
    std::string cull = meta.GetString("shader", "cull", "none");
    SDL_GPUCullMode cull_mode = SDL_GPU_CULLMODE_NONE;
    if (cull == "none") cull_mode = SDL_GPU_CULLMODE_NONE;
    else if (cull == "front") cull_mode = SDL_GPU_CULLMODE_FRONT;
    else if (cull == "back") cull_mode = SDL_GPU_CULLMODE_BACK;
    
    WriteU8(output_stream, (uint8_t)flags);
    WriteU32(output_stream, (uint32_t)src_blend_factor);
    WriteU32(output_stream, (uint32_t)dst_blend_factor);
    WriteU32(output_stream, (uint32_t)cull_mode);

    // Write vertex uniform buffer information
    for (const auto& ub : reflection.vertex_buffers)
    {
        WriteU32(output_stream, ub.size);
        WriteU32(output_stream, ub.offset);
    }

    // Write fragment uniform buffer information
    for (const auto& ub : reflection.fragment_buffers)
    {
        WriteU32(output_stream, ub.size);
        WriteU32(output_stream, ub.offset);
    }

    // Clean up
    SDL_free(vertex_spirv);
    SDL_free(fragment_spirv);
}

void ImportShader(const fs::path& source_path, Stream* output_stream, Props* config, Props* meta)
{
    // Read source file
    std::ifstream file(source_path, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("could not read file");

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();

    // Extract each stage and write the shader
    std::string vertex_shader = ExtractStage(source, "VERTEX_SHADER");
    std::string fragment_shader = ExtractStage(source, "FRAGMENT_SHADER");
    fs::path include_dir = source_path.parent_path();
    
    WriteCompiledShader(vertex_shader, fragment_shader, source, *meta, output_stream, include_dir, source_path.string());
}

bool DoesShaderDependOn(const fs::path& source_path, const fs::path& dependency_path)
{
    try
    {
        // Read source file
        std::ifstream file(source_path);
        if (!file.is_open())
            return false;
            
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();
        file.close();
        
        // Simple check for #include references to the dependency file
        std::string dependency_name = dependency_path.filename().string();
        std::string include_pattern = "#include";
        
        size_t pos = 0;
        while ((pos = source.find(include_pattern, pos)) != std::string::npos)
        {
            size_t line_end = source.find('\n', pos);
            if (line_end != std::string::npos)
            {
                std::string line = source.substr(pos, line_end - pos);
                if (line.find(dependency_name) != std::string::npos)
                    return true;
            }
            pos += include_pattern.length();
        }
        
        return false;
    }
    catch (...)
    {
        return false;
    }
}

static std::string ExtractStage(const std::string& source, const std::string& stage)
{
    std::string result = source;
    
    // Convert custom stage directives to #ifdef blocks based on current stage
    // Format: //@ VERTEX ... //@ END and //@ FRAGMENT ... //@ END
    
    // Use [\s\S] instead of . to match newlines (equivalent to dotall)
    std::regex vertex_pattern(R"(//@ VERTEX\s*\n([\s\S]*?)//@ END)");
    std::regex fragment_pattern(R"(//@ FRAGMENT\s*\n([\s\S]*?)//@ END)");
    
    if (stage == "VERTEX_SHADER")
    {
        // Keep vertex shader blocks, remove fragment shader blocks
        result = std::regex_replace(result, vertex_pattern, "$1");
        result = std::regex_replace(result, fragment_pattern, "");
    }
    else if (stage == "FRAGMENT_SHADER")
    {
        // Keep fragment shader blocks, remove vertex shader blocks
        result = std::regex_replace(result, fragment_pattern, "$1");
        result = std::regex_replace(result, vertex_pattern, "");
    }
    
    return result;
}

static uint32_t GetTypeSize(const std::string& type_name)
{
    // HLSL built-in type sizes (in bytes)
    if (type_name == "float" || type_name == "int" || type_name == "uint") return 4;
    if (type_name == "float2" || type_name == "int2" || type_name == "uint2") return 8;
    if (type_name == "float3" || type_name == "int3" || type_name == "uint3") return 12;
    if (type_name == "float4" || type_name == "int4" || type_name == "uint4") return 16;
    if (type_name == "matrix" || type_name == "float4x4") return 64;
    if (type_name == "float3x3") return 36;
    if (type_name == "float2x2") return 16;
    if (type_name == "double") return 8;
    if (type_name == "bool") return 4; // bools are usually padded to 4 bytes in cbuffers
    
    // Default to 4 bytes for unknown types
    return 4;
}

static std::string ReadIncludeFile(const fs::path& include_path)
{
    std::ifstream file(include_path);
    if (!file.is_open())
        return "";
        
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static std::vector<uint32_t> ParseUniformBufferSizes(const std::string& hlsl_source, const fs::path& include_dir, int depth)
{
    std::vector<uint32_t> buffer_sizes;
    
    std::string indent(depth * 2, ' ');
    
    
    Tokenizer tokenizer = {};
    Init(tokenizer, hlsl_source.c_str());
    
    Token token = {};
    bool found_any_cbuffer = false;
    while (NextToken(tokenizer, &token))
    {
        
        // Handle #include statements (# and include will be separate tokens)
        if (IsValue(token, "#"))
        {
            // Look for "include" token next
            if (!NextToken(tokenizer, &token))
                continue;
                
            if (!IsValue(token, "include"))
                continue;
                
            // Get the filename token (should be quoted string)
            if (!NextToken(tokenizer, &token))
                continue;
                
            if (token.type != TOKEN_TYPE_STRING)
                continue;
                
            std::string include_filename(token.value, token.length);
            fs::path include_path = include_dir / include_filename;
            
            std::string include_content = ReadIncludeFile(include_path);
            if (!include_content.empty())
            {
                // Recursively parse included file
                auto included_buffers = ParseUniformBufferSizes(include_content, include_dir, depth + 1);
                buffer_sizes.insert(buffer_sizes.end(), included_buffers.begin(), included_buffers.end());
            }
            continue;
        }
        
        // Look for cbuffer declarations
        if (token.type != TOKEN_TYPE_STRING || !IsValue(token, "cbuffer"))
            continue;

        // Skip cbuffer name and find opening brace
        bool found_brace = false;
        while (NextToken(tokenizer, &token))
        {
            if (token.type == TOKEN_TYPE_EOF)
                break;
            if (IsValue(token, "{"))
            {
                found_brace = true;
                break;
            }
        }
        
        if (!found_brace)
            continue;

        uint32_t buffer_size = 0;
        uint32_t current_offset = 0;
        
        // Parse cbuffer contents
        while (NextToken(tokenizer, &token))
        {
            if (token.type == TOKEN_TYPE_EOF || IsValue(token, "}"))
                break;
                
            if (token.type != TOKEN_TYPE_STRING) // Identifiers are TOKEN_TYPE_STRING
                continue;

            std::string type_name(token.value, token.length);
            
            // Skip variable name
            if (!NextToken(tokenizer, &token))
                break;
            if (token.type != TOKEN_TYPE_STRING) // Identifiers are TOKEN_TYPE_STRING
                continue;
                
            std::string var_name(token.value, token.length);
            std::cout << "    " << type_name << " " << var_name;
            
            uint32_t type_size = GetTypeSize(type_name);
            uint32_t array_size = 1;
            
            // Check for array declaration [size]
            if (NextToken(tokenizer, &token) && IsValue(token, "["))
            {
                // Get array size
                if (NextToken(tokenizer, &token) && token.type == TOKEN_TYPE_NUMBER)
                {
                    std::string size_str(token.value, token.length);
                    array_size = std::stoi(size_str);
                    std::cout << "[" << array_size << "]";
                    
                    // Skip closing bracket
                    NextToken(tokenizer, &token); // Should be "]"
                }
            }
            
            type_size *= array_size;
            std::cout << " = " << type_size << " bytes" << std::endl;
            
            // Apply 16-byte alignment for cbuffer packing rules
            if (current_offset % 16 != 0 && current_offset % 16 + type_size > 16)
            {
                current_offset = (current_offset + 15) & ~15; // Align to 16 bytes
            }
            
            current_offset += type_size;
            buffer_size = current_offset;
            
            // Skip to semicolon
            while (NextToken(tokenizer, &token))
            {
                if (IsValue(token, ";") || IsValue(token, "}") || token.type == TOKEN_TYPE_EOF)
                    break;
            }
        }
        
        // Pad buffer size to 16-byte boundary (cbuffer requirement)
        buffer_size = (buffer_size + 15) & ~15;
        buffer_sizes.push_back(buffer_size > 0 ? buffer_size : 16); // Minimum 16 bytes
    }
    
    return buffer_sizes;
}

static const char* g_shader_extensions[] = {
    ".hlsl",
    nullptr
};

static AssetImporterTraits g_shader_importer_traits = {
    .type_name = "Shader",
    .type = TYPE_SHADER,
    .signature = ASSET_SIGNATURE_SHADER,
    .file_extensions = g_shader_extensions,
    .import_func = ImportShader,
    .does_depend_on = DoesShaderDependOn
};

AssetImporterTraits* GetShaderImporterTraits()
{
    return &g_shader_importer_traits;
}
