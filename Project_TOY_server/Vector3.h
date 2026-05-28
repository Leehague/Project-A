#pragma once
#include "Protocol/Protocol.pb.h"

class Vector3 
{
public:
    float x, y, z;

public:
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    // Make Distance a static method
    static float Distance(const Vector3& a, const Vector3& b)
    {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        float dz = a.z - b.z;
        return sqrt(dx * dx + dy * dy + dz * dz);
    }

    static float DistanceSquared(const Vector3& a, const Vector3& b) 
    {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        float dz = a.z - b.z;
        return (dx * dx + dy * dy + dz * dz);
    }

    static Vector3 PosInfoToVector3(const Protocol::PosInfo* posinfo)
    {
        return Vector3(posinfo->x(), posinfo->y(), posinfo->z());

    }

    // 더하기
    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    // 빼기 (방향 벡터 구할 때 핵심: P2 - P1)
    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    // 스칼라 곱 (속도 조절 시 사용)
    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    // 복합 대입 연산자
    Vector3& operator+=(const Vector3& other) {
        x += other.x; y += other.y; z += other.z;
        return *this;
    }

    // 비교 연산 (A* 그리드 체크 등)
    bool operator==(const Vector3& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    // 벡터의 길이를 반환
    float Length() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    // 벡터의 길이의 제곱을 반환 (루트 연산이 없어 성능상 유리)
    float LengthSquared() const
    {
        return x * x + y * y + z * z;
    }

    // 자기 자신을 정규화
    void Normalize()
    {
        float len = Length();
        if (len > 0.000001f)
        {
            x /= len;
            y /= len;
            z /= len;
        }
    }

    // 정규화된 복사본 벡터를 반환
    Vector3 GetNormalized() const
    {
        float len = Length();
        if (len > 0.000001f)
            return Vector3(x / len, y / len, z / len);
        return Vector3(0.f, 0.f, 0.f);
    }
};

