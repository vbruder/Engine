#include "QuadTree.h"


QuadTree::QuadTree()
{
}


QuadTree::QuadTree(const QuadTree &)
{
}


QuadTree::~QuadTree()
{
}


bool QuadTree::Initialize(Terrain* terrain, ID3D11Device* device)
{
    int vertexCount;

    float centerX;
    float centerZ;
    float width;

    vertexCount = terrain->GetVertexCount();
    m_triangleCount = vertexCount / 3;

    m_vertexList.resize(vertexCount);

    m_vertexList = terrain->GetVertices();
    CalculateMeshDimensions(vertexCount, centerX, centerZ, width);

    // Create the parent node for the quad tree.
    m_parentNode = new NodeType;
    if (!m_parentNode)
    {
        return false;
    }

    // Recursively build the quad tree based on the vertex list data and mesh dimensions.
    CreateTreeNode(m_parentNode, centerX, centerZ, width, device);

    // Release the vertex list since the quad tree now has the vertices in each node.
    m_vertexList.clear();

    return true;
}


void QuadTree::Shutdown()
{
        // Recursively release the quad tree data.
        if (m_parentNode)
        {
            ReleaseNode(m_parentNode);
            delete m_parentNode;
            m_parentNode = 0;
        }

    return;
}


void QuadTree::Render(Frustum* frustum,
                      ID3D11DeviceContext* deviceContext,
                      TerrainShader* shader)
{
    // Reset the number of triangles that are drawn for this frame.
    m_drawCount = 0;

    // Render each node that is visible.
    RenderNode(m_parentNode, frustum, deviceContext, shader);

    return;
}


int QuadTree::GetDrawCount()
{
    return m_drawCount;
}


void QuadTree::CalculateMeshDimensions(int vertexCount,
                                       float &centerX,
                                       float &centerZ,
                                       float &meshWidth)
{
    float maxWidth;
    float maxDepth;

    float minWidth;
    float minDepth;

    float width;
    float depth;

    float maxX;
    float maxZ;

    // Initialize the center position of the mesh.
    centerX = 0.0f;
    centerZ = 0.0f;

    // Sum all the vertices in the mesh.
    for (int i = 0; i < vertexCount - 1; i++)
    {
        centerX += m_vertexList[i].position.x;
        centerZ += m_vertexList[i].position.z;
    }

    // And then divide it by the number of vertices to find the mid-point of the mesh.
    centerX = centerX / (float) vertexCount;
    centerZ = centerZ / (float) vertexCount;

    // Initialize the maximum and minimum size of the mesh.
    maxWidth = 0.0f;
    maxDepth = 0.0f;

    minWidth = fabsf(m_vertexList[0].position.x - centerX);
    minDepth = fabsf(m_vertexList[0].position.z - centerZ);

    // Find the maximum and minimum width and depth of the mesh.
    for (int i = 0; i < vertexCount; i++)
    {
        width = fabsf(m_vertexList[i].position.x - centerX);
        depth = fabsf(m_vertexList[i].position.z - centerZ);

        if (width > maxWidth) { maxWidth = width; }
        if (depth > maxDepth) { maxDepth = depth; }
        if (width < minWidth) { minWidth = width; }
        if (depth < minDepth) { minDepth = depth; }
    }

    // Find the absolute maximum value between the min and max depth and width.
    maxX = (float) max(fabs(minWidth), fabs(maxWidth));
    maxZ = (float) max(fabs(minDepth), fabs(maxDepth));

    // Calculate the maximum diameter of the mesh.
    meshWidth = max(maxX, maxZ) * 2.0f;

    return;
}


void QuadTree::CreateTreeNode(NodeType* node, float positionX, float positionZ, float width, ID3D11Device* device)
{
    int numTriangles;
    int count;
    int vertexCount;
    int index;
    int vertexIndex;

    float offsetX;
    float offsetZ;

    unsigned long* indices;
    bool result;
    
    VertexCombined* vertices;
    
    D3D11_BUFFER_DESC vertexBufferDesc;
    D3D11_BUFFER_DESC indexBufferDesc;

    D3D11_SUBRESOURCE_DATA vertexData;
    D3D11_SUBRESOURCE_DATA indexData;

    node->positionX = positionX;
    node->positionZ = positionZ;
    node->width = width;

    node->triangleCount = 0;
    node->vertexBuffer = 0;
    node->indexBuffer = 0;

    // initialize children
    node->nodes[0] = 0;
    node->nodes[1] = 0;
    node->nodes[2] = 0;
    node->nodes[3] = 0;

    // Count the number of triangles inside this node.
    numTriangles = CountTriangles(positionX, positionZ, width);
    if (numTriangles == 0)
    {
        return;
    }
    // Node too big -> create new nodes.
    else if (numTriangles > MAX_TRIANGLES)
    {
        for (int i = 0; i < MAX_CHILDREN; i++)
        {
            // Calculate the position offsets for the new child node.
            offsetX = (((i % 2) < 1) ? -1.0f : 1.0f) * (width / 4.0f);
            offsetZ = (((i % 4) < 2) ? -1.0f : 1.0f) * (width / 4.0f);

            // See if there are any triangles in the new node.
            count = CountTriangles((positionX + offsetX), (positionZ + offsetZ), (width / 2.0f));
            if (count > 0)
            {
                node->nodes[i] = new NodeType;

                // Extend the tree, starting from this new child node now.
                CreateTreeNode( node->nodes[i],
                               (positionX + offsetX),
                               (positionZ + offsetZ),
                               (width / 2.0f),
                               device);
            }
        }

        return;
    }
    // We have a leaf node.
    else
    {
        node->triangleCount = numTriangles;

        vertexCount = numTriangles * 3;

        vertices = new VertexCombined[vertexCount];
        indices = new unsigned long[vertexCount];

        Vec3f tmpVertex;

        index = 0;

        for (int i = 0; i < m_triangleCount; ++i)
        {
            // If the triangle is inside this node then add it to the vertex array.
            result = IsTriangleContained(i, positionX, positionZ, width);
            if (result)
            {
                // Calculate the index into the terrain vertex list.
                vertexIndex = i * 3;

                // TODO performance! -> shared vertex class?
                for (int j = 0; j < 3; j++)
                {
                    // Get the three vertices of this triangle from the vertex list.
                    vertices[index].position = XMFLOAT3(m_vertexList[vertexIndex].position.x, m_vertexList[vertexIndex].position.y, m_vertexList[vertexIndex].position.z);
                    vertices[index].texture = XMFLOAT3(m_vertexList[vertexIndex].texture.x, m_vertexList[vertexIndex].texture.y, m_vertexList[vertexIndex].texture.z);
                    vertices[index].normal = XMFLOAT3(m_vertexList[vertexIndex].normal.x, m_vertexList[vertexIndex].normal.y, m_vertexList[vertexIndex].normal.z);
                    vertices[index].color = XMFLOAT4(m_vertexList[vertexIndex].color.r, m_vertexList[vertexIndex].color.g, m_vertexList[vertexIndex].color.b, m_vertexList[vertexIndex].color.a);
                    indices[index] = index;
                    // Store the vertex position information in the vertex list for
                    // intersection test (height based movement)
                    tmpVertex.Set(m_vertexList[vertexIndex].position);
                    node->vertexList.push_back(tmpVertex);
                    index++;
                    vertexIndex++;
                }
            }
        }

        // Set up the description of the vertex buffer.
        vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexBufferDesc.ByteWidth = sizeof(VertexType) * vertexCount;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = 0;
        vertexBufferDesc.MiscFlags = 0;
        vertexBufferDesc.StructureByteStride = 0;

        // Give the sub-resource structure a pointer to the vertex data.
        vertexData.pSysMem = vertices;
        vertexData.SysMemPitch = 0;
        vertexData.SysMemSlicePitch = 0;

        device->CreateBuffer(&vertexBufferDesc, &vertexData, &node->vertexBuffer);

        // Set up the description of the index buffer.
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        indexBufferDesc.ByteWidth = sizeof(unsigned long) * vertexCount;
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexBufferDesc.CPUAccessFlags = 0;
        indexBufferDesc.MiscFlags = 0;
        indexBufferDesc.StructureByteStride = 0;

        // Give the sub-resource structure a pointer to the index data.
        indexData.pSysMem = indices;
        indexData.SysMemPitch = 0;
        indexData.SysMemSlicePitch = 0;

        device->CreateBuffer(&indexBufferDesc, &indexData, &node->indexBuffer);

        // clean-up
        delete[] vertices;
        vertices = 0;

        delete[] indices;
        indices = 0;

        return;
    }
}


int QuadTree::CountTriangles(float positionX, float positionZ, float width)
{
    int count = 0;
    int i;
    bool result;

    omp_set_num_threads(NUM_THREADS);
    // parallel section
#pragma omp parallel private(i, result) shared(count, positionX, positionZ, width)
    {
#pragma omp for
        for (i = 0; i < m_triangleCount; ++i)
        {
            result = IsTriangleContained(i, positionX, positionZ, width);
            if (result == true)
            {
#pragma omp atomic
                count++;
            }
        }
    }

    return count;
}


bool QuadTree::IsTriangleContained(int index, float positionX, float positionZ, float width)
{
    float radius;
    int vertexIndex;

    float x1;
    float x2;
    float x3;
    
    float z1;
    float z2;
    float z3;

    float minimumX;
    float maximumX;
    
    float minimumZ;
    float maximumZ;

    radius = width / 2.0f;

    // Get the index into the vertex list.
    vertexIndex = index * 3;

    // Get the three vertices of this triangle.
    x1 = m_vertexList[vertexIndex].position.x;
    z1 = m_vertexList[vertexIndex].position.z;
    vertexIndex++;

    x2 = m_vertexList[vertexIndex].position.x;
    z2 = m_vertexList[vertexIndex].position.z;
    vertexIndex++;

    x3 = m_vertexList[vertexIndex].position.x;
    z3 = m_vertexList[vertexIndex].position.z;

    // Min and max of x-coordinate inside the triangle?
    minimumX = min(x1, min(x2, x3));
    if (minimumX > (positionX + radius))
    {
        return false;
    }
    maximumX = max(x1, max(x2, x3));
    if (maximumX < (positionX - radius))
    {
        return false;
    }

    // Min and max of z-coordinate inside the triangle?
    minimumZ = min(z1, min(z2, z3));
    if (minimumZ > (positionZ + radius))
    {
        return false;
    }
    maximumZ = max(z1, max(z2, z3));
    if (maximumZ < (positionZ - radius))
    {
        return false;
    }

    return true;
}


void QuadTree::ReleaseNode(NodeType* node)
{
    // Recursively go down the tree and release the bottom nodes first.
    for (int i = 0; i < MAX_CHILDREN; i++)
    {
        if (node->nodes[i] != 0)
        {
            ReleaseNode(node->nodes[i]);
        }
    }

    if (node->vertexBuffer)
    {
        node->vertexBuffer->Release();
        node->vertexBuffer = 0;
    }

    if (node->indexBuffer)
    {
        node->indexBuffer->Release();
        node->indexBuffer = 0;
    }

    for (int i = 0; i < MAX_CHILDREN; i++)
    {
        if (node->nodes[i])
        {
            delete node->nodes[i];
            node->nodes[i] = 0;
        }
    }

    return;
}


void QuadTree::RenderNode(NodeType* node,
                          Frustum* frustum,
                          ID3D11DeviceContext* deviceContext,
                          TerrainShader* shader)
{
    bool result;

    int count;
    int indexCount;

    unsigned int stride;
    unsigned int offset;

    // Check to see if the node can be seen.
    // ATTENTION: y-Center value is assumed to be 0! 
    // -> Undefined behavior on "higher/lower" nodes.
    result = frustum->CheckCube(node->positionX, 0.0f, node->positionZ, (node->width / 2.0f));
    if (!result)
    {
        return;
    }

    count = 0;
    for (int i = 0; i < MAX_CHILDREN; ++i)
    {
        if (node->nodes[i] != 0)
        {
            count++;
            RenderNode(node->nodes[i], frustum, deviceContext, shader);
        }
    }
    if (count != 0)
    {
        return;
    }

    // Render
    // Set vertex buffer stride and offset.
    stride = sizeof(VertexType);
    offset = 0;

    // Set the vertex and index buffers to active in the input assembler for rendered.
    deviceContext->IASetVertexBuffers(0, 1, &node->vertexBuffer, &stride, &offset);
    deviceContext->IASetIndexBuffer(node->indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Determine the number of indices in this node.
    indexCount = node->triangleCount * 3;

    // Render the polygons in this node.
    shader->RenderShader(deviceContext, indexCount);

    // Increase the count of the number of polygons that have been rendered during this frame.
    m_drawCount += node->triangleCount;

    return;
}


bool QuadTree::GetHeightAtPosition(float posX, float posZ, float& height)
{
    float meshMinX = m_parentNode->positionX - (m_parentNode->width / 2.0f);
    float meshMaxX = m_parentNode->positionX + (m_parentNode->width / 2.0f);

    float meshMinZ = m_parentNode->positionZ - (m_parentNode->width / 2.0f);
    float meshMaxZ = m_parentNode->positionZ + (m_parentNode->width / 2.0f);

    // Make sure the coordinates are actually above a polygon.
    if ((posX < meshMinX) || (posX > meshMaxX) || (posZ < meshMinZ) || (posZ > meshMaxZ))
    {
        return false;
    }

    // Find the node which contains the polygon for this position.
    FindNode(m_parentNode, posX, posZ, height);

    return true;
}


void QuadTree::FindNode(NodeType* node, float x, float z, float& height)
{
    int count = 0;
    int index = 0;

    Vec3f v0;
    Vec3f v1;
    Vec3f v2;

    bool foundHeight = false;

    // Calculate the dimensions of this node.
    float xMin = node->positionX - (node->width / 2.0f);
    float xMax = node->positionX + (node->width / 2.0f);

    float zMin = node->positionZ - (node->width / 2.0f);
    float zMax = node->positionZ + (node->width / 2.0f);

    // Check if the x and z coordinate are in this node.
    if ((x < xMin) || (x > xMax) || (z < zMin) || (z > zMax))
    {
        return;
    }

    // Check nodes to see if children nodes exist.
    for (int i = 0; i < MAX_CHILDREN; i++)
    {
        if (node->nodes[i] != 0)
        {
            count++;
            // recursive call
            FindNode(node->nodes[i], x, z, height);
        }
    }

    // If there were children nodes then return since the polygon will be in one of these.
    if (count > 0)
    {
        return;
    }

    // If there are no children, the polygon must be in this node. 
    // Check all polygons in this node to find the height we are looking for.
    for (int i = 0; i < node->triangleCount; i++)
    {
        v0 = node->vertexList.at(index++);
        v1 = node->vertexList.at(index++);
        v2 = node->vertexList.at(index++);

        // Check if this is the polygon we are looking for.
        foundHeight = CheckHeightOfTriangle(x, z, height, v0, v1, v2);

        if (foundHeight)
        {
            return;
        }
    }

    return;
}


bool QuadTree::CheckHeightOfTriangle(float x, float z, float& height,
                                     Vec3f v0, Vec3f v1, Vec3f v2)
{
    // Starting position of the ray that is being cast.
    Vec3f startVector = Vec3f(x, 0.0f, z);

    // The direction the ray is being cast (straight down).
    Vec3f directionVector = Vec3f(0.0f, -1.0f, 0.0f);
    
    // Calculate the three edges of the triangle from the three points given.
    Vec3f edge1 = v1 - v0;
    Vec3f edge2 = v2 - v0;
    Vec3f normal = normal.GetCross(edge1, edge2);
    Vec3f w0 = startVector - v0;

    float numerator = -normal.Dot(w0);
    float denominator = normal.Dot(directionVector);

    // Make sure the result does not get too close to zero to prevent division overflow.
    if (fabs(denominator) < 0.000001f)
    {
        return false;
    }

    // Calculate where we intersect the triangle.
    float r = numerator / denominator;

    // Find the intersection vector.
    Vec3f intersectionVec = startVector + directionVector*r;

    // check if intersection occurs inside the triangle
    float uu = edge1.Dot(edge1);
    float uv = edge1.Dot(edge2);
    float vv = edge2.Dot(edge2);

    Vec3f w = intersectionVec - v0;
    float wu = w.Dot(edge1);
    float wv = w.Dot(edge2);
    float D = uv * uv - uu * vv;

    // get and test parametric coordinates
    float s = (uv * wv - vv * wu) / D;
    // Intersection is outside the triangle
    if (s < 0.0 || s > 1.0)
    {
        return false;
    }

    float t = (uv * wu - uu * wv) / D;
    // Intersection is outside the triangle
    if (t < 0.0 || (s + t) > 1.0)
    {
        return false;
    }

    // intersection is inside the triangle
    height = intersectionVec.y;

    return true;
}