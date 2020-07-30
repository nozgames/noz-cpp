STRINGIFY(precision highp float;)

#if 1
STRINGIFY(

	uniform sampler2D u_texture;

	in vec2 v_texcoord;
	in vec4 v_color;
	out vec4 color;

	void main() {
		float dist = texture(u_texture, v_texcoord).a - 0.5;
		//float edgeWidth = (1.0/255.0) * 0.7071 * u_test;
		float edgeWidth = 0.7071 * length(vec2(dFdx(dist), dFdy(dist)));
		float opacity = smoothstep(edgeWidth, -edgeWidth, dist);
		color = vec4(v_color.rgb, opacity * v_color.a);    
    //color = vec4(1,1,1,texture(u_texture, v_texcoord).a);
	}
)
#else
#if 1

STRINGIFY(

	uniform sampler2D u_texture;
	uniform float u_test;

	in vec2 v_texcoord;
	in vec4 v_color;
	out vec4 color;

	void main() {
		float dist = texture(u_texture, v_texcoord).a ;
		if(dist>0.499&&dist<=0.501) {
			color = vec4(1,1,1, 1);
		} else if (dist>0.5) {
			color = vec4(1.0f-(dist-0.5)/0.5,0,0,1);
		} else {
			color = vec4(0,(dist/0.5),0,1);
		}
	}
)

#else
STRINGIFY(

	uniform sampler2D u_texture;

	out vec2 v_texcoord;
	out vec4 v_color;

	void main() {
		float distance = texture(u_texture, v_texcoord).a - 0.5;
		vec2 vv=vec2(dFdx(distance), dFdy(distance));	
		vec2 grad_dist;
		if(vv.x != 0 || vv.y!=0) grad_dist = normalize(vv);
		vec2 Jdx = dFdx(v_texcoord);
		vec2 Jdy = dFdy(v_texcoord);
		vec2 grad = vec2(grad_dist.x * Jdx.x + grad_dist.y*Jdy.y, grad_dist.x * Jdx.y + grad_dist.y*Jdy.y);
		float afwidth = 0.7071*length(grad);
		float coverage = smoothstep(afwidth, -afwidth, distance);
		gl_FragColor = vec4(v_color.rgb, coverage);
	}
)
#endif
#endif

