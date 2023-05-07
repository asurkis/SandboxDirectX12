#pragma once

#include "pch.hpp"

class Camera
{
  public:
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Rot;
    DirectX::XMFLOAT3 xyz1;
    DirectX::XMFLOAT3 xyz2;

    float Depth1;
    float Depth2;

    DirectX::XMVECTOR Forward() const noexcept
    {
        using namespace DirectX;
        return XMVector3Rotate(XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f), CalcQuaternion());
    }

    DirectX::XMVECTOR Right() const noexcept
    {
        using namespace DirectX;
        return XMVector3Rotate(XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f), CalcQuaternion());
    }

    DirectX::XMVECTOR CalcQuaternion() const noexcept
    {
        using namespace DirectX;
        XMVECTOR quatX = XMQuaternionRotationNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f), Rot.x);
        XMVECTOR quatY = XMQuaternionRotationNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), Rot.y);
        XMVECTOR quatZ = XMQuaternionRotationNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f), Rot.z);
        return XMQuaternionMultiply(quatX, XMQuaternionMultiply(quatY, quatZ));
    }

    DirectX::XMMATRIX CalcMatrix() const noexcept
    {
        using namespace DirectX;
        XMVECTOR quat     = CalcQuaternion();
        XMMATRIX rotation = XMMatrixRotationQuaternion(quat * XMVectorSet(1.0f, 1.0f, 1.0f, -1.0f));
        XMMATRIX position = XMMatrixTranslation(-Pos.x, -Pos.y, -Pos.z);

        float widthRev  = 1.0f / (xyz2.x - xyz1.x);
        float heightRev = 1.0f / (xyz2.y - xyz1.y);
        float depthRev  = 1.0f / (xyz2.z - xyz1.z);

        XMMATRIX projection = XMMatrixSet(2.0f * widthRev,                                // row 0 col 0
                                          0.0f,                                           // row 1 col 0
                                          0.0f,                                           // row 2 col 0
                                          0.0f,                                           // row 3 col 0
                                          0.0f,                                           // row 0 col 1
                                          2.0f * heightRev,                               // row 1 col 1
                                          0.0f,                                           // row 2 col 1
                                          0.0f,                                           // row 3 col 1
                                          (xyz1.x + xyz2.x) * widthRev,                   // row 0 col 2
                                          (xyz1.y + xyz2.y) * heightRev,                  // row 1 col 2
                                          (Depth2 * xyz2.z - Depth1 * xyz1.z) * depthRev, // row 2 col 2
                                          1.0f,                                           // row 3 col 2
                                          0.0f,                                           // row 0 col 3
                                          0.0f,                                           // row 1 col 3
                                          (Depth1 - Depth2) * xyz1.z * xyz2.z * depthRev, // row 2 col 3
                                          0.0f                                            // row 3 col 3
        );
        return position * rotation * projection;
    }
};
