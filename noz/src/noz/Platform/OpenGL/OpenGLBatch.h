///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenGLBatch_h__
#define __noz_Platform_OpenGLBatch_h__

#include "OpenGLState.h"

namespace noz {
namespace Platform {

  struct OpenGLBatch {
    /// Linked list node for main linked list within context
    LinkedListNode<OpenGLBatch> lln_;

    /// Linked list node for open batches
    LinkedListNode<OpenGLBatch> lln_open_;

    /// Raw vertex data
    std::vector<noz_byte> vertex_data_;

    /// Raw index data.
    std::vector<noz_uint16> index_data_;

    // State 
    OpenGLState state_;

    /// Offset of first vertex in VBO
    noz_uint32 vertex_offset_;

    /// Number of verticies in the batch
    noz_uint32 vertex_count_;

    /// Size of an individual vertex 
    noz_uint32 vertex_size_;

    /// Offset of first index in element array
    noz_uint32 index_offset_;

    /// Number of indicies in the batch
    noz_uint32 index_count_;

    /// Bounding rectangle of all data within batch
    Rect bounds_;

    OpenGLBatch(void) : lln_(this), lln_open_(this) {
    }

    void Add (const OpenGLVertexXYZUVC* v, noz_uint32 vc, const noz_uint16* i, noz_uint32 ic) {Add((noz_float*)v,vc,sizeof(OpenGLVertexXYZUVC),i,ic);}

    void Add (const OpenGLVertexXYZC* v, noz_uint32 vc, const noz_uint16* i, noz_uint32 ic) {Add((noz_float*)v,vc,sizeof(OpenGLVertexXYZC),i,ic);}

    private: void Add (const noz_float* v, noz_uint32 vc, noz_uint32 vs, const noz_uint16* i, noz_uint32 ic);
  };


} // namespace Platform
} // namespace noz


#endif // __noz_Platform_OpenGLBatch_h__

