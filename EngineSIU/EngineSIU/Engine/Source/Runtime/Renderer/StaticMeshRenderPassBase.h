#pragma once
#include "IRenderPass.h"
#include "RendererHelpers.h"
#include "Container/Array.h"

class UStaticMeshComponent;
class UMaterial;

struct FMatrix;
struct FVector4;
struct FMeshMaterial;
struct FStaticMeshRenderData;
struct ID3D11Buffer;

class FStaticMeshRenderPassBase : public IRenderPass
{
public:
    FStaticMeshRenderPassBase();
    virtual ~FStaticMeshRenderPassBase() override;

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;

    virtual void PrepareRenderArr() override;

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void ClearRenderArr() override;

protected:
    virtual void CreateResource() {}

    virtual void ReleaseShader() {}

    virtual void PrepareRenderPass(const std::shared_ptr<FEditorViewportClient>& Viewport) = 0;

    virtual void CleanUpRenderPass(const std::shared_ptr<FEditorViewportClient>& Viewport) = 0;

    virtual void Render_Internal(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void RenderAllStaticMeshes(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void RenderPrimitive(FStaticMeshRenderData* RenderData, TArray<FMeshMaterial*> Materials, TArray<UMaterial*> OverrideMaterials, int32 SelectedSubMeshIndex) const;

    void RenderPrimitive(ID3D11Buffer* Buffer, UINT VerticesNum) const;

    void RenderPrimitive(ID3D11Buffer* VertexBuffer, ID3D11Buffer* IndexBuffer, UINT IndicesNum) const;

    void UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const;

    FDXDBufferManager* BufferManager;
    FGraphicsDevice* Graphics;
    FDXDShaderManager* ShaderManager;

    TArray<UStaticMeshComponent*> StaticMeshComponents;
    ID3D11ShaderResourceView* SpotShadowArraySRV = nullptr;
};

