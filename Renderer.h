#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>
#include <wrl/client.h>

#include <vector>

#include "Geometry.h"
#include "Shader.h"

class Profiler;

enum class GeometryType {
    Box,
    Cylinder,
    Sphere,
};

struct Entity {
    GeometryType geometry = GeometryType::Box;
    DirectX::XMFLOAT3 position = {};
    DirectX::XMFLOAT3 rotation = {};
    DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT4 albedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    float metallic = 0.0f;
    float roughness = 0.5f;
};

struct alignas(16) InstanceData {
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4 albedo;
    float metallic;
    float roughness;
    float padding[2];
};

class Renderer {
public:
    void Initialize(HWND hwnd, UINT width, UINT height);
    void Render(float timeSeconds, Profiler& profiler);
    void Present();
    void Resize(UINT width, UINT height);

private:
    void CreateDeviceAndSwapChain();
    void CreateRenderTargets();
    void CreateDepthBuffer();
    void CreateConstantBuffer();
    void CreateInstanceBuffer();
    void CreateRasterizerState();
    void CreateDepthStencilState();
    void UpdateCamera();
    std::vector<Entity> BuildBasketballScene() const;
    void ReleaseRenderTargets();

    HWND hwnd_ = nullptr;
    UINT width_ = 0;
    UINT height_ = 0;

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;

    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthTexture_;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView_;

    Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> instanceBuffer_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState_;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState_;

    Geometry boxGeometry_;
    Geometry cylinderGeometry_;
    Geometry sphereGeometry_;
    Shader shader_;
    std::vector<Entity> entities_;
    std::vector<InstanceData> instanceData_;

    DirectX::XMFLOAT4X4 view_ = {};
    DirectX::XMFLOAT4X4 projection_ = {};
    DirectX::XMFLOAT3 cameraPos_ = {};
    DirectX::XMFLOAT3 lightPos_ = {};
    DirectX::XMFLOAT3 lightColor_ = { 1.0f, 0.95f, 0.9f };
    float lightIntensity_ = 500.0f;
};
