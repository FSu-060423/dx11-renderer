#include "Geometry.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace {

constexpr float kPi = 3.14159265358979323846f;

void ThrowIfFailed(HRESULT result, const char* operation) {
    if (FAILED(result)) {
        char message[256] = {};
        std::snprintf(message, sizeof(message), "%s failed (HRESULT 0x%08X)",
                      operation, static_cast<unsigned int>(result));
        throw std::runtime_error(message);
    }
}

} // namespace

void Geometry::InitializeSphere(ID3D11Device* device, float radius,
                                UINT slices, UINT stacks) {
    if (slices < 3 || stacks < 2) {
        throw std::runtime_error("sphere requires at least 3 slices and 2 stacks");
    }

    std::vector<Vertex> vertices;
    vertices.reserve(static_cast<std::size_t>(stacks + 1) * (slices + 1));

    for (UINT stack = 0; stack <= stacks; ++stack) {
        const float v = static_cast<float>(stack) / static_cast<float>(stacks);
        const float phi = v * kPi;
        const float y = std::cos(phi);
        const float ringRadius = std::sin(phi);

        for (UINT slice = 0; slice <= slices; ++slice) {
            const float u =
                static_cast<float>(slice) / static_cast<float>(slices);
            const float theta = u * 2.0f * kPi;
            const float x = ringRadius * std::cos(theta);
            const float z = ringRadius * std::sin(theta);

            Vertex vertex = {};
            vertex.position = { x * radius, y * radius, z * radius };
            vertex.normal = { x, y, z };
            vertex.texcoord = { u, 1.0f - v };
            vertices.push_back(vertex);
        }
    }

    if (vertices.size() > 65535u) {
        throw std::runtime_error("sphere vertex count exceeds 16-bit index range");
    }

    std::vector<std::uint16_t> indices;
    indices.reserve(static_cast<std::size_t>(stacks) * slices * 6u);

    for (UINT stack = 0; stack < stacks; ++stack) {
        for (UINT slice = 0; slice < slices; ++slice) {
            const std::uint16_t i0 =
                static_cast<std::uint16_t>(stack * (slices + 1) + slice);
            const std::uint16_t i1 = static_cast<std::uint16_t>(i0 + 1);
            const std::uint16_t i2 =
                static_cast<std::uint16_t>(i0 + slices + 1);
            const std::uint16_t i3 = static_cast<std::uint16_t>(i2 + 1);

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i1);
            indices.push_back(i3);
            indices.push_back(i2);
        }
    }

    UploadMesh(device, vertices, indices);
}

void Geometry::CreateUnitCube(ID3D11Device* device) {
    constexpr float h = 0.5f;

    const Vertex vertices[] = {
        // +X
        {{ h, -h, -h }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
        {{ h,  h, -h }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},
        {{ h,  h,  h }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
        {{ h, -h,  h }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }},
        // -X
        {{-h, -h,  h }, {-1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
        {{-h,  h,  h }, {-1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},
        {{-h,  h, -h }, {-1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
        {{-h, -h, -h }, {-1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }},
        // +Y
        {{-h,  h, -h }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }},
        {{-h,  h,  h }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }},
        {{ h,  h,  h }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }},
        {{ h,  h, -h }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }},
        // -Y
        {{-h, -h,  h }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f }},
        {{-h, -h, -h }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }},
        {{ h, -h, -h }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }},
        {{ h, -h,  h }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f }},
        // +Z
        {{-h, -h,  h }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }},
        {{-h,  h,  h }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }},
        {{ h,  h,  h }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }},
        {{ h, -h,  h }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }},
        // -Z
        {{ h, -h, -h }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f }},
        {{ h,  h, -h }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f }},
        {{-h,  h, -h }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f }},
        {{-h, -h, -h }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f }},
    };

    const std::uint16_t indices[] = {
         0,  1,  2,   0,  2,  3,
         4,  5,  6,   4,  6,  7,
         8,  9, 10,   8, 10, 11,
        12, 13, 14,  12, 14, 15,
        16, 17, 18,  16, 18, 19,
        20, 21, 22,  20, 22, 23,
    };

    const std::size_t vertexCount = sizeof(vertices) / sizeof(vertices[0]);
    const std::size_t indexCount = sizeof(indices) / sizeof(indices[0]);
    std::vector<Vertex> vertexList(vertices, vertices + vertexCount);
    std::vector<std::uint16_t> indexList(indices, indices + indexCount);
    UploadMesh(device, vertexList, indexList);
}

void Geometry::CreateUnitCylinder(ID3D11Device* device, UINT segments) {
    if (segments < 3) {
        throw std::runtime_error("cylinder requires at least 3 segments");
    }

    constexpr float radius = 0.5f;
    constexpr float halfHeight = 0.5f;
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> indices;

    // Side wall: two vertices per segment boundary, bottom then top.
    for (UINT i = 0; i <= segments; ++i) {
        const float u = static_cast<float>(i) / static_cast<float>(segments);
        const float angle = u * 2.0f * kPi;
        const float cx = std::cos(angle);
        const float sz = std::sin(angle);
        const float x = radius * cx;
        const float z = radius * sz;

        vertices.push_back({ { x, -halfHeight, z },
                             { cx, 0.0f, sz },
                             { u, 0.0f } });
        vertices.push_back({ { x, halfHeight, z },
                             { cx, 0.0f, sz },
                             { u, 1.0f } });
    }

    for (UINT i = 0; i < segments; ++i) {
        const std::uint16_t bottom0 = static_cast<std::uint16_t>(i * 2);
        const std::uint16_t top0 = static_cast<std::uint16_t>(i * 2 + 1);
        const std::uint16_t bottom1 =
            static_cast<std::uint16_t>((i + 1) * 2);
        const std::uint16_t top1 =
            static_cast<std::uint16_t>((i + 1) * 2 + 1);

        indices.push_back(bottom0);
        indices.push_back(bottom1);
        indices.push_back(top0);
        indices.push_back(top0);
        indices.push_back(bottom1);
        indices.push_back(top1);
    }

    const UINT topRingBase = static_cast<UINT>(vertices.size());
    for (UINT i = 0; i <= segments; ++i) {
        const float angle =
            2.0f * kPi * static_cast<float>(i) / static_cast<float>(segments);
        const float x = radius * std::cos(angle);
        const float z = radius * std::sin(angle);
        vertices.push_back({ { x, halfHeight, z },
                             { 0.0f, 1.0f, 0.0f },
                             { x + 0.5f, z + 0.5f } });
    }
    const UINT topCenter = static_cast<UINT>(vertices.size());
    vertices.push_back(
        { { 0.0f, halfHeight, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.5f, 0.5f } });

    for (UINT i = 0; i < segments; ++i) {
        indices.push_back(static_cast<std::uint16_t>(topCenter));
        indices.push_back(static_cast<std::uint16_t>(topRingBase + i));
        indices.push_back(static_cast<std::uint16_t>(topRingBase + i + 1));
    }

    const UINT bottomRingBase = static_cast<UINT>(vertices.size());
    for (UINT i = 0; i <= segments; ++i) {
        const float angle =
            2.0f * kPi * static_cast<float>(i) / static_cast<float>(segments);
        const float x = radius * std::cos(angle);
        const float z = radius * std::sin(angle);
        vertices.push_back({ { x, -halfHeight, z },
                             { 0.0f, -1.0f, 0.0f },
                             { x + 0.5f, z + 0.5f } });
    }
    const UINT bottomCenter = static_cast<UINT>(vertices.size());
    vertices.push_back(
        { { 0.0f, -halfHeight, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.5f, 0.5f } });

    for (UINT i = 0; i < segments; ++i) {
        indices.push_back(static_cast<std::uint16_t>(bottomCenter));
        indices.push_back(static_cast<std::uint16_t>(bottomRingBase + i + 1));
        indices.push_back(static_cast<std::uint16_t>(bottomRingBase + i));
    }

    UploadMesh(device, vertices, indices);
}

void Geometry::UploadMesh(ID3D11Device* device,
                          const std::vector<Vertex>& vertices,
                          const std::vector<std::uint16_t>& indices) {
    D3D11_BUFFER_DESC vertexDesc = {};
    vertexDesc.ByteWidth =
        static_cast<UINT>(vertices.size() * sizeof(Vertex));
    vertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = vertices.data();
    ThrowIfFailed(device->CreateBuffer(&vertexDesc, &vertexData,
                                       vertexBuffer_.ReleaseAndGetAddressOf()),
                  "CreateBuffer(sphere vertices)");

    D3D11_BUFFER_DESC indexDesc = {};
    indexDesc.ByteWidth =
        static_cast<UINT>(indices.size() * sizeof(std::uint16_t));
    indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices.data();
    ThrowIfFailed(device->CreateBuffer(&indexDesc, &indexData,
                                       indexBuffer_.ReleaseAndGetAddressOf()),
                  "CreateBuffer(sphere indices)");

    indexCount_ = static_cast<UINT>(indices.size());
}

void Geometry::Bind(ID3D11DeviceContext* context) const {
    const UINT stride = sizeof(Vertex);
    const UINT offset = 0;
    ID3D11Buffer* vertexBuffers[] = { vertexBuffer_.Get() };

    context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
    context->IASetIndexBuffer(indexBuffer_.Get(), DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Geometry::DrawIndexed(ID3D11DeviceContext* context) const {
    context->DrawIndexed(indexCount_, 0, 0);
}

void Geometry::DrawIndexedInstanced(ID3D11DeviceContext* context,
                                    UINT instanceCount) const {
    context->DrawIndexedInstanced(indexCount_, instanceCount, 0, 0, 0);
}

void Geometry::Draw(ID3D11DeviceContext* context) const {
    Bind(context);
    DrawIndexed(context);
}
