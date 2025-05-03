#pragma once
#include "IRenderPass.h"
#include "EngineBaseTypes.h"
#include "Container/Set.h"

#include "Define.h"
#include "Components/Light/PointLightComponent.h"

struct FStaticMeshRenderData;
class FShadowManager;
class FDXDShaderManager;
class UWorld;
class UMaterial;
class FEditorViewportClient;
class UStaticMeshComponent;
struct FMeshMaterial;
class FShadowRenderPass;

class FStaticMeshRenderPass : public IRenderPass
{
public:
    FStaticMeshRenderPass();
    
    virtual ~FStaticMeshRenderPass();
    
    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    
    void InitializeShadowManager(class FShadowManager* InShadowManager);
    
    virtual void PrepareRenderArr() override;

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void ClearRenderArr() override;
    void RenderAllStaticMeshesForPointLight(const std::shared_ptr<FEditorViewportClient>& Viewport, UPointLightComponent*& PointLight);

    virtual void PrepareRenderState(const std::shared_ptr<FEditorViewportClient>& Viewport);

    virtual void RenderAllStaticMeshes(const std::shared_ptr<FEditorViewportClient>& Viewport);
    
    void UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const;
  
    void UpdateLitUnlitConstant(int32 isLit) const;

    void RenderPrimitive(FStaticMeshRenderData* RenderData, TArray<FMeshMaterial*> Materials, TArray<UMaterial*> OverrideMaterials, int SelectedSubMeshIndex) const;
    
    void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices) const;

    void RenderPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices) const;

    // Shader 관련 함수 (생성/해제 등)
    void CreateShader();
    void ReleaseShader();

    void ChangeViewMode(EViewModeIndex ViewMode);
    
protected:


    TArray<UStaticMeshComponent*> StaticMeshComponents;

    /*
    ID3D11VertexShader* VertexShader;
    ID3D11InputLayout* InputLayout;
    
    ID3D11PixelShader* PixelShader;
    ID3D11PixelShader* DebugDepthShader;
    ID3D11PixelShader* DebugWorldNormalShader;
    */

    FDXDBufferManager* BufferManager;
    FGraphicsDevice* Graphics;
    FDXDShaderManager* ShaderManager;
    
    FShadowManager* ShadowManager;
};
