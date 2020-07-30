STRINGIFY(precision highp float;)

STRINGIFY(

	uniform mat4 u_projection;

	in vec3 a_position;
	in vec4 a_color;

	out vec4 v_color;
	
	void main(void) {
	  v_color = a_color;
	  gl_Position = u_projection * vec4(a_position,1.0);
	}

)
