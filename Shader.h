#pragma once

#include <d3d11.h>
#include <wrl/client.h>

class Shader {
public:
    void Initialize(ID3D11Device* device);
    void Bind(ID3D11DeviceContext* context) const;

private:
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShaderInstanced_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayoutInstanced_;
};
