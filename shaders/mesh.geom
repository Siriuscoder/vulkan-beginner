#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) flat out vec3 vWNormal;

layout(push_constant) uniform Params
{
    float time;
    float aspect;
} params;

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

void main() 
{
    // 0. Матрицы преобразований
    mat4 view = lookAtRH(vec3(1.0, 1.0, 2.5), vec3(0.0, 0.0, 0.0));
    mat4 projection = perspectiveRH_ZO(radians(60.0), params.aspect, 0.1, 100.0);

    // 1. Вычисляем два ребра треугольника в мировых координатах
    vec3 edge1 = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 edge2 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    
    // 2. Находим перпендикуляр через векторное произведение и нормализуем
    vec3 faceNormal = normalize(cross(edge1, edge2));

    // 3. Перенаправляем вершины дальше по конвейеру
    for (int i = 0; i < 3; i++) 
    {
        gl_Position = projection * view * gl_in[i].gl_Position;
        
        vWNormal = -faceNormal;            // Для каждого фрагмента нормаль будет одинаковой
        EmitVertex();                      // Фиксируем вершину
    }
    
    EndPrimitive();                        // Завершаем треугольник
}
