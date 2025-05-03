#pragma once
#include "SkeletalMeshRenderPassBase.h"

class FSkeletalMeshRenderPass : public FSkeletalMeshRenderPassBase
{
protected:
    virtual void CreateResource();

    virtual void ReleaseResource();
    
    virtual void PrepareRenderPass(const std::shared_ptr<FEditorViewportClient>& Viewport);

    virtual void CleanUpRenderPass(const std::shared_ptr<FEditorViewportClient>& Viewport);

};
