STRINGIFY(precision highp float;)

STRINGIFY(

	uniform sampler2D u_texture;

	in vec2 v_texcoord;
	in vec4 v_color;
	out vec4 color;

	void main(void) {
		color = v_color * vec4(1,1,1,texture(u_texture,v_texcoord).a);
	}

)
