#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>

#include "sceneStructs.h"
#include "utilities.h"

/**
 * Handy-dandy hash function that provides seeds for random number generation.
 */
__host__ __device__ inline unsigned int utilhash(unsigned int a) {
    a = (a + 0x7ed55d16) + (a << 12);
    a = (a ^ 0xc761c23c) ^ (a >> 19);
    a = (a + 0x165667b1) + (a << 5);
    a = (a + 0xd3a2646c) ^ (a << 9);
    a = (a + 0xfd7046c5) + (a << 3);
    a = (a ^ 0xb55a4f09) ^ (a >> 16);
    return a;
}

// CHECKITOUT
/**
 * Compute a point at parameter value `t` on ray `r`.
 * Falls slightly short so that it doesn't intersect the object it's hitting.
 */
__host__ __device__ glm::vec3 getPointOnRay(Ray r, float t) {
    return r.origin + (t - .0001f) * glm::normalize(r.direction);
}

/**
 * Multiplies a mat4 and a vec4 and returns a vec3 clipped from the vec4.
 */
__host__ __device__ glm::vec3 multiplyMV(glm::mat4 m, glm::vec4 v) {
    return glm::vec3(m * v);
}

// CHECKITOUT
/**
 * Test intersection between a ray and a transformed cube. Untransformed,
 * the cube ranges from -0.5 to 0.5 in each axis and is centered at the origin.
 *
 * @param intersectionPoint  Output parameter for point of intersection.
 * @param normal             Output parameter for surface normal.
 * @param outside            Output param for whether the ray came from outside.
 * @return                   Ray parameter `t` value. -1 if no intersection.
 */
__host__ __device__ float boxIntersectionTest(Geom box, Ray r,
        glm::vec3 &intersectionPoint, glm::vec3 &normal, bool &outside) {
    Ray q;
    q.origin    =                multiplyMV(box.inverseTransform, glm::vec4(r.origin   , 1.0f));
    q.direction = glm::normalize(multiplyMV(box.inverseTransform, glm::vec4(r.direction, 0.0f)));

    float tmin = -1e38f;
    float tmax = 1e38f;
    glm::vec3 tmin_n;
    glm::vec3 tmax_n;
    for (int xyz = 0; xyz < 3; ++xyz) {
        float qdxyz = q.direction[xyz];
        /*if (glm::abs(qdxyz) > 0.00001f)*/ {
            float t1 = (-0.5f - q.origin[xyz]) / qdxyz;
            float t2 = (+0.5f - q.origin[xyz]) / qdxyz;
            float ta = glm::min(t1, t2);
            float tb = glm::max(t1, t2);
            glm::vec3 n;
            n[xyz] = t2 < t1 ? +1 : -1;
            if (ta > 0 && ta > tmin) {
                tmin = ta;
                tmin_n = n;
            }
            if (tb < tmax) {
                tmax = tb;
                tmax_n = n;
            }
        }
    }

    if (tmax >= tmin && tmax > 0) {
        outside = true;
        if (tmin <= 0) {
            tmin = tmax;
            tmin_n = tmax_n;
            outside = false;
        }
        intersectionPoint = multiplyMV(box.transform, glm::vec4(getPointOnRay(q, tmin), 1.0f));
        normal = glm::normalize(multiplyMV(box.invTranspose, glm::vec4(tmin_n, 0.0f)));
        return glm::length(r.origin - intersectionPoint);
    }
    return -1;
}

// CHECKITOUT
/**
 * Test intersection between a ray and a transformed sphere. Untransformed,
 * the sphere always has radius 0.5 and is centered at the origin.
 *
 * @param intersectionPoint  Output parameter for point of intersection.
 * @param normal             Output parameter for surface normal.
 * @param outside            Output param for whether the ray came from outside.
 * @return                   Ray parameter `t` value. -1 if no intersection.
 */
__host__ __device__ float sphereIntersectionTest(Geom sphere, Ray r,
        glm::vec3 &intersectionPoint, glm::vec3 &normal, bool &outside) {
    float radius = .5;

    glm::vec3 ro = multiplyMV(sphere.inverseTransform, glm::vec4(r.origin, 1.0f));
    glm::vec3 rd = glm::normalize(multiplyMV(sphere.inverseTransform, glm::vec4(r.direction, 0.0f)));

    Ray rt;
    rt.origin = ro;
    rt.direction = rd;

    float vDotDirection = glm::dot(rt.origin, rt.direction);
    float radicand = vDotDirection * vDotDirection - (glm::dot(rt.origin, rt.origin) - powf(radius, 2));
    if (radicand < 0) {
        return -1;
    }

    float squareRoot = sqrt(radicand);
    float firstTerm = -vDotDirection;
    float t1 = firstTerm + squareRoot;
    float t2 = firstTerm - squareRoot;

    float t = 0;
    if (t1 < 0 && t2 < 0) {
        return -1;
    } else if (t1 > 0 && t2 > 0) {
        t = min(t1, t2);
        outside = true;
    } else {
        t = max(t1, t2);
        outside = false;
    }

    glm::vec3 objspaceIntersection = getPointOnRay(rt, t);

    intersectionPoint = multiplyMV(sphere.transform, glm::vec4(objspaceIntersection, 1.f));
    normal = glm::normalize(multiplyMV(sphere.invTranspose, glm::vec4(objspaceIntersection, 0.f)));
    if (!outside) {
        normal = -normal;
    }

    return glm::length(r.origin - intersectionPoint);
}

__host__ __device__ float getArea(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
{
    glm::vec3 p1_temp, p2_temp, p3_temp;

    p1_temp.x = p1.x;
    p2_temp.x = p2.x;
    p3_temp.x = p3.x;

    p1_temp.y = p1.y;
    p2_temp.y = p2.y;
    p3_temp.y = p3.y;

    p1_temp.z = p2_temp.z = p3_temp.z = 0.f;

    float area = 0.5 * glm::length(glm::cross((p1_temp - p2_temp), (p3_temp - p2_temp)));
    return area;
}

//__host__ __device__ glm::vec3 barycentricInterpolation(Vertex v[3], glm::vec3 p)
//{
//    glm::vec3 p1, p2, p3;
//    p1 = v[0].pos;
//    p2 = v[1].pos;
//    p3 = v[2].pos;
//
//    float s, s1, s2, s3;
//    s = getArea(p1, p2, p3);
//    s1 = getArea(p, p2, p3);
//    s2 = getArea(p1, p, p3);
//    s3 = getArea(p1, p2, p);
//
//    glm::vec3 barycentric_influence(s1 / s, s2 / s, s3 / s);
//    return barycentric_influence;
//}
//
//__host__ __device__ float zInterpolate(Vertex v[3], glm::vec3 p)
//{
//    glm::vec3 weights = barycentricInterpolation(v, p);
//    float z_inverse = 0;
//    z_inverse = ((1 / (v[0].pos.z)) * weights[0]) +
//        ((1 / (v[1].pos.z)) * weights[1]) +
//        ((1 / (v[2].pos.z)) * weights[2]);
//    return 1 / z_inverse;
//}
//
//__host__ __device__ glm::vec3 interpolatedNormal(Vertex v[3], glm::vec3 p, float z)
//{
//    glm::vec3 weights = barycentricInterpolation(v, p);
//    glm::vec3 normals(0, 0, 0);
//
//    normals = (((v[0].nor) / (v[0].pos.z)) * weights[0]) +
//        (((v[1].nor) / (v[1].pos.z)) * weights[1]) +
//        (((v[2].nor) / (v[2].pos.z)) * weights[2]);
//
//    return (z * normals);
//}


__host__ __device__ float objIntersectionTest(Geom obj, Triangle *dev_tri, Ray r,
    glm::vec3& intersectionPoint, glm::vec3& normal, bool& outside) {
    
    bool isectFound = false;
    int triCount = obj.triCount;
    //glm::vec3 isectPoint;
    //glm::vec3 isectNor;
    glm::vec3 minVal = glm::vec3(INT_MAX, INT_MAX, INT_MAX);
    glm::vec3 barycentric;
    glm::vec3 n1, n2, n3;

    Triangle* dev_triItr = dev_tri;

    for (int i = 0; i < triCount; i++, dev_triItr++) {

        //printf("\n****GPU****\n");
        ////printf("\n %f, %f, %f", dev_triItr->pos[0].x, dev_triItr->pos[0].y, dev_triItr->pos[0].z);
        //printf("\n %f, %f, %f", dev_triItr->nor[0].x, dev_triItr->nor[0].y, dev_triItr->nor[0].z);

        glm::vec3 v1_pos = glm::vec3(obj.transform * glm::vec4(dev_triItr->pos[0].x, dev_triItr->pos[0].y, dev_triItr->pos[0].z, 1.0));
        glm::vec3 v2_pos = glm::vec3(obj.transform * glm::vec4(dev_triItr->pos[1].x, dev_triItr->pos[1].y, dev_triItr->pos[1].z, 1.0));
        glm::vec3 v3_pos = glm::vec3(obj.transform * glm::vec4(dev_triItr->pos[2].x, dev_triItr->pos[2].y, dev_triItr->pos[2].z, 1.0));

        isectFound = glm::intersectRayTriangle(r.origin, r.direction,
            v1_pos, v2_pos, v3_pos, barycentric);

        if (isectFound) {

            /*printf("\n****GPU****\n");
            printf("\n %f, %f, %f", dev_triItr->nor[0].x, dev_triItr->nor[0].y, dev_triItr->nor[0].z);*/

            if (barycentric[2] >= 0 && barycentric[2] < minVal[2])
            {
                minVal = barycentric;
            }
            /*n1 = glm::vec3(obj.invTranspose * glm::vec4(dev_triItr->nor[0].x, dev_triItr->nor[0].y, dev_triItr->nor[0].z, 1.0));
            n2 = glm::vec3(obj.invTranspose * glm::vec4(dev_triItr->nor[1].x, dev_triItr->nor[1].y, dev_triItr->nor[1].z, 1.0));
            n3 = glm::vec3(obj.invTranspose * glm::vec4(dev_triItr->nor[2].x, dev_triItr->nor[2].y, dev_triItr->nor[2].z, 1.0));*/
            n1 = dev_triItr->nor[0];
            n2 = dev_triItr->nor[1];
            n3 = dev_triItr->nor[2];

            //printf("NOR1: %f, %f, %f\n", n1.x, n1.y, n1.z);
            //printf("NOR2: %f, %f, %f\n", n2.x, n2.y, n2.z);
            //printf("NOR3: %f, %f, %f\n", n3.x, n3.y, n3.z);

            break;
        }
    }

    if (isectFound)
    {
        float u = minVal[0];
        float v = minVal[1];
        float t = minVal[2];
        intersectionPoint = getPointOnRay(r, t);
        normal = glm::vec3(u * n1 + v * n2 + (1 - u - v) * n3);
        /*intersectionPoint = isectPoint;
        normal = isectNor;*/
        //printf("INOR: %f, %f, %f\n", normal.x, normal.y, normal.z);
        return glm::length(r.origin - intersectionPoint);
    }
    return -1;

    /*printf("\n***********");
    for (int i = 0; i < obj.triCount; i++)
    {
        printf("\n%f, %f, %f", dev_tri->pos[0].x, dev_tri->pos[0].y, dev_tri->pos[0].z);
        dev_tri++;
    }
    printf("\n###########\n");*/
}



//__host__ __device__ float triangleIntersectionTest(Geom geom, Ray r,
//    glm::vec3& intersectionPoint, glm::vec3& normal, bool& outside) {
//
//    /*glm::vec3 ro = multiplyMV(geom.inverseTransform, glm::vec4(r.origin, 1.0f));
//    glm::vec3 rd = glm::normalize(multiplyMV(geom.inverseTransform, glm::vec4(r.direction, 0.0f)));*/
//    glm::vec3 ro = r.origin;
//    glm::vec3 rd = r.direction;
//
//    Ray rt;
//    rt.origin = ro;
//    rt.direction = rd;
//
//    glm::vec3 intersectResult;
//    glm::intersectRayTriangle(ro, rd,
//        geom.triangle.vertices[0].pos, geom.triangle.vertices[1].pos, geom.triangle.vertices[2].pos,
//        intersectResult);
//    float t = intersectResult.z;
//    glm::vec3 objspaceIntersection = getPointOnRay(rt, t);
//    //printf("%f", &t);
//    
//    /*intersectionPoint = multiplyMV(geom.transform, glm::vec4(objspaceIntersection, 1.f));
//    normal = glm::normalize(multiplyMV(geom.invTranspose, glm::vec4(objspaceIntersection, 0.f)));*/
//
//    intersectionPoint = objspaceIntersection;
//    normal = geom.triangle.vertices[0].nor;
//    
//    return t;
//
//    //return glm::length(r.origin - intersectionPoint);
//    //Ray rt;
//    //rt.origin = ro;
//    //rt.direction = rd;
//    //float t = 0;
//    //glm::vec3 P = geom.triangle.vertices[0].pos;    // P is some point on the plane
//    //glm::vec3 N = geom.triangle.vertices[0].nor;
//    ////float z = zInterpolate(geom.triangle.vertices, P);
//    ////glm::vec3 N = interpolatedNormal(geom.triangle.vertices, P, z);
//
//    //t = dot(N, (P - ro)) / dot(N, rd);
//    //glm::vec3 objspaceIntersection = getPointOnRay(rt, t);
//
//    //// check if objectspaceIntersection is within area of triangle using barycentric coordinates
//    //glm::vec3 S = barycentricInterpolation(geom.triangle.vertices, objspaceIntersection);
//    //if ((S[0] >= 0 && S[0] <= 1)
//    //    && (S[1] >= 0 && S[1] <= 1)
//    //    && (S[2] >= 0 && S[2] <= 1)
//    //    && (S[0] + S[1] + S[2] == 1.f)) {
//    //    
//    //    intersectionPoint = multiplyMV(geom.transform, glm::vec4(objspaceIntersection, 1.f));
//    //    normal = glm::normalize(multiplyMV(geom.invTranspose, glm::vec4(objspaceIntersection, 0.f)));
//    //    return glm::length(r.origin - intersectionPoint);
//    //}
//    //else {
//    //    return -1.f;
//    //}
//}