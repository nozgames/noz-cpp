//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz {

    extern void InitDebugGizmos();
    extern void InitDebugUI();
    extern void ShutdownDebugGizmos();
    extern void ShutdownDebugUI();

    struct Debug {
    };

    static Debug g_debug = {};

    void InitDebug() {
        InitDebugGizmos();
        InitDebugUI();
    }

    void ShutdownDebug() {
        ShutdownDebugUI();
        ShutdownDebugGizmos();
    }
}
