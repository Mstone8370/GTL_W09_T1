#include "SkeletalMeshRenderPass.h"

#include "UnrealClient.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "UnrealEd/EditorViewportClient.h"

void FSkeletalMeshRenderPass::CreateResource()
{
    FSkeletalMeshRenderPassBase::CreateResource();

    HRESULT hr = ShaderManager->AddPixelShader(L"SkeletalMeshPixelShader", L"Shaders/SkeletalMeshPixelShader.hlsl", "mainPS");
    if (FAILED(hr))
    {
        return;
    }
}

void FSkeletalMeshRenderPass::ReleaseResource()
{
    FSkeletalMeshRenderPassBase::ReleaseResource();
}

void FSkeletalMeshRenderPass::PrepareRenderPass(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    ID3D11VertexShader* VertexShader = ShaderManager->GetVertexShaderByKey(L"SkeletalMeshVertexShader");
    ID3D11InputLayout* InputLayout = ShaderManager->GetInputLayoutByKey(L"SkeletalMeshVertexShader");
    ID3D11PixelShader* PixelShader = ShaderManager->GetPixelShaderByKey(L"SkeletalMeshPixelShader");
    
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);

    
    Graphics->DeviceContext->RSSetViewports(1, &Viewport->GetViewportResource()->GetD3DViewport());

    const EResourceType ResourceType = EResourceType::ERT_Scene;
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(ResourceType);
    FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(ResourceType);

    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, DepthStencilRHI->DSV);
}

void FSkeletalMeshRenderPass::CleanUpRenderPass(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}
