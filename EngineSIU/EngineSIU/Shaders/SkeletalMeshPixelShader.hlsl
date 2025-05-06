
#include "ShaderRegisters.hlsl"

cbuffer MaterialConstants : register(b1)
{
    FMaterial Material;
}

float4 mainPS(PS_INPUT_SkeletalMesh Input) : SV_TARGET
{
    // Diffuse
    float3 DiffuseColor = Material.DiffuseColor;
    float3 WorldNormal = normalize(Input.WorldNormal);
    float3 LightDir = float3(1.0f, 1.0f, 1.0f);
    if (Material.TextureFlag & TEXTURE_FLAG_DIFFUSE)
    {
        float4 DiffuseColor4 = MaterialTextures[TEXTURE_SLOT_DIFFUSE].Sample(MaterialSamplers[TEXTURE_SLOT_DIFFUSE], Input.UV);
        if (DiffuseColor4.a <= 0.01f)
        {
            discard;
        }
        DiffuseColor = DiffuseColor4.rgb;
    }

	return float4(DiffuseColor, 1.0f);
}
