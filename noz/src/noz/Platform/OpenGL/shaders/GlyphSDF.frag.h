STRINGIFY(
#version 150
precision highp float;
)

STRINGIFY(

	uniform sampler2D u_texture;

	in vec2 v_texcoord;
	in vec4 v_color;
	out vec4 color;

	const float smoothing = 16.0/16.0;

	float contour(in float d, in float w) {
		// smoothstep(lower edge0, upper edge1, x)
		return smoothstep(0.5 - w, 0.5 + w, d);
	}

	float samp(in vec2 uv, float w) {
		return contour(texture(u_texture, uv).a, w);
	}

	void main() {
/*
		float dist = texture(u_texture, v_texcoord).a;
		float width = fwidth(dist);
		float alpha = smoothstep(0.5-width, 0.5+width, dist);
*/
		vec2 uv = v_texcoord;

		// retrieve distance from texture
		float dist = texture(u_texture, uv).a;

		// fwidth helps keep outlines a constant width irrespective of scaling
		// GLSL's fwidth = abs(dFdx(uv)) + abs(dFdy(uv))
		float width = fwidth(dist);
		// Stefan Gustavson's fwidth
		//float width = 0.7 * length(vec2(dFdx(dist), dFdy(dist)));

	// basic version
		//float alpha = smoothstep(0.5 - width, 0.5 + width, dist);

	// supersampled version

		float alpha = contour( dist, width );
		//float alpha = aastep( 0.5, dist );

		// ------- (comment this block out to get your original behavior)
		// Supersample, 4 extra points
		float dscale = 0.354; // half of 1/sqrt2; you can play with this
		//vec2 duv = dscale * (dFdx(uv) + dFdy(uv));
		vec2 duv = 1/512.0;
		vec4 box = vec4(uv-duv, uv+duv);

/*
		float asum = samp( box.xy, width )
				   + samp( box.zw, width )
				   + samp( box.xw, width )
				   + samp( box.zy, width );
*/
		float asum = samp( box.xy, width )
				   + samp( box.zw, width )
				   + samp( box.xw, width )
				   + samp( box.zy, width )
				   + samp( vec2(0,box.y), width )
				   + samp( vec2(0,box.w), width )
				   + samp( vec2(box.x,0), width )
				   + samp( vec2(box.z,0), width );

		// weighted average, with 4 extra points having 0.5 weight each,
		// so 1 + 0.5*4 = 3 is the divisor
		alpha = (alpha + 0.5 * asum) / 5.0;
		//alpha = (alpha + 0.7 * asum) / 3.8;

		//alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, dist);
		alpha = smoothstep(0.5 - width, 0.5 + width, dist);

		color = vec4(v_color.rgb, alpha * v_color.a);
	}

)
