#include "collisions.h"

// Teste de colisão: esferac-esfera
bool checkSphereSphereCollision(BoundingSphere sphere1, BoundingSphere sphere2){
    glm::vec3 delta = sphere1.center - sphere2.center;
    float distanceSquared = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
    float radiusSum = sphere1.radius + sphere2.radius;
    return distanceSquared <= radiusSum * radiusSum;
}

// Teste de colisão: esfera-circulo
bool checkSphereCircleCollision(BoundingSphere sphere, BoundingCircle circle){
    glm::vec3 distanceCenters = sphere.center - circle.center;
    float distanceToPlaneSquared = dotproduct(glm::vec4(distanceCenters.x, distanceCenters.y, distanceCenters.z, 0),
                                                glm::vec4(circle.normal.x, circle.normal.y, circle.normal.z, 0));
    distanceToPlaneSquared *= distanceToPlaneSquared;  // Squaring to compare

    if (distanceToPlaneSquared > sphere.radius * sphere.radius) return false;

    glm::vec3 intersectionPoint = sphere.center - std::sqrt(distanceToPlaneSquared) * circle.normal;
    glm::vec3 delta = intersectionPoint - circle.center;
    float distanceToCircleCenterSquared = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;

    return distanceToCircleCenterSquared <= circle.radius * circle.radius;
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
