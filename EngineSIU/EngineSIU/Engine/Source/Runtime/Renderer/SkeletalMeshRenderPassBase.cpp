#include "SkeletalMeshRenderPassBase.h"

#include <format>

#include "UObject/UObjectIterator.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/EditorEngine.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/Casts.h"
#include "Editor/PropertyEditor/ShowFlags.h"
#include "Engine/Asset/SkeletalMeshAsset.h"

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
    for (USkeletalMeshComponent* Component : TObjectRange<USkeletalMeshComponent>())
    {
        if (!Component || Component->GetWorld() != GEngine->ActiveWorld) // 유효성 및 가시성 검사 추가
        {
            continue;
        }
        // --- CPU 스키닝 시작 ---
        const FSkeletalMeshRenderData* RenderData = Component->GetSkeletalMesh()->GetRenderData();
        if (!RenderData || RenderData->Vertices.IsEmpty() || RenderData->SkeletonBones.IsEmpty())
        {
            // 스키닝에 필요한 데이터 없음
            continue;
        }

        // 1. 현재 애니메이션 포즈 가져오기 (로컬 변환)
        TArray<FTransform> CurrentLocalTransforms;
        if (!Component->GetCurrentLocalBoneTransforms(CurrentLocalTransforms))
        {
            // 애니메이션 데이터 가져오기 실패 (애니메이션 없음 등)
            // 이 경우, 참조 포즈(T-Pose)로 렌더링하거나 컴포넌트를 건너뛸 수 있음
            // 여기서는 T-Pose 렌더링을 위해 항등 로컬 변환 사용 가정 (GetCurrentLocalBoneTransforms 기본 구현)
             if(CurrentLocalTransforms.Num() != RenderData->SkeletonBones.Num()) {
                 // GetCurrentLocalBoneTransforms가 실패했거나 크기가 안맞음
                 continue; // 또는 기본 포즈 처리
             }
        }
        // 로컬 변환 배열 크기 검증
        if (CurrentLocalTransforms.Num() != RenderData->SkeletonBones.Num())
        {
             OutputDebugStringA("Error: Mismatch between bone count and current local transforms count.\n");
             continue;
        }

        // --- 테스트 코드 시작 ---
        int32 TargetBoneIndex = 2; // 예시: 테스트할 뼈의 인덱스
        float Count = 0.01f;
        static float GameTime = 0.1f;
        GameTime += Count;
        float OscillationSpeed = 2.0f;
        float MaxRotationDegrees = 45.0f;
        
        if (TargetBoneIndex >= 0 && TargetBoneIndex < CurrentLocalTransforms.Num())
        {
            FTransform OriginalLocalTransform = CurrentLocalTransforms[TargetBoneIndex]; // 참조 포즈 로컬 변환 사용

            // 시간에 따라 변하는 회전 각도 계산 (Sine 함수 사용)
            float CurrentAngleDegrees = FMath::Sin(GameTime * OscillationSpeed) * MaxRotationDegrees;
            FQuat OscillationRotation = FQuat(FVector::ForwardVector, FMath::DegreesToRadians(CurrentAngleDegrees)); // 예: Forward 축 기준

            // 참조 포즈 회전에 추가 회전 적용
            // 중요: 애니메이션이 없으므로 CurrentLocalTransforms는 참조 포즈 로컬 변환이어야 함
            FTransform ModifiedLocalTransform = OriginalLocalTransform;
            ModifiedLocalTransform.Rotation = (OscillationRotation * OriginalLocalTransform.GetRotation());

            CurrentLocalTransforms[TargetBoneIndex] = ModifiedLocalTransform;
        }
        // --- 테스트 코드 끝 ---


        // 2. 전역 뼈 변환 계산 (컴포넌트 공간 기준)
        TArray<FMatrix> GlobalBoneMatrices;
        GlobalBoneMatrices.SetNum(RenderData->SkeletonBones.Num());

        for (int32 BoneIndex = 0; BoneIndex < RenderData->SkeletonBones.Num(); ++BoneIndex)
        {
            const FBoneInfo& BoneInfo = RenderData->SkeletonBones[BoneIndex];
            const FTransform& LocalTransform = CurrentLocalTransforms[BoneIndex];
            FMatrix LocalMatrix = LocalTransform.ToMatrixWithScale(); // FTransform -> FMatrix

            if (BoneInfo.ParentIndex == INDEX_NONE) // 루트 뼈
            {
                GlobalBoneMatrices[BoneIndex] = LocalMatrix;
            }
            else
            {
                // 부모 인덱스 유효성 검사
                if (BoneInfo.ParentIndex >= 0 && BoneInfo.ParentIndex < BoneIndex) // 부모는 항상 현재 인덱스보다 앞에 있어야 함
                {
                    GlobalBoneMatrices[BoneIndex] = LocalMatrix * GlobalBoneMatrices[BoneInfo.ParentIndex];
                }
                else
                {
                    // 잘못된 부모 인덱스 또는 순서 오류
                    OutputDebugStringA(std::format("Error: Invalid parent index ({}) for bone '{}' (index {}).\n", BoneInfo.ParentIndex, (*BoneInfo.Name), BoneIndex).c_str());
                    GlobalBoneMatrices[BoneIndex] = LocalMatrix; // 오류 시 로컬 행렬 사용 (임시방편)
                }
            }
        }

        // 3. 스키닝 행렬 계산
        TArray<FMatrix> SkinningMatrices;
        SkinningMatrices.SetNum(RenderData->SkeletonBones.Num());
        for (int32 BoneIndex = 0; BoneIndex < RenderData->SkeletonBones.Num(); ++BoneIndex)
        {
            // 역 바인드 포즈 행렬 유효성 검사
            if (BoneIndex >= RenderData->InverseBindPoseMatrices.Num()) {
                 OutputDebugStringA(std::format("Error: Missing inverse bind pose matrix for bone index {}.\n", BoneIndex).c_str());
                 SkinningMatrices[BoneIndex] = FMatrix::Identity; // 오류 시 항등 행렬
                 continue;
            }
            // 스키닝 행렬 = 전역 뼈 변환 * 역 바인드 포즈 변환
            SkinningMatrices[BoneIndex] = RenderData->InverseBindPoseMatrices[BoneIndex] * GlobalBoneMatrices[BoneIndex];
            // 주의: 곱셈 순서 확인 필요. UE는 보통 MatrixA * MatrixB = A를 먼저 적용.
            // 스키닝 공식: FinalPos = sum( weight * (VertexPos * InvBindPose * GlobalBone) )
            // 따라서 SkinningMatrix = InvBindPose * GlobalBone 이 맞음.
        }


        // 4. 정점 변형 (Deformation)
        const int32 NumVertices = RenderData->Vertices.Num();
        Component->SkinnedPositions.SetNum(NumVertices); // 결과 버퍼 크기 설정
        Component->SkinnedNormals.SetNum(NumVertices);
        Component->SkinnedVertices.SetNum(NumVertices);
        // Component->SkinnedTangents.SetNum(NumVertices); // 필요 시

        for (int32 VertIndex = 0; VertIndex < NumVertices; ++VertIndex)
        {
            const FSkeletalMeshVertex& RefVertex = RenderData->Vertices[VertIndex];
            Component->SkinnedVertices[VertIndex] = RefVertex;
            FSkeletalMeshVertex& OutVertex = Component->SkinnedVertices[VertIndex]; // 출력할 정점 참조

            // 참조 포즈 데이터 (모델 공간)
            const FVector RefPosition = FVector(RefVertex.X, RefVertex.Y, RefVertex.Z); // 모델 공간 위치
            const FVector RefNormal = FVector(RefVertex.NormalX, RefVertex.NormalY, RefVertex.NormalZ); // 모델 공간 노멀
            // const FVector RefTangent = RefVertex.Tangent; // 필요 시

            FVector SkinnedPosition = FVector::ZeroVector;
            FVector SkinnedNormal = FVector::ZeroVector;
            // FVector SkinnedTangent = FVector::ZeroVector; // 필요 시

            // 최대 4개의 뼈 영향력 처리
            for (int32 InfluenceIndex = 0; InfluenceIndex < 4; ++InfluenceIndex)
            {
                const uint8 BoneIndex = RefVertex.BoneIndices[InfluenceIndex];
                const float Weight = RefVertex.BoneWeights[InfluenceIndex];

                if (Weight > KINDA_SMALL_NUMBER) // 유효한 가중치만 처리
                {
                    // 뼈 인덱스 유효성 검사
                    if (BoneIndex >= SkinningMatrices.Num()) {
                         OutputDebugStringA(std::format("Error: Invalid bone index ({}) found in vertex {}.\n", BoneIndex, VertIndex).c_str());
                         continue;
                    }

                    const FMatrix& SkinMat = SkinningMatrices[BoneIndex];

                    // 위치 변형 및 가중 합
                    SkinnedPosition += SkinMat.TransformPosition(RefPosition) * Weight;

                    // 노멀 변형 및 가중 합 (3x3 부분 사용, 비균등 스케일 시 부정확할 수 있음)
                    // 더 정확하려면: SkinMat.InverseFast().GetTransposed().TransformVector(RefNormal)
                    SkinnedNormal += SkinMat.TransformVector(RefNormal) * Weight;

                    // 탄젠트 변형 (필요 시 노멀과 동일하게 처리)
                    // SkinnedTangent += SkinMat.TransformVector(RefTangent) * Weight;
                }
            }

            // --- 최종 변형된 데이터를 OutVertex에 저장 ---
            OutVertex.X = SkinnedPosition.X;
            OutVertex.Y = SkinnedPosition.Y;
            OutVertex.Z = SkinnedPosition.Z;

            FVector Normal = SkinnedNormal.GetSafeNormal();
            OutVertex.NormalX = Normal.X;
            OutVertex.NormalY = Normal.Y;
            OutVertex.NormalZ = Normal.Z;
            
            // OutVertex.Tangent = SkinnedTangent.GetSafeNormal(); // 필요 시

            // // UV, 정점 색상 등 스키닝되지 않는 데이터는 원본 복사
            // OutVertex.TexCoord = RefVertex.TexCoord;
            // OutVertex.Color = RefVertex.Color;
        }

        // --- CPU 스키닝 완료 ---

        // 스키닝된 컴포넌트를 렌더링 목록에 추가
        SkeletalMeshComponents.Add(Component);
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

        RenderSkeletalMesh(Comp);

        if (Viewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_AABB))
        {
            FEngineLoop::PrimitiveDrawBatch.AddAABBToBatch(Comp->GetBoundingBox(), Comp->GetWorldLocation(), WorldMatrix);
        }
    }
}

void FSkeletalMeshRenderPassBase::RenderSkeletalMesh(USkeletalMeshComponent* Component) const
{
    UINT Stride = sizeof(FSkeletalMeshVertex);
    UINT Offset = 0;
    const FSkeletalMeshRenderData* RenderData = Component->GetSkeletalMesh()->GetRenderData();

    FVertexInfo VertexInfo;
    BufferManager->CreateDynamicVertexBuffer(RenderData->ObjectName, Component->SkinnedVertices, VertexInfo);

    BufferManager->UpdateDynamicVertexBuffer(RenderData->ObjectName, Component->SkinnedVertices);
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &Stride, &Offset);

    FIndexInfo IndexInfo;
    BufferManager->CreateIndexBuffer(RenderData->ObjectName, RenderData->Indices, IndexInfo);
    if (IndexInfo.IndexBuffer)
    {
        Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        Graphics->DeviceContext->DrawIndexed(RenderData->Indices.Num(), 0, 0);
    }
    else
    {
        Graphics->DeviceContext->Draw(RenderData->Vertices.Num(), 0);
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
