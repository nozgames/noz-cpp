//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

bool HitTest(Geometry& geom, const Mat3& transform, const Vec2& point, GeometryHitTestResult& result) {
    const float radius_sqr = g_view.select_size * g_view.select_size;

    result.edge_index = U16_MAX;
    result.face_index = U16_MAX;
    result.vertx_index = U16_MAX;

    // vert?
    for (u16 vi=0; vi<geom.vert_count; vi++) {
        Vec2 v_pos = TransformPoint(transform, geom.verts[vi].position);
        float dist_sqr = LengthSqr(point - v_pos);
        if(dist_sqr <= radius_sqr) {
            result.vertx_index = vi;
            return true;
        }
    }

    // edge?
    for (u16 ei=0; ei<geom.edge_count; ei++) {
        EdgeData e = geom.edges[ei];
        Vec2 v0 = TransformPoint(transform, geom.verts[e.v0].position);
        Vec2 v1 = TransformPoint(transform, geom.verts[e.v1].position);

        Vec2 edge_dir = v1 - v0;
        float edge_length = Length(edge_dir);
        edge_dir = Normalize(edge_dir);

        Vec2 to_point = point - v0;
        float proj = Dot(to_point, edge_dir);
        if(proj >= 0 && proj <= edge_length) {
            Vec2 closest_point = v0 + edge_dir * proj;
            float dist_sqr = LengthSqr(point - closest_point);
            if(dist_sqr <= radius_sqr) {
                result.edge_index = ei;
                return true;
            }
        }
    }

    return false;
}
