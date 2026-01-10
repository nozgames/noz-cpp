//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz::editor {

    struct EventDocument : Document {
        int id;
    };

    extern EventDocument* NewEventDocument(const std::filesystem::path& path);
}

