STRINGIFY(precision highp float;)

STRINGIFY(

	attribute vec3 a_position;
	attribute vec4 a_color;
	attribute vec2 a_uv;

	uniform mat4 u_projection;
	
	varying vec2 v_texcoord;
	varying vec4 v_color;
	
	void main(void) {
	  v_texcoord = a_uv;
	  v_color = a_color;
	  gl_Position = u_projection * vec4(a_position,1.0);
	}

)
