#!/bin/bash
# Convert old .mesh format to new .sprite format
# Usage: ./convert_mesh_to_sprite.sh input.mesh > output.sprite

if [ -z "$1" ]; then
    echo "Usage: $0 <input.mesh>" >&2
    exit 1
fi

awk '
BEGIN {
    vertex_count = 0
    edge_count = 0
    face_count = 0
}

/^v / {
    vertices[vertex_count, 0] = $2
    vertices[vertex_count, 1] = $3
    vertex_count++
    next
}

/^e / {
    idx1 = $2
    idx2 = $3
    edge_key1 = idx1 "," idx2
    edge_key2 = idx2 "," idx1
    if (NF >= 5) {
        ox = $4
        oy = $5
        edge_ox[edge_key1] = ox
        edge_oy[edge_key1] = oy
        edge_ox[edge_key2] = ox
        edge_oy[edge_key2] = oy
    }
    next
}

/^f / {
    color = 0
    face_vertex_count = 0
    for (i = 2; i <= NF; i++) {
        if ($i == "c") {
            color = $(i + 1)
            break
        }
        face_vertices[face_count, face_vertex_count] = $i
        face_vertex_count++
    }
    face_sizes[face_count] = face_vertex_count
    face_colors[face_count] = color
    face_count++
    next
}

/^p / {
    palette = $2
    next
}

END {
    print "c " palette
    print ""

    for (f = 0; f < face_count; f++) {
        print "p c " face_colors[f]

        size = face_sizes[f]
        for (i = 0; i < size; i++) {
            vi = face_vertices[f, i]
            x = vertices[vi, 0]
            y = vertices[vi, 1]

            next_i = (i + 1) % size
            next_vi = face_vertices[f, next_i]
            edge_key = vi "," next_vi

            ox = edge_ox[edge_key]
            oy = edge_oy[edge_key]

            if (ox != "" || oy != "") {
                nx = vertices[next_vi, 0]
                ny = vertices[next_vi, 1]
                dx = nx - x
                dy = ny - y

                cross = dx * oy - dy * ox
                magnitude = sqrt(ox * ox + oy * oy)

                if (cross < 0) {
                    curve = -magnitude
                } else {
                    curve = magnitude
                }

                printf "a %s %s %s\n", x, y, curve
            } else {
                printf "a %s %s\n", x, y
            }
        }
        print ""
    }
}
' "$1"
