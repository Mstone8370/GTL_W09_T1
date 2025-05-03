
#include "GizmoRenderPass.h"

#include <array>

#include "UObject/UObjectIterator.h"
#include "UObject/Casts.h"

#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"

#include "RendererHelpers.h"
#include "Math/JungleMath.h"

#include "World/World.h"

#include "Actors/Player.h"

#include "BaseGizmos/GizmoBaseComponent.h"
#include "BaseGizmos/TransformGizmo.h"

#include "UnrealEd/EditorViewportClient.h"

#include "EngineLoop.h"
#include "UnrealClient.h"

#include "UObject/ObjectTypes.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/EditorEngine.h"


FGizmoRenderPass::FGizmoRenderPass()
    : BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
{
}

FGizmoRenderPass::~FGizmoRenderPass()
{
}

void FGizmoRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;

    CreateBuffer();
    CreateShader();

    D3D11_SAMPLER_DESC SamplerDesc = {};
    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    SamplerDesc.MinLOD = 0;
    SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    
    Graphics->Device->CreateSamplerState(&SamplerDesc, &Sampler);
}

void FGizmoRenderPass::CreateShader()
{
    VertexShader = ShaderManager->GetVertexShaderByKey(L"StaticMeshVertexShader");
    InputLayout = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader");

    HRESULT hr = ShaderManager->AddPixelShader(L"GizmoPixelShader", L"Shaders/GizmoPixelShader.hlsl", "mainPS");
    if (FAILED(hr))
    {
        return;
    }
    PixelShader = ShaderManager->GetPixelShaderByKey(L"GizmoPixelShader");
}

void FGizmoRenderPass::UpdateShader()
{
    VertexShader = ShaderManager->GetVertexShaderByKey(L"StaticMeshVertexShader");
    InputLayout = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader");
    PixelShader = ShaderManager->GetPixelShaderByKey(L"GizmoPixelShader");
}

void FGizmoRenderPass::ReleaseShader()
{
}

void FGizmoRenderPass::CreateBuffer()
{
    BufferManager->CreateBufferGeneric<FViewportSize>("FViewportSize", nullptr, sizeof(FViewportSize), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

void FGizmoRenderPass::ClearRenderArr()
{
}

void FGizmoRenderPass::PrepareRenderState() const
{
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);

    BufferManager->BindConstantBuffer(TEXT("FMaterialConstants"), 1, EShaderStage::Pixel);

    BufferManager->BindConstantBuffer(TEXT("FViewportSize"), 2, EShaderStage::Pixel);
    
    Graphics->DeviceContext->PSSetSamplers(0, 1, &Sampler);
}

void FGizmoRenderPass::PrepareRenderArr()
{
}

void FGizmoRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(EResourceType::ERT_Editor);
    FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(EResourceType::ERT_Gizmo);
    
    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, DepthStencilRHI->DSV);

    // 씬 뎁스를 쉐이더 리소스로 사용
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_SceneDepth), 1, &ViewportResource->GetDepthStencil(EResourceType::ERT_Scene)->SRV);
    
    Graphics->DeviceContext->RSSetState(FEngineLoop::GraphicDevice.RasterizerSolidBack);

    FViewportSize ViewportSize;
    ViewportSize.ViewportSize.X = Viewport->GetViewport()->GetRect().Width;
    ViewportSize.ViewportSize.Y = Viewport->GetViewport()->GetRect().Height;
    BufferManager->UpdateConstantBuffer(TEXT("FViewportSize"), ViewportSize);
    
    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    if (!Engine)
    {
        UE_LOG(ELogLevel::Error, TEXT("Gizmo RenderPass : Render : Engine is not valid."));
        return;
    }
    
    EControlMode Mode = Engine->GetEditorPlayer()->GetControlMode();
    if (Mode == CM_TRANSLATION)
    {
        for (UStaticMeshComponent* StaticMeshComp : Viewport->GetGizmoActor()->GetArrowArr())
        {
            UGizmoBaseComponent* GizmoComp = Cast<UGizmoBaseComponent>(StaticMeshComp);
            RenderGizmoComponent(GizmoComp, Viewport);
        }
    }
    else if (Mode == CM_SCALE)
    {
        for (UStaticMeshComponent* StaticMeshComp : Viewport->GetGizmoActor()->GetScaleArr())
        {
            UGizmoBaseComponent* GizmoComp = Cast<UGizmoBaseComponent>(StaticMeshComp);
            RenderGizmoComponent(GizmoComp, Viewport);
        }
    }
    else if (Mode == CM_ROTATION)
    {
        for (UStaticMeshComponent* StaticMeshComp : Viewport->GetGizmoActor()->GetDiscArr())
        {
            UGizmoBaseComponent* GizmoComp = Cast<UGizmoBaseComponent>(StaticMeshComp);
            RenderGizmoComponent(GizmoComp, Viewport);
        }
    }
    
    Graphics->DeviceContext->RSSetState(Graphics->GetCurrentRasterizer()); // TODO: 이 래스터라이저 안씀.

    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(static_cast<UINT>(EShaderSRVSlot::SRV_SceneDepth), 1, NullSRV);
}

void FGizmoRenderPass::UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const
{
    FObjectConstantBuffer ObjectData = {};
    ObjectData.WorldMatrix = WorldMatrix;
    ObjectData.InverseTransposedWorld = FMatrix::Transpose(FMatrix::Inverse(WorldMatrix));
    ObjectData.UUIDColor = UUIDColor;
    ObjectData.bIsSelected = bIsSelected;
    
    BufferManager->UpdateConstantBuffer(TEXT("FObjectConstantBuffer"), ObjectData);
}

void FGizmoRenderPass::RenderGizmoComponent(UGizmoBaseComponent* GizmoComp, const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    if (!Engine)
    {
        return;
    }

    USceneComponent* SelectedComponent = Engine->GetSelectedComponent();
    AActor* SelectedActor = Engine->GetSelectedActor();
    if (SelectedComponent == nullptr && SelectedActor == nullptr)
    {
        return;
    }
    
    if (!GizmoComp->GetStaticMesh())
    {
        return;
    }
    
    FStaticMeshRenderData* RenderData = GizmoComp->GetStaticMesh()->GetRenderData();
    if (!RenderData)
    {
        return;
    }

    UpdateShader();

    PrepareRenderState();
    
    // 오브젝트 버퍼 업데이트
    FMatrix WorldMatrix = GizmoComp->GetWorldMatrix();
    FVector4 UUIDColor = GizmoComp->EncodeUUID() / 255.0f;
    bool bIsSelected = (GizmoComp == Viewport->GetPickedGizmoComponent());
    UpdateObjectConstant(WorldMatrix, UUIDColor, bIsSelected);

    UINT Stride = sizeof(FStaticMeshVertex);
    UINT Offset = 0;

    FVertexInfo VertexInfo;
    BufferManager->CreateVertexBuffer(RenderData->ObjectName, RenderData->Vertices, VertexInfo);

    FIndexInfo IndexInfo;
    BufferManager->CreateIndexBuffer(RenderData->ObjectName, RenderData->Indices, IndexInfo);
    
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &Stride, &Offset);

    if (!IndexInfo.IndexBuffer)
    {
        // TODO: 인덱스 버퍼가 없는 경우?
    }
    Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    
    if (RenderData->MaterialSubsets.Num() == 0)
    {
        Graphics->DeviceContext->DrawIndexed(RenderData->Indices.Num(), 0, 0);
    }
    else
    {
        // TODO: 현재 기즈모 메시는 머티리얼이 하나뿐이지만, 추후 여러 머티리얼을 사용하는 경우가 생길 수 있음.
        for (int SubMeshIndex = 0; SubMeshIndex < RenderData->MaterialSubsets.Num(); SubMeshIndex++)
        {
            int32 MaterialIndex = RenderData->MaterialSubsets[SubMeshIndex].MaterialIndex;

            FSubMeshConstants SubMeshData = FSubMeshConstants(false);
            BufferManager->UpdateConstantBuffer(TEXT("FSubMeshConstants"), SubMeshData);

            TArray<UMaterial*> OverrideMaterials = GizmoComp->GetOverrideMaterials();
            if (OverrideMaterials[MaterialIndex] != nullptr)
            {
                MaterialUtils::UpdateMaterial(BufferManager, Graphics, OverrideMaterials[MaterialIndex]->GetMaterialInfo());
            }
            else
            {
                TArray<FMeshMaterial*> Materials = GizmoComp->GetStaticMesh()->GetMaterials();
                MaterialUtils::UpdateMaterial(BufferManager, Graphics, Materials[MaterialIndex]->Material->GetMaterialInfo());
            }

            uint32 StartIndex = RenderData->MaterialSubsets[SubMeshIndex].IndexStart;
            uint32 IndexCount = RenderData->MaterialSubsets[SubMeshIndex].IndexCount;

            Graphics->DeviceContext->DrawIndexed(IndexCount, StartIndex, 0);
        }
    }
}
