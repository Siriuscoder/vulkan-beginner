#version 450

layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

// NOTICE: Vulkan rasterizer pipeline transformations
//
// Vertex Shader:
//   gl_Position = (x, y, z, w)
// 
// Clipping:
//   -w ≤ x ≤ +w
//   -w ≤ y ≤ +w
//    0 ≤ z ≤ +w
// 
// Perspective divide:
//   x_ndc = x / w
//   y_ndc = y / w
//   z_ndc = z / w
// 
// Viewport transform:
//   x_window = x0 + (x_ndc + 1) * w_vp / 2
//   y_window = y0 + (y_ndc + 1) * h_vp / 2
//   z_window = minD + z_ndc * (maxD - minD)
//


void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
