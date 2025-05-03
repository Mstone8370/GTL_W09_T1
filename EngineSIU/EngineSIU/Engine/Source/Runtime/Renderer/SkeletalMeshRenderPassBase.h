#pragma once
#include "IRenderPass.h"
#include "Container/Array.h"

class UMaterial;
struct FSkeletalMeshRenderData;
class USkeletalMeshComponent;

struct FMatrix;
struct FVector4;
struct FStaticMaterial;
struct FStaticMeshRenderData;
struct ID3D11Buffer;

class FSkeletalMeshRenderPassBase : public IRenderPass
{
public:
    FSkeletalMeshRenderPassBase();
    virtual ~FSkeletalMeshRenderPassBase() override;

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;

    virtual void PrepareRenderArr() override;

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void ClearRenderArr() override;

protected:
    virtual void CreateResource() {}

    virtual void ReleaseResource() {}

    virtual void PrepareRenderPass(const std::shared_ptr<FEditorViewportClient>& Viewport) = 0;

    virtual void CleanUpRenderPass(const std::shared_ptr<FEditorViewportClient>& Viewport) = 0;

    virtual void Render_Internal(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void RenderAllSkeletalMeshes(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void RenderSkeletalMesh(const FSkeletalMeshRenderData* RenderData) const;

    void RenderSkeletalMesh(ID3D11Buffer* Buffer, UINT VerticesNum) const;

    void RenderSkeletalMesh(ID3D11Buffer* VertexBuffer, ID3D11Buffer* IndexBuffer, UINT IndicesNum) const;

    void UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const;

    FDXDBufferManager* BufferManager;
    FGraphicsDevice* Graphics;
    FDXDShaderManager* ShaderManager;

    TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
};
