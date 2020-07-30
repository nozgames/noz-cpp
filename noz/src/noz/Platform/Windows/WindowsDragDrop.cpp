///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Windows.pch.h"
#include "WindowsDragDrop.h"

using namespace noz;
using namespace noz::Platform;
using namespace noz::Platform::Windows;

IEnumFORMATETCVtbl WindowsDragDrop::vtbl_format_etc_;
IDataObjectVtbl WindowsDragDrop::vtbl_data_object_;
IDropSourceVtbl WindowsDragDrop::vtbl_drop_source_;
IDropTargetVtbl WindowsDragDrop::vtbl_drop_target_;
CLIPFORMAT WindowsDragDrop::clip_format_ = 0;



void WindowsDragDrop::Initialize (void) {
  static bool initialized = false;
  if(initialized) return;
  initialized = true;

  clip_format_ = ::RegisterClipboardFormat("noz::Object");

	// IEnumFORMATETC
	vtbl_format_etc_.QueryInterface = (HRESULT (__stdcall *)(IEnumFORMATETC *,const IID &,void **)) EnumFormatEtcQueryInterface;
	vtbl_format_etc_.AddRef = (ULONG (__stdcall *)(IEnumFORMATETC *)) EnumFormatEtcAddRef;
	vtbl_format_etc_.Release = (ULONG (__stdcall *)(IEnumFORMATETC *)) EnumFormatEtcRelease;
	vtbl_format_etc_.Next = (HRESULT (__stdcall *)(IEnumFORMATETC *,ULONG,FORMATETC* ,ULONG *))EnumFormatEtcNext;
	vtbl_format_etc_.Skip = (HRESULT (__stdcall *)(IEnumFORMATETC *,ULONG))EnumFormatEtcSkip;
	vtbl_format_etc_.Reset = (HRESULT (__stdcall *)(IEnumFORMATETC *))EnumFormatEtcReset;
	vtbl_format_etc_.Clone = (HRESULT (__stdcall *)(IEnumFORMATETC *,IEnumFORMATETC **))EnumFormatEtcClone;
	
	// IDataSource
	vtbl_data_object_.QueryInterface = (HRESULT (__stdcall *)(IDataObject *,const IID &,void **))DataObjectQueryInterface;
	vtbl_data_object_.AddRef = (ULONG (__stdcall *)(IDataObject *))DataObjectAddRef;
	vtbl_data_object_.Release = (ULONG (__stdcall *)(IDataObject *))DataObjectRelease;
	vtbl_data_object_.GetData = (HRESULT (__stdcall *)(IDataObject *,FORMATETC* ,STGMEDIUM *))DataObjectGetData;
	vtbl_data_object_.GetDataHere = (HRESULT (__stdcall *)(IDataObject *,FORMATETC* ,STGMEDIUM *))DataObjectGetDataHere;
	vtbl_data_object_.QueryGetData = (HRESULT (__stdcall *)(IDataObject *,FORMATETC* ))DataObjectQueryGetData;
	vtbl_data_object_.GetCanonicalFormatEtc = (HRESULT (__stdcall *)(IDataObject *,FORMATETC* ,FORMATETC* ))DataObjectGetCanonicalFormatEtc;
	vtbl_data_object_.SetData = (HRESULT (__stdcall *)(IDataObject *,FORMATETC* ,STGMEDIUM *,BOOL))DataObjectSetData;
	vtbl_data_object_.EnumFormatEtc = (HRESULT (__stdcall *)(IDataObject *,DWORD,IEnumFORMATETC **))DataObjectEnumFormatEtc;
	vtbl_data_object_.DAdvise = (HRESULT (__stdcall *)(IDataObject *,FORMATETC* ,DWORD,IAdviseSink *,DWORD *))DataObjectDAdvise;
	vtbl_data_object_.DUnadvise = (HRESULT (__stdcall *)(IDataObject *,DWORD))DataObjectDUnadvise;
	vtbl_data_object_.EnumDAdvise = (HRESULT (__stdcall *)(IDataObject *,IEnumSTATDATA **))DataObjectEnumDAdvise;
	
	// IDropSource 
	vtbl_drop_source_.QueryInterface = (HRESULT (__stdcall *)(IDropSource *,const IID &,void **))DropSourceQueryInterface;
	vtbl_drop_source_.AddRef = (ULONG(__stdcall*)(IDropSource*)) DropSourceAddRef;
	vtbl_drop_source_.Release = (ULONG(__stdcall*)(IDropSource*)) DropSourceRelease;
	vtbl_drop_source_.QueryContinueDrag = (HRESULT (__stdcall *)(IDropSource *,BOOL,DWORD))DropSourceQueryContinueDrag;
	vtbl_drop_source_.GiveFeedback = (HRESULT (__stdcall *)(IDropSource *,DWORD))DropSourceGiveFeedback;
	
	// IDragTarget 
	vtbl_drop_target_.QueryInterface = (HRESULT (__stdcall *)(IDropTarget *,const IID &,void **))DropTargetQueryInterface;
	vtbl_drop_target_.AddRef = (ULONG (__stdcall *)(IDropTarget *))DropTargetAddRef;
	vtbl_drop_target_.Release = (ULONG (__stdcall *)(IDropTarget *))DropTargetRelease;
	vtbl_drop_target_.DragEnter = (HRESULT (__stdcall *)(IDropTarget *,IDataObject *,DWORD,POINTL,DWORD *))DropTargetDragEnter;
	vtbl_drop_target_.DragOver = (HRESULT (__stdcall *)(IDropTarget *,DWORD,POINTL,DWORD *))DropTargetDragOver;
	vtbl_drop_target_.DragLeave = (HRESULT (__stdcall *)(IDropTarget *))DropTargetDragLeave;
	vtbl_drop_target_.Drop = (HRESULT (__stdcall *)(IDropTarget *,IDataObject *,DWORD,POINTL,DWORD *))DropTargetDrop;
}


static void CopySTGMEDIUM(STGMEDIUM *pdest, const STGMEDIUM *psrc, const FORMATETC* fmt) {
	pdest->tymed = psrc->tymed;
	switch (psrc->tymed) {
	case TYMED_HGLOBAL:
		pdest->u.hGlobal = (HGLOBAL)OleDuplicateData(psrc->u.hGlobal, fmt->cfFormat, (UINT)NULL);
		break;
	case TYMED_GDI:
		pdest->u.hBitmap = (HBITMAP)OleDuplicateData(psrc->u.hBitmap, fmt->cfFormat, (UINT)NULL);
		break;
	case TYMED_MFPICT:
		pdest->u.hMetaFilePict = (HMETAFILEPICT)OleDuplicateData(psrc->u.hMetaFilePict, fmt->cfFormat, (UINT)NULL);
		break;
	case TYMED_ENHMF:
		pdest->u.hEnhMetaFile = (HENHMETAFILE)OleDuplicateData(psrc->u.hEnhMetaFile, fmt->cfFormat, (UINT)NULL);
		break;
	case TYMED_FILE:
		pdest->u.lpszFileName = (LPOLESTR)OleDuplicateData(psrc->u.lpszFileName, fmt->cfFormat, (UINT)NULL);
		break;
	case TYMED_ISTREAM:
		pdest->u.pstm = psrc->u.pstm;
		IStream_AddRef(psrc->u.pstm);
		break;
	case TYMED_ISTORAGE:
		pdest->u.pstg = psrc->u.pstg;
		IStorage_AddRef(psrc->u.pstg);
		break;
	case TYMED_NULL:
	default:
		break;
	}
	
	pdest->pUnkForRelease = psrc->pUnkForRelease;
	if (psrc->pUnkForRelease != NULL)
		IUnknown_AddRef(psrc->pUnkForRelease);
}


WindowsDragDrop::EnumFormatEtc* WindowsDragDrop::CreateEnumFormatEtc (FORMATETC* *array, ULONG size) {
	EnumFormatEtc* ptr = (EnumFormatEtc*) Alloc(sizeof(EnumFormatEtc));
  if(nullptr == ptr) return nullptr;
	
  ptr->super_.lpVtbl = &vtbl_format_etc_;
  ptr->refs_ = 0;
  ptr->array_ = (FORMATETC*) Alloc(sizeof(FORMATETC) * size);
  for (ULONG i = 0; i < size; i++) ptr->array_[i] = *array[i];
  ptr->size_ = size;
  ptr->current_ = 0;

  return ptr;
}

WindowsDragDrop::EnumFormatEtc* WindowsDragDrop::EnumFormatEtcCopy(EnumFormatEtc *src) {
	EnumFormatEtc *ptr = (EnumFormatEtc*)Alloc(sizeof(EnumFormatEtc));
  if(nullptr == ptr) return nullptr;

  ptr->super_.lpVtbl = &vtbl_format_etc_;
	ptr->refs_ = 0;
	ptr->array_ = (FORMATETC*) Alloc(sizeof(FORMATETC) * src->size_);
	CopyMemory(ptr->array_, src->array_, sizeof(FORMATETC) * src->size_);
	ptr->size_ = src->size_;
	ptr->current_ = src->current_;

	return ptr;
}

STDMETHODIMP WindowsDragDrop::EnumFormatEtcQueryInterface (EnumFormatEtc *_this, REFIID riid, void** object) {
	*object = NULL;
	
  if (!IsEqualIID(riid, IID_IUnknown) && !IsEqualIID(riid, IID_IEnumFORMATETC)) return E_NOINTERFACE;

  *object = _this;
  IUnknown_AddRef((IUnknown*)*object);
	return S_OK;
}

STDMETHODIMP_(ULONG) WindowsDragDrop::EnumFormatEtcAddRef (EnumFormatEtc *_this) {
	return ++_this->refs_;
}

STDMETHODIMP_(ULONG) WindowsDragDrop::EnumFormatEtcRelease (EnumFormatEtc* _this) {
	ULONG tmp = --_this->refs_;
	if (tmp == 0) {
	  Free(_this->array_);
	  Free(_this);
  }
	return tmp;
}

STDMETHODIMP WindowsDragDrop::EnumFormatEtcNext(EnumFormatEtc *_this, ULONG celt, LPFORMATETC fmt, ULONG* celt_fetched) {
	ULONG num = celt;
	
	if (celt_fetched != NULL) *celt_fetched = 0;
	
	if (celt <= 0 || fmt == NULL || _this->current_ >= _this->size_) return S_FALSE;
	
	/* celt_fetched can be NULL only for 1 item request */
	if (celt_fetched == NULL && celt != 1) return S_FALSE;
	
	while (_this->current_<_this->size_ && num > 0) {
		*fmt++ = _this->array_[_this->current_++];
		--num;
	}
	
  if (celt_fetched != NULL) *celt_fetched = celt - num;
	
	return (num == 0) ? S_OK : S_FALSE;
}

STDMETHODIMP WindowsDragDrop::EnumFormatEtcSkip (EnumFormatEtc *_this, ULONG celt) {
	if (_this->current_+celt >= _this->size_) return S_FALSE;
	_this->current_ += celt;
	return S_OK;
}

STDMETHODIMP WindowsDragDrop::EnumFormatEtcReset (EnumFormatEtc* _this) {
	_this->current_ = 0;
	return S_OK;
}

STDMETHODIMP WindowsDragDrop::EnumFormatEtcClone (EnumFormatEtc *_this, IEnumFORMATETC** result) {
	if (result==NULL) return E_POINTER;

	EnumFormatEtc* clone = EnumFormatEtcCopy(_this);
	if (clone==NULL) return E_OUTOFMEMORY;
	IUnknown_AddRef((IUnknown*)clone);
	*result = (IEnumFORMATETC*)clone;
	return S_OK;
}

STDMETHODIMP WindowsDragDrop::DataObjectQueryInterface(DataObject* _this, REFIID riid, void** object) {
	*object = NULL;
	if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDataObject)) {
		*object = _this;
		IUnknown_AddRef((IUnknown *)*object);
		return S_OK;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) WindowsDragDrop::DataObjectAddRef(DataObject* _this) {
	return ++_this->refs_;
}

STDMETHODIMP_(ULONG) WindowsDragDrop::DataObjectRelease(DataObject* _this) {
	ULONG tmp = --_this->refs_;
	if (tmp != 0) return tmp;

  ULONG i;
	for (i=0; i<_this->size_; i++) {
		Free(_this->formats_[i]);
		ReleaseStgMedium(_this->mediums_[i]);
		Free(_this->mediums_[i]);
	}

	Free(_this->formats_);
	Free(_this->mediums_);
	Free(_this);

	return 0;
}

STDMETHODIMP WindowsDragDrop::DataObjectGetData(WindowsDragDrop::DataObject* _this, FORMATETC* format_etc, STGMEDIUM* medium) {
	if (format_etc == NULL || medium == NULL) return E_INVALIDARG;
	
	medium->u.hGlobal = NULL;
	for (ULONG i=0; i<_this->size_; i++) {
		FORMATETC* result=_this->formats_[i];
		if (format_etc->tymed & result->tymed
		 && format_etc->dwAspect == result->tymed
		 && format_etc->cfFormat == result->cfFormat) {
			CopySTGMEDIUM(medium, _this->mediums_[i], result);
			return S_OK;
		}
	}
	return DV_E_FORMATETC;
}

STDMETHODIMP WindowsDragDrop::DataObjectGetDataHere(WindowsDragDrop::DataObject* _this, FORMATETC* format_etc, STGMEDIUM* medium) {
	return E_NOTIMPL;
}

STDMETHODIMP WindowsDragDrop::DataObjectQueryGetData(WindowsDragDrop::DataObject* _this, FORMATETC* format_etc) {
	if (format_etc == NULL)	return E_INVALIDARG;
	if (!(DVASPECT_CONTENT & format_etc->dwAspect)) return DV_E_DVASPECT;
	
	HRESULT result = DV_E_TYMED;
	for (ULONG i = 0; i < _this->size_; i++){
		if (format_etc->tymed & _this->formats_[i]->tymed) {
			if (format_etc->cfFormat == _this->formats_[i]->cfFormat)
				return S_OK;
			result = DV_E_CLIPFORMAT;
		} else {
			result = DV_E_TYMED;
		}
	}
	return result;
}

STDMETHODIMP WindowsDragDrop::DataObjectGetCanonicalFormatEtc(WindowsDragDrop::DataObject* _this, FORMATETC* format_in, FORMATETC* format_out) {
	if (format_out == NULL)
		return E_INVALIDARG;
	
	return DATA_S_SAMEFORMATETC;
}

STDMETHODIMP WindowsDragDrop::DataObjectSetData(WindowsDragDrop::DataObject* _this, FORMATETC* format_etc, STGMEDIUM* medium, BOOL fRelease) {
	FORMATETC* fmt;
	STGMEDIUM *pstg;
	
	if(format_etc == NULL || medium == NULL)
		return E_INVALIDARG;
	
	fmt = (FORMATETC*)Alloc(sizeof(FORMATETC));
	pstg = (STGMEDIUM*)Alloc(sizeof(STGMEDIUM));
	if (fmt == NULL || pstg == NULL) {
		Free(fmt);
		Free(pstg);
		return E_OUTOFMEMORY;
	}
	
	*fmt = *format_etc;
	if (fRelease) {
		*pstg = *medium;
	} else {
		CopySTGMEDIUM(pstg, medium, format_etc);
	}
	
	if (_this->size_ == 0) {
		_this->formats_ = (FORMATETC**)Alloc(sizeof(FORMATETC* ));
		_this->mediums_ = (STGMEDIUM**)Alloc(sizeof(STGMEDIUM *));
		_this->formats_[0] = fmt;
		_this->mediums_[0] = pstg;
		_this->size_ = 1;
	} else {
		FORMATETC* *oldfmts = _this->formats_;
		STGMEDIUM **oldstgs = _this->mediums_;
		ULONG oldsize = _this->size_;
		
		_this->formats_ = (FORMATETC**)Alloc(sizeof(FORMATETC* ) * (oldsize + 1));
		_this->mediums_ = (STGMEDIUM**)Alloc(sizeof(STGMEDIUM *) * (oldsize + 1));
		CopyMemory(_this->formats_, oldfmts, sizeof(FORMATETC* ) * oldsize);
		CopyMemory(_this->mediums_, oldstgs, sizeof(STGMEDIUM *) * oldsize);
		_this->formats_[oldsize] = fmt;
		_this->mediums_[oldsize] = pstg;
		_this->size_ = oldsize + 1;
		Free(oldfmts);
		Free(oldstgs);
	}
	return S_OK;
}

STDMETHODIMP WindowsDragDrop::DataObjectEnumFormatEtc(WindowsDragDrop::DataObject* _this, DWORD direction, IEnumFORMATETC **format_etc) {
	if(format_etc == NULL) return E_POINTER;
	
	*format_etc = NULL;

	switch (direction) {
	  case DATADIR_GET:
		  *format_etc = (IEnumFORMATETC *)CreateEnumFormatEtc (_this->formats_, _this->size_);
		  if (*format_etc == NULL) return E_OUTOFMEMORY;
		  IEnumFORMATETC_AddRef(*format_etc);
		  break;

	  case DATADIR_SET:
	  default:
		  return E_NOTIMPL;
	}
	
	return S_OK;
}

STDMETHODIMP WindowsDragDrop::DataObjectDAdvise(WindowsDragDrop::DataObject* _this, FORMATETC* format_etc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection) {
	return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP WindowsDragDrop::DataObjectDUnadvise(WindowsDragDrop::DataObject* _this, DWORD connection) {
	return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP WindowsDragDrop::DataObjectEnumDAdvise(WindowsDragDrop::DataObject* _this, IEnumSTATDATA **ppenumAdvise) {
	return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP WindowsDragDrop::DropSourceQueryInterface (DropSource* _this, REFIID riid, void** object) {
	*object = NULL;
	
  if (!IsEqualIID(riid, IID_IUnknown) && !IsEqualIID(riid, IID_IDropSource)) return E_NOINTERFACE;
  
  *object = _this;
	IUnknown_AddRef((IUnknown *)*object);
	return S_OK;
}

STDMETHODIMP_(ULONG) WindowsDragDrop::DropSourceAddRef(DropSource* _this) {
	return ++_this->refs_;
}

STDMETHODIMP_(ULONG) WindowsDragDrop::DropSourceRelease (DropSource* _this) {
	ULONG tmp = --_this->refs_;
	if (tmp == 0) Free(_this);
	return tmp;
}

STDMETHODIMP WindowsDragDrop::DropSourceQueryContinueDrag (DropSource* _this, BOOL esc_pressed, DWORD key_state) {
  // If a callback is specified call it
	if (_this->callbacks_.query_continue_drag_ != NULL) {
		return _this->callbacks_.query_continue_drag_(_this->window_, esc_pressed, key_state);
  }
	
  // Escape cancels the drag drop
	if (esc_pressed) return DRAGDROP_S_CANCEL;

  // If left or right button has been released we are done
	if (!(key_state & (MK_LBUTTON|MK_RBUTTON))) return DRAGDROP_S_DROP;

  // Continue
	return S_OK;
}

STDMETHODIMP WindowsDragDrop::DropSourceGiveFeedback(DropSource* _this, DWORD effect) {
  // Callback overrides default implementation
	if (_this->callbacks_.give_feedback_ != NULL) {
		return _this->callbacks_.give_feedback_(_this->window_, effect);
  }
  		
	return DRAGDROP_S_USEDEFAULTCURSORS;
}


STDMETHODIMP WindowsDragDrop::DropTargetQueryInterface (DropTarget* _this, REFIID riid, void **object) {
	*object = NULL;
	if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDropTarget)) {
		*object = _this;
		IUnknown_AddRef((IUnknown *)*object);
		return S_OK;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) WindowsDragDrop::DropTargetAddRef(WindowsDragDrop::DropTarget *_this) {
	return ++_this->refs_;
}

STDMETHODIMP_(ULONG) WindowsDragDrop::DropTargetRelease(DropTarget* _this) {
	ULONG tmp = --_this->refs_;
	if (tmp == 0) Free(_this);
	return tmp;
}

STDMETHODIMP WindowsDragDrop::DropTargetDragEnter(WindowsDragDrop::DropTarget* _this, IDataObject* data_object, DWORD key_state, POINTL pt, DWORD* effect) {
	return _this->callbacks_.drag_enter_(_this->window_, data_object, key_state, pt, effect);
}

STDMETHODIMP WindowsDragDrop::DropTargetDragOver(WindowsDragDrop::DropTarget *_this, DWORD key_state, POINTL pt, DWORD *effect) {
	if (_this->callbacks_.drag_over_ != NULL)
		return _this->callbacks_.drag_over_(_this->window_, key_state, pt, effect);
	
	/* ??????? */
	*effect = DROPEFFECT_NONE;
	return S_OK;
}

STDMETHODIMP WindowsDragDrop::DropTargetDragLeave(WindowsDragDrop::DropTarget *_this) {
	if (_this->callbacks_.drag_leave_ != NULL)
		return _this->callbacks_.drag_leave_(_this->window_);
	
	/* ??????? */
	return S_OK;
}

STDMETHODIMP WindowsDragDrop::DropTargetDrop(WindowsDragDrop::DropTarget *_this, IDataObject *data_object, DWORD key_state, POINTL pt, DWORD *effect) {
	if (_this->callbacks_.drop_ != NULL)
		return _this->callbacks_.drop_(_this->window_, data_object, key_state, pt, effect);
	
	/* ??????? */
	*effect = DROPEFFECT_NONE;
	return S_OK;
}

WindowsDragDrop::DataObject* WindowsDragDrop::CreateDataObject(void) {
	DataObject *ptr = (DataObject*)Alloc(sizeof(WindowsDragDrop::DataObject));
  if(nullptr==ptr) return nullptr;

  ptr->super_.lpVtbl = &vtbl_data_object_;
  ptr->refs_ = 0;
  ptr->formats_ = NULL;
  ptr->mediums_ = NULL;
  ptr->size_ = 0;
	return ptr;
}

WindowsDragDrop::DropSource* WindowsDragDrop::CreateDropSource (DropSourceCallbacks* callbacks, Window* window) {
  noz_assert(callbacks);

	DropSource* ptr = (DropSource*)Alloc(sizeof(DropSource));
  if(nullptr == ptr) return nullptr;

  ptr->super_.lpVtbl = &vtbl_drop_source_;
  ptr->refs_ = 0;
  ptr->callbacks_ = *callbacks;
  ptr->window_ = window;;
	return ptr;
}

WindowsDragDrop::DropTarget* WindowsDragDrop::CreateDropTarget(DropTargetCallbacks* callbacks, Window* window) {
  noz_assert(callbacks);

	DropTarget* ptr = (DropTarget*)Alloc(sizeof(DropTarget));
  if(nullptr == ptr) return nullptr;

  ptr->super_.lpVtbl = &vtbl_drop_target_;
  ptr->refs_ = 0;
  ptr->callbacks_ = *callbacks;
  ptr->window_ = window;
	return ptr;
}

Object* WindowsDragDrop::DataObjectToObject (IDataObject* data_object) {
  FORMATETC fmtetc;
  fmtetc.cfFormat = clip_format_;
  fmtetc.dwAspect = DVASPECT_CONTENT;
  fmtetc.lindex = -1;
  fmtetc.ptd = NULL;
  fmtetc.tymed = TYMED_HGLOBAL;

  STGMEDIUM stgmed;
  stgmed.pUnkForRelease = NULL;
  stgmed.tymed = TYMED_HGLOBAL;
  stgmed.u.hGlobal = NULL;  

  // Get the HGLOBAL
  if(S_OK != data_object->lpVtbl->GetData(data_object, &fmtetc, &stgmed)) return nullptr;

  // Ensure its valid.
  if(stgmed.u.hGlobal == NULL) return nullptr;

  // Lock the global to get the pointer
  void* ptr_value = GlobalLock(stgmed.u.hGlobal);

  Object* object = nullptr;
  memcpy(&object, ptr_value, sizeof(Object*));
  GlobalUnlock(stgmed.u.hGlobal);

  return object;
}

IDataObject* WindowsDragDrop::ObjectToDataObject (Object* object) {
  // Create a global to store the object pointer.
  HGLOBAL hglobal = GlobalAlloc(GHND,sizeof(Object*));
  memcpy(GlobalLock(hglobal),&object,sizeof(Object*));
  GlobalUnlock(hglobal);

  FORMATETC fmtetc;
  fmtetc.cfFormat = clip_format_;
  fmtetc.dwAspect = DVASPECT_CONTENT;
  fmtetc.lindex = -1;
  fmtetc.ptd = NULL;
  fmtetc.tymed = TYMED_HGLOBAL;

  STGMEDIUM stgmed;
  stgmed.pUnkForRelease = NULL;
  stgmed.tymed = TYMED_HGLOBAL;
  stgmed.u.hGlobal = hglobal;

  // Create the data object.
  DataObject* data_object = WindowsDragDrop::CreateDataObject();  
  DataObjectSetData(data_object, &fmtetc, &stgmed, FALSE);
  
  return (IDataObject*)data_object;
}

DragDropEffects DragDrop::DoDragDropImplementation (Window* window, Object* data, DragDropEffects allowedEffects) {
  IDataObject* data_object = WindowsDragDrop::ObjectToDataObject(data);

  // Create the drop source
  WindowsDragDrop::DropSourceCallbacks callbacks;
  ZeroMemory(&callbacks, sizeof(callbacks));
  WindowsDragDrop::DropSource* source = WindowsDragDrop::CreateDropSource(&callbacks,window);
  
  // Begin the drag drop which will block..
  DWORD effect = DROPEFFECT_NONE;
  HRESULT result = ::DoDragDrop((LPDATAOBJECT)data_object, (LPDROPSOURCE)source, DROPEFFECT_MOVE|DROPEFFECT_COPY, &effect);

  // Cleanup
  WindowsDragDrop::DropSourceRelease(source);
  WindowsDragDrop::DataObjectRelease((WindowsDragDrop::DataObject*)data_object);

  // Return None if the drag drop failed.
  if(result != S_OK) return DragDropEffects::None;

  // Convert the drop effect to the 
  switch(effect) {
    case DROPEFFECT_MOVE: return DragDropEffects::Move;
    case DROPEFFECT_COPY: return DragDropEffects::Copy;
  }

  

  return DragDropEffects::None;
}

