///////////////////////////////////////////////////////////////////////////////
// noZ Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_Windows_WindowsDragDrop_h__
#define __noz_Platform_Windows_WindowsDragDrop_h__

#include <noz/UI/DragDrop.h>

namespace noz {
namespace Platform {
namespace Windows {

  class WindowsWindow;

  class WindowsDragDrop {
    public: typedef HRESULT (*QueryContinueDragCallback) (Window* window, BOOL esc_pressed, DWORD key_state);
    public: typedef HRESULT (*GiveFeedbackCallback) (Window* window, DWORD effect);
    
    public: struct DropSourceCallbacks {
	    QueryContinueDragCallback query_continue_drag_;
	    GiveFeedbackCallback give_feedback_;
    };

    public: typedef HRESULT (*DragEnterCallback) (Window* window, IDataObject* data_object, DWORD key_state, POINTL pt, DWORD* peffect);
    public: typedef HRESULT (*DragOverCallback) (Window* window, DWORD key_state, POINTL pt, DWORD *peffect);
    public: typedef HRESULT (*DragLeaveCallback) (Window* window);
    public: typedef HRESULT (*DropCallback) (Window* window, IDataObject* data_object, DWORD key_state, POINTL pt, DWORD* peffect);
    public: struct DropTargetCallbacks {
	    DragEnterCallback drag_enter_;
	    DragOverCallback drag_over_;
	    DragLeaveCallback drag_leave_;
	    DropCallback drop_;
    };

    public: struct EnumFormatEtc {
	    IEnumFORMATETC super_;
	    ULONG refs_;
	    FORMATETC* array_;
	    ULONG size_;
      ULONG current_;
    };

    struct DataObject {
	    IDataObject super_;
	    ULONG refs_;
	    FORMATETC** formats_;
	    STGMEDIUM** mediums_;
	    ULONG size_;
    };

    struct DropSource {
	    IDropSource super_;
	    ULONG refs_;
	    DropSourceCallbacks callbacks_;
	    ObjectPtr<Window> window_;
    };

    struct DropTarget {
	    IDropTarget super_;
	    ULONG refs_;
	    DropTargetCallbacks callbacks_;
	    ObjectPtr<Window> window_;
    };

    private: static inline void* Alloc(noz_uint32 size) {
      return (void*)::GlobalAlloc(GPTR, size);
    }

    private: static inline void Free(void* p) {
      ::GlobalFree((HGLOBAL)p);
    }

    private: static CLIPFORMAT clip_format_;
    private: static IEnumFORMATETCVtbl vtbl_format_etc_;
    private: static IDataObjectVtbl vtbl_data_object_;
    private: static IDropSourceVtbl vtbl_drop_source_;
    private: static IDropTargetVtbl vtbl_drop_target_;

    public: static void Initialize (void);

    public: static DataObject* CreateDataObject (void);
    public: static DropSource* CreateDropSource (DropSourceCallbacks*, Window* window);
    public: static DropTarget* CreateDropTarget (DropTargetCallbacks*, Window* window);
    public: static EnumFormatEtc* CreateEnumFormatEtc (FORMATETC** array, ULONG size);

    public: static Object* DataObjectToObject (IDataObject* data_object);
    public: static IDataObject* ObjectToDataObject (Object* object);

    public: static STDMETHODIMP_(ULONG) DataObjectAddRef(DataObject* _this);
    public: static STDMETHODIMP_(ULONG) DataObjectRelease(DataObject* _this);
    public: static STDMETHODIMP DataObjectSetData(DataObject* _this, FORMATETC* format_etc, STGMEDIUM* medium, BOOL fRelease);
    private: static STDMETHODIMP DataObjectQueryInterface(DataObject* _this, REFIID riid, void** object);
    private: static STDMETHODIMP DataObjectGetData(DataObject* _this, FORMATETC* format_etc, STGMEDIUM* medium);
    private: static STDMETHODIMP DataObjectGetDataHere(DataObject* _this, FORMATETC* format_etc, STGMEDIUM* medium);
    private: static STDMETHODIMP DataObjectQueryGetData(DataObject* _this, FORMATETC* format_etc);
    private: static STDMETHODIMP DataObjectGetCanonicalFormatEtc(DataObject* _this, FORMATETC* format_in, FORMATETC* format_out);
    private: static STDMETHODIMP DataObjectEnumFormatEtc(DataObject* _this, DWORD direction, IEnumFORMATETC **ppenumFormatetc);
    private: static STDMETHODIMP DataObjectDAdvise(DataObject* _this, FORMATETC* format_etc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
    private: static STDMETHODIMP DataObjectDUnadvise(DataObject* _this, DWORD connection);
    private: static STDMETHODIMP DataObjectEnumDAdvise(DataObject* _this, IEnumSTATDATA **ppenumAdvise);

    public: static STDMETHODIMP_(ULONG) EnumFormatEtcAddRef (EnumFormatEtc *_this);
    public: static STDMETHODIMP_(ULONG) EnumFormatEtcRelease (EnumFormatEtc* _this);
    private: static STDMETHODIMP EnumFormatEtcQueryInterface (EnumFormatEtc *_this, REFIID riid, void** object);
    private: static EnumFormatEtc* EnumFormatEtcCopy (EnumFormatEtc* src);
    private: static STDMETHODIMP EnumFormatEtcClone (EnumFormatEtc *_this, IEnumFORMATETC** out); 
    private: static STDMETHODIMP EnumFormatEtcReset (EnumFormatEtc* _this);
    private: static STDMETHODIMP EnumFormatEtcSkip (EnumFormatEtc* _this, ULONG celt);   
    private: static STDMETHODIMP EnumFormatEtcNext(EnumFormatEtc* _this, ULONG celt, LPFORMATETC fmt, ULONG* celt_fetched);

    public: static STDMETHODIMP_(ULONG) DropSourceAddRef (DropSource* _this);
    public: static STDMETHODIMP_(ULONG) DropSourceRelease (DropSource* _this);
    private: static STDMETHODIMP DropSourceQueryInterface (DropSource* _this, REFIID riid, void **object);
    private: static STDMETHODIMP DropSourceQueryContinueDrag (DropSource* _this, BOOL esc_pressed, DWORD key_state);
    private: static STDMETHODIMP DropSourceGiveFeedback (DropSource* _this, DWORD effect);

    public: static STDMETHODIMP_(ULONG) DropTargetRelease(DropTarget* _this);
    public: static STDMETHODIMP_(ULONG) DropTargetAddRef(DropTarget *_this);
    private: static STDMETHODIMP DropTargetQueryInterface (DropTarget* _this, REFIID riid, void **object);
    private: static STDMETHODIMP DropTargetDragEnter(DropTarget *_this, IDataObject *data_object, DWORD key_state, POINTL pt, DWORD *peffect);
    private: static STDMETHODIMP DropTargetDragOver(DropTarget *_this, DWORD key_state, POINTL pt, DWORD *peffect);
    private: static STDMETHODIMP DropTargetDragLeave(DropTarget *_this);
    private: static STDMETHODIMP DropTargetDrop(DropTarget *_this, IDataObject *data_object, DWORD key_state, POINTL pt, DWORD *peffect);
  };

} // namespace Windows
} // namespace Platform
} // namespace noz

#endif // __noz_Platform_Windows_WindowsDragDrop_h__

