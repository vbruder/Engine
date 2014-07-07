#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include <vector>

#include "Util.h"
#include "Texture.h"
#include "Vec3f.h"
#include "Vec4f.h"
#include "VertexType.h"

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

using namespace math;

// GLOBALS
const int TEXTURE_REPEAT = 8;

class Terrain
{
public:
    Terrain();
    Terrain(const Terrain &);
    ~Terrain();

    bool Initialize(ID3D11Device *device,
                    WCHAR *hightmapFilename,
                    WCHAR *texFilename,
                    WCHAR *colorMapFilename,
                    Util  *util);
    void Shutdown();

    int GetVertexCount();
    vector<VertexType> Terrain::GetVertices();
    ID3D11ShaderResourceView* GetTexture();
    float GetScalingFactor();
    // Set the scaling for the terrain height.
    void SetScalingFactor(float);


private:
    bool InitializeBuffers();
    void ShutdownBuffers();

    // Height map
    bool LoadHeightMap(WCHAR *heightmapFilename);
    bool LoadColorMap(WCHAR *colorMapFilename);
    void NormalizeHeightMap();
    bool CalculateNormals();
    void ShutdownHeightMap();

    // Texturing
    void CalculateTextureCoordinates();
    bool LoadTexture(ID3D11Device *device, WCHAR *texFilename);
    void ReleaseTexture();

private:
    int m_terrainWidth;
    int m_terrainHeight;

    float m_scaling;

    int m_vertexCount;

    Util *m_Util;
    Texture *m_pTexture;

    vector<VertexType> m_heightMap;
    vector<VertexType> m_vertices;
};
