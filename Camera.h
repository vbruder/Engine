#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace DirectX;
using namespace DirectX::PackedVector;

class Camera
{
public:
    Camera();
    Camera(const Camera &);
    ~Camera();

    void SetPosition(float, float, float);
    void SetRotation(float, float, float);

    XMFLOAT3 GetPosition();
    XMFLOAT3 GetRotation();
    void GetViewMatrix(XMMATRIX &viewMatrix);

    void Render();

private:
    XMFLOAT3 m_position;
    XMFLOAT3 m_rotation;

    XMMATRIX m_viewMatrix;
};

