//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {

    constexpr int MAX_UNDO = EDITOR_MAX_DOCUMENTS * 2;

    struct UndoItem {
        Document* doc;      // The live document pointer
        void* snapshot;     // Raw memory snapshot of the document
        int size;           // Size of the snapshot
        int group_id;
    };

    struct UndoSystem {
        RingBuffer* undo;
        RingBuffer* redo;
        int next_group_id;
        int current_group_id;
        Document* pending_callbacks[MAX_UNDO];
        int pending_count;
    };

    static UndoSystem g_undo = {};

    static void FreeUndoItem(UndoItem& item) {
        if (item.snapshot) {
            Free(item.snapshot);
            item.snapshot = nullptr;
        }
        item.doc = nullptr;
        item.size = 0;
        item.group_id = -1;
    }

    // Save a snapshot of the document's current state
    static void* SaveSnapshot(Document* doc, int* out_size) {
        int size = doc->def->size;
        void* snapshot = Alloc(ALLOCATOR_DEFAULT, size);
        memcpy(snapshot, doc, size);
        *out_size = size;
        return snapshot;
    }

    // Restore a snapshot into the document, returns the old state
    static void* RestoreSnapshot(Document* doc, void* snapshot, int size) {
        // Save current state before overwriting
        void* old_state = Alloc(ALLOCATOR_DEFAULT, size);
        memcpy(old_state, doc, size);

        // Restore the snapshot
        bool was_editing = doc->editing;
        memcpy(doc, snapshot, size);
        doc->editing = was_editing;

        return old_state;
    }

    static void CallPendingCallbacks() {
        for (int i = 0; i < g_undo.pending_count; i++) {
            Document* doc = g_undo.pending_callbacks[i];
            if (doc->vtable.undo_redo)
                doc->vtable.undo_redo(doc);
        }
        g_undo.pending_count = 0;
    }

    bool Undo() {
        if (IsEmpty(g_undo.undo))
            return false;

        UndoItem* item = (UndoItem*)GetBack(g_undo.undo);
        int group_id = item->group_id;

        while (!IsEmpty(g_undo.undo)) {
            item = (UndoItem*)GetBack(g_undo.undo);
            if (group_id != -1 && item->group_id != group_id)
                break;

            Document* doc = item->doc;
            assert(doc);

            // Swap states: restore old state, save current for redo
            void* current_state = RestoreSnapshot(doc, item->snapshot, item->size);

            // Push to redo stack (evict oldest if full)
            if (IsFull(g_undo.redo)) {
                UndoItem& old = *(UndoItem*)GetFront(g_undo.redo);
                FreeUndoItem(old);
                PopFront(g_undo.redo);
            }
            UndoItem* redo_item = (UndoItem*)PushBack(g_undo.redo);
            redo_item->doc = doc;
            redo_item->snapshot = current_state;
            redo_item->size = item->size;
            redo_item->group_id = item->group_id;

            // Free the undo item's snapshot
            Free(item->snapshot);
            item->snapshot = nullptr;

            MarkModified(doc);
            g_undo.pending_callbacks[g_undo.pending_count++] = doc;

            int item_group_id = item->group_id;
            PopBack(g_undo.undo);

            if (item_group_id == -1)
                break;
        }

        CallPendingCallbacks();
        return true;
    }

    bool Redo() {
        if (IsEmpty(g_undo.redo))
            return false;

        UndoItem* item = (UndoItem*)GetBack(g_undo.redo);
        int group_id = item->group_id;

        while (!IsEmpty(g_undo.redo)) {
            item = (UndoItem*)GetBack(g_undo.redo);
            if (group_id != -1 && item->group_id != group_id)
                break;

            Document* doc = item->doc;
            assert(doc);

            // Swap states: restore redo state, save current for undo
            void* current_state = RestoreSnapshot(doc, item->snapshot, item->size);

            // Push to undo stack (evict oldest if full)
            if (IsFull(g_undo.undo)) {
                UndoItem& old = *(UndoItem*)GetFront(g_undo.undo);
                FreeUndoItem(old);
                PopFront(g_undo.undo);
            }
            UndoItem* undo_item = (UndoItem*)PushBack(g_undo.undo);
            undo_item->doc = doc;
            undo_item->snapshot = current_state;
            undo_item->size = item->size;
            undo_item->group_id = item->group_id;

            // Free the redo item's snapshot
            Free(item->snapshot);
            item->snapshot = nullptr;

            MarkModified(doc);
            g_undo.pending_callbacks[g_undo.pending_count++] = doc;

            int item_group_id = item->group_id;
            PopBack(g_undo.redo);

            if (item_group_id == -1)
                break;
        }

        CallPendingCallbacks();
        return true;
    }

    void CancelUndo() {
        if (IsEmpty(g_undo.undo))
            return;

        UndoItem* item = (UndoItem*)GetBack(g_undo.undo);
        int group_id = item->group_id;

        while (!IsEmpty(g_undo.undo)) {
            item = (UndoItem*)GetBack(g_undo.undo);
            if (group_id != -1 && item->group_id != group_id)
                break;

            Document* doc = item->doc;

            // Restore without saving to redo
            bool was_editing = doc->editing;
            memcpy(doc, item->snapshot, item->size);
            doc->editing = was_editing;

            MarkModified(doc);
            g_undo.pending_callbacks[g_undo.pending_count++] = doc;

            int item_group_id = item->group_id;
            FreeUndoItem(*item);
            PopBack(g_undo.undo);

            if (item_group_id == -1)
                break;
        }

        CallPendingCallbacks();
    }

    void BeginUndoGroup() {
        g_undo.current_group_id = g_undo.next_group_id++;
    }

    void EndUndoGroup() {
        g_undo.current_group_id = -1;
    }

    void RecordUndo() {
        RecordUndo(GetActiveDocument());
        MarkModified();
    }

    void RecordUndo(Document* doc) {
        // Evict oldest if full
        if (IsFull(g_undo.undo)) {
            UndoItem& old = *(UndoItem*)GetFront(g_undo.undo);
            FreeUndoItem(old);
            PopFront(g_undo.undo);
        }

        // Save current state
        UndoItem* item = (UndoItem*)PushBack(g_undo.undo);
        item->doc = doc;
        item->snapshot = SaveSnapshot(doc, &item->size);
        item->group_id = g_undo.current_group_id;

        // Clear redo stack
        while (!IsEmpty(g_undo.redo)) {
            UndoItem& old = *(UndoItem*)GetBack(g_undo.redo);
            FreeUndoItem(old);
            PopBack(g_undo.redo);
        }
    }

    void RemoveFromUndoRedo(Document* doc) {
        for (u32 i = GetCount(g_undo.undo); i > 0; i--) {
            UndoItem& item = *(UndoItem*)GetAt(g_undo.undo, i - 1);
            if (item.doc != doc) continue;
            FreeUndoItem(item);
            RemoveAt(g_undo.undo, i - 1);
        }

        for (u32 i = GetCount(g_undo.redo); i > 0; i--) {
            UndoItem& item = *(UndoItem*)GetAt(g_undo.redo, i - 1);
            if (item.doc != doc) continue;
            FreeUndoItem(item);
            RemoveAt(g_undo.redo, i - 1);
        }
    }

    void InitUndo() {
        assert(!g_undo.undo);
        g_undo.undo = CreateRingBuffer(ALLOCATOR_DEFAULT, sizeof(UndoItem), MAX_UNDO);
        g_undo.redo = CreateRingBuffer(ALLOCATOR_DEFAULT, sizeof(UndoItem), MAX_UNDO);
        g_undo.current_group_id = -1;
        g_undo.next_group_id = 1;
        g_undo.pending_count = 0;
    }

    void ShutdownUndo() {
        assert(g_undo.undo);

        while (!IsEmpty(g_undo.undo)) {
            UndoItem& item = *(UndoItem*)GetBack(g_undo.undo);
            FreeUndoItem(item);
            PopBack(g_undo.undo);
        }

        while (!IsEmpty(g_undo.redo)) {
            UndoItem& item = *(UndoItem*)GetBack(g_undo.redo);
            FreeUndoItem(item);
            PopBack(g_undo.redo);
        }

        Free(g_undo.undo);
        Free(g_undo.redo);
        g_undo = {};
    }
}
