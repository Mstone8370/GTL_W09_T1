
#include "ShaderRegisters.hlsl"

cbuffer MaterialConstants : register(b1)
{
    FMaterial Material;
}

float4 mainPS(PS_INPUT_SkeletalMesh Input) : SV_TARGET
{
    float3 DiffuseColor = Material.DiffuseColor;
    if (Material.TextureFlag & TEXTURE_FLAG_DIFFUSE)
    {
        float4 DiffuseColor4 = MaterialTextures[TEXTURE_SLOT_DIFFUSE].Sample(MaterialSamplers[TEXTURE_SLOT_DIFFUSE], Input.UV);
        if (DiffuseColor4.a < 0.1f)
        {
            discard;
        }
        DiffuseColor = DiffuseColor4.rgb;
    }
    float3 WorldNormal = normalize(Input.WorldNormal);
    float3 LightDir = float3(1.0f, 1.0f, 1.0f);
	return float4(dot(LightDir, WorldNormal) * DiffuseColor, 1.0f);
}
