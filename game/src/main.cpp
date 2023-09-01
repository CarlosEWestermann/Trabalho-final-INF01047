#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <glad/glad.h>   
#include <GLFW/glfw3.h>  
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_obj_loader.h>
#include <stb_image.h>
#include "utils.h"
#include "matrices.h"
struct ObjModel
{
	tinyobj::attrib_t                 attrib;
	std::vector<tinyobj::shape_t>     shapes;
	std::vector<tinyobj::material_t>  materials;
	ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
	{
		printf("Carregando objetos do arquivo \"%s\"...\n", filename);
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
struct SceneObject
{
	std::string  name;                      
	size_t       first_index;               
	size_t       num_indices;               
	GLenum       rendering_mode;            
	GLuint       vertex_array_object_id;    
	glm::vec3    bbox_min;                  
	glm::vec3    bbox_max;
};
void BuildTrianglesAndAddToVirtualScene(ObjModel*);                            
void ComputeNormals(ObjModel* model);                                          
void LoadShadersFromFiles();                                                   
void LoadTextureImage(const char* filename);                                   
void DrawVirtualObject(const char* object_name);                               
GLuint LoadShader_Vertex(const char* filename);                                
GLuint LoadShader_Fragment(const char* filename);                              
void LoadShader(const char* filename, GLuint shader_id);                       
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);   
void PrintObjModelInfo(ObjModel*);                                             
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
std::map<std::string, SceneObject> g_VirtualScene;
std::stack<glm::mat4>  g_MatrixStack;

float g_ScreenRatio = 1.0f;
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false;  
bool g_MiddleMouseButtonPressed = false; 
bool g_IsFreeCamera = false;
double g_LastCursorPosX, g_LastCursorPosY;
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;
GLuint g_NumLoadedTextures = 0;
bool g_ShowInfoText = true;
float g_CameraTheta = 0.0f;       
float g_CameraPhi = 0.15f;        
float g_CameraDistance = 3.5f;
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

#define CAMERA_ACCELERATION 1.0f
#define MAX_CAMERA_SPEED 20.0f


int main(int argc, char* argv[])
{
	int success = glfwInit();
	if (!success)
	{
		fprintf(stderr, "ERROR: glfwInit() failed.\n");
		std::exit(EXIT_FAILURE);
	}
	glfwSetErrorCallback(ErrorCallback);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window;
	window = glfwCreateWindow(800, 600, "Spaceship Game", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
		std::exit(EXIT_FAILURE);
	}
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, CursorPosCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
	FramebufferSizeCallback(window, 800, 600); 
	const GLubyte *vendor      = glGetString(GL_VENDOR);
	const GLubyte *renderer    = glGetString(GL_RENDERER);
	const GLubyte *glversion   = glGetString(GL_VERSION);
	const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);
	printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);
	LoadShadersFromFiles();
	LoadTextureImage("../../data/universe.png");
	ObjModel spheremodel("../../data/sphere.obj");
	ComputeNormals(&spheremodel);
	BuildTrianglesAndAddToVirtualScene(&spheremodel);
	LoadTextureImage("../../data/metal.png");
	ObjModel spaceshipmodel("../../data/spaceship.obj");
	ComputeNormals(&spaceshipmodel);
	BuildTrianglesAndAddToVirtualScene(&spaceshipmodel);
	TextRendering_Init();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    char lastPressedKey;

	while (!glfwWindowShouldClose(window))
	{
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(g_GpuProgramID);
        glm::vec4 camera_position_c;
        glm::vec4 camera_lookat_l;
        glm::vec4 camera_view_vector; 
        glm::vec4 camera_up_vector;
        if (g_IsFreeCamera){
            if (g_WKeyPressed || g_SKeyPressed || g_AKeyPressed || g_DKeyPressed){
                g_CameraSpeed += CAMERA_ACCELERATION * deltaTime;
                if (g_CameraSpeed > MAX_CAMERA_SPEED){
                    g_CameraSpeed = MAX_CAMERA_SPEED;
                }
            }
            else{
                g_CameraSpeed -= CAMERA_ACCELERATION * deltaTime * 2.0f;
                if (g_CameraSpeed < 0.0f)
                {
                    g_CameraSpeed = 0.0f;
                }
            }

            if (g_WKeyPressed){
                lastPressedKey = 'W';
                g_FreeCameraPosition += g_CameraSpeed * g_FreeCameraFront * deltaTime;
            }
            if (g_SKeyPressed){
                lastPressedKey = 'S';
                g_FreeCameraPosition -= g_CameraSpeed * g_FreeCameraFront * deltaTime;
            }
            if (g_AKeyPressed){
                lastPressedKey = 'A';
                g_FreeCameraPosition -= glm::normalize(glm::cross(g_FreeCameraFront, g_FreeCameraUp)) * g_CameraSpeed * deltaTime;
            }
            if (g_DKeyPressed){
                lastPressedKey = 'D';
                g_FreeCameraPosition += glm::normalize(glm::cross(g_FreeCameraFront, g_FreeCameraUp)) * g_CameraSpeed * deltaTime;
            }

            if (!(g_WKeyPressed || g_SKeyPressed || g_AKeyPressed || g_DKeyPressed)){
                if (lastPressedKey == 'W'){
                g_FreeCameraPosition += g_CameraSpeed * g_FreeCameraFront * deltaTime;
                }
                if (lastPressedKey == 'S'){
                    g_FreeCameraPosition -= g_CameraSpeed * g_FreeCameraFront * deltaTime;
                }
                if (lastPressedKey == 'A'){
                    g_FreeCameraPosition -= glm::normalize(glm::cross(g_FreeCameraFront, g_FreeCameraUp)) * g_CameraSpeed * deltaTime;
                }
                if (lastPressedKey == 'D'){
                    g_FreeCameraPosition += glm::normalize(glm::cross(g_FreeCameraFront, g_FreeCameraUp)) * g_CameraSpeed * deltaTime;
                }
            }
            
            camera_position_c = glm::vec4(g_FreeCameraPosition, 1.0f);
            camera_view_vector = glm::vec4(g_FreeCameraFront, 0.0f);
            camera_up_vector = glm::vec4(g_FreeCameraUp, 0.0f);
        }
        else {
            float r = g_CameraDistance;
            float y = r * sin(g_CameraPhi);
            float z = r * cos(g_CameraPhi) * cos(g_CameraTheta);
            float x = r * cos(g_CameraPhi) * sin(g_CameraTheta);

            camera_position_c = glm::vec4(x, y, z, 1.0f);
            camera_lookat_l = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            camera_view_vector = camera_lookat_l - camera_position_c;
            camera_up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        }   

		glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);
		glm::mat4 projection;
		float nearplane = -1.0f;                
		float farplane  = -100.0f;               
		float field_of_view = 3.141592 / 3.0f;  
		projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
		glm::mat4 model = Matrix_Identity(); 
		glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
		glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));
		#define SPHERE 0
		#define SPACESHIP 1

		//draw space sphere
		model = Matrix_Translate(camera_position_c.x, camera_position_c.y, camera_position_c.z)*Matrix_Scale(2.0f,2.0f,-2.0f);
		glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
		glUniform1i(g_object_id_uniform, SPHERE);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		DrawVirtualObject("the_sphere");
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);

		//draw spaceship
		model = Matrix_Translate(1.0f,0.0f,0.0f);
		glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
		glUniform1i(g_object_id_uniform, SPACESHIP);
		DrawVirtualObject("the_spaceship");
		TextRendering_ShowEulerAngles(window);
		TextRendering_ShowFramesPerSecond(window);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
    glfwSwapBuffers(window);
    glfwPollEvents();
	glfwTerminate();
    
	return 0;
}
void LoadTextureImage(const char* filename)
{
	printf("Carregando imagem \"%s\"... ", filename);
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
	GLuint texture_id;
	GLuint sampler_id;
	glGenTextures(1, &texture_id);
	glGenSamplers(1, &sampler_id);
	glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
void DrawVirtualObject(const char* object_name)
{
	glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);
	glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
	glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
	glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
	glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);
	glDrawElements(
		g_VirtualScene[object_name].rendering_mode,
		g_VirtualScene[object_name].num_indices,
		GL_UNSIGNED_INT,
		(void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
	);
	glBindVertexArray(0);
}
void LoadShadersFromFiles()
{
	GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
	GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");
	if ( g_GpuProgramID != 0 )
		glDeleteProgram(g_GpuProgramID);
	g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);
	g_model_uniform           = glGetUniformLocation(g_GpuProgramID, "model"); 
	g_view_uniform            = glGetUniformLocation(g_GpuProgramID, "view"); 
	g_projection_uniform      = glGetUniformLocation(g_GpuProgramID, "projection"); 
	g_object_id_uniform       = glGetUniformLocation(g_GpuProgramID, "object_id"); 
	g_bbox_min_uniform        = glGetUniformLocation(g_GpuProgramID, "bbox_min");
	g_bbox_max_uniform        = glGetUniformLocation(g_GpuProgramID, "bbox_max");
	glUseProgram(g_GpuProgramID);
	glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage0"), 0);
	glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage1"), 1);
	glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
	glUseProgram(0);
}
void ComputeNormals(ObjModel* model)
{
	if ( !model->attrib.normals.empty() )
		return;
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
				model_coefficients.push_back( vx ); 
				model_coefficients.push_back( vy ); 
				model_coefficients.push_back( vz ); 
				model_coefficients.push_back( 1.0f ); 
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
					normal_coefficients.push_back( nx ); 
					normal_coefficients.push_back( ny ); 
					normal_coefficients.push_back( nz ); 
					normal_coefficients.push_back( 0.0f ); 
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
		theobject.first_index    = first_index; 
		theobject.num_indices    = last_index - first_index + 1; 
		theobject.rendering_mode = GL_TRIANGLES;       
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
	GLuint location = 0; 
	GLint  number_of_dimensions = 4; 
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
		location = 1; 
		number_of_dimensions = 4; 
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
		location = 2; 
		number_of_dimensions = 2; 
		glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(location);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	GLuint indices_id;
	glGenBuffers(1, &indices_id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
	glBindVertexArray(0);
}
GLuint LoadShader_Vertex(const char* filename)
{   
	GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
	LoadShader(filename, vertex_shader_id);
	return vertex_shader_id;
}
GLuint LoadShader_Fragment(const char* filename)
{
	GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
	LoadShader(filename, fragment_shader_id);
	return fragment_shader_id;
}
void LoadShader(const char* filename, GLuint shader_id)
{  
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
	glShaderSource(shader_id, 1, &shader_string, &shader_string_length);
	glCompileShader(shader_id);
	GLint compiled_ok;
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);
	GLint log_length = 0;
	glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
	GLchar* log = new GLchar[log_length];
	glGetShaderInfoLog(shader_id, log_length, &log_length, log);
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
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
	GLuint program_id = glCreateProgram();
	glAttachShader(program_id, vertex_shader_id);
	glAttachShader(program_id, fragment_shader_id);
	glLinkProgram(program_id);
	GLint linked_ok = GL_FALSE;
	glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);
	if ( linked_ok == GL_FALSE )
	{
		GLint log_length = 0;
		glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
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
	glDeleteShader(vertex_shader_id);
	glDeleteShader(fragment_shader_id);
	return program_id;
}
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{   
	glViewport(0, 0, width, height);
	g_ScreenRatio = (float)width / height;
}
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
		g_LeftMouseButtonPressed = true;
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		g_LeftMouseButtonPressed = false;
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
		g_RightMouseButtonPressed = true;
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
	{
		g_RightMouseButtonPressed = false;
	}
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
	{
		glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
		g_MiddleMouseButtonPressed = true;
	}
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
	{
		g_MiddleMouseButtonPressed = false;
	}
}
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
            front.x = cos(glm::radians(g_Yaw)) * cos(glm::radians(g_Pitch));
            front.y = sin(glm::radians(g_Pitch));
            front.z = sin(glm::radians(g_Yaw)) * cos(glm::radians(g_Pitch));
            g_FreeCameraFront = glm::normalize(front);
        }
		g_CameraTheta -= 0.1f*dx;
		g_CameraPhi   += 0.1f*dy;
		float phimax = 3.141592f/2;
		float phimin = -phimax;
		if (g_CameraPhi > phimax)
			g_CameraPhi = phimax;
		if (g_CameraPhi < phimin)
			g_CameraPhi = phimin;
		g_LastCursorPosX = xpos;
		g_LastCursorPosY = ypos;
	}
}
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	g_CameraDistance -= 0.1f*yoffset;
	const float verysmallnumber = std::numeric_limits<float>::epsilon();
	if (g_CameraDistance < verysmallnumber)
		g_CameraDistance = verysmallnumber;
}
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        g_IsFreeCamera = !g_IsFreeCamera;
    }
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key == GLFW_KEY_H && action == GLFW_PRESS)
	{
		g_ShowInfoText = !g_ShowInfoText;
	}
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		LoadShadersFromFiles();
		fprintf(stdout,"Shaders recarregados!\n");
		fflush(stdout);
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
}
void ErrorCallback(int error, const char* description)
{
	fprintf(stderr, "ERROR: GLFW: %s\n", description);
}
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
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
	if ( !g_ShowInfoText )
		return;
	float pad = TextRendering_LineHeight(window);
	char buffer[80];
	snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);
	TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
	if ( !g_ShowInfoText )
		return;
	static float old_seconds = (float)glfwGetTime();
	static int   ellapsed_frames = 0;
	static char  buffer[20] = "?? fps";
	static int   numchars = 7;
	ellapsed_frames += 1;
	float seconds = (float)glfwGetTime();
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
		for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
		size_t fnum = shapes[i].mesh.num_face_vertices[f];
		printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
				static_cast<unsigned long>(fnum));
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