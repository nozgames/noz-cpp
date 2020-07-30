STRINGIFY(precision highp float;)

STRINGIFY(

	in vec3 a_position;
	in vec4 a_color;
	in vec2 a_uv;
	in float test;

	uniform mat4 u_projection;
	
	out vec2 v_texcoord;
	out vec4 v_color;
	
	void main(void) {
	  v_texcoord = a_uv;
	  v_color = a_color;
	  //gl_Position = u_projection * vec4(float(int(a_position.x))+0.375,float(int(a_position.y))+0.375,a_position.z,1.0);
	  gl_Position = u_projection * vec4(a_position,1.0);
	}

)
