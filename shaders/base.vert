#version 450

layout(location = 0) out vec3 vWNormal;
layout(push_constant) uniform Params
{
    float time;
    float aspect;
} params;

// 5 вершин: 4 основания + вершина
const vec3 P[5] = vec3[](
    vec3(-1.0, -1.0, 0.0),
    vec3( 1.0, -1.0, 0.0),
    vec3( 1.0,  1.0, 0.0),
    vec3(-1.0,  1.0, 0.0),
    vec3( 0.0,  0.0, 1.5)  // вершина вверх по Z
);

// 6 треугольников (18 вершин) CCW
const int I[18] = int[](
    // Основание
    0,1,2,
    0,2,3,
    1,0,4,
    2,1,4,
    3,2,4,
    0,3,4
);

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

// Vulkan-friendly perspective: Right Hand, depth 0..1
mat4 perspectiveRH_ZO(float fovy, float aspect, float zNear, float zFar)
{
    float f = 1.0 / tan(fovy * 0.5);

    // column-major mat4(vec4 col0, col1, col2, col3)
    return mat4(
        vec4(f / aspect, 0.0, 0.0,                              0.0),
        vec4(0.0,         -f, 0.0,                              0.0),              // -f: flip Y (Vulkan specific)
        vec4(0.0,        0.0, zFar / (zNear - zFar),           -1.0),
        vec4(0.0,        0.0, (zNear * zFar) / (zNear - zFar),  0.0)
    );
}

// translation (column-major)
mat4 translate(vec3 t)
{
    return mat4(
        vec4(1,0,0,0),
        vec4(0,1,0,0),
        vec4(0,0,1,0),
        vec4(t,1)
    );
}

// lookAt Right Hand 
mat4 lookAtRH(vec3 eye, vec3 target)
{
    vec3 worldUp = vec3(0,0,1);
    vec3 F = normalize(target - eye);
    vec3 R = normalize(cross(worldUp, F));
    vec3 U = cross(F, R);

    mat4 m = mat4(
        vec4(R, 0.0),
        vec4(U, 0.0),
        vec4(-F, 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );

    return transpose(m) * translate(-eye);
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
    vec3 pos = P[I[gl_VertexIndex]];

    int tri = gl_VertexIndex / 3;
    int i0 = I[tri*3 + 0];
    int i1 = I[tri*3 + 1];
    int i2 = I[tri*3 + 2];

    // нормаль по треугольнику
    vec3 normal = normalize(cross(P[i0] - P[i2], P[i0] - P[i1]));

    mat4 model = rotateZ(params.time);
    mat4 view = lookAtRH(vec3(1.0, 1.0, 2.5), vec3(0.0, 0.0, 0.0));
    mat4 projection = perspectiveRH_ZO(radians(60.0), params.aspect, 0.1, 100.0);

    gl_Position = projection * view * model * vec4(pos, 1.0);
    vWNormal = mat3(model) * normal;
}
