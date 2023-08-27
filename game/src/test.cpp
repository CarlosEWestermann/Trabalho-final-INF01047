//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                  TRABALHO FINAL
//
#if _WIN64 || __amd64__
#define PORTABLE_64_BIT
#else
#define PORTABLE_32_BIT
#endif

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
#include "collisions.h"

// Bibliotecas para uso de sons
#include <conio.h>

#ifdef PORTABLE_32_BIT
#include <irrKlang/irrKlang.h>
#endif

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando modelo \"%s\"... ", filename);

        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        printf("OK.\n");
    }
};


// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Calcula a posicao de um ponto usando uma curva de Bezier cubica
glm::vec4 calculate_Bezier_position(glm::vec4 p1, glm::vec4 p2, glm::vec4 p3, glm::vec4 p4, float t);

// Reseta os parâmetros iniciais do jogo
void resetGame();

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// Cena virtual
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Objetos do jogo (serve para testes de colisão)
std::map<std::string, GameObject> gameObjectCollection;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraDistance = 3.5f; // Distância da câmera para a origem
float g_CameraTheta = - M_PI_2; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraSpeed[4] = {0.0f};
glm::vec4 camera_position_c  = glm::vec4(1.0f,2.0f,4.0f,1.0f);

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint vertex_shader_id;
GLuint fragment_shader_id;
GLuint program_id = 0;
GLint model_uniform;
GLint view_uniform;
GLint projection_uniform;
GLint object_id_uniform;
GLint bbox_min_uniform;
GLint bbox_max_uniform;
GLboolean use_gouraud_shading_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

// Variável que determina que as caixas foram para o lugar (jogo terminou)
bool victory = false;

// Variável que controla a posição do trofeu na curva de Bezier
float t = 0.0f;
bool rising = true;

// Variável que determina um instante de tempo da aplicação
float delta_t = 0.01f;
static double limitFPS = 1.0f / 75.0f;

// Tamanho da tela
#define SCREEN_WIDTH    1024
#define SCREEN_HEIGHT   768

// Definições dos tipos de objetos de cena
#define MAPA    0
#define HAND    1
#define CUBO    2
#define TROPHY  3
#define OBJETIVO 4

// Posições iniciais dos cubos renderizados
glm::vec4 cube_pos[4];
// Posições dos retângulos que apontam para onde devem ser empurradas as caixas
glm::vec4 obj_pos[4];
// Posições dos cantos
glm::vec3 cantos[4];

// Definindo variáveis para controle de sons
int hit_sound_counter, last_sound_counter;
#ifdef PORTABLE_32_BIT
irrklang::ISoundEngine* engine = nullptr;
#endif

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com SCREEN_WIDTH colunas e SCREEN_HEIGHT linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "INF01047 - Trabalho Final - Sokoban", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);


    glfwSetCursorPos(window, SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);


    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, SCREEN_WIDTH, SCREEN_HEIGHT); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 176-196 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Carregamos as imagens para serem utilizadas como textura
    LoadTextureImage("../../data/chumbo.jpg");      // TextureImage0
    LoadTextureImage("../../data/wood.png"); // TextureImage1
    LoadTextureImage("../../data/hand/textures/handtexture.jpg"); // TextureImage2

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel mapmodel("../../data/mapa.obj");
    ComputeNormals(&mapmodel);
    BuildTrianglesAndAddToVirtualScene(&mapmodel);

    ObjModel handmodel("../../data/hand/hand.obj");
    ComputeNormals(&handmodel);
    BuildTrianglesAndAddToVirtualScene(&handmodel);

    ObjModel cubemodel("../../data/cubo.obj");
    ComputeNormals(&cubemodel);
    BuildTrianglesAndAddToVirtualScene(&cubemodel);

    ObjModel trophymodel("../../data/trophy/trophy.obj");
    ComputeNormals(&trophymodel);
    BuildTrianglesAndAddToVirtualScene(&trophymodel);

    ObjModel objmodel("../../data/objetivo.obj");
    ComputeNormals(&objmodel);
    BuildTrianglesAndAddToVirtualScene(&objmodel);

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Variáveis auxiliares utilizadas para chamada à função
    // TextRendering_ShowModelViewProjection(), armazenando matrizes 4x4.
    glm::mat4 the_projection;
    glm::mat4 the_model;
    glm::mat4 the_view;

    // Definindo os planos de limite do mapa do jogo
    gameObjectCollection["planeEast"] = {"planeEast", PLANE, glm::vec4(), glm::vec3(4.0f, 0.0f, INFINITY) , 0.0f};
    gameObjectCollection["planeWest"] = {"planeWest", PLANE, glm::vec4(), glm::vec3(-6.0f, 0.0f, INFINITY), 0.0f};
    gameObjectCollection["planeNorth"] = {"planeNorth", PLANE, glm::vec4(), glm::vec3(INFINITY, 0.0f, 5.0f) , 0.0f};
    gameObjectCollection["planeSouth"] = {"planeSouth", PLANE, glm::vec4(), glm::vec3(INFINITY, 0.0f, -5.0f), 0.0f};

    // Definindo as paredes internas do mapa
    gameObjectCollection["wall1"] = {"wall1", CUBE, glm::vec4(-1.5f, 0.0f, 0.0f, 1.0f), glm::vec3(0.5f, 5.0f, 3.0f), 0.0f};
    gameObjectCollection["wall2"] = {"wall2", CUBE, glm::vec4(-1.0f, 0.0f, -0.5f, 1.0f), glm::vec3(3.0f, 5.0f, 0.5f), 0.0f};



    // Controle de tempo da aplicação
    double last_update = glfwGetTime();
    double delta_time = 0, curr_time = 0;

    // Iniciando a engine de sons
    #ifdef PORTABLE_32_BIT
    engine = irrklang::createIrrKlangDevice();
    
    if (!engine) {
        fprintf(stdout, "ERROR: Sound engine not loaded!");
    }
    #endif

    // variável que controla quando acionar o som de slide da caixa
    double last_slide_sound = glfwGetTime();

    // Definindo as posições iniciais dos cubos
    resetGame();

    // Ficamos em loop, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        curr_time = glfwGetTime();
        delta_time += (curr_time - last_update) / limitFPS;
        last_update = curr_time;
        if (delta_time >= 1.0) {
            // Cor de fundo do framebuffer
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

            // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
            // e também resetamos todos os pixels do Z-buffer (depth buffer).
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
            // os shaders de vértice e fragmentos).
            glUseProgram(program_id);

            // Agora computamos a matriz de Projeção.
            glm::mat4 projection;

            float nearplane = -0.1f;  // Posição do "near plane"
            float farplane  = -15.0f; // Posição do "far plane"

            // Projeção perspectiva na câmera
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);

            if (victory){
                // Computamos a posição da câmera utilizando coordenadas esféricas.  As
                // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
                // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
                // e ScrollCallback().
                float r = g_CameraDistance;
                float y = r*sin(g_CameraPhi);
                float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
                float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

                // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
                // Veja slides 195-227 e 229-234 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
                glm::vec4 camera_position_c  = glm::vec4(x,y,z,1.0f); // Ponto "c", centro da câmera
                glm::vec4 camera_lookat_l    = glm::vec4(0.0f,0.0f,0.0f,1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
                glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
                glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)

                // Computamos a matriz "View" utilizando os parâmetros da câmera para
                // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
                glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

                glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

                // Enviamos as matrizes "view" e "projection" para a placa de vídeo
                // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
                // efetivamente aplicadas em todos os pontos.
                glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
                glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

                // Desenhamos o modelo do trofeu principal
                model = Matrix_Translate(0.0f,-1.0f,0.0f)
                    * Matrix_Scale(0.1f, 0.1f, 0.1f)
                    * Matrix_Rotate_X(-1.6f)
                    * Matrix_Rotate_Z((float)glfwGetTime() * 0.2f);
                glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(object_id_uniform, TROPHY);
                glUniform1i(use_gouraud_shading_uniform, true);
                DrawVirtualObject("trophy");

                // Determina o acrescimo ou decrescimo no valor de t
                t += rising ? 0.005 : -0.005;

                // Verifica se t precisa aumentar ou diminuir
                if (t >= 1) {
                    rising = false;
                    t = 1;
                } else if (t <= 0) {
                    rising = true;
                    t = 0;
                }

                // Desenhamos então os outros dois troféus menores que se deslocam através de curvas de Bézier

                // Pontos de controle para a curva de Bezier de um dos trofeis que se deslocam
                glm::vec4 p1 = glm::vec4(-2.0f,-1.0f,1.0f,1.0f);
                glm::vec4 p2 = glm::vec4(-1.0f,1.0f,-1.0f,1.0f);
                glm::vec4 p3 = glm::vec4(1.0f,1.0f,-1.0f,1.0f);
                glm::vec4 p4 = glm::vec4(2.0f,-1.0f,1.0f,1.0f);
                glm::vec4 tr_position = calculate_Bezier_position(p1, p2, p3, p4, t);

                model = Matrix_Translate(tr_position.x, tr_position.y, tr_position.z)
                    * Matrix_Scale(0.05f, 0.05f, 0.05f)
                    * Matrix_Rotate_X(-1.6f)
                    * Matrix_Rotate_Z(1.0f);
                glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(object_id_uniform, TROPHY);
                glUniform1i(use_gouraud_shading_uniform, true);
                DrawVirtualObject("trophy");

                // Pontos de controle para a curva de Bezier o outro trofeu que se desloca
                p1 = glm::vec4(2.0f,-1.0f,-1.0f,1.0f);
                p2 = glm::vec4(1.0f,1.0f,1.0f,1.0f);
                p3 = glm::vec4(-1.0f,1.0f,1.0f,1.0f);
                p4 = glm::vec4(-2.0f,-1.0f,-1.0f,1.0f);
                tr_position = calculate_Bezier_position(p1, p2, p3, p4, t);

                model = Matrix_Translate(tr_position.x, tr_position.y, tr_position.z)
                    * Matrix_Scale(0.05f, 0.05f, 0.05f)
                    * Matrix_Rotate_X(-1.6f)
                    * Matrix_Rotate_Z(-1.0f);
                glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(object_id_uniform, TROPHY);
                glUniform1i(use_gouraud_shading_uniform, true);
                DrawVirtualObject("trophy");
            } else {
                // Computação da posição da câmera
                float y = sin(g_CameraPhi);
                float x = cos(g_CameraPhi)*cos(g_CameraTheta);
                float z = cos(g_CameraPhi)*sin(g_CameraTheta);

                // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
                glm::vec4 camera_view_vector = glm::vec4(x, y, z, 0.0f); // Vetor "view", sentido para onde a câmera está virada
                glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)
                glm::vec4 front_vec = camera_view_vector;
                front_vec.y = 0.0f;
                front_vec = front_vec / norm(front_vec);
                glm::vec4 side_vec = crossproduct(front_vec, camera_up_vector);
                glm::vec4 camera_new_position = camera_position_c + front_vec * g_CameraSpeed[0]
                                                                  - front_vec * g_CameraSpeed[2]
                                                                  - side_vec  * g_CameraSpeed[1]
                                                                  + side_vec  * g_CameraSpeed[3];
                glm::vec4 move_direction = camera_new_position - camera_position_c;

                // Computamos a matriz "View" utilizando os parâmetros da câmera para
                glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

                glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

                // Enviamos as matrizes "view" e "projection" para a placa de vídeo
                // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
                // efetivamente aplicadas em todos os pontos.
                glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
                glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

                // Desenhamos o modelo do mapa
                model = Matrix_Translate(-1.0f,0.0f,0.0f);
                glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(object_id_uniform, MAPA);
                glUniform1i(use_gouraud_shading_uniform, false);
                DrawVirtualObject("mapa");

                // Desenhamos os cubos
                for (int aux=0; aux < 4; aux++){
                    model = Matrix_Translate(cube_pos[aux].x, cube_pos[aux].y, cube_pos[aux].z);
                    glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                    glUniform1i(object_id_uniform, CUBO);
                    glUniform1i(use_gouraud_shading_uniform, false);
                    DrawVirtualObject("cubo");
                    glm::vec3 cube_bbox = glm::abs(g_VirtualScene["cubo"].bbox_min) + glm::abs(g_VirtualScene["cubo"].bbox_max);
                    cube_bbox /= 2;
                    gameObjectCollection["cube"+std::to_string(aux)] = {"cube"+std::to_string(aux), CUBE, cube_pos[aux], cube_bbox, 0.0f};
                }

                // Desenhamos os objetivos
                for (int aux=0; aux < 4; aux++){
                    model = Matrix_Translate(obj_pos[aux].x, obj_pos[aux].y, obj_pos[aux].z);
                    glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                    glUniform1i(object_id_uniform, OBJETIVO);
                    glUniform1i(use_gouraud_shading_uniform, false);
                    DrawVirtualObject("objetivo");
                    glm::vec3 obj_bbox = glm::abs(g_VirtualScene["objetivo"].bbox_min) + glm::abs(g_VirtualScene["objetivo"].bbox_max);
                    obj_bbox /= 2;
                    //gameObjectCollection["objetivo"+std::to_string(aux)] = {"objetivo"+std::to_string(aux), OBJETIVO, obj_pos[aux], obj_bbox, 0.0f};
                }

                // Desenhando a mão
                model = glm::inverse(view)
                    * Matrix_Translate(0.3f, -0.3f, -1.0f)
                    * Matrix_Scale(0.1f, 0.1f, 0.1f)
                    * Matrix_Rotate_X(1.57)
                    * Matrix_Rotate_Y(-1.37);
                glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(object_id_uniform, HAND);
                glUniform1i(use_gouraud_shading_uniform, false);
                glDisable(GL_DEPTH_TEST);
                DrawVirtualObject("hand");
                glEnable(GL_DEPTH_TEST);

                // Modelo de uma esfera para testar as colisões da mão com os cubos do jogo
                glm::vec4 hand_displacement(0.3f, -2.5f, 0.0f, 1.0f);
                glm::vec4 hand_position = model * hand_displacement;
                GameObject handObj = {"hand", SPHERE, hand_position, glm::vec3(), 0.4f};
                gameObjectCollection["hand"] = handObj;

                // Cria um objeto do jogo do player para fazer testes de colisão
                GameObject playerObj;
                playerObj.name = "player";
                playerObj.type = CUBE;
                playerObj.position_center = camera_new_position;
                playerObj.bbox = glm::vec3(0.3f, 2.0f, 0.3f);
                playerObj.radius = 0.0f;

                // Testa colisão do player para atualizar posição do player
                std::vector<std::string> collided_with = collided(playerObj, gameObjectCollection);
                if (!collided_with.empty()) {
                    for (std::string objName : collided_with) {
                        // Atualiza direção do movimento de acordo com a direção do objeto colidido
                        move_direction = updateMovementDirection(playerObj, objName, move_direction, gameObjectCollection);
                    }
                }

                // Teste de colisões da mão
                collided_with = collided(handObj, gameObjectCollection);
                if (!collided_with.empty()) {
                    for (std::string objName : collided_with) {
                        // Caso a mão tenha colidido com um cubo, é preciso atualizar a possição do cubo caso possível
                        if (objName.find("cube") != std::string::npos) {
                            glm::vec4 closest_point = getClosestPointToCenter(gameObjectCollection[objName], handObj);
                            glm::vec4 cube_dir = closest_point - handObj.position_center;

                            // Caso haja colisão, mas a direção do movimento é diferente da direção do cubo, ele não deve se movimentar
                            if (norm(cube_dir) != 0 && closest_direction(cube_dir) != closest_direction(move_direction))
                                continue;

                            int cube_idx = objName[4] - '0';
                            glm::vec4 cube_old_pos = cube_pos[cube_idx];
                            glm::vec4 cube_new_pos = cube_old_pos + move_direction;
                            GameObject cubeObj = {objName, CUBE, cube_new_pos, gameObjectCollection[objName].bbox, 0.0f};
                            std::vector<std::string> cube_collided = collided(cubeObj, gameObjectCollection);
                            if (cube_collided.empty()) {
                                // Atualiza a posição do cubo caso não tenha colidido com nada
                                cube_pos[cube_idx] = cube_new_pos;
                            } else {
                                for (std::string name : cube_collided) {
                                    if (name.compare("hand") != 0) {
                                        // Caso o cubo tenha colidido com algum objeto além da mão, é nessário atualizar a direção do movimento
                                        move_direction = updateMovementDirection(cubeObj, name, move_direction, gameObjectCollection);
                                    }
                                }
                                cube_pos[cube_idx] += move_direction;
                            }
                            if (cube_old_pos != cube_pos[cube_idx]) {
                                // Caso alguma caixa tenha se mexido e já tenha passado mais do que 400ms, tocar som da caixa deslizando
                                if (curr_time - last_slide_sound >= 0.4) {
                                    #ifdef PORTABLE_32_BIT
                                    if (engine) engine->play2D("../../data/sounds/slide.wav");
                                    #endif  
                                    last_slide_sound = curr_time;
                                }
                            }

                            // Atualiza posição do cubo nos objetos da cena
                            cubeObj.position_center = cube_pos[cube_idx];
                            gameObjectCollection[objName] = cubeObj;
                        } else {
                            // Atualiza a direção do movimento da câmera (não ajusta rotação) de acorco com colisão da mão
                            move_direction = updateMovementDirection(handObj, objName, move_direction, gameObjectCollection);
                        }
                    }
                }

                // Atualiza posição do player de acordo com o movimento computado após testes de colisão
                camera_position_c += move_direction;
                playerObj.position_center = camera_position_c;

                // Avalia se o cubo está numa posição final e sinaliza se o jogador ganhou
                int box_in_position = 0;
                for (int i = 0; i < 4; i++){
                    for (int j = 0; j < 4; j++){
                        float distance = glm::distance(cantos[j], {cube_pos[i].x, cube_pos[i].y, cube_pos[i].z});
                        if (distance < 0.6f){
                            box_in_position++;
                            hit_sound_counter |= 1 << i;
                            break;
                        }
                    }
                }
                // Caso alguma das caixas tenha atingido o objetivo, mas não seja a última ainda, tocar som de hit
                if (last_sound_counter != hit_sound_counter && box_in_position != 4) {
                    #ifdef PORTABLE_32_BIT
                    if (engine) engine->play2D("../../data/sounds/hit.wav");
                    #endif
                    last_sound_counter = hit_sound_counter;
                }

                // após a verificação, caso os cubos estiverem nos cantos (posição objetivo),
                // é declarada a vitória
                if (box_in_position == 4) {
                    victory = true;
                    // Som de vitória ao final
                    #ifdef PORTABLE_32_BIT
                    if (engine) engine->play2D("../../data/sounds/tada.wav");
                    #endif

                    // Reseta a posição para a câmera look-at
                    g_CameraDistance = 3.5f;
                    g_CameraTheta = - M_PI_2; 
                    g_CameraPhi = 0.0f;     
                }


            }
            delta_time--;
        }

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 176-196 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( program_id != 0 )
        glDeleteProgram(program_id);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    model_uniform           = glGetUniformLocation(program_id, "model"); // Variável da matriz "model"
    view_uniform            = glGetUniformLocation(program_id, "view"); // Variável da matriz "view" em shader_vertex.glsl
    projection_uniform      = glGetUniformLocation(program_id, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    object_id_uniform       = glGetUniformLocation(program_id, "object_id"); // Variável "object_id" em shader_fragment.glsl
    bbox_min_uniform        = glGetUniformLocation(program_id, "bbox_min");
    bbox_max_uniform        = glGetUniformLocation(program_id, "bbox_max");

    use_gouraud_shading_uniform = glGetUniformLocation(program_id, "use_gouraud_shading");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(program_id);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage2"), 2);
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        if (xpos != SCREEN_WIDTH/2 || ypos != SCREEN_HEIGHT/2){
            float dx = xpos - SCREEN_WIDTH/2;
            float dy = SCREEN_HEIGHT/2 - ypos;

            // Atualizamos parâmetros da câmera com os deslocamentos
            g_CameraTheta += 0.005f*dx;
            g_CameraPhi      += 0.005f*dy;

            // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
            float phimax = M_PI_2;
            float phimin = -phimax;

            if (g_CameraPhi > phimax)
                g_CameraPhi = phimax;

            if (g_CameraPhi < phimin)
                g_CameraPhi = phimin;

            // Atualizamos as variáveis globais para armazenar a posição atual do
            // cursor como sendo a última posição conhecida do cursor.
            g_LastCursorPosX = xpos;
            g_LastCursorPosY = ypos;

            glfwSetCursorPos(window, SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
        }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // // Atualizamos a distância da câmera para a origem utilizando a
    // // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.2f*yoffset;

    // // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Tecla de testes, caso pressionada T, altera o estado da var victory
    if (key == GLFW_KEY_T && action == GLFW_PRESS)
    {
        victory = !victory;
        
        // Reseta a posição para a câmera look-at
        g_CameraDistance = 3.5f;
        g_CameraTheta = - M_PI_2; 
        g_CameraPhi = 0.0f;   
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        resetGame();
    }

     if (key == GLFW_KEY_W){ // Movimento para frente
        if (action == GLFW_PRESS) {
            g_CameraSpeed[0] = 0.05f;
        } if(action == GLFW_RELEASE) {
            g_CameraSpeed[0] = 0.0f;
        }
    }

    if (key == GLFW_KEY_A) { // Movimento para a esquerda
        if (action == GLFW_PRESS) {
            g_CameraSpeed[1] = 0.05f;
        } if(action == GLFW_RELEASE) {
            g_CameraSpeed[1] = 0.0f;
        }
    }

    if (key == GLFW_KEY_S) { // Movimento para tras
        if (action == GLFW_PRESS) {
            g_CameraSpeed[2] = 0.05f;
        } if(action == GLFW_RELEASE) {
            g_CameraSpeed[2] = 0.0f;
        }
    }

    if (key == GLFW_KEY_D) { // Movimento para a direita
        if (action == GLFW_PRESS) {
            g_CameraSpeed[3] = 0.05f;
        } if(action == GLFW_RELEASE) {
            g_CameraSpeed[3] = 0.0f;
        }
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

// Função que calcula a posição na curve de Bezier de acordo com os pontos de controle e o parâmetro t
glm::vec4 calculate_Bezier_position(glm::vec4 p1, glm::vec4 p2, glm::vec4 p3, glm::vec4 p4, float t){
    float b03 = (1 - t) * (1 - t) * (1 - t);
    float b13 = 3 * t * (1 - t) * (1 - t);
    float b23 = 3 * t * t * (1 - t);
    float b33 = t * t * t;

    return b03 * p1 + b13 * p2 + b23 * p3 + b33 * p4;
}

// Função que reseta os parâmetros do jogo
void resetGame() {
    // Reseta os valores referentes à câmera
    g_CameraTheta = - M_PI_2; // Ângulo no plano ZX em relação ao eixo Z
    g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
    g_CameraSpeed[0] = 0.0f;
    g_CameraSpeed[1] = 0.0f;
    g_CameraSpeed[2] = 0.0f;
    g_CameraSpeed[3] = 0.0f;
    camera_position_c  = glm::vec4(1.0f,2.0f,4.0f,1.0f);

    // Seta a flag de jogo completo como falsa
    victory = false;

    // Reposiciona os cubos na posição inicial
    cube_pos[0] = glm::vec4(-3.5f, -0.3f, 3.5f, 1.0f);
    cube_pos[1] = glm::vec4(2.5f, -0.3f, 3.5f, 1.0f);
    cube_pos[2] = glm::vec4(-3.5f, -0.3f, -3.5f, 1.0f);
    cube_pos[3] = glm::vec4(2.5f, -0.3f, -3.5f, 1.0f);

    // Posiciona os objetivos na posição
    obj_pos[0] = glm::vec4(-5.5f, 0.0f, 4.5f, 1.0f);
    obj_pos[1] = glm::vec4(3.5f, 0.0f, 4.5f, 1.0f);
    obj_pos[2] = glm::vec4(-5.5f, 0.0f, -4.5f, 1.0f);
    obj_pos[3] = glm::vec4(3.5f, 0.0f, -4.5f, 1.0f);

    // Defino as posições dos cantos
    cantos[0] = glm::vec3(-5.5f, 0.2f, 4.5f);
    cantos[1] = glm::vec3(3.5f, 0.2f, 4.5f);
    cantos[2] = glm::vec3(-5.5f, 0.2f, -4.5f);
    cantos[3] = glm::vec3(3.5f, 0.2f, -4.5f);

    // reinicia o vetor de verificação dos cubos nos cantos (local correto)
    //cubos_in_corner[0] = 0;
    //cubos_in_corner[1] = 0;
    //cubos_in_corner[2] = 0;
    //cubos_in_corner[3] = 0;

    // Reseta os parâmetros para as curvas de bezier dos troféus
    t = 0.0f;
    rising = true;

    // Reseta variáveis de controle de hit para o som e também toca som de início de jogo
    hit_sound_counter = last_sound_counter = 0;
    #ifdef PORTABLE_32_BIT
    if (engine) engine->play2D("../../data/sounds/start.wav");
    #endif
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :

