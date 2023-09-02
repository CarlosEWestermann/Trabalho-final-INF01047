#ifndef _MATRICES_H
#define _MATRICES_H


#include <cstdio>
#include <cstdlib>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Esta função Matrix() auxilia na criação de matrizes usando a biblioteca GLM.
// Note que em OpenGL (e GLM) as matrizes são definidas como "column-major",
// onde os elementos da matriz são armazenadas percorrendo as COLUNAS da mesma.
// Por exemplo, seja
//
//       [a b c]
//   M = [d e f]
//       [g h i]
//
// uma matriz 3x3. Em memória, na representação "column-major" de OpenGL, essa
// matriz é representada pelo seguinte array:
//
//   M[] = {  a,d,g,    b,e,h,    c,f,i  };
//              ^         ^         ^
//              |         |         |
//           coluna 1  coluna 2  coluna 3
//
// Para conseguirmos definir matrizes através de suas LINHAS, a função Matrix()
// computa a transposta usando os elementos passados por parâmetros.
glm::mat4 Matrix(
    float m00, float m01, float m02, float m03, // LINHA 1
    float m10, float m11, float m12, float m13, // LINHA 2
    float m20, float m21, float m22, float m23, // LINHA 3
    float m30, float m31, float m32, float m33  // LINHA 4
);

// Matriz identidade.
glm::mat4 Matrix_Identity();
// Matriz de translação T. Seja p=[px,py,pz,pw] um ponto e t=[tx,ty,tz,0] um
// vetor em coordenadas homogêneas, definidos em um sistema de coordenadas
// Cartesiano. Então, a matriz T é definida pela seguinte igualdade:
//
//     T*p = p+t.
//
glm::mat4 Matrix_Translate(float tx, float ty, float tz);

// Matriz S de "escalamento de um ponto" em relação à origem do sistema de
// coordenadas. Seja p=[px,py,pz,pw] um ponto em coordenadas homogêneas.
// Então, a matriz S é definida pela seguinte igualdade:
//
//     S*p = [sx*px, sy*py, sz*pz, pw].
//
glm::mat4 Matrix_Scale(float sx, float sy, float sz);

// Matriz R de "rotação de um ponto" em relação à origem do sistema de
// coordenadas e em torno do eixo X (primeiro vetor da base do sistema de
// coordenadas). Seja p=[px,py,pz,pw] um ponto em coordenadas homogêneas.
// Então, a matriz R é definida pela seguinte igualdade:
//
//   R*p = [ px, c*py-s*pz, s*py+c*pz, pw ];
//
// onde 'c' e 's' são o cosseno e o seno do ângulo de rotação, respectivamente.
glm::mat4 Matrix_Rotate_X(float angle);
// Matriz R de "rotação de um ponto" em relação à origem do sistema de
// coordenadas e em torno do eixo Y (segundo vetor da base do sistema de
// coordenadas). Seja p=[px,py,pz,pw] um ponto em coordenadas homogêneas.
// Então, a matriz R é definida pela seguinte igualdade:
//
//   R*p = [ c*px+s*pz, py, -s*px+c*pz, pw ];
//
// onde 'c' e 's' são o cosseno e o seno do ângulo de rotação, respectivamente.
glm::mat4 Matrix_Rotate_Y(float angle);

// Matriz R de "rotação de um ponto" em relação à origem do sistema de
// coordenadas e em torno do eixo Z (terceiro vetor da base do sistema de
// coordenadas). Seja p=[px,py,pz,pw] um ponto em coordenadas homogêneas.
// Então, a matriz R é definida pela seguinte igualdade:
//
//   R*p = [ c*px-s*py, s*px+c*py, pz, pw ];
//
// onde 'c' e 's' são o cosseno e o seno do ângulo de rotação, respectivamente.
glm::mat4 Matrix_Rotate_Z(float angle);

// Função que calcula a norma Euclidiana de um vetor cujos coeficientes são
// definidos em uma base ortonormal qualquer.
float norm(glm::vec4 v);

// Matriz R de "rotação de um ponto" em relação à origem do sistema de
// coordenadas e em torno do eixo definido pelo vetor 'axis'. Esta matriz pode
// ser definida pela fórmula de Rodrigues. Lembre-se que o vetor que define o
// eixo de rotação deve ser normalizado!
glm::mat4 Matrix_Rotate(float angle, glm::vec4 axis);

// Produto vetorial entre dois vetores u e v definidos em um sistema de
// coordenadas ortonormal.
glm::vec4 crossproduct(glm::vec4 u, glm::vec4 v);

// Produto escalar entre dois vetores u e v definidos em um sistema de
// coordenadas ortonormal.
float dotproduct(glm::vec4 u, glm::vec4 v);

// Matriz de mudança de coordenadas para o sistema de coordenadas da Câmera.
glm::mat4 Matrix_Camera_View(glm::vec4 position_c, glm::vec4 view_vector, glm::vec4 up_vector);

// Matriz de projeção paralela ortográfica
glm::mat4 Matrix_Orthographic(float l, float r, float b, float t, float n, float f);
// Matriz de projeção perspectiva
glm::mat4 Matrix_Perspective(float field_of_view, float aspect, float n, float f);

// Função que imprime uma matriz M no terminal
void PrintMatrix(glm::mat4 M);

// Função que imprime um vetor v no terminal
void PrintVector(glm::vec4 v);

// Função que imprime o produto de uma matriz por um vetor no terminal
void PrintMatrixVectorProduct(glm::mat4 M, glm::vec4 v);

// Função que imprime o produto de uma matriz por um vetor, junto com divisão
// por w, no terminal.
void PrintMatrixVectorProductDivW(glm::mat4 M, glm::vec4 v);


#endif // _MATRICES_H
// vim: set spell spelllang=pt_br :
