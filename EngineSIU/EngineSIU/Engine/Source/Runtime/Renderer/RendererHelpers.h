#pragma once
#include "Launch/EngineLoop.h"

enum class EShaderSRVSlot : int8
{
    SRV_SpotLight = 50,
    SRV_DirectionalLight = 51,
    SRV_PointLight = 52,
    SRV_SceneDepth = 99,
    SRV_Scene = 100,
    SRV_PostProcess = 101,
    SRV_EditorOverlay = 102,
    SRV_Fog = 103,
    SRV_Debug = 104,
    SRV_CameraEffect = 105,
    SRV_Viewport = 120,

    SRV_MAX = 127,
};

namespace MaterialUtils
{
    inline void UpdateMaterial(FDXDBufferManager* BufferManager, FGraphicsDevice* Graphics, const FMaterialInfo& MaterialInfo)
    {
        FMaterialConstants Data;
        
        Data.TextureFlag = MaterialInfo.TextureFlag;
        
        Data.DiffuseColor = MaterialInfo.DiffuseColor;
        
        Data.SpecularColor = MaterialInfo.SpecularColor;
        Data.Shininess = MaterialInfo.SpecularExponent;
        
        Data.EmissiveColor = MaterialInfo.EmissiveColor;
        Data.Transparency = MaterialInfo.Transparency;

        Data.Metallic = MaterialInfo.Metallic;
        Data.Roughness = MaterialInfo.Roughness;

        BufferManager->UpdateConstantBuffer(TEXT("FMaterialConstants"), Data);

        ID3D11ShaderResourceView* SRVs[9] = {};
        ID3D11SamplerState* Samplers[9] = {};

        for (uint8 i = 0; i < static_cast<uint8>(EMaterialTextureSlots::MTS_MAX); ++i)
        {
            if (MaterialInfo.TextureFlag & (1 << i)) // EMaterialTextureFlags와 EMaterialTextureSlots의 순서가 일치한다는 전제 조건.
            {
                std::shared_ptr<FTexture> Texture = FEngineLoop::ResourceManager.GetTexture(MaterialInfo.TextureInfos[i].TexturePath);
                SRVs[i] = Texture->TextureSRV;
                Samplers[i] = Texture->SamplerState;

                if (i == static_cast<uint8>(EMaterialTextureSlots::MTS_Diffuse))
                {
                    // for Gouraud shading
                    Graphics->DeviceContext->VSSetShaderResources(0, 1, &Texture->TextureSRV);
                    Graphics->DeviceContext->VSSetSamplers(0, 1, &Texture->SamplerState);
                }
            }
        }

        Graphics->DeviceContext->PSSetShaderResources(0, 9, SRVs);
        Graphics->DeviceContext->PSSetSamplers(0, 9, Samplers);
    }
}
