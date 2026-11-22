#include "Shader.h"

#include <d3dcompiler.h>

#include <cstdio>
#include <stdexcept>
#include <string>

#pragma comment(lib, "d3dcompiler.lib")

namespace {

const char kShaderSource[] = R"hlsl(
static const float PI = 3.14159265359;

struct PointLight
{
    float3 lightPos;
    float lightIntensity;
    float3 lightColor;
    float padding;
};

cbuffer SceneConstants : register(b0)
{
    row_major float4x4 gView;
    row_major float4x4 gProjection;
    float3 gCameraPos;
    float gCameraPadding;
    PointLight gLight;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
    float4 albedo : TEXCOORD2;
    float metallic : TEXCOORD3;
    float roughness : TEXCOORD4;
};

VSOutput VSInstanced(VSInput input,
                     row_major float4x4 worldMatrix : INSTANCE_WORLD,
                     float4 albedo : INSTANCE_ALBEDO,
                     float2 material : INSTANCE_MATERIAL)
{
    VSOutput output;
    float4 worldPosition = mul(float4(input.position, 1.0f), worldMatrix);

    output.position = mul(mul(worldPosition, gView), gProjection);
    output.worldPos = worldPosition.xyz;
    output.worldNormal = mul(input.normal, (float3x3)worldMatrix);
    output.albedo = albedo;
    output.metallic = material.x;
    output.roughness = material.y;
    return output;
}

float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 1e-6);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / max(NdotV * (1.0 - k) + k, 1e-5);
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    return GeometrySchlickGGX(NdotV, roughness) *
           GeometrySchlickGGX(NdotL, roughness);
}

float3 FresnelSchlick(float VdotH, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - VdotH), 5.0);
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    float3 N = normalize(input.worldNormal);
    float3 V = normalize(gCameraPos - input.worldPos);
    float3 L = normalize(gLight.lightPos - input.worldPos);
    float3 H = normalize(V + L);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    float roughness = clamp(input.roughness, 0.05, 1.0);
    float3 albedo = input.albedo.rgb;
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, input.metallic);

    float D = DistributionGGX(NdotH, roughness);
    float G = GeometrySmith(NdotV, NdotL, roughness);
    float3 F = FresnelSchlick(VdotH, F0);

    float3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-4);
    float3 kD = (1.0 - F) * (1.0 - input.metallic);
    float3 diffuse = kD * albedo / PI;

    float3 toLight = gLight.lightPos - input.worldPos;
    float distanceSq = max(dot(toLight, toLight), 0.01);
    float3 radiance = gLight.lightColor * gLight.lightIntensity / distanceSq;

    float3 color = (diffuse + specular) * radiance * NdotL;
    color += albedo * 0.03 + 0.005;

    color = color / (color + 1.0);
    color = pow(saturate(color), 1.0 / 2.2);

    return float4(color, 1.0);
}
)hlsl";

void ThrowIfFailed(HRESULT result, const char* operation) {
    if (FAILED(result)) {
        char message[256] = {};
        std::snprintf(message, sizeof(message), "%s failed (HRESULT 0x%08X)",
                      operation, static_cast<unsigned int>(result));
        throw std::runtime_error(message);
    }
}

Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(const char* entryPoint,
                                                const char* target) {
    Microsoft::WRL::ComPtr<ID3DBlob> bytecode;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;

    const UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
    const HRESULT result = D3DCompile(
        kShaderSource, sizeof(kShaderSource) - 1, "EmbeddedShader", nullptr,
        nullptr, entryPoint, target, flags, 0, bytecode.GetAddressOf(),
        errors.GetAddressOf());

    if (FAILED(result)) {
        std::string message = "D3DCompile failed for ";
        message += entryPoint;
        message += " (target ";
        message += target;
        message += ")";
        if (errors) {
            message += ": ";
            message.append(static_cast<const char*>(errors->GetBufferPointer()),
                           errors->GetBufferSize());
        }
        throw std::runtime_error(message);
    }

    return bytecode;
}

} // namespace

void Shader::Initialize(ID3D11Device* device) {
    const auto vertexBytecode = CompileShader("VSInstanced", "vs_5_0");
    const auto pixelBytecode = CompileShader("PSMain", "ps_5_0");

    ThrowIfFailed(
        device->CreateVertexShader(vertexBytecode->GetBufferPointer(),
                                   vertexBytecode->GetBufferSize(), nullptr,
                                   vertexShaderInstanced_.GetAddressOf()),
        "CreateVertexShader(instanced)");
    ThrowIfFailed(
        device->CreatePixelShader(pixelBytecode->GetBufferPointer(),
                                  pixelBytecode->GetBufferSize(), nullptr,
                                  pixelShader_.GetAddressOf()),
        "CreatePixelShader");

    const D3D11_INPUT_ELEMENT_DESC instancedLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
          D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "INSTANCE_WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,
          D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16,
          D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32,
          D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48,
          D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_ALBEDO", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64,
          D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_MATERIAL", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 80,
          D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    };

    ThrowIfFailed(
        device->CreateInputLayout(
            instancedLayout,
            static_cast<UINT>(sizeof(instancedLayout) /
                              sizeof(instancedLayout[0])),
            vertexBytecode->GetBufferPointer(),
            vertexBytecode->GetBufferSize(),
            inputLayoutInstanced_.GetAddressOf()),
        "CreateInputLayout(instanced)");
}

void Shader::Bind(ID3D11DeviceContext* context) const {
    context->IASetInputLayout(inputLayoutInstanced_.Get());
    context->VSSetShader(vertexShaderInstanced_.Get(), nullptr, 0);
    context->PSSetShader(pixelShader_.Get(), nullptr, 0);
}
