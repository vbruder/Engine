#pragma once
#include "ScreenElement.h"
class Element2d :
    public ScreenElement
{
public:
    Element2d();
    Element2d(const Element2d &);
    ~Element2d();

    bool Initialize(ID3D11Device*, int, int, WCHAR*, int, int);
    void Shutdown();
    bool Render(ID3D11DeviceContext*, int, int);

private:

    struct VertexType
    {
        XMFLOAT3 position;
        XMFLOAT2 texture;
    };

    bool InitializeBuffers(ID3D11Device *);
    void ShutdownBuffers();
    bool UpdateBuffers(ID3D11DeviceContext *, int, int);
    void RenderBuffers(ID3D11DeviceContext *);

private:

    int m_screenWidth;
    int m_screenHeight;

    int m_bitmapWidth;
    int m_bitmapHeight;

    int m_previousPosX;
    int m_previousPosY;
};
