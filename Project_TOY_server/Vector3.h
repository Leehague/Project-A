#pragma once
#include "Protocol/Protocol.pb.h"

class Vector3 
{
public:
    float x, y, z;

public:
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& other) const { return Vector3(x + other.x, y + other.y, z + other.z); }

    // Make Distance a static method
    static float Distance(const Vector3& a, const Vector3& b)
    {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        float dz = a.z - b.z;
        return sqrt(dx * dx + dy * dy + dz * dz);
    }

    static Vector3 PosInfoToVector3(Protocol::PosInfo posinfo)
    {
        return Vector3(posinfo.x(), posinfo.y(), posinfo.z());

    }
};

