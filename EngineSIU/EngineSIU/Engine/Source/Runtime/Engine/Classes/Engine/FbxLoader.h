#pragma once

#include <fbxsdk.h>

#include "Define.h"
#include "Container/Map.h"
#include "HAL/PlatformType.h"
#include "Math/Color.h"


class USkeletalMesh;
struct FSkeletalMeshRenderData;



// class FFbxMaterialSettingData
// {
//     std::string Name;
//     // 픽셀쉐이더에서 그냥 최종색상에 곱해주면 되는 색상.
//     FLinearColor			 DifColor;
//     FLinearColor			 AmbColor; // 빛
//     FLinearColor			 SpcColor; // 빛
//     FLinearColor			 EmvColor; // 빛
//     float			 SpecularPower = 0.f;		// 빛의 강도
//     float			 TransparencyFactor = 0.f;	// 빛의 강도
//     float			 Shininess = 0.f;			// 빛의 강도
//     std::string DifTexturePath;	// 텍스처경로 
//     std::string NorTexturePath; // 텍스처경로
//     std::string SpcTexturePath; // 텍스처경로
//
//     std::string DifTextureName;	// 텍스처경로 
//     std::string NorTextureName; // 텍스처경로
//     std::string SpcTextureName; // 텍스처경로
// };

class FFbxLoader
{
public:
    FFbxLoader();
    ~FFbxLoader();

    bool LoadFBX(const FString& InFilePath, FSkeletalMeshRenderData& OutRenderData);

private:
    FbxManager* Manager;
    FbxImporter* Importer;
    FbxScene* Scene;

    // 노드 계층 구조를 순회하며 본을 수집하는 재귀 헬퍼 함수
    void CollectBoneRecursive(
        FbxNode* CurrentNode,
        int32 ParentBoneIndex, // OutRenderData.SkeletonBones에 있는 부모 본의 인덱스(INDEX)를 전달
        const FbxPose* BindPose, // 가져온 바인드 포즈 객체를 전달
        FSkeletalMeshRenderData& OutRenderData // 채워 넣을 메인 데이터 구조체를 전달
    );
    // 씬에서 유효한 바인드 포즈를 찾는 헬퍼
    FbxPose* GetValidBindPose(FbxScene* Scene); // RetrievePoseFromBindPose에서처럼 NodeArray가 필요할 수 있음

    bool IsBone(FbxNode* Node)
    {
        return Node && Node->GetNodeAttribute() && Node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton;
    }
    bool ProcessSkeleton(FSkeletalMeshRenderData& OutRenderData);

    /**
    * @brief 주어진 FbxMesh에서 스키닝 가중치 정보를 추출하여 SkinWeightMap을 채웁니다.
    * @param Mesh 스키닝 정보를 추출할 FbxMesh 객체 포인터.
    * @param BoneNodeToIndexMap ProcessSkeleton에서 생성된, FbxNode*를 BoneIndex(int32)로 매핑하는 맵.
    * @param OutSkinWeightMap 결과를 저장할 맵 (ControlPointIndex -> TArray<TPair<BoneIndex, Weight>>).
    * @return 성공적으로 처리했으면 true, 오류 발생 시 false (예: 유효하지 않은 입력).
    */
    bool FillSkinWeightMap(
        FbxMesh* Mesh,
        const TMap<FbxNode*, int32>& BoneNodeToIndexMap,
        TMap<int32, TArray<TPair<int32, double>>>& OutSkinWeightMap
    );

    void TraverseNodeRecursive(FbxNode* Node, FSkeletalMeshRenderData& OutRenderData);
    void ProcessMesh(FbxNode* Node, FSkeletalMeshRenderData& OutRenderData);

    void ProcessMatreiral(FbxNode* Node, FSkeletalMeshRenderData& OutRenderData);
    bool FBXConvertScene();
    

    FLinearColor GetMaterialColor(FbxSurfaceMaterial* Mtrl, const char* ColorName, const char* FactorName);
    float GetMaterialFactor(FbxSurfaceMaterial* Mtrl, const char* FactorName);
    FWString GetMaterialTexturePath(FbxSurfaceMaterial* Mtrl, const char* TextureName);

    void SetMaterialTexture(FbxSurfaceMaterial* Mtrl, const char* InTexturePath, EMaterialTextureSlots SlotIdx, FObjMaterialInfo& OutFObjMaterialInfo);

    
};

class FFbxManager
{
public:
    static USkeletalMesh* GetSkeletalMesh(const FWString& FilePath);

protected:
    static std::unique_ptr<FSkeletalMeshRenderData> LoadFbxSkeletalMeshAsset(const FWString& FilePath);
    static USkeletalMesh* CreateMesh(const FWString& FilePath);

    inline static TMap<FString, USkeletalMesh*> SkeletalMeshMap;
};
