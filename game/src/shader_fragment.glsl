#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define SPHERE 0
#define SPACESHIP 1
#define ASTEROID 2
#define COIN 3
#define REDBALL 4
#define BLUEBALL 5
#define ROCKET 6
uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;
uniform sampler2D TextureImage4;
uniform sampler2D TextureImage5;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida através da interpolação, feita pelo rasterizador, da posição de cada vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(camera_position - p);

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    // Vetor que define o sentido da reflexão especular ideal.
    vec4 r = -l + 2*n*dot(n,l);

    float lambert = max(0,dot(n,l));
    float phong = max(0,dot(r,v));

    if (object_id == SPHERE)
    {
        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;

        float raio = length(position_model - bbox_center);
        float theta = atan(position_model.x, position_model.z);
        float phi = asin(position_model.y/raio);

        float U = (theta + M_PI)/(2*M_PI);
        float V = (phi + M_PI/2)/M_PI;

        vec3 Kd = texture(TextureImage0, vec2(U,V)).rgb;
        vec3 Ia = vec3(0.8,0.8,0.8);

        vec3 lambert_diffuse_term = Kd * lambert;
        vec3 ambient_term = Kd * Ia;

        color.rgb = lambert_diffuse_term + ambient_term;
        color.a = 1;
    }
    else if (object_id == SPACESHIP)
    {
        vec3 Kd1 = texture(TextureImage1, texcoords).rgb;
        vec3 Kd2 = texture(TextureImage2, texcoords).rgb;
        vec3 Kd = (Kd1 + Kd2) / 2.0;

        vec3 Ks = vec3(0.6,0.6,0.6);
        vec3 Ka = vec3(0.2, 0.2, 0.2);
        float q = 30.0;

        vec3 I = vec3(1.0,1.0,1.0);
        vec3 Ia = vec3(0.2,0.2,0.2);

        vec3 lambert_diffuse_term = Kd * I * max(0, dot(n, l));

        vec3 ambient_term = Ka * Ia;

        vec3 phong_specular_term = Ks * I * pow(max(0, dot(r, v)), q);

        color.rgb = lambert_diffuse_term + ambient_term + phong_specular_term;
        color.a = 1.0;
    }
    else if (object_id == COIN)
    {
        vec3 Kd = texture(TextureImage4, texcoords).rgb;
        vec3 I = vec3(1.0, 1.0, 1.0);

        vec3 lambert_diffuse_term = Kd * I * max(0, dot(n, l));

        color.rgb = lambert_diffuse_term;
        color.a = 1.0;
    }
    else if (object_id == REDBALL)
    {
        vec3 Kd = texture(TextureImage3, texcoords).rgb;
        vec3 lambert_diffuse_term = Kd * lambert;

        color.rgb = lambert_diffuse_term + vec3(0.5,0,0);
        color.a = 1;
    }
    else if (object_id == BLUEBALL)
    {
        vec3 Kd = texture(TextureImage3, texcoords).rgb;
        vec3 lambert_diffuse_term = Kd * lambert;

        color.rgb = lambert_diffuse_term + vec3(0,0,0.2);
        color.a = 1;
    }
<<<<<<< HEAD
    else if (object_id == ROCKET)
    {
        vec3 Kd = texture(TextureImage5, texcoords).rgb;
        vec3 lambert_diffuse_term = Kd * lambert;

        color.rgb = lambert_diffuse_term;
        color.a = 1;
=======
    else if (object_id == ASTEROID)
    {
        vec4 h = normalize(l + v);

        float lambert = max(0.0, dot(n, l));

        float blinn_phong = pow(max(0.0, dot(n, h)), 20.0);

        vec3 Kd = texture(TextureImage3, texcoords).rgb;
        vec3 Ks = vec3(0.3, 0.3, 0.3);
        vec3 Ka = vec3(0.0, 0.0, 0.0);
        vec3 I = vec3(1.0, 1.0, 1.0);
        vec3 Ia = vec3(0.2, 0.2, 0.2);

        vec3 lambert_diffuse_term = Kd * I * lambert;

        vec3 ambient_term = Ka * Ia;

        vec3 blinn_phong_specular_term = Ks * I * blinn_phong;

        color.rgb = lambert_diffuse_term + ambient_term + blinn_phong_specular_term;
        color.a = 1.0;
>>>>>>> b6166c5ffb39578e1466d236b97fc29b455702c2
    }

    // Cor final com correção gamma, considerando monitor sRGB.
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
}

