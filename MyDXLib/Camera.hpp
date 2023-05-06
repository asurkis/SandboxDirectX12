#pragma once

#include "pch.hpp"

class Camera
{
  public:
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Rot;
    DirectX::XMFLOAT3 xyz1;
    DirectX::XMFLOAT3 xyz2;
    float             Depth1;
    float             Depth2;

    DirectX::XMMATRIX CalcMatrix(bool zLess = false)
    {
        using namespace DirectX;
        XMMATRIX rotation = XMMatrixRotationRollPitchYaw(Rot.x, Rot.y, Rot.z);
        XMMATRIX position = XMMatrixTranslation(-Pos.x, -Pos.y, -Pos.z);
        XMMATRIX rotPos   = XMMatrixMultiply(position, rotation);

        // rotPos = XMMatrixLookAtLH(XMVectorSet(Pos.x, Pos.y, Pos.z, 1.0f),
        //                           XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
        //                           XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f));

        float widthRev  = 1.0f / (xyz2.x - xyz1.x);
        float heightRev = 1.0f / (xyz2.y - xyz1.y);
        float depthRev  = 1.0f / (xyz2.z - xyz1.z);

        XMMATRIX projection = XMMatrixSet(2.0f * widthRev,                                // row 0 col 0
                                          0.0f,                                           // row 1 col 0
                                          0.0f,                                           // row 2 col 0
                                          0.0f,                                           // row 3 col  0
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
        // projection = XMMatrixPerspectiveOffCenterLH(xyz1.x, xyz2.x, xyz1.y, xyz2.y, xyz1.z, xyz2.z);
        // return XMMatrixMultiply(rotPos, projection);
        return position * rotation * projection;
    }
};
