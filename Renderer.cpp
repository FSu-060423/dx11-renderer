#include "Renderer.h"

#include "Profiler.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace {

void ThrowIfFailed(HRESULT result, const char* operation) {
    if (FAILED(result)) {
        char message[256] = {};
        std::snprintf(message, sizeof(message), "%s failed (HRESULT 0x%08X)",
                      operation, static_cast<unsigned int>(result));
        throw std::runtime_error(message);
    }
}

struct alignas(16) PointLight {
    DirectX::XMFLOAT3 lightPos;
    float lightIntensity;
    DirectX::XMFLOAT3 lightColor;
    float padding;
};

struct alignas(16) SceneConstants {
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;
    DirectX::XMFLOAT3 cameraPos;
    float cameraPadding;
    PointLight light;
};

static_assert(sizeof(PointLight) == 32, "PointLight must be 32 bytes");
static_assert(offsetof(SceneConstants, view) == 0,
              "view must start at byte 0");
static_assert(offsetof(SceneConstants, projection) == 64,
              "projection must start at byte 64");
static_assert(offsetof(SceneConstants, cameraPos) == 128,
              "cameraPos must start at byte 128");
static_assert(offsetof(SceneConstants, light) == 144,
              "PointLight must start at byte 144");
static_assert(sizeof(SceneConstants) == 176,
              "SceneConstants must match the HLSL cbuffer layout");
static_assert(sizeof(SceneConstants) % 16 == 0,
              "constant buffer size must be a multiple of 16 bytes");
static_assert(sizeof(InstanceData) == 96, "InstanceData must be 96 bytes");
static_assert(offsetof(InstanceData, world) == 0,
              "InstanceData.world must start at byte 0");
static_assert(offsetof(InstanceData, albedo) == 64,
              "InstanceData.albedo must start at byte 64");
static_assert(offsetof(InstanceData, metallic) == 80,
              "InstanceData.metallic must start at byte 80");
static_assert(offsetof(InstanceData, roughness) == 84,
              "InstanceData.roughness must start at byte 84");
static_assert(offsetof(InstanceData, padding) == 88,
              "InstanceData.padding must start at byte 88");

} // namespace

void Renderer::Initialize(HWND hwnd, UINT width, UINT height) {
    hwnd_ = hwnd;
    width_ = width;
    height_ = height;

    CreateDeviceAndSwapChain();
    CreateRenderTargets();
    CreateDepthBuffer();
    shader_.Initialize(device_.Get());
    boxGeometry_.CreateUnitCube(device_.Get());
    cylinderGeometry_.CreateUnitCylinder(device_.Get());
    sphereGeometry_.InitializeSphere(device_.Get(), 1.0f, 64, 32);
    CreateConstantBuffer();
    CreateRasterizerState();
    CreateDepthStencilState();
    UpdateCamera();

    entities_ = BuildBasketballScene();
    CreateInstanceBuffer();
}

void Renderer::CreateDeviceAndSwapChain() {
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferDesc.Width = width_;
    swapChainDesc.BufferDesc.Height = height_;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.OutputWindow = hwnd_;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    D3D_FEATURE_LEVEL obtainedFeatureLevel = D3D_FEATURE_LEVEL_10_0;
    ThrowIfFailed(
        D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevels,
            static_cast<UINT>(sizeof(featureLevels) / sizeof(featureLevels[0])),
            D3D11_SDK_VERSION, &swapChainDesc, swapChain_.GetAddressOf(),
            device_.GetAddressOf(), &obtainedFeatureLevel,
            context_.GetAddressOf()),
        "D3D11CreateDeviceAndSwapChain");
}

void Renderer::CreateRenderTargets() {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    ThrowIfFailed(
        swapChain_->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())),
        "IDXGISwapChain::GetBuffer");
    ThrowIfFailed(
        device_->CreateRenderTargetView(backBuffer.Get(), nullptr,
                                        renderTargetView_.GetAddressOf()),
        "CreateRenderTargetView");
}

void Renderer::CreateDepthBuffer() {
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = width_;
    depthDesc.Height = height_;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ThrowIfFailed(
        device_->CreateTexture2D(&depthDesc, nullptr,
                                 depthTexture_.GetAddressOf()),
        "CreateTexture2D(depth)");
    ThrowIfFailed(
        device_->CreateDepthStencilView(depthTexture_.Get(), nullptr,
                                        depthStencilView_.GetAddressOf()),
        "CreateDepthStencilView");
}

void Renderer::CreateConstantBuffer() {
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = static_cast<UINT>(sizeof(SceneConstants));
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ThrowIfFailed(
        device_->CreateBuffer(&bufferDesc, nullptr,
                              constantBuffer_.GetAddressOf()),
        "CreateBuffer(constant)");
}

void Renderer::CreateInstanceBuffer() {
    const UINT entityCount = static_cast<UINT>(entities_.size());
    instanceData_.resize(entityCount);

    UINT instanceByteWidth =
        static_cast<UINT>(sizeof(InstanceData)) * entityCount;
    instanceByteWidth = (instanceByteWidth + 15u) & ~15u;

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = instanceByteWidth;
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ThrowIfFailed(
        device_->CreateBuffer(&bufferDesc, nullptr,
                              instanceBuffer_.GetAddressOf()),
        "CreateBuffer(instance)");
}

void Renderer::CreateRasterizerState() {
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.DepthClipEnable = TRUE;

    ThrowIfFailed(
        device_->CreateRasterizerState(&rasterizerDesc,
                                       rasterizerState_.GetAddressOf()),
        "CreateRasterizerState");
}

void Renderer::CreateDepthStencilState() {
    D3D11_DEPTH_STENCIL_DESC depthStateDesc = {};
    depthStateDesc.DepthEnable = TRUE;
    depthStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStateDesc.StencilEnable = FALSE;

    ThrowIfFailed(
        device_->CreateDepthStencilState(&depthStateDesc,
                                         depthStencilState_.GetAddressOf()),
        "CreateDepthStencilState");
}

void Renderer::UpdateCamera() {
    if (height_ == 0) {
        return;
    }

    const float aspect =
        static_cast<float>(width_) / static_cast<float>(height_);
    const DirectX::XMVECTOR eye =
        DirectX::XMVectorSet(0.0f, 8.5f, -20.0f, 1.0f);
    const DirectX::XMVECTOR target =
        DirectX::XMVectorSet(0.0f, 2.5f, 0.0f, 1.0f);
    const DirectX::XMVECTOR up =
        DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    const DirectX::XMMATRIX view =
        DirectX::XMMatrixLookAtLH(eye, target, up);
    const DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(55.0f), aspect, 0.1f, 100.0f);

    DirectX::XMStoreFloat4x4(&view_, view);
    DirectX::XMStoreFloat4x4(&projection_, projection);
    DirectX::XMStoreFloat3(&cameraPos_, eye);
}

std::vector<Entity> Renderer::BuildBasketballScene() const {
    std::vector<Entity> entities;
    entities.reserve(12);

    const DirectX::XMFLOAT4 wood = { 0.6f, 0.4f, 0.2f, 1.0f };
    const DirectX::XMFLOAT4 darkMetal = { 0.3f, 0.3f, 0.3f, 1.0f };
    const DirectX::XMFLOAT4 white = { 0.9f, 0.9f, 0.9f, 1.0f };
    const DirectX::XMFLOAT4 redMetal = { 1.0f, 0.1f, 0.1f, 1.0f };
    const DirectX::XMFLOAT4 orange = { 1.0f, 0.4f, 0.1f, 1.0f };

    Entity floor = {};
    floor.geometry = GeometryType::Box;
    floor.position = { 0.0f, 0.0f, 0.0f };
    floor.scale = { 30.0f, 0.2f, 15.0f };
    floor.albedo = wood;
    floor.metallic = 0.0f;
    floor.roughness = 0.4f;
    entities.push_back(floor);

    for (float x : { -12.0f, 12.0f }) {
        Entity pillar = {};
        pillar.geometry = GeometryType::Cylinder;
        pillar.position = { x, 4.0f, 5.0f };
        pillar.scale = { 0.5f, 8.0f, 0.5f };
        pillar.albedo = darkMetal;
        pillar.metallic = 1.0f;
        pillar.roughness = 0.2f;
        entities.push_back(pillar);

        Entity backboard = {};
        backboard.geometry = GeometryType::Box;
        backboard.position = { x, 5.0f, 2.5f };
        // Rotate the 0.2 x 2 x 4 box so its 4-unit dimension spans the court
        // width and its broad face points toward the court.
        backboard.rotation = { 0.0f, DirectX::XM_PIDIV2, 0.0f };
        backboard.scale = { 0.2f, 2.0f, 4.0f };
        backboard.albedo = white;
        backboard.metallic = 0.0f;
        backboard.roughness = 0.5f;
        entities.push_back(backboard);

        Entity supportArm = {};
        supportArm.geometry = GeometryType::Box;
        supportArm.position = { x, 5.0f, 3.75f };
        supportArm.scale = { 0.15f, 0.15f, 2.5f };
        supportArm.albedo = darkMetal;
        supportArm.metallic = 1.0f;
        supportArm.roughness = 0.2f;
        entities.push_back(supportArm);

        Entity hoop = {};
        hoop.geometry = GeometryType::Cylinder;
        hoop.position = { x, 3.8f, 1.9f };
        hoop.scale = { 1.0f, 0.1f, 1.0f };
        hoop.albedo = redMetal;
        hoop.metallic = 1.0f;
        hoop.roughness = 0.1f;
        entities.push_back(hoop);
    }

    const DirectX::XMFLOAT3 ballPositions[3] = {
        { -4.0f, 0.5f, 1.0f },
        { 2.5f, 0.5f, -1.5f },
        { 6.0f, 0.5f, 2.5f },
    };

    for (const DirectX::XMFLOAT3& position : ballPositions) {
        Entity ball = {};
        ball.geometry = GeometryType::Sphere;
        ball.position = position;
        ball.scale = { 0.4f, 0.4f, 0.4f };
        ball.albedo = orange;
        ball.metallic = 0.0f;
        ball.roughness = 0.8f;
        entities.push_back(ball);
    }

    return entities;
}

void Renderer::Render(float timeSeconds, Profiler& profiler) {
    if (!context_ || !swapChain_) {
        return;
    }

    lightPos_ = {
        std::cos(timeSeconds) * 8.0f,
        10.0f,
        -std::sin(timeSeconds) * 8.0f,
    };

    const float clearColor[4] = { 0.015f, 0.018f, 0.025f, 1.0f };
    context_->ClearRenderTargetView(renderTargetView_.Get(), clearColor);
    context_->ClearDepthStencilView(
        depthStencilView_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
        1.0f, 0);

    ID3D11RenderTargetView* renderTargets[] = { renderTargetView_.Get() };
    context_->OMSetRenderTargets(1, renderTargets, depthStencilView_.Get());

    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width_);
    viewport.Height = static_cast<float>(height_);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);
    context_->RSSetState(rasterizerState_.Get());
    context_->OMSetDepthStencilState(depthStencilState_.Get(), 0);

    SceneConstants constants = {};
    constants.view = view_;
    constants.projection = projection_;
    constants.cameraPos = cameraPos_;
    constants.cameraPadding = 0.0f;
    constants.light.lightPos = lightPos_;
    constants.light.lightIntensity = lightIntensity_;
    constants.light.lightColor = lightColor_;
    constants.light.padding = 0.0f;

    D3D11_MAPPED_SUBRESOURCE constantMapped = {};
    ThrowIfFailed(
        context_->Map(constantBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                      &constantMapped),
        "Map(constant buffer)");
    std::memcpy(constantMapped.pData, &constants, sizeof(constants));
    context_->Unmap(constantBuffer_.Get(), 0);

    ID3D11Buffer* constantBuffers[] = { constantBuffer_.Get() };
    context_->VSSetConstantBuffers(0, 1, constantBuffers);
    context_->PSSetConstantBuffers(0, 1, constantBuffers);

    shader_.Bind(context_.Get());

    struct Batch {
        GeometryType type;
        Geometry* geometry;
    };

    const Batch batches[] = {
        { GeometryType::Box, &boxGeometry_ },
        { GeometryType::Cylinder, &cylinderGeometry_ },
        { GeometryType::Sphere, &sphereGeometry_ },
    };

    for (const Batch& batch : batches) {
        UINT instanceCount = 0;
        for (const Entity& entity : entities_) {
            if (entity.geometry != batch.type) {
                continue;
            }

            const DirectX::XMMATRIX world =
                DirectX::XMMatrixScaling(entity.scale.x, entity.scale.y,
                                         entity.scale.z) *
                DirectX::XMMatrixRotationRollPitchYaw(
                    entity.rotation.x, entity.rotation.y,
                    entity.rotation.z) *
                DirectX::XMMatrixTranslation(entity.position.x,
                                             entity.position.y,
                                             entity.position.z);

            InstanceData& data = instanceData_[instanceCount++];
            DirectX::XMStoreFloat4x4(&data.world, world);
            data.albedo = entity.albedo;
            data.metallic = entity.metallic;
            data.roughness = entity.roughness;
            data.padding[0] = 0.0f;
            data.padding[1] = 0.0f;
        }

        if (instanceCount == 0) {
            continue;
        }

        D3D11_MAPPED_SUBRESOURCE instanceMapped = {};
        ThrowIfFailed(
            context_->Map(instanceBuffer_.Get(), 0,
                          D3D11_MAP_WRITE_DISCARD, 0, &instanceMapped),
            "Map(instance buffer)");
        std::memcpy(instanceMapped.pData, instanceData_.data(),
                    sizeof(InstanceData) * instanceCount);
        context_->Unmap(instanceBuffer_.Get(), 0);

        batch.geometry->Bind(context_.Get());

        ID3D11Buffer* instanceBuffers[] = { instanceBuffer_.Get() };
        const UINT instanceStride =
            static_cast<UINT>(sizeof(InstanceData));
        const UINT instanceOffset = 0;
        context_->IASetVertexBuffers(1, 1, instanceBuffers, &instanceStride,
                                     &instanceOffset);

        profiler.BeginDrawSubmit();
        batch.geometry->DrawIndexedInstanced(context_.Get(), instanceCount);
        profiler.EndDrawSubmit();
        profiler.RecordDrawCall();
    }
}

void Renderer::Present() {
    ThrowIfFailed(swapChain_->Present(1, 0), "IDXGISwapChain::Present");
}

void Renderer::Resize(UINT width, UINT height) {
    if (!swapChain_ || !context_ || width == 0 || height == 0) {
        return;
    }

    context_->OMSetRenderTargets(0, nullptr, nullptr);
    ReleaseRenderTargets();

    width_ = width;
    height_ = height;

    ThrowIfFailed(swapChain_->ResizeBuffers(0, width_, height_,
                                             DXGI_FORMAT_UNKNOWN, 0),
                  "IDXGISwapChain::ResizeBuffers");

    CreateRenderTargets();
    CreateDepthBuffer();
    UpdateCamera();
}

void Renderer::ReleaseRenderTargets() {
    renderTargetView_.Reset();
    depthStencilView_.Reset();
    depthTexture_.Reset();
}
