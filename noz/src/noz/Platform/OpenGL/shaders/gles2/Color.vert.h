STRINGIFY(precision highp float;)

STRINGIFY(

	uniform mat4 u_projection;

	attribute vec3 a_position;
	attribute vec4 a_color;

	varying vec4 v_color;
	
	void main(void) {
	  v_color = a_color;
	  gl_Position = u_projection * vec4(a_position,1.0);
	}

)
