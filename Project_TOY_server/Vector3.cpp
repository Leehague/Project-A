#include "Vector3.h"
#include "Protocol/Protocol.pb.h"
#include "InfoSturct.h"

float Vector3::Distance(const Vector3& a, const Vector3& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

float Vector3::DistanceSquared(const Vector3& a, const Vector3& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return (dx * dx + dy * dy + dz * dz);
}

float Vector3::XZDistance(const Vector3& a, const Vector3& b)
{
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return sqrt(dx * dx + dz * dz);
}

float Vector3::XZDistanceSquared(const Vector3& a, const Vector3& b)
{
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return (dx * dx + dz * dz);
}

Vector3 Vector3::PosInfoToVector3(const Protocol::PosInfo* posinfo)
{
    return Vector3(posinfo->x(), posinfo->y(), posinfo->z());

}

Vector3 Vector3::PosInfoToVector3(const Core::PosInfo* posinfo)
{
    return Vector3(posinfo->x, posinfo->y, posinfo->z);
}


// 자기 자신을 정규화
void Vector3::Normalize()
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
Vector3 Vector3::GetNormalized() const
{
    float len = Length();
    if (len > 0.000001f)
        return Vector3(x / len, y / len, z / len);
    return Vector3(0.f, 0.f, 0.f);
}

// Vector3를 점으로 해석하고 해당 점이 삼각형 내부에 있는지 판단하는 함수 (Barycentric coordinate 방식)
bool Vector3::IsPointInTriangle(Triangle tri, float Ypadding) {
    // 1. 높이(Y) 검증: 점 P가 삼각형의 평면 높이와 너무 멀리 떨어져 있으면 false
    // NavMesh 데이터이므로 삼각형 정점들의 평균 Y값이나 
    // 최소/최대 Y 범위를 기준으로 허용 오차를 둡니다.
    float minY = std::min({ tri.v1.y, tri.v2.y, tri.v3.y }) - Ypadding;
    float maxY = std::max({ tri.v1.y, tri.v2.y, tri.v3.y }) + Ypadding;
    if (this->y < minY || this->y > maxY) return false;

    // 2. XZ 평면 판정 (Barycentric Coordinate)
    // 벡터 정의
    float v0x = tri.v3.x - tri.v1.x; float v0z = tri.v3.z - tri.v1.z; // AC
    float v1x = tri.v2.x - tri.v1.x; float v1z = tri.v2.z - tri.v1.z; // AB
    float v2x = this->x - tri.v1.x;      float v2z = this->z - tri.v1.z;      // AP

    // 도트 프로덕트(내적) 계산 (XZ 평면용)
    float dot00 = v0x * v0x + v0z * v0z;
    float dot01 = v0x * v1x + v0z * v1z;
    float dot02 = v0x * v2x + v0z * v2z;
    float dot11 = v1x * v1x + v1z * v1z;
    float dot12 = v1x * v2x + v1z * v2z;

    // Barycentric 좌표(u, v) 계산
    float denom = (dot00 * dot11 - dot01 * dot01);
    // 분모가 0에 가깝다면 (삼각형이 일직선이거나 데이터가 깨진 경우)
    if (std::abs(denom) < 0.000001f)
    {
        // NaN을 반환하지 말고, 계산이 불가능함을 알리거나 기본값 반환 
        return false;
    }

    float invDenom = 1.0f / denom;
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    // 최종 판정: u >= 0, v >= 0, u + v <= 1 이면 내부
    // 부동 소수점 오차를 고려하여 아주 작은 값(EPSILON)을 적용합니다.
    const float EPSILON = 0.001f;
    return (u >= -EPSILON) && (v >= -EPSILON) && (u + v <= 1.0f + EPSILON);
}


float Vector3::CalculateYaw(Vector3 dir)
{
    // 이동 거리가 거의 없으면 각도를 변경하지 않음 (0으로 나누기 방지)
    if (std::abs(dir.x) < EPSILON && std::abs(dir.z) < EPSILON)
        return 0.0f; // 혹은 기존 yaw 유지

    // atan2는 라디안 값을 반환 (-PI ~ PI)
    float radian = std::atan2(dir.x, dir.z);

    // 라디안 -> 도 변환
    float degree = radian * (180.0f / 3.1415926535f);

    // 결과를 0~360 범위로 정규화 (선택 사항)
    if (degree < 0) degree += 360.0f;

    if (!std::isfinite(degree))
        return 0.0f;

    return degree;
}
