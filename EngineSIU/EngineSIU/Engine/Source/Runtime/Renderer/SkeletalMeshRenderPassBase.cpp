#include "SkeletalMeshRenderPassBase.h"

#include "UObject/UObjectIterator.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/EditorEngine.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/Casts.h"
#include "Editor/PropertyEditor/ShowFlags.h"
#include "Engine/Asset/SkeletalMeshAsset.h"
#include <stack>

class UEditorEngine;

FSkeletalMeshRenderPassBase::FSkeletalMeshRenderPassBase()
    : BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
{
}

FSkeletalMeshRenderPassBase::~FSkeletalMeshRenderPassBase()
{
    ReleaseResource();
}

void FSkeletalMeshRenderPassBase::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;

    CreateResource();
}

void FSkeletalMeshRenderPassBase::PrepareRenderArr()
{
    for (const auto iter : TObjectRange<USkeletalMeshComponent>())
    {
        if (iter->GetWorld() != GEngine->ActiveWorld)
        {
            continue;
        }
        SkeletalMeshComponents.Add(iter);
    }
}

void FSkeletalMeshRenderPassBase::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareRenderPass(Viewport);

    Render_Internal(Viewport);

    CleanUpRenderPass(Viewport);
}

void FSkeletalMeshRenderPassBase::ClearRenderArr()
{
    SkeletalMeshComponents.Empty();
}

void FSkeletalMeshRenderPassBase::Render_Internal(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    RenderAllSkeletalMeshes(Viewport);
}

void FSkeletalMeshRenderPassBase::RenderAllSkeletalMeshes(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    for (USkeletalMeshComponent* Comp : SkeletalMeshComponents)
    {
        if (!Comp || !Comp->GetSkeletalMesh())
        {
            continue;
        }

        const FSkeletalMeshRenderData* RenderData = Comp->GetSkeletalMesh()->GetRenderData();
        if (RenderData == nullptr)
        {
            continue;
        }

        UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);

        FMatrix WorldMatrix = Comp->GetWorldMatrix();
        FVector4 UUIDColor = Comp->EncodeUUID() / 255.0f;
        const bool bIsSelected = (Engine && Engine->GetSelectedActor() == Comp->GetOwner());

        UpdateObjectConstant(WorldMatrix, UUIDColor, bIsSelected);

        RenderSkeletalMesh(RenderData);

        if (Viewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_AABB))
        {
            FEngineLoop::PrimitiveDrawBatch.AddAABBToBatch(Comp->GetBoundingBox(), Comp->GetWorldLocation(), WorldMatrix);
        }
    }
}

void FSkeletalMeshRenderPassBase::RenderSkeletalMesh(const FSkeletalMeshRenderData* RenderData) const
{
    UINT Stride = sizeof(FSkeletalMeshVertex);
    UINT Offset = 0;

    FSkeletalMeshRenderData TempSkinnedData = *RenderData;
    for (FSkeletalMeshVertex& Vtx : TempSkinnedData.Vertices)
    {
        FVector SkinnedPos = SkinVertexPosition(Vtx, *RenderData);
        Vtx.X = SkinnedPos.X;
        Vtx.Y = SkinnedPos.Y;
        Vtx.Z = SkinnedPos.Z;
    }

    FVertexInfo VertexInfo;
    BufferManager->CreateVertexBuffer(TempSkinnedData.ObjectName, TempSkinnedData.Vertices, VertexInfo);

    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &Stride, &Offset);

    FIndexInfo IndexInfo;
    BufferManager->CreateIndexBuffer(TempSkinnedData.ObjectName, TempSkinnedData.Indices, IndexInfo);
    if (IndexInfo.IndexBuffer)
    {
        Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        Graphics->DeviceContext->DrawIndexed(TempSkinnedData.Indices.Num(), 0, 0);
    }
    else
    {
        Graphics->DeviceContext->Draw(TempSkinnedData.Vertices.Num(), 0);
    }
}

void FSkeletalMeshRenderPassBase::RenderSkeletalMesh(ID3D11Buffer* Buffer, UINT VerticesNum) const
{
}

void FSkeletalMeshRenderPassBase::RenderSkeletalMesh(ID3D11Buffer* VertexBuffer, ID3D11Buffer* IndexBuffer, UINT IndicesNum) const
{
}

void FSkeletalMeshRenderPassBase::UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const
{
    FObjectConstantBuffer ObjectData = {};
    ObjectData.WorldMatrix = WorldMatrix;
    ObjectData.InverseTransposedWorld = FMatrix::Transpose(FMatrix::Inverse(WorldMatrix));
    ObjectData.UUIDColor = UUIDColor;
    ObjectData.bIsSelected = bIsSelected;

    BufferManager->UpdateConstantBuffer(TEXT("FObjectConstantBuffer"), ObjectData);
}

FVector FSkeletalMeshRenderPassBase::SkinVertexPosition(const FSkeletalMeshVertex& Vertex, const FSkeletalMeshRenderData& RenderData) const
{
    FVector OriginalPos(Vertex.X, Vertex.Y, Vertex.Z);
    FVector SkinnedPos(0.f, 0.f, 0.f);
    float TotalWeight = 0.f;

    for (int i = 0; i < 4; i++) {
        int32 BoneIndex = Vertex.BoneIndices[i];
        float Weight = Vertex.BoneWeights[i];
        if (Weight > 0.f
            && BoneIndex >= 0 && BoneIndex < RenderData.BoneBindPoseTransforms.Num() && BoneIndex < RenderData.BoneLocalTransforms.Num())
        {
            FTransform CurrentGlobal = ComputeGlobalTransform(RenderData, BoneIndex, RenderData.BoneLocalTransforms);

            // 바인드 포즈 (글로벌)
            const FTransform& BindGlobal = RenderData.BoneBindPoseTransforms[BoneIndex];
            FMatrix Current = CurrentGlobal.ToMatrixWithScale();
            FMatrix Bind = BindGlobal.ToMatrixWithScale();
            Bind = Bind.Inverse(Bind);

            FMatrix SkinMatrix = Current * Bind;
            SkinnedPos += SkinMatrix.TransformPosition(OriginalPos) * Weight;
            TotalWeight += Weight;
        }
    }

    if (TotalWeight <= 0.f) {
        return OriginalPos;
    }

    if (TotalWeight < 1.f) {
        SkinnedPos += OriginalPos * (1.f - TotalWeight);
    }
    return SkinnedPos;
    //FVector Result = { 0, 0, 0 };
    //FVector Pos(Vertex.X, Vertex.Y, Vertex.Z);

    //for (int i = 0; i < 4; ++i) {
    //    int BoneIndex = Vertex.BoneIndices[i];
    //    float weight = Vertex.BoneWeights[i];

    //    if (weight > 0.f && BoneIndex >= 0 && BoneIndex < RenderData.BoneBindPoseTransforms.Num() && BoneIndex < RenderData.BoneLocalTransforms.Num())
    //    {
    //        /*const FTransform& CurrentPose = RenderData.BoneLocalTransforms[BoneIndex];
    //        const FTransform& BindPose = RenderData.BoneBindPoseTransforms[BoneIndex];
    //        
    //        FMatrix SkinMatrix = (CurrentPose.ToMatrixWithScale()) * FMatrix::Inverse(BindPose.ToMatrixWithScale());

    //        Result += SkinMatrix.TransformPosition(Pos) * weight;*/

    //        FTransform GlobalPose = ComputeGlobalTransform(RenderData, BoneIndex, RenderData.BoneLocalTransforms);
    //        //FTransform BindPose = ComputeGlobalTransform(RenderData, BoneIndex, RenderData.BoneBindPoseTransforms);

    //        //FMatrix SkinMatrix = RenderData.BoneLocalTransforms[BoneIndex].ToMatrixWithScale() * FMatrix::Inverse(RenderData.BoneBindPoseTransforms[BoneIndex].ToMatrixWithScale());
    //        //FMatrix SkinMatrix = GlobalPose.ToMatrixWithScale() * FMatrix::Inverse(BindPose.ToMatrixWithScale());
    //        FMatrix SkinMatrix = GlobalPose.ToMatrixWithScale() * FMatrix::Inverse(RenderData.BoneBindPoseTransforms[BoneIndex].ToMatrixWithScale());
    //        Result += SkinMatrix.TransformPosition(Pos) * weight;


    //        /*FMatrix SkinMatrix = RenderData.BoneBindPoseTransforms[BoneIndex].ToMatrixWithScale();
    //        FVector Pos(Vertex.X, Vertex.Y, Vertex.Z);
    //        FVector Transformed = SkinMatrix.TransformPosition(Pos);
    //        Result = Result + (FVector{ Transformed.X, Transformed.Y, Transformed.Z } *weight);*/
    //    }
    //}
    //return Result;
}

FTransform FSkeletalMeshRenderPassBase::ComputeGlobalTransform(const FSkeletalMeshRenderData& RenderData, int32 BoneIndex, const TArray<FTransform>& LocalTransforms) const
{
    std::stack<FTransform> TransformStack;

    int32 CurrentIndex = BoneIndex; 
    while (CurrentIndex >= 0)
    {
        TransformStack.push(LocalTransforms[CurrentIndex]);
        CurrentIndex = RenderData.BoneParents[CurrentIndex];
    }

    FTransform Result = TransformStack.top();
    TransformStack.pop();
    
    while (!TransformStack.empty())
    {
        Result = Result * TransformStack.top();
        TransformStack.pop();
    }

    return Result;
    /*FTransform Result = LocalTransforms[BoneIndex];

    int32 ParentIndex = RenderData.BoneParents[BoneIndex];
    while (ParentIndex >= 0)
    {
        Result = LocalTransforms[ParentIndex] * Result;
        ParentIndex = RenderData.BoneParents[ParentIndex];
    }

    return Result;*/
}
