//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::editor {

    constexpr int MAX_UNDO = EDITOR_MAX_DOCUMENTS * 2;

    struct UndoItem {
        Document* saved_doc;
        Document* doc;
        int group_id;
    };

    struct UndoSystem {
        RingBuffer* undo;
        RingBuffer* redo;
        int next_group_id;
        int current_group_id;
        Document* temp[MAX_UNDO];
        int temp_count;
    };

    static UndoSystem g_undo = {};

    inline UndoItem* GetBackItem(RingBuffer* buffer) {
        return (UndoItem*)GetBack(buffer);
    }

    inline int GetBackGroupId(RingBuffer* buffer) {
        return GetBackItem(buffer)->group_id;
    }

    static void FreeUndoItem(UndoItem& item) {
        if (item.saved_doc) {
            Free(item.saved_doc);
            item.saved_doc = nullptr;
        }
        item.group_id = -1;
    }

    static void CallUndoRedo() {
        for (int i=0; i<g_undo.temp_count; i++) {
            Document* doc = g_undo.temp[i];
            if (doc->vtable.undo_redo)
                doc->vtable.undo_redo(doc);
        }

        g_undo.temp_count = 0;
    }

    static bool UndoInternal(bool allow_redo) {
        if (IsEmpty(g_undo.undo))
            return false;

        int group_id = GetBackGroupId(g_undo.undo);

        while (!IsEmpty(g_undo.undo)) {
            UndoItem* item = GetBackItem(g_undo.undo);
            if (group_id != -1 && item->group_id != group_id)
                break;

            Document* undo_asset = item->doc;
            assert(undo_asset);
            assert(undo_asset->def->type == item->saved_doc->def->type);

            if (allow_redo) {
                UndoItem* redo_item = static_cast<UndoItem*>(PushBack(g_undo.redo));
                redo_item->group_id = group_id;
                redo_item->doc = item->doc;
                redo_item->saved_doc = Clone(item->doc);
            }

            CloneInto(item->saved_doc, undo_asset);
            Free(item->saved_doc);
            item->saved_doc = nullptr;
            MarkModified(undo_asset);

            g_undo.temp[g_undo.temp_count++] = undo_asset;

            PopBack(g_undo.undo);

            if (item->group_id == -1)
                break;
        }

        CallUndoRedo();

        return true;
    }

    bool Undo()
    {
        return UndoInternal(true);
    }

    bool Redo()
    {
        if (IsEmpty(g_undo.redo))
            return false;

        int group_id = ((UndoItem*)GetBack(g_undo.redo))->group_id;

        while (!IsEmpty(g_undo.redo))
        {
            UndoItem& redo_item = *(UndoItem*)GetBack(g_undo.redo);
            if (group_id != -1 && redo_item.group_id != group_id)
                break;

            Document* redo_asset = redo_item.doc;
            assert(redo_asset);
            assert(redo_asset->def->type == redo_item.saved_doc->def->type);

            UndoItem& undo_item = *(UndoItem*)PushBack(g_undo.undo);
            undo_item.group_id = group_id;
            undo_item.doc = redo_item.doc;
            undo_item.saved_doc = Clone(redo_asset);

            CloneInto(redo_item.saved_doc, redo_asset);
            Free(redo_item.saved_doc);
            redo_item.saved_doc = nullptr;
            MarkModified(redo_asset);

            g_undo.temp[g_undo.temp_count++] = redo_asset;

            PopBack(g_undo.redo);

            if (redo_item.group_id == -1)
                break;
        }

        CallUndoRedo();

        return true;
    }

    void CancelUndo()
    {
        if (GetCount(g_undo.undo) == 0)
            return;

        UndoInternal(false);
    }

    void BeginUndoGroup()
    {
        g_undo.current_group_id = g_undo.next_group_id++;
    }

    void EndUndoGroup() {
        g_undo.current_group_id = -1;
    }

    void RecordUndo() {
        RecordUndo(GetActiveDocument());
    }

    void RecordUndo(Document* doc) {
        // Maxium undo size
        if (IsFull(g_undo.undo)) {
            UndoItem& old = *(UndoItem*)GetFront(g_undo.undo);
            FreeUndoItem(old);
            PopBack(g_undo.undo);
        }

        UndoItem& item = *(UndoItem*)PushBack(g_undo.undo);
        item.group_id = g_undo.current_group_id;
        item.doc = doc;
        item.saved_doc = Clone(doc);

        // Clear the redo
        while (!IsEmpty(g_undo.redo)) {
            UndoItem& old = *(UndoItem*)GetFront(g_undo.redo);
            FreeUndoItem(old);
            PopBack(g_undo.redo);
        }
    }

    void RemoveFromUndoRedo(Document* doc) {
        for (u32 i=GetCount(g_undo.undo); i>0; i--) {
            UndoItem& undo_item = *(UndoItem*)GetAt(g_undo.undo, i-1);
            if (undo_item.doc != doc) continue;
            FreeUndoItem(undo_item);
            RemoveAt(g_undo.undo, i-1);
        }

        for (u32 i=GetCount(g_undo.redo); i>0; i--) {
            UndoItem& redo_item = *(UndoItem*)GetAt(g_undo.redo, i-1);
            if (redo_item.doc != doc) continue;
            FreeUndoItem(redo_item);
            RemoveAt(g_undo.redo, i-1);
        }
    }

    void InitUndo() {
        assert(!g_undo.undo);
        g_undo.undo = CreateRingBuffer(ALLOCATOR_DEFAULT, sizeof(UndoItem), MAX_UNDO);
        g_undo.redo = CreateRingBuffer(ALLOCATOR_DEFAULT, sizeof(UndoItem), MAX_UNDO);
        g_undo.current_group_id = -1;
        g_undo.next_group_id = 1;
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
