#version 450

layout(location = 0) in vec4 vertex;

layout(push_constant) uniform Params
{
    float time;
    float aspect;
} params;

mat4 rotateZ(float a)
{
    float c = cos(a), s = sin(a);
    return mat4(
        vec4( c,  s, 0.0, 0.0),
        vec4(-s,  c, 0.0, 0.0),
        vec4(0.0,0.0,1.0, 0.0),
        vec4(0.0,0.0,0.0, 1.0)
    );
}

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


void main() 
{
    mat4 model = rotateZ(params.time);
    gl_Position = model * vertex;
}
