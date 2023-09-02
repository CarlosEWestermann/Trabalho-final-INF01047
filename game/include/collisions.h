#ifndef _COLLISIONS_H
#define _COLLISIONS_H

#include "matrices.h"

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

bool checkSphereSphereCollision(BoundingSphere sphere1, BoundingSphere sphere2);
bool checkSphereCircleCollision(BoundingSphere sphere, BoundingCircle circle);
bool checkSphereCubeCollision(BoundingSphere sphere, BoundingCube cube);



#endif 