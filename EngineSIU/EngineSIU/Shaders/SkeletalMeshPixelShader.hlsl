
#include "ShaderRegisters.hlsl"

cbuffer MaterialConstants : register(b1)
{
    FMaterial Material;
}

float4 mainPS(PS_INPUT_SkeletalMesh Input) : SV_TARGET
{
    float3 DiffuseColor = Material.DiffuseColor;
    float3 WorldNormal = normalize(Input.WorldNormal);
    float3 LightDir = float3(1.0f, 1.0f, 1.0f);
	return float4(dot(LightDir, WorldNormal) * DiffuseColor, 1.0f);
}
