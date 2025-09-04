//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <glslang_c_interface.h>
#include "../../../src/internal.h"
#include "../props.h"

namespace fs = std::filesystem;

static std::string ExtractStage(const std::string& source, const std::string& stage);
static std::vector<uint32_t> CompileGLSLToSPIRV(const std::string& source, glslang_stage_t stage, const std::string& filename);

static void WriteCompiledShader(
    const std::string& vertex_shader,
    const std::string& fragment_shader,
    const std::string& original_source,
    const Props& meta,
    Stream* output_stream,
    const fs::path& include_dir,
    const std::string& source_path)
{
    // Compile GLSL shaders to SPIR-V using glslang
    std::vector<uint32_t> vertex_spirv = CompileGLSLToSPIRV(vertex_shader, GLSLANG_STAGE_VERTEX, source_path + ".vert");
    if (vertex_spirv.empty())
        throw std::runtime_error("Failed to compile vertex shader");

    std::vector<uint32_t> fragment_spirv = CompileGLSLToSPIRV(fragment_shader, GLSLANG_STAGE_FRAGMENT, source_path + ".frag");
    if (fragment_spirv.empty())
        throw std::runtime_error("Failed to compile fragment shader");

    // Hardcoded uniform buffer layout (no reflection needed)
    int vertex_uniform_count = 2;    // Camera + Transform
    int fragment_uniform_count = 1;  // Color
    int sampler_count = 1;           // Texture sampler

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
    WriteU32(output_stream, (uint32_t)(vertex_spirv.size() * sizeof(uint32_t)));
    WriteBytes(output_stream, vertex_spirv.data(), vertex_spirv.size() * sizeof(uint32_t));
    WriteU32(output_stream, (uint32_t)(fragment_spirv.size() * sizeof(uint32_t)));
    WriteBytes(output_stream, fragment_spirv.data(), fragment_spirv.size() * sizeof(uint32_t));

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
    uint32_t src_blend_factor = 1; // One
    if (src_blend == "zero") src_blend_factor = 0;
    else if (src_blend == "one") src_blend_factor = 1;
    else if (src_blend == "src_alpha") src_blend_factor = 5;
    else if (src_blend == "one_minus_src_alpha") src_blend_factor = 6;
    
    std::string dst_blend = meta.GetString("shader", "dst_blend_factor", "zero");
    uint32_t dst_blend_factor = 0; // Zero
    if (dst_blend == "zero") dst_blend_factor = 0;
    else if (dst_blend == "one") dst_blend_factor = 1;
    else if (dst_blend == "src_alpha") dst_blend_factor = 5;
    else if (dst_blend == "one_minus_src_alpha") dst_blend_factor = 6;
    
    // Parse cull mode from meta file
    std::string cull = meta.GetString("shader", "cull", "none");
    uint32_t cull_mode = 0; // None
    if (cull == "none") cull_mode = 0;
    else if (cull == "front") cull_mode = 1;
    else if (cull == "back") cull_mode = 2;
    
    WriteU8(output_stream, (uint8_t)flags);
    WriteU32(output_stream, (uint32_t)src_blend_factor);
    WriteU32(output_stream, (uint32_t)dst_blend_factor);
    WriteU32(output_stream, (uint32_t)cull_mode);

    // Write hardcoded vertex uniform buffer information
    // Camera uniform buffer
    WriteU32(output_stream, 128);  // size (mat4 view + mat4 projection = 64+64)
    WriteU32(output_stream, 0);    // offset
    
    // Transform uniform buffer  
    WriteU32(output_stream, 64);   // size (mat4 model = 64)
    WriteU32(output_stream, 128);  // offset

    // Write hardcoded fragment uniform buffer information
    // Color uniform buffer
    WriteU32(output_stream, 16);   // size (vec4 color = 16)
    WriteU32(output_stream, 192);  // offset

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
    std::string ext = dependency_path.extension().string();
    if (ext != ".glsl" && ext != ".vert" && ext != ".frag")
        return false;

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
    ".glsl",
    ".vert",
    ".frag",
    nullptr
};

static AssetImporterTraits g_shader_importer_traits = {
    .type_name = "Shader",
    .signature = ASSET_SIGNATURE_SHADER,
    .file_extensions = g_shader_extensions,
    .import_func = ImportShader,
    .does_depend_on = DoesShaderDependOn
};

AssetImporterTraits* GetShaderImporterTraits()
{
    return &g_shader_importer_traits;
}

static std::vector<uint32_t> CompileGLSLToSPIRV(const std::string& source, glslang_stage_t stage, const std::string& filename)
{
    // Initialize glslang if not already done
    static bool initialized = false;
    if (!initialized) {
        glslang_initialize_process();
        initialized = true;
    }

    // Create default resource limits
    static glslang_resource_t resource = {
        .max_lights = 32,
        .max_clip_planes = 6,
        .max_texture_units = 32,
        .max_texture_coords = 32,
        .max_vertex_attribs = 64,
        .max_vertex_uniform_components = 4096,
        .max_varying_floats = 64,
        .max_vertex_texture_image_units = 32,
        .max_combined_texture_image_units = 80,
        .max_texture_image_units = 32,
        .max_fragment_uniform_components = 4096,
        .max_draw_buffers = 32,
        .max_vertex_uniform_vectors = 128,
        .max_varying_vectors = 8,
        .max_fragment_uniform_vectors = 16,
        .max_vertex_output_vectors = 16,
        .max_fragment_input_vectors = 15,
        .min_program_texel_offset = -8,
        .max_program_texel_offset = 7,
        .max_clip_distances = 8,
        .max_compute_work_group_count_x = 65535,
        .max_compute_work_group_count_y = 65535,
        .max_compute_work_group_count_z = 65535,
        .max_compute_work_group_size_x = 1024,
        .max_compute_work_group_size_y = 1024,
        .max_compute_work_group_size_z = 64,
        .max_compute_uniform_components = 1024,
        .max_compute_texture_image_units = 16,
        .max_compute_image_uniforms = 8,
        .max_compute_atomic_counters = 8,
        .max_compute_atomic_counter_buffers = 1,
        .max_varying_components = 60,
        .max_vertex_output_components = 64,
        .max_geometry_input_components = 64,
        .max_geometry_output_components = 128,
        .max_fragment_input_components = 128,
        .max_image_units = 8,
        .max_combined_image_units_and_fragment_outputs = 8,
        .max_combined_shader_output_resources = 8,
        .max_image_samples = 0,
        .max_vertex_image_uniforms = 0,
        .max_tess_control_image_uniforms = 0,
        .max_tess_evaluation_image_uniforms = 0,
        .max_geometry_image_uniforms = 0,
        .max_fragment_image_uniforms = 8,
        .max_combined_image_uniforms = 8,
        .max_geometry_texture_image_units = 16,
        .max_geometry_output_vertices = 256,
        .max_geometry_total_output_components = 1024,
        .max_geometry_uniform_components = 1024,
        .max_geometry_varying_components = 64,
        .max_tess_control_input_components = 128,
        .max_tess_control_output_components = 128,
        .max_tess_control_texture_image_units = 16,
        .max_tess_control_uniform_components = 1024,
        .max_tess_control_total_output_components = 4096,
        .max_tess_evaluation_input_components = 128,
        .max_tess_evaluation_output_components = 128,
        .max_tess_evaluation_texture_image_units = 16,
        .max_tess_evaluation_uniform_components = 1024,
        .max_tess_patch_components = 120,
        .max_patch_vertices = 32,
        .max_tess_gen_level = 64,
        .max_viewports = 16,
        .max_vertex_atomic_counters = 0,
        .max_tess_control_atomic_counters = 0,
        .max_tess_evaluation_atomic_counters = 0,
        .max_geometry_atomic_counters = 0,
        .max_fragment_atomic_counters = 8,
        .max_combined_atomic_counters = 8,
        .max_atomic_counter_bindings = 1,
        .max_vertex_atomic_counter_buffers = 0,
        .max_tess_control_atomic_counter_buffers = 0,
        .max_tess_evaluation_atomic_counter_buffers = 0,
        .max_geometry_atomic_counter_buffers = 0,
        .max_fragment_atomic_counter_buffers = 1,
        .max_combined_atomic_counter_buffers = 1,
        .max_atomic_counter_buffer_size = 16384,
        .max_transform_feedback_buffers = 4,
        .max_transform_feedback_interleaved_components = 64,
        .max_cull_distances = 8,
        .max_combined_clip_and_cull_distances = 8,
        .max_samples = 4,
        .max_mesh_output_vertices_nv = 256,
        .max_mesh_output_primitives_nv = 512,
        .max_mesh_work_group_size_x_nv = 32,
        .max_mesh_work_group_size_y_nv = 1,
        .max_mesh_work_group_size_z_nv = 1,
        .max_task_work_group_size_x_nv = 32,
        .max_task_work_group_size_y_nv = 1,
        .max_task_work_group_size_z_nv = 1,
        .max_mesh_view_count_nv = 4,
        .max_dual_source_draw_buffers_ext = 1,
        .limits = {
            .non_inductive_for_loops = true,
            .while_loops = true,
            .do_while_loops = true,
            .general_uniform_indexing = true,
            .general_attribute_matrix_vector_indexing = true,
            .general_varying_indexing = true,
            .general_sampler_indexing = true,
            .general_variable_indexing = true,
            .general_constant_matrix_vector_indexing = true,
        }
    };

    // Create input
    glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = stage,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_0,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_0,
        .code = source.c_str(),
        .default_version = 450,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = &resource,
    };

    // Create shader and parse
    glslang_shader_t* shader = glslang_shader_create(&input);
    if (!glslang_shader_preprocess(shader, &input)) {
        printf("GLSL preprocessing failed for %s:\n%s\n", filename.c_str(), glslang_shader_get_info_log(shader));
        glslang_shader_delete(shader);
        return {};
    }

    if (!glslang_shader_parse(shader, &input)) {
        printf("GLSL parsing failed for %s:\n%s\n", filename.c_str(), glslang_shader_get_info_log(shader));
        glslang_shader_delete(shader);
        return {};
    }

    // Create program and link
    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);

    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
        printf("GLSL linking failed for %s:\n%s\n", filename.c_str(), glslang_program_get_info_log(program));
        glslang_program_delete(program);
        glslang_shader_delete(shader);
        return {};
    }

    // Generate SPIR-V
    glslang_program_SPIRV_generate(program, stage);
    
    if (glslang_program_SPIRV_get_messages(program)) {
        printf("SPIR-V generation messages for %s:\n%s\n", filename.c_str(), glslang_program_SPIRV_get_messages(program));
    }

    // Get SPIR-V data
    size_t spirv_size = glslang_program_SPIRV_get_size(program);
    const uint32_t* spirv_data = glslang_program_SPIRV_get_ptr(program);
    
    std::vector<uint32_t> spirv(spirv_data, spirv_data + spirv_size);

    // Cleanup
    glslang_program_delete(program);
    glslang_shader_delete(shader);

    return spirv;
}
