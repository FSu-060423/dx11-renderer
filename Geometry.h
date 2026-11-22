#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>

#include <cstdint>
#include <vector>

struct Vertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 texcoord;
};

class Geometry {
public:
    void InitializeSphere(ID3D11Device* device, float radius = 1.0f,
                          UINT slices = 64, UINT stacks = 32);
    void CreateUnitCube(ID3D11Device* device);
    void CreateUnitCylinder(ID3D11Device* device, UINT segments = 48);
    void Bind(ID3D11DeviceContext* context) const;
    void DrawIndexed(ID3D11DeviceContext* context) const;
    void DrawIndexedInstanced(ID3D11DeviceContext* context,
                              UINT instanceCount) const;
    void Draw(ID3D11DeviceContext* context) const;

    UINT IndexCount() const { return indexCount_; }

private:
    void UploadMesh(ID3D11Device* device,
                    const std::vector<Vertex>& vertices,
                    const std::vector<std::uint16_t>& indices);

    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer_;
    UINT indexCount_ = 0;
};
