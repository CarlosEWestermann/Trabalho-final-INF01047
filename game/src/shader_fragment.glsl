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
uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;

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

    // Parâmetros que definem as propriedades espectrais da superfície
    vec3 Kd; // Refletância difusa
    vec3 Ks; // Refletância especular
    vec3 Ka; // Refletância ambiente
    float q; // Expoente especular para o modelo de iluminação de Phong
    vec3 Ia; // Espectro da luz ambiente
    float lambert = max(0,dot(n,l));
    vec3 lambert_diffuse_term = vec3(0,0,0);
    float phong = max(0,dot(r,v));
    vec3 phong_specular_term = vec3(0,0,0);
    vec3 ambient_term = vec3(0,0,0);

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

    if (object_id == SPHERE)
    {
        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;

        float raio = length(position_model - bbox_center);
        float theta = atan(position_model.x, position_model.z);
        float phi = asin(position_model.y/raio);

        U = (theta + M_PI)/(2*M_PI);
        V = (phi + M_PI/2)/M_PI;

        // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
        Kd = texture(TextureImage0, vec2(U,V)).rgb;
        Ia = vec3(0.8,0.8,0.8);

        lambert_diffuse_term = Kd * lambert;
        ambient_term = Kd * Ia;
    }
    else if (object_id == SPACESHIP)
    {
       Kd = vec3(0.5, 0.5, 0.5);
       lambert_diffuse_term = Kd * lambert;
    }

    // Cor final do fragmento calculada com uma combinação dos termos difuso, especular, e ambiente.
    color.rgb = lambert_diffuse_term + ambient_term + phong_specular_term;
    color.a = 1;

    // Cor final com correção gamma, considerando monitor sRGB.
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
}

