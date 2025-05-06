
#include "SkeletalMeshComponent.h"

#include "Engine/SkeletalMesh.h"
#include "Engine/Asset/SkeletalMeshAsset.h"

bool USkeletalMeshComponent::GetCurrentLocalBoneTransforms(TArray<FTransform>& OutLocalTransforms) const
{
    {
        //테스트 코드
        // TODO: 실제 애니메이션 시스템과 연동하여 구현 필요!
        // 예시: 현재 재생 중인 애니메이션 클립, 시간 등을 기반으로 계산
        //       OutLocalTransforms 배열의 크기를 스켈레톤 본 개수에 맞게 조절하고 채워야 함.
        //       성공 시 true, 실패(애니메이션 없음 등) 시 false 반환
        if (SkeletalMesh) {
            // 임시: T포즈만 구현
            const struct FSkeletalMeshRenderData* RenderData = SkeletalMesh->GetRenderData();
            if(RenderData) {
                OutLocalTransforms.SetNum(RenderData->LocalBindPoseTransforms.Num());
                for (int32 i = 0; i < RenderData->LocalBindPoseTransforms.Num(); ++i) {
                    OutLocalTransforms[i] = RenderData->LocalBindPoseTransforms[i];
                }
                return true;

            }
        }
        return false;
    }
}
