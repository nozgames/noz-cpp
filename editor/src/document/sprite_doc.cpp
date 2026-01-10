//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "sprite_doc.h"

namespace noz::editor {

    void InitSpriteDocument(Document* doc) {
        SpriteDocument* sdoc = static_cast<SpriteDocument*>(doc);
        
    }

    void InitSpriteDocumentDef() {
        InitDocumentDef({
            .type = ASSET_TYPE_SPRITE,
            .size = sizeof(SpriteDocument),
            .ext = ".sprite",
            .init_func = InitSpriteDocument
        });    
    }
}

