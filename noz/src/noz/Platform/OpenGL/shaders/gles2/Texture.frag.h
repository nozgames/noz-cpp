STRINGIFY(precision highp float;)

STRINGIFY(

	uniform sampler2D u_texture;

	varying vec2 v_texcoord;
	varying vec4 v_color;

	void main(void) {
		gl_FragColor = v_color * texture2D(u_texture,v_texcoord);
	}

)
