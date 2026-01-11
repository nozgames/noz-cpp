//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {

    static void InitVfxDocument(VfxDocument* vdoc);

    static void CloneVfxDocument(Document* doc) {
        assert(doc->def->type == ASSET_TYPE_VFX);
        VfxDocument* v = static_cast<VfxDocument*>(doc);
        v->vfx = nullptr;
        v->handle = INVALID_VFX_HANDLE;
    }

    static void DestroyVfxDocument(Document* doc) {
        VfxDocument* v = static_cast<VfxDocument*>(doc);
        Stop(v->handle);
        Free(v->vfx);
        v->vfx = nullptr;
    }

    static void DrawVfXDocument(Document* doc) {
        VfxDocument* vdoc = static_cast<VfxDocument*>(doc);
        assert(vdoc);
        assert(vdoc->def->type == ASSET_TYPE_VFX);

        if (!vdoc->playing || vdoc->emitter_count == 0 ) {
            BindMaterial(g_workspace.shaded_material);
            BindColor(COLOR_WHITE);
            DrawMesh(SPRITE_ASSET_ICON_VFX, Translate(doc->position));
            return;
        }

        if (!IsPlaying(vdoc->handle) && vdoc->vfx)
            vdoc->handle = Play(vdoc->vfx, doc->position);
    }

    static bool ParseCurveType(Tokenizer& tk, VfxCurveType* curve_type) {
        if (!ExpectIdentifier(tk))
            return false;

        *curve_type = VFX_CURVE_TYPE_UNKNOWN;

        if (Equals(tk, "linear", true))
            *curve_type = VFX_CURVE_TYPE_LINEAR;
        else if (Equals(tk, "easein", true))
            *curve_type = VFX_CURVE_TYPE_EASE_IN;
        else if (Equals(tk, "easeout"))
            *curve_type = VFX_CURVE_TYPE_EASE_OUT;
        else if (Equals(tk, "easeinout"))
            *curve_type = VFX_CURVE_TYPE_EASE_IN_OUT;
        else if (Equals(tk, "quadratic"))
            *curve_type = VFX_CURVE_TYPE_QUADRATIC;
        else if (Equals(tk, "cubic"))
            *curve_type = VFX_CURVE_TYPE_CUBIC;
        else if (Equals(tk, "sine"))
            *curve_type = VFX_CURVE_TYPE_SINE;

        return *curve_type != VFX_CURVE_TYPE_UNKNOWN;
    }

    static bool ParseVec2(Tokenizer& tk, VfxVec2* value)
    {
        // Non range
        if (!ExpectDelimiter(tk, '['))
        {
            Vec2 v;
            if (!ExpectVec2(tk, &v))
                return false;

            *value = {v, v};
            return true;
        }

        // Range
        Vec2 min = {0,0};
        if (!ExpectVec2(tk, &min))
            return false;

        if (!ExpectDelimiter(tk, ','))
            return false;

        Vec2 max = {0,0};
        if (!ExpectVec2(tk, &max))
            return false;

        if (!ExpectDelimiter(tk, ']'))
            return false;

        *value = { Min(min,max), Max(min,max) };
        return true;
    }

    static VfxVec2 ParseVec2(const std::string& str, const VfxVec2& default_value)
    {
        if (str.empty())
            return default_value;

        Tokenizer tk;
        Init(tk, str.c_str());
        VfxVec2 value = {};
        if (!ParseVec2(tk, &value))
            return default_value;
        return value;
    }

    static bool ParseFloat(Tokenizer& tk, VfxFloat* value)
    {
        // Non range
        if (!ExpectDelimiter(tk, '['))
        {
            float v;
            if (!ExpectFloat(tk, &v))
                return false;

            *value = {v, v};
            return true;
        }

        // Range
        float min = 0.0f;
        if (!ExpectFloat(tk, &min))
            return false;

        if (!ExpectDelimiter(tk, ','))
            return false;

        float max = 0.0f;
        if (!ExpectFloat(tk, &max))
            return false;

        if (!ExpectDelimiter(tk, ']'))
            return false;

        *value = { Min(min,max), Max(min,max) };
        return true;
    }

    static VfxFloat ParseFloat(const std::string& value, VfxFloat default_value)
    {
        if (value.empty())
            return default_value;

        Tokenizer tk;
        Init(tk, value.c_str());
        ParseFloat(tk, &default_value);
        return default_value;
    }

    static VfxFloatCurve ParseFloatCurve(const std::string& str, VfxFloatCurve default_value)
    {
        Tokenizer tk;
        Init(tk, str.c_str());

        VfxFloatCurve value = { VFX_CURVE_TYPE_LINEAR };
        if (!ParseFloat(tk, &value.start))
            return default_value;

        if (!ExpectDelimiter(tk, '='))
        {
            value.end = value.start;
            return value;
        }

        if (!ExpectDelimiter(tk, '>'))
            return default_value;

        if (!ParseFloat(tk, &value.end))
            return default_value;

        if (!ExpectDelimiter(tk, ':'))
            return value;

        VfxCurveType curve_type = VFX_CURVE_TYPE_UNKNOWN;
        if (!ParseCurveType(tk, &curve_type))
            return default_value;

        value.type = curve_type;
        return value;
    }

    static VfxInt ParseInt(const std::string& value, VfxInt default_value)
    {
        if (value.empty())
            return default_value;

        Tokenizer tk;
        Init(tk, value.c_str());

        // Single int?
        if (!ExpectDelimiter(tk,'['))
        {
            i32 ivalue = 0;
            if (!ExpectInt(tk, &ivalue))
                return default_value;

            return { ivalue, ivalue };
        }

        // Range
        i32 min = 0;
        if (!ExpectInt(tk, &min))
            return default_value;

        if (!ExpectDelimiter(tk, ','))
            return default_value;

        i32 max = 0;
        if (!ExpectInt(tk, &max))
            return default_value;

        if (!ExpectDelimiter(tk, ']'))
            return default_value;

        return { Min(min,max), Max(min,max) };
    }

    static bool ParseColor(Tokenizer& tk, VfxColor* value)
    {
        if (!ExpectDelimiter(tk, '['))
        {
            Color cvalue = {1.0f, 1.0f, 1.0f, 1.0f};
            if (!ExpectColor(tk, &cvalue))
                return false;

            *value = {cvalue, cvalue};
            return true;
        }

        // Range
        Color min = {0,0,0,0};
        if (!ExpectColor(tk, &min))
            return false;

        if (!ExpectDelimiter(tk, ','))
            return false;

        Color max = {0,0,0,0};
        if (!ExpectColor(tk, &max))
            return false;

        if (!ExpectDelimiter(tk, ']'))
            return false;

        *value = { min, max };
        return true;
    }

    static VfxColorCurve ParseColorCurve(const std::string& str, const VfxColorCurve& default_value)
    {
        Tokenizer tk;
        Init(tk, str.c_str());

        VfxColorCurve value = { VFX_CURVE_TYPE_LINEAR };
        if (!ParseColor(tk, &value.start))
            return default_value;

        if (!ExpectDelimiter(tk, '='))
        {
            value.end = value.start;
            return value;
        }

        if (!ExpectDelimiter(tk, '>'))
            return default_value;

        if (!ParseColor(tk, &value.end))
            return default_value;

        if (!ExpectDelimiter(tk, ':'))
            return value;

        VfxCurveType curve_type = VFX_CURVE_TYPE_UNKNOWN;
        if (!ParseCurveType(tk, &curve_type))
            return default_value;

        value.type = curve_type;
        return value;
    }

    static Bounds2 CalculateBounds(VfxDocument* vdoc) {
        Bounds2 bounds = {-VEC2_ONE * 0.5f, VEC2_ONE * 0.5f};

        for (int i=0, c=vdoc->emitter_count; i<c; i++) {
            const VfxEmitterDef& e = vdoc->emitters[i].def;
            const VfxParticleDef& p = e.particle_def;
            Bounds2 eb = { e.spawn.min, e.spawn.max };
            float ssmax = Max(p.size.start.min,p.size.start.max);
            float semax = Max(p.size.end.min,p.size.end.max);
            float smax = Max(ssmax, semax);
            eb = Expand(eb, smax);

            float speed_max = Max(p.speed.start.max, p.speed.end.max);
            float duration_max = p.duration.max;
            eb = Expand(eb, speed_max * duration_max);

            if (i == 0)
                bounds = eb;
            else
                bounds = Union(bounds, eb);
        }

        return bounds;
    }

    static void Serialize(VfxDocument* vdoc, Stream* stream) {
        AssetHeader header = {};
        header.signature = ASSET_SIGNATURE;
        header.type = ASSET_TYPE_VFX;
        header.version = 1;
        header.flags = 0;
        WriteAssetHeader(stream, &header);

        WriteStruct<Bounds2>(stream, CalculateBounds(vdoc));
        WriteStruct(stream, vdoc->duration);
        WriteBool(stream, vdoc->loop);
        WriteU32(stream, vdoc->emitter_count);

        for (int i=0, c=vdoc->emitter_count; i<c; i++) {
            const EditorVfxEmitter& emitter = vdoc->emitters[i];
            WriteStruct(stream, emitter.def.rate);
            WriteStruct(stream, emitter.def.burst);
            WriteStruct(stream, emitter.def.duration);
            WriteStruct(stream, emitter.def.angle);
            WriteStruct(stream, emitter.def.spawn);
            WriteStruct(stream, emitter.def.direction);

            const VfxParticleDef& particle = emitter.def.particle_def;
            WriteStruct(stream, particle.duration);
            WriteStruct(stream, particle.size);
            WriteStruct(stream, particle.speed);
            WriteStruct(stream, particle.color);
            WriteStruct(stream, particle.opacity);
            WriteStruct(stream, particle.gravity);
            WriteStruct(stream, particle.drag);
            WriteStruct(stream, particle.rotation);
            WriteName(stream, particle.mesh_name);
        }
    }

    Vfx* ToVfx(Allocator* allocator, VfxDocument* v, const Name* name) {
        Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 8192);
        if (!stream)
            return nullptr;

        Serialize(v, stream);
        SeekBegin(stream, 0);

        Vfx* vfx = (Vfx*)LoadAssetInternal(allocator, name, ASSET_TYPE_VFX, LoadVfx, stream);
        Free(stream);

        return vfx;
    }

    static void LoadVfxDocument(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_VFX);
        VfxDocument* vdoc = static_cast<VfxDocument*>(doc);

        Stream* input_stream = LoadStream(ALLOCATOR_DEFAULT, doc->path.value);
        if (!input_stream)
            throw std::runtime_error("could not read file");

        Props* source = Props::Load(input_stream);
        if (!source) {
            Free(input_stream);
            throw std::runtime_error("could not load source file");
        }

        vdoc->emitter_count = 0;
        vdoc->duration = ParseFloat(source->GetString("VFX", "duration", "5.0"), {5,5});
        vdoc->loop = source->GetBool("vfx", "loop", false);

        auto emitter_names = source->GetKeys("emitters");
        for (const auto& emitter_name : emitter_names) {
            if (!source->HasGroup(emitter_name.c_str()))
                throw std::exception((std::string("missing emitter ") + emitter_name).c_str());

            std::string particle_section = source->GetString(emitter_name.c_str(), "particle", "");
            if (particle_section.empty())
                particle_section = emitter_name + ".particle";

            if (!source->HasGroup(particle_section.c_str()))
                throw std::exception((std::string("missing particle ") + particle_section).c_str());

            // Emitter
            EditorVfxEmitter& emitter = vdoc->emitters[vdoc->emitter_count++];;
            emitter.name = GetName(emitter_name.c_str());
            emitter.def.rate = ParseInt(source->GetString(emitter_name.c_str(), "rate", "0"), VFX_INT_ZERO);
            emitter.def.burst = ParseInt(source->GetString(emitter_name.c_str(), "burst", "0"), VFX_INT_ZERO);
            emitter.def.duration = ParseFloat(source->GetString(emitter_name.c_str(), "duration", "1.0"), VFX_FLOAT_ONE);
            emitter.def.angle = ParseFloat(source->GetString(emitter_name.c_str(), "angle", "0..360"), {0.0f, 360.0f});
            //emitter.def.radius = ParseFloat(source->GetString(emitter_name.c_str(), "radius", "0"), VFX_FLOAT_ZERO);
            emitter.def.spawn = ParseVec2(source->GetString(emitter_name.c_str(), "spawn", "(0, 0)"), VFX_VEC2_ZERO);
            emitter.def.direction = ParseVec2(source->GetString(emitter_name.c_str(), "direction", "(0, 0)"), VFX_VEC2_ZERO);

            // Particle
            emitter.def.particle_def.duration = ParseFloat(source->GetString(particle_section.c_str(), "duration", "1.0"), VFX_FLOAT_ONE);
            emitter.def.particle_def.size = ParseFloatCurve(source->GetString(particle_section.c_str(), "size", "1.0"), VFX_FLOAT_CURVE_ONE);
            emitter.def.particle_def.speed = ParseFloatCurve(source->GetString(particle_section.c_str(), "speed", "0"), VFX_FLOAT_CURVE_ZERO);
            emitter.def.particle_def.color = ParseColorCurve(source->GetString(particle_section.c_str(), "color", "white"), VFX_COLOR_CURVE_WHITE);
            emitter.def.particle_def.opacity = ParseFloatCurve(source->GetString(particle_section.c_str(), "opacity", "1.0"), VFX_FLOAT_CURVE_ONE);
            emitter.def.particle_def.gravity = ParseVec2(source->GetString(particle_section.c_str(), "gravity", "(0, 0, 0)"), VFX_VEC2_ZERO);
            emitter.def.particle_def.drag = ParseFloat(source->GetString(particle_section.c_str(), "drag", "0"), VFX_FLOAT_ZERO);
            emitter.def.particle_def.rotation = ParseFloatCurve(source->GetString(particle_section.c_str(), "rotation", "0.0"), VFX_FLOAT_CURVE_ZERO);

            std::string mesh_name = source->GetString(particle_section.c_str(), "mesh", "");
            if (mesh_name != "")
                emitter.def.particle_def.mesh_name = GetName(mesh_name.c_str());
        }

        vdoc->vfx = ToVfx(ALLOCATOR_DEFAULT, vdoc, vdoc->name);
        vdoc->bounds = GetBounds(vdoc->vfx);
    }

    static VfxDocument* LoadVfxDocument(const std::filesystem::path& path) {
        std::string contents = ReadAllText(ALLOCATOR_DEFAULT, path);
        Tokenizer tk;
        Init(tk, contents.c_str());

        VfxDocument* v = static_cast<VfxDocument*>(CreateDocument(path));
        assert(v);
        InitVfxDocument(v);
        LoadVfxDocument(v);
        MarkModified(v);
        return v;
    }

    VfxDocument* NewVfxDocument(const std::filesystem::path& path) {
        std::filesystem::path full_path = path.is_relative() ?  std::filesystem::path(g_editor.project_path) / "assets" / "vfx" / path : path;
        full_path += ".vfx";

        Stream* stream = CreateStream(ALLOCATOR_DEFAULT, 4096);
        WriteCSTR(stream, "[vfx]\nduration = 1\n");
        SaveStream(stream, full_path);
        Free(stream);

        QueueImport(full_path);
        WaitForImportTasks();
        return LoadVfxDocument(full_path);
    }

    static void ReloadVfxDocument(Document* doc) {
        VfxDocument* vdoc = static_cast<VfxDocument*>(doc);
        assert(vdoc);

        Stop(vdoc->handle);
        Free(vdoc->vfx);

        try {
            LoadVfxDocument(doc);
        } catch (const std::exception& e) {
            LogError("failed to reload vfx '%s': %s", doc->name->value, e.what());
        }

        vdoc->vfx = ToVfx(ALLOCATOR_DEFAULT, vdoc, vdoc->name);
        vdoc->bounds = GetBounds(vdoc->vfx);
        vdoc->handle = INVALID_VFX_HANDLE;;
    }

    static void PlayVfxDocument(Document* doc) {
        VfxDocument* vdoc = static_cast<VfxDocument*>(doc);
        vdoc->playing = !vdoc->playing;

        if (!vdoc->playing) {
            Stop(vdoc->handle);
            vdoc->handle = INVALID_VFX_HANDLE;
        }
    }

    static void ImportVfx(Document* doc, const std::filesystem::path& path, Props* config, Props* meta) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_VFX);
        VfxDocument* vdoc = static_cast<VfxDocument*>(doc);

        Stream* stream = CreateStream(nullptr, 4096);
        Serialize(vdoc, stream);
        SaveStream(stream, path);
        Free(stream);
    }

    static void InitVfxDocument(VfxDocument* vdoc) {
        vdoc->vfx = nullptr;
        vdoc->handle = INVALID_VFX_HANDLE;
        vdoc->vtable = {
            .destructor = DestroyVfxDocument,
            .load = LoadVfxDocument,
            .reload = ReloadVfxDocument,
            .draw = DrawVfXDocument,
            .play = PlayVfxDocument,
            .clone = CloneVfxDocument
        };
    }

    static void InitVfxDocument(Document* doc) {
        assert(doc);
        assert(doc->def->type == ASSET_TYPE_VFX);
        InitVfxDocument(static_cast<VfxDocument*>(doc));
    }

	void InitVfxDocumentDef() {
        InitDocumentDef({
            .type = ASSET_TYPE_VFX,
            .size = sizeof(VfxDocument),
            .init_func = InitVfxDocument,
            .import_func = ImportVfx
        });
    }
}
