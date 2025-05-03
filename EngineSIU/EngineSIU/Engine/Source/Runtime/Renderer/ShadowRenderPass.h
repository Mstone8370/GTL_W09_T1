#pragma once
#include "IRenderPass.h"
#include "EngineBaseTypes.h"
#include "Container/Set.h"
#include "Define.h"
#include "UnrealClient.h" // Depth Stencil View
#include <d3d11.h>

#include "Components/Light/PointLightComponent.h"


// ShadowMap을 생성하기 위한 Render Pass입니다.
// Depth 값만을 추출하는 Vertex Shader Only 패스
// 모든 Light에 대한 Depth Map Texture를 생성합니다.
// ViewMode != Unlit일 때에만 Static Mesh Render Pass 에서 실행됩니다

struct FStaticMeshRenderData;
class FDXDBufferManager;
class FDXDShaderManager;
class FGraphicsDevice;
class ULightComponentBase;

class FShadowRenderPass : public IRenderPass
{
public:
    FShadowRenderPass();
    virtual ~FShadowRenderPass() override;
    void CreateShader();
    void PrepareCubeMapRenderState(
    );
    void UpdateCubeMapConstantBuffer(UPointLightComponent*& PointLight, const FMatrix& WorldMatrix) const;
    void RenderCubeMap(const std::shared_ptr<FEditorViewportClient>& Viewport, UPointLightComponent*& PointLight);
    void SetLightData(const TArray<class UPointLightComponent*>& InPointLights, const TArray<class USpotLightComponent*>& InSpotLights);
    
    void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    void InitializeShadowManager(class FShadowManager* InShadowManager);
    void PrepareRenderState();
    void PrepareCSMRenderState();
    virtual void PrepareRenderArr() override;
    void UpdateIsShadowConstant(int32 isShadow) const;
    void Render(ULightComponentBase* Light);
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;    
    virtual void ClearRenderArr() override;

    void RenderPrimitive(FStaticMeshRenderData* render_data, const TArray<FMeshMaterial*> array, TArray<UMaterial*> materials, int32 SelectedSubMeshIndex);
    virtual void RenderAllStaticMeshes(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void RenderAllStaticMeshesForCSM(const std::shared_ptr<FEditorViewportClient>& Viewport,
                                     FCascadeConstantBuffer FCasCadeData);
    void BindResourcesForSampling();

    void UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const;

    void RenderAllStaticMeshesForPointLight(const std::shared_ptr<FEditorViewportClient>& Viewport, UPointLightComponent*& PointLight);


private:

    
    TArray<class UStaticMeshComponent*> StaticMeshComponents;
    TArray<UPointLightComponent*> PointLights;
    TArray<USpotLightComponent*> SpotLights;
    
    FDXDBufferManager* BufferManager;
    FGraphicsDevice* Graphics;
    FDXDShaderManager* ShaderManager;
    FShadowManager* ShadowManager;

    ID3D11InputLayout* StaticMeshIL;
    ID3D11VertexShader* DepthOnlyVS;
    ID3D11PixelShader* DepthOnlyPS;
    ID3D11SamplerState* Sampler;

    ID3D11VertexShader* DepthCubeMapVS;
    ID3D11GeometryShader* DepthCubeMapGS;


    ID3D11VertexShader* CascadedShadowMapVS;
    ID3D11GeometryShader* CascadedShadowMapGS;
    ID3D11PixelShader* CascadedShadowMapPS;

    D3D11_VIEWPORT ShadowViewport;


    uint32 ShadowMapWidth = 2048;
    uint32 ShadowMapHeight = 2048;

    FLOAT ClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};
