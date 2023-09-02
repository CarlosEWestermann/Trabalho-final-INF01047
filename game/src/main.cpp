/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////// INCLUINDO BIBLIOTECAS ///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers específicos de C++
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
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////// DEFINIÇÃO DE ESTRUTURAS /////////////////////////////////////////////////////////////////////////////////////////////////////////
int globalBateu = 0;
// Estrutura que representa um modelo geométrico carregado a partir de um arquivo ".obj".
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        // Se basepath == NULL, então setamos basepath como o dirname do filename, para que os arquivos MTL sejam corretamente carregados caso estejam no mesmo diretório dos arquivos OBJ.
        std::string fullpath(filename);
        std::string dirname;

        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i+1);
                basepath = dirname.c_str();
            }
        }

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "*********************************************\n",
                    filename);
                throw std::runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }
        printf("OK.\n");
    }
};

// Definimos uma estrutura que armazenará dados necessários para renderizar cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;                      // Nome do objeto
    size_t       first_index;               // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices;               // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode;            // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id;    // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min;                  // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

struct BoundingSphere
{
    glm::vec3 center;
    float radius;
};

struct BoundingCube
{
    glm::vec3 upperFrontRight;
    glm::vec3 lowerBackLeft;
};

struct BoundingCircle
{
    glm::vec3 center;
    float radius;
    glm::vec3 normal;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////// CABEÇALHO DAS FUNÇÕES ///////////////////////////////////////////////////////////////////////////////////////////////////////////

void BuildTrianglesAndAddToVirtualScene(ObjModel*);                            // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model);                                          // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles();                                                   // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename);                                   // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name);                               // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);                                // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename);                              // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id);                       // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);   // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*);                                             // Função para debugging

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

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e outras informações do programa.
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do usuário.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

//funcoes auxiliares
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////// DEFINIÇÃO DE VARIÁVEIS GLOBAIS //////////////////////////////////////////////////////////////////////////////////////////////////

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário (map).
// Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos objetos dentro da variável g_VirtualScene.
// Veja na função main() como estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false;  // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Armazenam a última posição do cursor do mouse, para que possamos calcular quanto que o mouse se movimentou entre dois instantes de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

bool g_IsFreeCamera = true;
glm::vec3 g_FreeCameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_FreeCameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 g_FreeCameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float g_CameraSpeed = 0.5f;
float g_Yaw = -90.0f;
float g_Pitch = 0.0f;
bool g_WKeyPressed = false;
bool g_SKeyPressed = false;
bool g_AKeyPressed = false;
bool g_DKeyPressed = false;
bool g_SpaceKeyPressed = false;

#define CAMERA_ACCELERATION 3.0f
#define MAX_CAMERA_SPEED 20.0f

// Teste de colisão: esfera-esfera
bool checkSphereSphereCollision(BoundingSphere sphere1, BoundingSphere sphere2)
{
    float distance = glm::length(sphere1.center - sphere2.center); // dist um centro do outro
    return distance <= (sphere1.radius + sphere2.radius);          // verifica se tem intersecao
}

// Teste de colisão: esfera-circulo
bool checkSphereCircleCollision(BoundingSphere sphere, BoundingCircle circle)
{
    /* RASCUNHO
    // para um ponto p estar contido na esfera:
    glm::length(p - sphere.center) <= sphere.radius;
    // para o mesmo ponto p estar contido no circulo:
    glm::length(p - circle.center) <= circle.radius;
    // E
    dotproduct(p - circle.center, circle.normal) == 0  // o vetor (p - circle.center) é perpendicular a normal do circuloww
    */
}

// Teste de colisão: esfera-cubo
bool checkSphereCubeCollision(BoundingSphere sphere, BoundingCube cube)
{
    /* RASCUNHO
    // para um ponto p estar contido na esfera:
    glm::length(p - sphere.center) <= sphere.radius;
    // para o mesmo ponto p tambem estar contido no cubo:
    (p.x >= cube.lowerBackLeft.x && p.x <= cube.upperFrontRight.x)
    // E
    (p.y >= cube.lowerBackLeft.y && p.y <= cube.upperFrontRight.y)
    // E
    (p.z >= cube.lowerBackLeft.z && p.z <= cube.upperFrontRight.z)
     */
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////// IMPLEMENTAÇÃO DAS FUNÇÕES //////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do sistema operacional, onde poderemos renderizar com OpenGL.
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

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas de pixels, e com título "Spaceship Game".
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "Spaceship Game", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for redimensionada, por consequência alterando o tamanho do "framebuffer".
    // (região de memória onde são armazenados os pixels da imagem)
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados para renderização.
    LoadShadersFromFiles();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////// CARREGANDO TEXTURAS E MODELOS 3D NO FORMATO OBJ ////////////////////////////////////////////////////

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/universe.png");                       // TextureImage0
    LoadTextureImage("../../data/spaceshiptextures/emi.jpg");          // TextureImage1
    LoadTextureImage("../../data/spaceshiptextures/blender.jpg");      // TextureImage2
    LoadTextureImage("../../data/asteroidtextures/rock_texture.jpg");  // TextureImage3
    LoadTextureImage("../../data/cointextures/gold.png");              // TextureImage4
    LoadTextureImage("../../data/rockettextures/rocket.png");          // TextureImage5

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel spheremodel("../../data/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel spaceshipmodel("../../data/spaceship.obj");
    ComputeNormals(&spaceshipmodel);
    BuildTrianglesAndAddToVirtualScene(&spaceshipmodel);

    ObjModel asteroidmodel("../../data/asteroid.obj");
    ComputeNormals(&asteroidmodel);
    BuildTrianglesAndAddToVirtualScene(&asteroidmodel);

    ObjModel coinmodel("../../data/coin.obj");
    ComputeNormals(&coinmodel);
    BuildTrianglesAndAddToVirtualScene(&coinmodel);

    ObjModel rocketmodel("../../data/rocket.obj");
    ComputeNormals(&rocketmodel);
    BuildTrianglesAndAddToVirtualScene(&rocketmodel);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    char lastPressedKey;

    float meteorStartTime = 0;
    float meteorTime;
    int meteorColor;

    glm::vec4 bezierControlPoint1;
    glm::vec4 bezierControlPoint2;
    glm::vec4 bezierControlPoint3;
    glm::vec4 bezierControlPoint4;

    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Calculando o tempo passado desde a última animação
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Definimos a cor do "fundo" do framebuffer como branco.
        //           R     G     B     A
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima, e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /////// CALCULANDO POSIÇÃO E SENTIDO DA CAMERA /////////////////////////////////////////////////////////////

        glm::vec4 camera_position_c;
        glm::vec4 camera_lookat_l;
        glm::vec4 camera_view_vector;
        glm::vec4 camera_up_vector;

        if (g_IsFreeCamera)
        {
            if (g_WKeyPressed || g_SKeyPressed || g_AKeyPressed || g_DKeyPressed)
            {
                g_CameraSpeed += CAMERA_ACCELERATION * deltaTime;
                if (g_CameraSpeed > MAX_CAMERA_SPEED)
                {
                    g_CameraSpeed = MAX_CAMERA_SPEED;
                }
            }
            else
            {
                g_CameraSpeed -= CAMERA_ACCELERATION * deltaTime * 2.0f;
                if (g_CameraSpeed < 0.0f)
                {
                    g_CameraSpeed = 0.0f;
                }
            }
            if (g_WKeyPressed)
            {
                lastPressedKey = 'W';
                g_FreeCameraPosition += g_CameraSpeed * g_FreeCameraFront * deltaTime;
            }
            if (g_SKeyPressed)
            {
                lastPressedKey = 'S';
                g_FreeCameraPosition -= g_CameraSpeed * g_FreeCameraFront * deltaTime;
            }
            if (g_AKeyPressed)
            {
                lastPressedKey = 'A';
                g_FreeCameraPosition -= glm::normalize(glm::cross(g_FreeCameraFront, g_FreeCameraUp)) * g_CameraSpeed * deltaTime; //TODO: PODE USAR ESSAS FUNCOES?
            }
            if (g_DKeyPressed)
            {
                lastPressedKey = 'D';
                g_FreeCameraPosition += glm::normalize(glm::cross(g_FreeCameraFront, g_FreeCameraUp)) * g_CameraSpeed * deltaTime; //TODO: PODE USAR ESSAS FUNCOES?
            }

            if (!(g_WKeyPressed || g_SKeyPressed || g_AKeyPressed || g_DKeyPressed))
            {
                if (lastPressedKey == 'W')
                {
                g_FreeCameraPosition += g_CameraSpeed * g_FreeCameraFront * deltaTime;
                }
                if (lastPressedKey == 'S')
                {
                    g_FreeCameraPosition -= g_CameraSpeed * g_FreeCameraFront * deltaTime;
                }
                if (lastPressedKey == 'A')
                {
                    g_FreeCameraPosition -= glm::normalize(glm::cross(g_FreeCameraFront, g_FreeCameraUp)) * g_CameraSpeed * deltaTime; //TODO: PODE USAR ESSAS FUNCOES?
                }
                if (lastPressedKey == 'D')
                {
                    g_FreeCameraPosition += glm::normalize(glm::cross(g_FreeCameraFront, g_FreeCameraUp)) * g_CameraSpeed * deltaTime; //TODO: PODE USAR ESSAS FUNCOES?
                }
            }
            camera_position_c = glm::vec4(g_FreeCameraPosition, 1.0f);
            camera_view_vector = glm::vec4(g_FreeCameraFront, 0.0f);
            camera_up_vector = glm::vec4(g_FreeCameraUp, 0.0f);
        }
        else
        {
            // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
            //camera_position_c  = glm::vec4(0,0,-100,1.0f);              // Ponto "c", centro da câmera
            camera_lookat_l    = glm::vec4(0.0f,0.0f,-300.0f,1.0f);       // Ponto "l", para onde a câmera (look-at) estará sempre olhando
            camera_view_vector = camera_lookat_l - camera_position_c;  // Vetor "view", sentido para onde a câmera está virada
            camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f);       // Vetor "up" fixado para apontar para o "céu" (eito Y global)
        }

        // Computamos a matriz "View" utilizando os parâmetros da câmera para definir o sistema de coordenadas da câmera.
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far estão no sentido negativo!
        float nearplane = -0.1f;                // Posição do "near plane"
        float farplane  = -500.0f;               // Posição do "far plane"
        float field_of_view = 3.141592 / 3.0f;  // FOV PI/3 radianos = 60 graus

        // Projeção Perspectiva.
        projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo (GPU).
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /////// POSICIONANDO OBJETOS VIRTUAIS NA CENA //////////////////////////////////////////////////////////////

        #define SPHERE 0
        #define SPACESHIP 1
        #define ASTEROID 2
        #define COIN 3
        #define REDBALL 4
        #define BLUEBALL 5
        #define ROCKET 6

        glm::mat4 identity = Matrix_Identity();
        glm::mat4 model = Matrix_Identity();


        // Desenhamos o modelo da esfera
        model = Matrix_Translate(camera_position_c.x, camera_position_c.y, camera_position_c.z);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, SPHERE);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        DrawVirtualObject("the_sphere");
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);


        if (g_SpaceKeyPressed)
        {
            // Desenhamos o modelo do foguete
            model = Matrix_Translate(0, 0, -15)*Matrix_Rotate_Y(0.1,0.1,0.1);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, ROCKET);
            glDisable(GL_CULL_FACE);
            DrawVirtualObject("the_rocket");
            glEnable(GL_CULL_FACE);
        }

/*
        // Desenhamos vários meteoros
        if (meteorStartTime == 0)
        {
            meteorStartTime = currentFrame;
            meteorTime = 2 + (rand() % 5);  // sorteia entre 2 e 6 segundos
            meteorColor = 4 + (rand() % 2); // sorteia entre REDBALL(4) e BLUEBALL(5)
            bezierControlPoint1 = glm::vec4(-200 + rand() % 500, 300, -300.0f, 1);
            bezierControlPoint2 = glm::vec4(-200 + rand() % 500, 100.0f, -300.0f, 1);
            bezierControlPoint3 = glm::vec4(-200 + rand() % 500, -100.0f, -300.0f, 1);
            bezierControlPoint4 = glm::vec4(-200 + rand() % 500, -300.0f, -300.0f, 1);
        }
        else
        {
            if (currentFrame - meteorStartTime >= meteorTime)
            {
                meteorStartTime = 0;
            }
            else
            {
                float t =  (1/meteorTime)*(currentFrame-meteorStartTime); // mapeia o intervalo [meteorStartTime, meteorStartTime + meteorTime] -> [0, 1]
                glm::vec4 point_on_curve = (float)(pow(1-t,3))*bezierControlPoint1 + (float)(3*t*pow(1-t,2))*bezierControlPoint2 + (float)(3*pow(t,2)*(1-t))*bezierControlPoint3 + (float)(pow(t,3))*bezierControlPoint4;
                model = Matrix_Translate(point_on_curve.x, point_on_curve.y, point_on_curve.z)*Matrix_Scale(1.0f/300.0f, 1.0f/300.0f, 1.0f/300.0f);
                glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE , glm::value_ptr(model));
                glUniform1i(g_object_id_uniform, meteorColor);
                DrawVirtualObject("asteroid");
            }
        }

        // Desenhamos o modelo da moeda
        model = Matrix_Translate(0, 0, -15)*Matrix_Scale(1,1,0.2);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, COIN);
        DrawVirtualObject("the_coin");

        // Desenhamos os asteroides
        glm::vec3 asteroidsCenter[] = {
            glm::vec3(0,10,-36),
            glm::vec3(10,10,-18),
            glm::vec3(18,30,-36)
        };

        // bounding spheres dos asteroides
        BoundingSphere asteroidBoundingSpheres[] = {
            { asteroidsCenter[0], 2.5f },
            { asteroidsCenter[1], 2.5f },
            { asteroidsCenter[2], 5.0f }
        };

        model = Matrix_Identity();

        PushMatrix(model);

            model = model*Matrix_Translate(asteroidsCenter[0].x, asteroidsCenter[0].y, asteroidsCenter[0].z)*Matrix_Scale(1.0f/300.0f, 1.0f/300.0f, 1.0f/300.0f);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, ASTEROID);
            DrawVirtualObject("asteroid");

        PopMatrix(model);

        PushMatrix(model);
            model = model*Matrix_Translate(asteroidsCenter[1].x, asteroidsCenter[1].y, asteroidsCenter[1].z)*Matrix_Scale(1.0f/300.0f, 1.0f/300.0f, 1.0f/300.0f);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, ASTEROID);
            DrawVirtualObject("asteroid");

        PopMatrix(model);

        PushMatrix(model);

            model = model*Matrix_Translate(asteroidsCenter[2].x, asteroidsCenter[2].y, asteroidsCenter[2].z)*Matrix_Scale(1.0f/150.0f, 1.0f/150.0f, 1.0f/150.0f);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, ASTEROID);
            DrawVirtualObject("asteroid");

        PopMatrix(model);


        // Desenhamos modelo da nave
        model = Matrix_Translate(0,0,-18)*Matrix_Rotate_Y(3.141592);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE , glm::value_ptr(model));
        glUniformMatrix4fv(g_view_uniform, 1, GL_FALSE, glm::value_ptr(identity));
        glUniform1i(g_object_id_uniform, SPACESHIP);
        DrawVirtualObject("the_spaceship");

        glm::vec3 shipPosition = glm::vec3(camera_position_c.x, camera_position_c.y, camera_position_c.z) - glm::vec3(camera_view_vector.x* -18, camera_view_vector.y* -18, camera_view_vector.z* -18);

        // Bounding spheres da nave
        BoundingSphere shipBoundingSphere  = { glm::vec3(shipPosition.x, shipPosition.y, shipPosition.z), 1.0f };

        // verifica nave (esfera) vs cada asteroide (esfera)
        bool toggle = false;
        for(int i = 0; i < 3; ++i) {
            if (checkSphereSphereCollision(shipBoundingSphere, asteroidBoundingSpheres[i])) {
                toggle = true;
            }
        }
*/
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Imprimimos na tela informação sobre o número de quadros renderizados por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // O framebuffer onde OpenGL executa as operações de renderização não é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas pela biblioteca GLFW.
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

// Função que desenha um objeto armazenado em g_VirtualScene.
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ apontados pelo VAO como linhas.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão utilizados para renderização.
void LoadShadersFromFiles()
{
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo (GPU)!
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model");       // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view");        // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection");  // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id");   // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage4"), 4);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage5"), 5);
    glUseProgram(0);
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto por Gouraud, onde a normal de cada vértice vai ser a média das normais de todas as faces que compartilham este vértice.
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

                model_coefficients.push_back( vx );   // X
                model_coefficients.push_back( vy );   // Y
                model_coefficients.push_back( vz );   // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx );    // X
                    normal_coefficients.push_back( ny );    // Y
                    normal_coefficients.push_back( nz );    // Z
                    normal_coefficients.push_back( 0.0f );  // W
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
        theobject.first_index    = first_index;                     // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1;    // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;                    // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
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
    GLuint location = 0;             // "(location = 0)" em "shader_vertex.glsl"
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
        location = 1;               // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4;   // vec4 em "shader_vertex.glsl"
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
        location = 2;                // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2;    // vec2 em "shader_vertex.glsl"
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

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename" e colocamos seu conteúdo em memória, apontado pela variável "shader_string".
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

    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um Vertex Shader e um Fragment Shader.
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
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema operacional for redimensionada, por consequência alterando o tamanho do "framebuffer".
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A função "glViewport" define o mapeamento das "normalized device coordinates" (NDC) para "pixel coordinates".
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura / altura), a qual será utilizada na definição das matrizes de projeção.
    g_ScreenRatio = (float)width / height;
}

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a posição atual do cursor nas variáveis g_LastCursorPosX e g_LastCursorPosY.
        // Também, setamos a variável g_LeftMouseButtonPressed como true, para saber que o usuário está com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão direito do mouse, guardamos a posição atual do cursor nas variáveis g_LastCursorPosX e g_LastCursorPosY.
        // Também, setamos a variável g_RightMouseButtonPressed como true, para saber que o usuário está com o botão direito pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão direito do mouse, atualizamos a variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão do meio do mouse, guardamos a posição atual do cursor nas variáveis g_LastCursorPosX e g_LastCursorPosY.
        // Também, setamos a variável g_MiddleMouseButtonPressed como true, para saber que o usuário está com o botão do meio pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão do meio do mouse, atualizamos a variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (g_LeftMouseButtonPressed)
	{
		float dx = xpos - g_LastCursorPosX;
		float dy = ypos - g_LastCursorPosY;
        float sensitivity = 0.05f;
        dx *= sensitivity;
        dy *= sensitivity;

        if (g_IsFreeCamera)
        {
            g_Yaw += dx;
            g_Pitch -= dy;

            if (g_Pitch > 89.0f)
                g_Pitch = 89.0f;
            if (g_Pitch < -89.0f)
                g_Pitch = -89.0f;

            glm::vec3 front;
            front.x = cos(glm::radians(g_Yaw)) * cos(glm::radians(g_Pitch)); //TODO: PODE USAR ISSO?
            front.y = sin(glm::radians(g_Pitch));
            front.z = sin(glm::radians(g_Yaw)) * cos(glm::radians(g_Pitch));
            g_FreeCameraFront = glm::normalize(front);
        }

		g_LastCursorPosX = xpos;
		g_LastCursorPosY = ypos;
	}
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{

}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla F, fazemos um "toggle" do tipo de câmera.
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        g_IsFreeCamera = !g_IsFreeCamera;
    }

    if (g_IsFreeCamera)
    {
        if (key == GLFW_KEY_W)
        {
            g_WKeyPressed = (action != GLFW_RELEASE);
        }
        if (key == GLFW_KEY_S)
        {
            g_SKeyPressed = (action != GLFW_RELEASE);
        }
        if (key == GLFW_KEY_A)
        {
            g_AKeyPressed = (action != GLFW_RELEASE);
        }
        if (key == GLFW_KEY_D)
        {
            g_DKeyPressed = (action != GLFW_RELEASE);
        }
    }

    // Se o usuário apertar espaço
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        g_SpaceKeyPressed = true;
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas subsequentes da função!
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

// Função para debugging: imprime no terminal todas informações de um modelo geométrico carregado de um arquivo ".obj".
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
