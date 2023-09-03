#include "collisions.h"

// Teste de colisão: esferac-esfera
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
    glm::vec3 cubeCenter = getCubeCenter(cube);
    float cubeSide = getCubeSide(cube);

    float sphereXDistance = abs(sphere.center.x - cubeCenter.x);
    float sphereYDistance = abs(sphere.center.y - cubeCenter.y);
    float sphereZDistance = abs(sphere.center.z - cubeCenter.z);

    if (sphereXDistance >= (cubeSide + sphere.radius)) return false;
    if (sphereYDistance >= (cubeSide + sphere.radius)) return false; 
    if (sphereZDistance >= (cubeSide + sphere.radius)) return false; 

    if (sphereXDistance < (cubeSide)) return true;
    if (sphereYDistance < (cubeSide)) return true; 
    if (sphereZDistance < (cubeSide)) return true; 

   float cornerDistance_sq = ((sphereXDistance - cubeSide) * (sphereXDistance - cubeSide)) +
                         ((sphereYDistance - cubeSide) * (sphereYDistance - cubeSide)) +
                         ((sphereZDistance - cubeSide) * (sphereZDistance - cubeSide));

    return (cornerDistance_sq < (sphere.radius * sphere.radius));
}

glm::vec3 getCubeCenter(BoundingCube cube){
    return (cube.lowerBackLeft + cube.upperFrontRight) * 0.5f;
}

float getCubeSide(BoundingCube cube){
    return abs(cube.upperFrontRight.x - cube.lowerBackLeft.x);
}
