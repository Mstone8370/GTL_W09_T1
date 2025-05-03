
#include "ShaderRegisters.hlsl"


PS_INPUT_SkeletalMesh mainVS(VS_INPUT_SkeletalMesh Input)
{
    PS_INPUT_SkeletalMesh Output;
    
    Output.Position = float4(Input.Position, 1.0);
    Output.Position = mul(Output.Position, WorldMatrix);
    Output.WorldPosition = Output.Position.xyz;
    
    Output.Position = mul(Output.Position, ViewMatrix);
    Output.Position = mul(Output.Position, ProjectionMatrix);
    
    Output.WorldNormal = mul(Input.Normal, (float3x3)InverseTransposedWorld);

    // Begin Tangent
    float3 WorldTangent = mul(Input.Tangent.xyz, (float3x3)WorldMatrix);
    WorldTangent = normalize(WorldTangent);
    WorldTangent = normalize(WorldTangent - Output.WorldNormal * dot(Output.WorldNormal, WorldTangent));

    Output.WorldTangent = float4(WorldTangent, Input.Tangent.w);
    // End Tangent

    Output.Color = Input.Color;
    Output.UV = Input.UV;

    return Output;
}
