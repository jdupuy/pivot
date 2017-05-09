// --------------------------------------------------
// Uniforms
// --------------------------------------------------
uniform vec3 u_ClearColor;

// --------------------------------------------------
// Vertex shader
// --------------------------------------------------
#ifdef VERTEX_SHADER
void main() {
	// draw a full screen quad
	vec2 p = vec2(gl_VertexID & 1, gl_VertexID >> 1 & 1) * 2.0 - 1.0;
	gl_Position = vec4(p, 0.999, 1); // make sure the cubemap is visible
}
#endif

// --------------------------------------------------
// Fragment shader
// --------------------------------------------------
#ifdef FRAGMENT_SHADER
layout(location = 0) out vec4 o_FragColor;

void main() {
	o_FragColor = vec4(u_ClearColor, 1);
}
#endif


