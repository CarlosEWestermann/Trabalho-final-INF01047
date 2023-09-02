#include "collisions.h"

bool checkSphereSphereCollision(BoundingSphere sphere1, BoundingSphere sphere2){
    float distance = glm::length(sphere1.center - sphere2.center); // dist um centro do outro
    return distance <= (sphere1.radius + sphere2.radius);          // verifica se tem intersecao
}

// Teste de colisão: esfera-circulo
bool checkSphereCircleCollision(BoundingSphere sphere, BoundingCircle circle){
    glm::vec3 distanceCenters = sphere.center - circle.center;
    float distanceToPlane = std::abs(dotproduct(glm::vec4(distanceCenters.x, distanceCenters.y, distanceCenters.z, 0),
                                                            glm::vec4(circle.normal.x, circle.normal.y, circle.normal.z, 0)));

    if (distanceToPlane > sphere.radius)return false;

    
    glm::vec3 intersectionPoint = sphere.center - distanceToPlane * circle.normal;

    float distanceToCircleCenter = glm::length(intersectionPoint - circle.center);
    return distanceToCircleCenter <= circle.radius;
}

// Teste de colisão: esfera-cubo
bool checkSphereCubeCollision(BoundingSphere sphere, BoundingCube cube){
    return true;
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