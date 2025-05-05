
#include "FbxLoader.h"

#include <format>

#include "FObjLoader.h"
#include "Asset/SkeletalMeshAsset.h"
#include "UObject/ObjectFactory.h"
#include "SkeletalMesh.h"

struct FVertexKey
{
    int32 PositionIndex;
    int32 NormalIndex;
    int32 TangentIndex;
    int32 UVIndex;
    int32 ColorIndex;

    FVertexKey(int32 Pos, int32 N, int32 T, int32 UV, int32 Col)
        : PositionIndex(Pos)
        , NormalIndex(N)
        , TangentIndex(T)
        , UVIndex(UV)
        , ColorIndex(Col)
    {
        Hash = std::hash<int32>()(PositionIndex << 0)
             ^ std::hash<int32>()(NormalIndex   << 1)
             ^ std::hash<int32>()(TangentIndex  << 2)
             ^ std::hash<int32>()(UVIndex       << 3)
             ^ std::hash<int32>()(ColorIndex    << 4);
    }

    bool operator==(const FVertexKey& Other) const
    {
        return PositionIndex == Other.PositionIndex
            && NormalIndex   == Other.NormalIndex
            && TangentIndex  == Other.TangentIndex
            && UVIndex       == Other.UVIndex
            && ColorIndex    == Other.ColorIndex;
    }

    SIZE_T GetHash() const { return Hash; }

private:
    SIZE_T Hash;
};

namespace std
{
    template<>
    struct hash<FVertexKey>
    {
        size_t operator()(const FVertexKey& Key) const
        {
            return Key.GetHash();
        }
    };
}

// 헬퍼 함수: FbxVector4를 FSkeletalMeshVertex의 XYZ로 변환 (좌표계 변환 포함)
inline void SetVertexPosition(FSkeletalMeshVertex& Vertex, const FbxVector4& Pos)
{
    // FbxAxisSystem::Max.ConvertScene(Scene) 사용 시 (Max Z-up -> Unreal Z-up 가정)
    // X -> X, Y -> -Y, Z -> Z
    Vertex.X = static_cast<float>(Pos[0]);
    Vertex.Y = static_cast<float>(Pos[1]); // Y축 반전
    Vertex.Z = static_cast<float>(Pos[2]);
}

// 헬퍼 함수: FbxVector4를 FSkeletalMeshVertex의 Normal XYZ로 변환 (좌표계 변환 포함)
inline void SetVertexNormal(FSkeletalMeshVertex& Vertex, const FbxVector4& Normal)
{
    // FbxAxisSystem::Max.ConvertScene(Scene) 사용 시
    Vertex.NormalX = static_cast<float>(Normal[0]);
    Vertex.NormalY = static_cast<float>(Normal[1]); // Y축 반전
    Vertex.NormalZ = static_cast<float>(Normal[2]);
}

// 헬퍼 함수: FbxVector4를 FSkeletalMeshVertex의 Tangent XYZW로 변환 (좌표계 변환 포함)
inline void SetVertexTangent(FSkeletalMeshVertex& Vertex, const FbxVector4& Tangent)
{
    // FbxAxisSystem::Max.ConvertScene(Scene) 사용 시
    Vertex.TangentX = static_cast<float>(Tangent[0]);
    Vertex.TangentY = static_cast<float>(Tangent[1]); // Y축 반전
    Vertex.TangentZ = static_cast<float>(Tangent[2]);
    Vertex.TangentW = static_cast<float>(Tangent[3]); // W (Handedness)
}

// 헬퍼 함수: FbxColor를 FSkeletalMeshVertex의 RGBA로 변환
inline void SetVertexColor(FSkeletalMeshVertex& Vertex, const FbxColor& Color)
{
    Vertex.R = static_cast<float>(Color.mRed);
    Vertex.G = static_cast<float>(Color.mGreen);
    Vertex.B = static_cast<float>(Color.mBlue);
    Vertex.A = static_cast<float>(Color.mAlpha);
}

// 헬퍼 함수: FbxVector2를 FSkeletalMeshVertex의 UV로 변환 (좌표계 변환 포함)
inline void SetVertexUV(FSkeletalMeshVertex& Vertex, const FbxVector2& UV)
{
    Vertex.U = static_cast<float>(UV[0]);
    Vertex.V = 1.0f - static_cast<float>(UV[1]); // V 좌표는 보통 뒤집힘 (DirectX 스타일)
}

// FbxAMatrix를 언리얼 엔진의 FMatrix로 변환하는 헬퍼 함수
// 참고: 이 함수는 좌표계 변환(예: Y-up에서 Z-up으로)이
// 이미 FBXConvertScene 또는 유사한 함수에 의해 처리되었다고 가정합니다.
// 그렇지 않다면, 여기서 추가적인 축 교환/부호 반전이 필요할 수 있습니다.
inline FMatrix ConvertMatrixUE(const FbxAMatrix& FbxMatrix)
{
    FMatrix UEMatrix;

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            // FBX 행렬은 Get(행, 열)을 사용한 메모리 접근 시 행 우선(row-major)입니다
            // 언리얼 FMatrix도 초기화 리스트/생성자에서 사실상 행 우선으로 처리됩니다
            UEMatrix.M[i][j] = static_cast<float>(FbxMatrix.Get(i, j));
        }
    }
    return UEMatrix;
}

// FbxLayerElementTemplate에서 데이터를 가져오는 일반화된 헬퍼 함수
template<typename FbxLayerElementType, typename TDataType>
bool GetVertexElementData(const FbxLayerElementType* Element, int32 ControlPointIndex, int32 VertexIndex, TDataType& OutData)
{
    if (!Element)
    {
        return false;
    }

    const auto MappingMode = Element->GetMappingMode();
    const auto ReferenceMode = Element->GetReferenceMode();

    // eAllSame: 모든 정점이 같은 값
    if (MappingMode == FbxLayerElement::eAllSame)
    {
        if (Element->GetDirectArray().GetCount() > 0)
        {
            OutData = Element->GetDirectArray().GetAt(0);
            return true;
        }
        return false;
    }

    // 2) 인덱스 결정 (eByControlPoint, eByPolygonVertex만 처리)
    int32 Index = -1;
    if (MappingMode == FbxLayerElement::eByControlPoint)
    {
        Index = ControlPointIndex;
    }
    else if (MappingMode == FbxLayerElement::eByPolygonVertex)
    {
        Index = VertexIndex;
    }
    else
    {
        // eByPolygon, eByEdge 등 필요시 추가
        return false;
    }

    // 3) ReferenceMode별 분리 처리
    if (ReferenceMode == FbxLayerElement::eDirect)
    {
        // DirectArray 크기만 검사
        if (Index >= 0 && Index < Element->GetDirectArray().GetCount())
        {
            OutData = Element->GetDirectArray().GetAt(Index);
            return true;
        }
    }
    else if (ReferenceMode == FbxLayerElement::eIndexToDirect)
    {
        // IndexArray, DirectArray 순차 검사
        if (Index >= 0 && Index < Element->GetIndexArray().GetCount())
        {
            int32 DirectIndex = Element->GetIndexArray().GetAt(Index);
            if (DirectIndex >= 0 && DirectIndex < Element->GetDirectArray().GetCount())
            {
                OutData = Element->GetDirectArray().GetAt(DirectIndex);
                return true;
            }
        }
    }

    return false;
}

FFbxLoader::FFbxLoader()
    : Manager(nullptr)
    , Importer(nullptr)
    , Scene(nullptr)
{
    Manager = FbxManager::Create();

    FbxIOSettings* IOSettings = FbxIOSettings::Create(Manager, IOSROOT);
    Manager->SetIOSettings(IOSettings);

    IOSettings->SetBoolProp(IMP_FBX_MATERIAL, true);
    IOSettings->SetBoolProp(IMP_FBX_TEXTURE, true);
    IOSettings->SetBoolProp(IMP_FBX_ANIMATION, true);
    
    Importer = FbxImporter::Create(Manager, "");
    Scene = FbxScene::Create(Manager, "");
}

FFbxLoader::~FFbxLoader()
{
    if (Scene)
    {
        Scene->Destroy();
    }
    if (Importer)
    {
        Importer->Destroy();
    }
    if (Manager)
    {
        Manager->Destroy();
    }
}

bool FFbxLoader::LoadFBX(const FString& InFilePath, FSkeletalMeshRenderData& OutRenderData)
{
    bool bRet = false;
    if (Importer->Initialize(*InFilePath, -1, Manager->GetIOSettings()))
    {
        bRet = Importer->Import(Scene);
    }
    if (!bRet)
    {
        return false;
    }
    
    FBXConvertScene();

    // Basic Setup
    OutRenderData.ObjectName = InFilePath.ToWideString();
    OutRenderData.DisplayName = ""; // TODO: temp

    const FbxGlobalSettings& GlobalSettings = Scene->GetGlobalSettings();
    FbxSystemUnit SystemUnit = GlobalSettings.GetSystemUnit();
    const double ScaleFactor = SystemUnit.GetScaleFactor();
    OutputDebugStringA(std::format("### FBX ###\nScene Scale: {} cm\n", ScaleFactor).c_str());

    //메쉬 하기전에 스켈레톤 진행
    ProcessSkeleton(OutRenderData);

    if (FbxNode* RootNode = Scene->GetRootNode())
    {
        FbxGeometryConverter Converter(Manager);
        Converter.Triangulate(Scene, true);
        
        TraverseNodeRecursive(RootNode, OutRenderData);
    }
    
    return true;
}

void FFbxLoader::TraverseNodeRecursive(FbxNode* Node, FSkeletalMeshRenderData& OutRenderData)
{
    if (!Node)
    {
        return;
    }

    FbxNodeAttribute* Attribute = Node->GetNodeAttribute();
    if (Attribute)
    {
        switch (Attribute->GetAttributeType())
        {
        case FbxNodeAttribute::eMesh:
            ProcessMesh(Node, OutRenderData);
            break;
        case FbxNodeAttribute::eSkeleton:
            break;
        default:
            break;
        }
    }

    for (int32 i = 0; i < Node->GetChildCount(); ++i)
    {
        TraverseNodeRecursive(Node->GetChild(i), OutRenderData);
    }
}

/**
 * 씬과 관련된 첫 번째 유효한 Bind Pose를 찾습니다.
 */
FbxPose* FFbxLoader::GetValidBindPose(FbxScene* pScene)
{
    const int PoseCount = pScene->GetPoseCount();
    for (int PoseIndex = 0; PoseIndex < PoseCount; PoseIndex++)
    {
        FbxPose* CurrentPose = pScene->GetPose(PoseIndex);
        if (CurrentPose && CurrentPose->IsBindPose())
        {
            // 바인드 포즈를 찾았습니다. 실제 시나리오에서는 IsValidBindPose 또는
            // IsValidBindPoseVerbose를 사용하여 메시 노드나 스켈레톤 루트에 대해
            // 추가적으로 유효성을 검증하는 것이 좋을 수 있습니다 (필요한 경우).
            return CurrentPose;
        }
    }
    OutputDebugStringA("경고: 씬에서 유효한 Bind Pose를 찾을 수 없습니다.\n");
    return nullptr; // 유효한 바인드 포즈를 찾지 못함
}


/**
 * 스켈레톤 계층 구조를 추출하고, 인덱스를 할당하며,
 * BoneNodeToIndexMap을 생성하고, 역 바인드 포즈 행렬을 계산하는 메인 함수.
 */
bool FFbxLoader::ProcessSkeleton(FSkeletalMeshRenderData& OutRenderData)
{
    // 이전 스켈레톤 데이터가 있다면 비움
    OutRenderData.SkeletonBones.Empty();
    OutRenderData.InverseBindPoseMatrices.Empty();
    OutRenderData.BoneNodeToIndexMap.Empty();

    // 1. Bind Pose 찾기
    // 참고: 더 견고한 접근 방식은 관련 메시 노드를 전달하여
    // 포즈를 검증하는 것일 수 있습니다.
    FbxPose* BindPose = GetValidBindPose(Scene);
    if (!BindPose)
    {
        // 찾지 못한 경우 누락된 바인드 포즈 생성 시도
        // (이 로직을 GetValidBindPose 내부로 옮기거나 여기서 처리하는 것을 고려)
         const int PoseCount = Scene->GetPoseCount();
         bool bFoundBindPose = false;
         for (int PoseIndex = PoseCount - 1; PoseIndex >= 0; --PoseIndex)
         {
             FbxPose* CurrentPose = Scene->GetPose(PoseIndex);
             if (CurrentPose && CurrentPose->IsBindPose())
             {
                 // 하나를 찾았지만 GetValidBindPose가 반환하지 않았다면,
                 // 아마 "유효"하다고 간주되지 않았을 수 있습니다. 기존 포즈를
                 // 제거하고 다시 생성해 볼 수 있습니다.
                 // 지금은, 전혀 없는 경우에만 생성을 시도합니다.
                 bFoundBindPose = true;
                 break;
             }
         }

         if (!bFoundBindPose) {
             OutputDebugStringA("누락된 바인드 포즈 생성을 시도합니다...\n");
             Manager->CreateMissingBindPoses(Scene);
             BindPose = GetValidBindPose(Scene); // 다시 찾아보기
         }

        if (!BindPose) {
             OutputDebugStringA("오류: 유효한 Bind Pose를 찾거나 생성할 수 없습니다. 스켈레톤 처리를 중단합니다.\n");
             return false; // 바인드 포즈 없이는 진행 불가
        }
    }


    // 2. 루트 노드부터 시작하여 씬 그래프를 재귀적으로 순회하면서
    //    본 계층 구조를 수집하고 행렬을 계산합니다.
    FbxNode* RootNode = Scene->GetRootNode();
    if (RootNode)
    {
        CollectBoneRecursive(RootNode, INDEX_NONE, BindPose, OutRenderData);
    }

    // 3. 최종 확인
    if (OutRenderData.SkeletonBones.IsEmpty())
    {
        OutputDebugStringA("경고: 스켈레톤 본을 찾거나 처리하지 못했습니다.\n");
        // 스태틱 메시인 경우 괜찮을 수 있지만, ProcessSkeleton은
        // 스켈레톤을 기대하고 호출되었을 가능성이 높습니다.
        return false; // 스켈레톤이 처리되지 않았음을 나타냄
    }

    // InverseBindPoseMatrices 배열 크기가 본 개수와 일치하는지 확인
    if (OutRenderData.SkeletonBones.Num() != OutRenderData.InverseBindPoseMatrices.Num())
    {
        OutputDebugStringA("오류: 본 개수와 역 바인드 포즈 행렬 개수가 일치하지 않습니다.\n");
        // CollectBoneRecursive 중 오류가 발생했음을 나타냄
        return false;
    }

    OutputDebugStringA(std::format("성공적으로 {}개의 본을 처리했습니다.\n", OutRenderData.SkeletonBones.Num()).c_str());
    return true;
}
/**
 * @brief 주어진 FbxMesh에서 스키닝 가중치 정보를 추출하여 SkinWeightMap을 채웁니다.
 * @param Mesh 스키닝 정보를 추출할 FbxMesh 객체 포인터.
 * @param BoneNodeToIndexMap ProcessSkeleton에서 생성된, FbxNode*를 BoneIndex(int32)로 매핑하는 맵.
 * @param OutSkinWeightMap 결과를 저장할 맵 (ControlPointIndex -> TArray<TPair<BoneIndex, Weight>>).
 * @return 성공적으로 처리했으면 true, 오류 발생 시 false (예: 유효하지 않은 입력).
 */
bool FFbxLoader::FillSkinWeightMap(
    FbxMesh* Mesh,
    const TMap<FbxNode*, int32>& BoneNodeToIndexMap,
    TMap<int32, TArray<TPair<int32, double>>>& OutSkinWeightMap)
{
    if (!Mesh)
    {
        OutputDebugStringA("Error: FillSkinWeightMap: Input FbxMesh is null.\n");
        return false;
    }
    
    OutSkinWeightMap.Empty();

    // 메시에 스킨 디포머가 있는지 확인 (스켈레탈 메시인지 판별)
    const int32 SkinDeformerCount = Mesh->GetDeformerCount(FbxDeformer::eSkin);
    if (SkinDeformerCount == 0)
    {
        // 스킨 디포머가 없으면 스켈레탈 메시가 아님. 가중치 정보 없음.
        // 오류는 아니므로 true 반환 (처리할 가중치가 없는 것뿐임)
        return true;
    }

    bool bEncounteredIssues = false; // 처리 중 잠재적 문제 발생 여부 플래그
    const char* MeshNameAnsi = Mesh->GetName() ? Mesh->GetName() : "Unnamed Mesh"; // Get ANSI name for formatting

    // 모든 스킨 디포머 순회 (보통 하나만 존재)
    for (int32 DeformerIdx = 0; DeformerIdx < SkinDeformerCount; ++DeformerIdx)
    {
        FbxSkin* Skin = static_cast<FbxSkin*>(Mesh->GetDeformer(DeformerIdx, FbxDeformer::eSkin));
        if (!Skin)
        {
            // UE_LOG(LogTemp, Warning, TEXT("FillSkinWeightMap: Found null FbxSkin at index %d for mesh '%s'."), DeformerIdx, *FString(UTF8_TO_TCHAR(Mesh->GetName())));
            OutputDebugStringA(std::format("Warning: FillSkinWeightMap: Found null FbxSkin at index {} for mesh '{}'.\n", DeformerIdx, MeshNameAnsi).c_str());
            continue; // 다음 디포머로 이동
        }

        // 스킨 내의 모든 클러스터 순회 (각 클러스터는 보통 하나의 뼈에 해당)
        const int32 ClusterCount = Skin->GetClusterCount();
        for (int32 ClusterIdx = 0; ClusterIdx < ClusterCount; ++ClusterIdx)
        {
            FbxCluster* Cluster = Skin->GetCluster(ClusterIdx);
            if (!Cluster)
            {
                OutputDebugStringA(std::format("Warning: FillSkinWeightMap: Found null FbxCluster at index {} for mesh '{}'.\n", ClusterIdx, MeshNameAnsi).c_str());
                continue; // 다음 클러스터로 이동
            }

            // 클러스터에 연결된 뼈 노드(FbxNode*) 가져오기
            FbxNode* LinkNode = Cluster->GetLink();
            if (!LinkNode)
            {
                // 클러스터가 뼈에 연결되지 않은 경우 (비정상적일 수 있음)
                // UE_LOG(LogTemp, Warning, TEXT("FillSkinWeightMap: Cluster %d for mesh '%s' has no linked bone node."), ClusterIdx, *FString(UTF8_TO_TCHAR(Mesh->GetName())));
                 OutputDebugStringA(std::format("Warning: FillSkinWeightMap: Cluster {} for mesh '{}' has no linked bone node.\n", ClusterIdx, MeshNameAnsi).c_str());
                continue; // 다음 클러스터로 이동
            }

            // --- 중요: BoneNodeToIndexMap을 사용하여 FbxNode*를 int32 BoneIndex로 변환 ---
            const int32* BoneIndexPtr = BoneNodeToIndexMap.Find(LinkNode);
            if (!BoneIndexPtr)
            {
                // 이 뼈 노드가 ProcessSkeleton에서 처리되지 않았거나, 매핑에 없음.
                // 스키닝 데이터는 있지만 해당 뼈를 추적하지 않는 경우 발생 가능.
                const char* BoneNameAnsi = LinkNode->GetName() ? LinkNode->GetName() : "Unnamed Bone";
                OutputDebugStringA(std::format("Warning: FillSkinWeightMap: Bone '{}' (linked to cluster {}) influencing mesh '{}' was not found in BoneNodeToIndexMap. Skipping its influence data.\n",
                    BoneNameAnsi, ClusterIdx, MeshNameAnsi).c_str());

                bEncounteredIssues = true; // 잠재적 문제 발생 기록
                continue; // 이 클러스터의 가중치 정보는 처리할 수 없으므로 다음 클러스터로 이동
            }
            int32 CurrentBoneIndex = *BoneIndexPtr; // 찾은 정수형 뼈 인덱스
            // --- 변환 완료 ---

            // 이 클러스터(뼈)에 영향을 받는 제어점(Control Point)들의 인덱스 배열 가져오기
            int32 const* ControlPointIndices = Cluster->GetControlPointIndices();
            // 해당 제어점들의 가중치(Weight) 배열 가져오기
            double const* ControlPointWeights = Cluster->GetControlPointWeights();
            // 영향을 받는 제어점의 총 개수
            int32 InfluenceCount = Cluster->GetControlPointIndicesCount();

            // 데이터 포인터 유효성 검사
            if (!ControlPointIndices || !ControlPointWeights) {
                // UE_LOG(LogTemp, Warning, TEXT("FillSkinWeightMap: Cluster %d for mesh '%s' has null indices or weights array."), ClusterIdx, *FString(UTF8_TO_TCHAR(Mesh->GetName())));
                OutputDebugStringA(std::format("Warning: FillSkinWeightMap: Cluster {} for mesh '{}' has null indices or weights array.\n", ClusterIdx, MeshNameAnsi).c_str());
                continue; // 다음 클러스터로 이동
            }

            // 이 클러스터에 의해 영향을 받는 모든 제어점 순회
            for (int32 InfluenceIdx = 0; InfluenceIdx < InfluenceCount; ++InfluenceIdx)
            {
                int32 CPIndex = ControlPointIndices[InfluenceIdx]; // 현재 제어점 인덱스
                double Weight = ControlPointWeights[InfluenceIdx]; // 현재 가중치

                // 매우 작은 가중치는 무시 (부동소수점 오류나 불필요한 영향 제거)
                const double MIN_WEIGHT_THRESHOLD = 1e-5; // 임계값 정의 (필요에 따라 조정)
                if (Weight > MIN_WEIGHT_THRESHOLD)
                {
                    // OutSkinWeightMap에서 해당 제어점 인덱스(CPIndex)에 대한 TArray를 찾거나 새로 생성
                    TArray<TPair<int32, double>>& InfluenceList = OutSkinWeightMap.FindOrAdd(CPIndex);

                    // 생성된 리스트에 (BoneIndex, Weight) 쌍 추가
                    InfluenceList.Emplace(CurrentBoneIndex, Weight);
                }
            } // End: 이 클러스터의 영향력 순회 종료
        } // End: 스킨 내 클러스터 순회 종료
    } // End: 스킨 디포머 순회 종료

    // 함수 실행 중 잠재적 문제가 있었는지 여부에 따라 true 또는 false 반환 가능
    // 여기서는 일단 성공(true)으로 간주하고, 로그를 통해 경고를 확인하도록 함
    // 만약 BoneNodeToIndexMap에서 뼈를 못 찾는 것이 치명적 오류라면 false 반환 고려
    return true; // 혹은 !bEncounteredIssues;
}

/**
 * 노드를 재귀적으로 순회하며, 본을 식별하고, 인덱스를 할당하며,
 * 계층 구조를 구축하고, 역 바인드 포즈 행렬을 계산하는 헬퍼 함수.
 */
void FFbxLoader::CollectBoneRecursive(
    FbxNode* CurrentNode,
    int32 ParentBoneIndex,
    const FbxPose* BindPose,
    FSkeletalMeshRenderData& OutRenderData)
{
    if (!CurrentNode)
    {
        return;
    }

    int32 CurrentBoneIndex = INDEX_NONE; // 이 노드가 본일 경우의 인덱스
    bool bIsActualBone = IsBone(CurrentNode); // 현재 노드가 본으로 간주되는지 확인

    if (bIsActualBone)
    {
        // 이 본 노드가 이미 처리되었는지 확인 (예: 복잡한 계층 구조)
        if (OutRenderData.BoneNodeToIndexMap.Contains(CurrentNode))
        {
            // 이미 처리됨, 아마도 다른 경로를 통해 여기에 도달했을 수 있음.
            // 이 경로를 통해 자식들을 다시 처리할 필요는 없음.
             return;
        }

        // --- 처리할 새로운 본 ---

        // 1. 인덱스 할당
        CurrentBoneIndex = OutRenderData.SkeletonBones.Num(); // 다음 사용 가능한 인덱스

        // 2. BoneNodeToIndexMap에 추가
        OutRenderData.BoneNodeToIndexMap.Add(CurrentNode, CurrentBoneIndex);

        // 3. FBoneInfo 생성 및 추가
        FBoneInfo NewBoneInfo;
        NewBoneInfo.Name = FString(CurrentNode->GetName()); // 이름 변환
        NewBoneInfo.Index = CurrentBoneIndex;
        NewBoneInfo.ParentIndex = ParentBoneIndex; // 부모의 *할당된* 인덱스에 연결
        // NewBoneInfo.FbxNodePtr = CurrentNode; // 선택 사항: 디버깅을 위해 저장
        OutRenderData.SkeletonBones.Add(NewBoneInfo);

        // 4. 역 바인드 포즈 행렬 계산
        FbxAMatrix BindPoseGlobalMatrix;
        int PoseNodeIndex = BindPose->Find(CurrentNode);

        if (PoseNodeIndex >= 0)
        {
            // FbxPose 객체에서 직접 행렬 가져오기
            // 참고: FbxPose는 FbxMatrix(비-아핀)를 저장하므로 주의 깊은 캐스팅/복사 필요
            FbxMatrix NonAffineMatrix = BindPose->GetMatrix(PoseNodeIndex);
            // FbxAMatrix로 요소 복사 (직접 캐스팅보다 안전)
            for(int r=0; r<4; ++r) for(int c=0; c<4; ++c) BindPoseGlobalMatrix[r][c]=NonAffineMatrix.Get(r,c);

        }
        else
        {
            // FbxPose 객체에서 본을 찾을 수 없음! 포즈가 유효하다면 발생해서는 안 됨.
            // 대체 방안: 클러스터에서 가져오기? 여기서 복잡함.
            // 대체 방안: EvaluateGlobalTransform 사용? 바인드 포즈가 아닐 수 있음.
            // 최선 조치: 오류 기록 및 항등 행렬 또는 마지막으로 알려진 양호한 행렬 사용.
            OutputDebugStringA(std::format("오류: 제공된 Bind Pose에서 본 '{}'을(를) 찾을 수 없습니다! 항등 역 바인드 포즈를 사용합니다.\n", CurrentNode->GetName()).c_str());
            BindPoseGlobalMatrix.SetIdentity(); // 대체 전역 행렬로 항등 행렬 사용

            // 대안 (덜 안전함): 가능하다면 클러스터에서 가져오기
            // FbxMesh/FbxSkin/FbxCluster에 접근해야 하므로 여기서 직접 사용 불가.
            // FbxCluster* Cluster = FindClusterForNode(CurrentNode); // 구현 필요
            // if (Cluster) Cluster->GetTransformLinkMatrix(BindPoseGlobalMatrix);
        }

        // 역행렬 계산 및 FMatrix로 변환
        FbxAMatrix InverseBindMatrix = BindPoseGlobalMatrix.Inverse();
        FMatrix UeInverseBindMatrix = ConvertMatrixUE(InverseBindMatrix);

        // 특정 인덱스에 추가하기 전에 배열이 충분히 큰지 확인
        // (CurrentBoneIndex = Num()이므로 항상 참이어야 함)
        if (OutRenderData.InverseBindPoseMatrices.Num() == CurrentBoneIndex)
        {
             OutRenderData.InverseBindPoseMatrices.Add(UeInverseBindMatrix);
        }
        else
        {
            // 이 경우는 인덱스 할당 로직 오류를 나타냄
             OutputDebugStringA(std::format("오류: 본 '{}'에 대한 역 바인드 포즈 행렬 추가 시 인덱스 불일치.\n", CurrentNode->GetName()).c_str());
             // 오류 처리: 크기 조정/신중하게 삽입 또는 assert
             OutRenderData.InverseBindPoseMatrices[CurrentBoneIndex] = (UeInverseBindMatrix); // 덜 효율적
        }


        // *이* 본의 자식들을 위한 ParentBoneIndex 업데이트
        ParentBoneIndex = CurrentBoneIndex;
    }
    // 그 외: CurrentNode는 본이 아님. 여전히 자식들을 순회하지만,
    // 자식들은 이 노드 *위에서* 온 ParentBoneIndex를 상속받음.

    // 5. 자식들을 재귀적으로 처리
    for (int i = 0; i < CurrentNode->GetChildCount(); ++i)
    {
        CollectBoneRecursive(CurrentNode->GetChild(i), ParentBoneIndex, BindPose, OutRenderData);
    }
}

void FFbxLoader::ProcessMesh(FbxNode* Node, FSkeletalMeshRenderData& OutRenderData)
{
    FbxMesh* Mesh = Node->GetMesh();
    if (!Mesh)
    {
        return;
    }

    // 이미 데이터가 있다면 병합하지 않고 반환 (여러 메쉬 노드를 어떻게 처리할지 정책 필요)
    // 여기서는 첫 번째 찾은 메쉬만 사용한다고 가정
    if (!OutRenderData.Vertices.IsEmpty())
    {
        OutputDebugStringA(std::format("Skipping additional mesh node: {}. Already processed one.\n", Node->GetName()).c_str());
        return;
    }

    const FbxAMatrix LocalTransformMatrix = Node->EvaluateLocalTransform();

    // 정점 데이터 추출 및 병합
    const int32 PolygonCount = Mesh->GetPolygonCount(); // 삼각형 개수 (Triangulate 후)
    const FbxVector4* ControlPoints = Mesh->GetControlPoints(); // 제어점 (정점 위치) 배열
    const int32 ControlPointsCount = Mesh->GetControlPointsCount();

    // 정점 병합을 위한 맵
    TMap<FVertexKey, uint32> UniqueVertices;
    OutRenderData.Vertices.Reserve(ControlPointsCount); // 대략적인 크기 예약 (정확하지 않음)
    OutRenderData.Indices.Reserve(PolygonCount * 3);

    // 레이어 요소 가져오기 (UV, Normal, Tangent, Color 등은 레이어에 저장됨)
    // 보통 Layer 0을 사용
    FbxLayer* BaseLayer = Mesh->GetLayer(0);
    if (!BaseLayer)
    {
        OutputDebugStringA("Error: Mesh has no Layer 0.\n");
        return;
    }

    const FbxLayerElementNormal* NormalElement = BaseLayer->GetNormals();
    const FbxLayerElementTangent* TangentElement = BaseLayer->GetTangents();
    const FbxLayerElementUV* UVElement = BaseLayer->GetUVs();
    const FbxLayerElementVertexColor* ColorElement = BaseLayer->GetVertexColors();

    // 컨트롤 포인트별 본·스킨 가중치 맵
    TMap<int32, TArray<TPair<int32, double>>> SkinWeightMap;
    if (!FillSkinWeightMap(Mesh, OutRenderData.BoneNodeToIndexMap, SkinWeightMap))
    {
        const char* NodeNameAnsi = Node->GetName() ? Node->GetName() : "Unnamed Node";
        OutputDebugStringA(std::format("Error: Failed to fill skin weight map for mesh '{}'.\n", NodeNameAnsi).c_str());
        // return; // 또는 다른 오류 처리
    }
    /*
    for (int32 DeformerIdx = 0; DeformerIdx < Mesh->GetDeformerCount(FbxDeformer::eSkin); ++DeformerIdx)
    {
        FbxSkin* Skin = static_cast<FbxSkin*>(Mesh->GetDeformer(DeformerIdx, FbxDeformer::eSkin));
        for (int32 ClusterIdx = 0; ClusterIdx < Skin->GetClusterCount(); ++ClusterIdx)
        {
            FbxCluster* Cluster = Skin->GetCluster(ClusterIdx);
            int32 BoneIndex = 본 인덱스 테이블에서 Cl->GetLink() 찾기;
            auto const* CPoints = Cluster->GetControlPointIndices();
            auto const* Weights = Cluster->GetControlPointWeights();
            for (int ControlPointIdx = 0; ControlPointIdx < Cluster->GetControlPointIndicesCount(); ++ControlPointIdx)
            {
                SkinWeightMap[CPoints[ControlPointIdx]].Emplace(BoneIndex, Weights[ControlPointIdx]);
            }
        }
    }
*/

    int VertexCounter = 0; // 폴리곤 정점 인덱스 (eByPolygonVertex 모드용)

    // 폴리곤(삼각형) 순회
    for (int32 i = 0; i < PolygonCount; ++i)
    {
        // 각 폴리곤(삼각형)의 정점 3개 순회
        for (int32 j = 0; j < 3; ++j)
        {
            const int32 ControlPointIndex = Mesh->GetPolygonVertex(i, j);

            FbxVector4 Position = ControlPoints[ControlPointIndex];
            FbxVector4 Normal;
            FbxVector4 Tangent;
            FbxVector2 UV;
            FbxColor Color;
            
            int NormalIndex = (NormalElement) ? (NormalElement->GetMappingMode() == FbxLayerElement::eByControlPoint ? ControlPointIndex : VertexCounter) : -1;
            int TangentIndex = (TangentElement) ? (TangentElement->GetMappingMode() == FbxLayerElement::eByControlPoint ? ControlPointIndex : VertexCounter) : -1;
            int UVIndex = (UVElement) ? (UVElement->GetMappingMode() == FbxLayerElement::eByPolygonVertex ? Mesh->GetTextureUVIndex(i, j) : ControlPointIndex) : -1;
            int ColorIndex = (ColorElement) ? (ColorElement->GetMappingMode() == FbxLayerElement::eByControlPoint ? ControlPointIndex : VertexCounter) : -1;
            
            // 정점 병합 키 생성
            FVertexKey Key(ControlPointIndex, NormalIndex, TangentIndex, UVIndex, ColorIndex);

            // 맵에서 키 검색
            if (const uint32* Found = UniqueVertices.Find(Key))
            {
                OutRenderData.Indices.Add(*Found);
            }
            else
            {
                FSkeletalMeshVertex NewVertex;

                // Position
                if (ControlPointIndex < ControlPointsCount)
                {
                    Position = LocalTransformMatrix.MultT(Position);
                    SetVertexPosition(NewVertex, Position);
                }

                // Normal
                if (NormalElement && GetVertexElementData(NormalElement, ControlPointIndex, VertexCounter, Normal))
                {
                    Normal = LocalTransformMatrix.Inverse().Transpose().MultT(Normal);
                    SetVertexNormal(NewVertex, Normal);
                }

                // Tangent
                if (TangentElement && GetVertexElementData(TangentElement, ControlPointIndex, VertexCounter, Tangent))
                {
                     SetVertexTangent(NewVertex, Tangent);
                }

                // UV
                if(UVElement && GetVertexElementData(UVElement, ControlPointIndex, VertexCounter, UV))
                {
                    SetVertexUV(NewVertex, UV);
                }

                // Vertex Color
                if (ColorElement && GetVertexElementData(ColorElement, ControlPointIndex, VertexCounter, Color))
                {
                     SetVertexColor(NewVertex, Color);
                }

                // 본 데이터 설정
                auto& InfluenceList = SkinWeightMap[ControlPointIndex];
                std::sort(InfluenceList.begin(), InfluenceList.end(),
                    [](auto const& A, auto const& B)
                    {
                        return A.Value > B.Value; // Weight 기준 내림차순 정렬
                    }
                );
                
                double TotalWeight = 0.0;
                for (int32 BoneIdx = 0; BoneIdx < 4 && BoneIdx < InfluenceList.Num(); ++BoneIdx)
                {
                    NewVertex.BoneIndices[BoneIdx] = InfluenceList[BoneIdx].Key;
                    NewVertex.BoneWeights[BoneIdx] = static_cast<float>(InfluenceList[BoneIdx].Value);
                    TotalWeight += InfluenceList[BoneIdx].Value;
                }
                if (TotalWeight > 0.0)
                {
                    for (int BoneIdx = 0; BoneIdx < 4; ++BoneIdx)
                    {
                        NewVertex.BoneWeights[BoneIdx] /= static_cast<float>(TotalWeight);
                    }
                }

                // 새로운 정점을 Vertices 배열에 추가
                OutRenderData.Vertices.Add(NewVertex);
                // 새 정점의 인덱스 계산
                uint32 NewIndex = static_cast<uint32>(OutRenderData.Vertices.Num() - 1);
                // 인덱스 버퍼에 새 인덱스 추가
                OutRenderData.Indices.Add(NewIndex);
                // 맵에 새 정점 정보 추가
                UniqueVertices.Add(Key, NewIndex);
            }

            VertexCounter++; // 다음 폴리곤 정점으로 이동
        } // End for each vertex in polygon
    } // End for each polygon


    SetFbxMatreiral(Node, OutRenderData);
}


void FFbxLoader::SetFbxMatreiral(FbxNode* _Node, FSkeletalMeshRenderData& OutRenderData)
{
    int MtrlCount = _Node->GetMaterialCount();

    if (MtrlCount > 0)
    {
        TArray<FObjMaterialInfo> Materials =  OutRenderData.Materials;

        for (int MtrlIndex = 0; MtrlIndex < MtrlCount; MtrlIndex++)
        {
            FbxSurfaceMaterial* pMtrl = _Node->GetMaterial(MtrlIndex);

            FObjMaterialInfo MatrialInfo;

            MatrialInfo.MaterialName = pMtrl->GetName();
            //MatrialInfo.DifTexturePath = pMtrl->FindProperty(FbxSurfaceMaterial::sDiffuse)->GetSrcObject<FbxFileTexture>();

            MatrialInfo.DiffuseColor = GetMaterialColor(pMtrl, FbxSurfaceMaterial::sDiffuse, FbxSurfaceMaterial::sDiffuseFactor).ToVector();
            MatrialInfo.AmbientColor = GetMaterialColor(pMtrl, FbxSurfaceMaterial::sAmbient, FbxSurfaceMaterial::sAmbientFactor).ToVector();
            MatrialInfo.SpecularColor = GetMaterialColor(pMtrl, FbxSurfaceMaterial::sSpecular, FbxSurfaceMaterial::sSpecularFactor).ToVector();
            MatrialInfo.EmissiveColor = GetMaterialColor(pMtrl, FbxSurfaceMaterial::sEmissive, FbxSurfaceMaterial::sEmissiveFactor).ToVector();
            
            SetMatrialTexture(pMtrl, FbxSurfaceMaterial::sDiffuse, EMaterialTextureSlots::MTS_Diffuse, MatrialInfo);
            SetMatrialTexture(pMtrl, FbxSurfaceMaterial::sNormalMap, EMaterialTextureSlots::MTS_Normal, MatrialInfo);
            SetMatrialTexture(pMtrl, FbxSurfaceMaterial::sSpecular, EMaterialTextureSlots::MTS_Specular, MatrialInfo);

            //FWString SpecularTexturePath = GetMaterialTexturePath(pMtrl, FbxSurfaceMaterial::sSpecular);
            //FWString NormalTexturePath = GetMaterialTexturePath(pMtrl, FbxSurfaceMaterial::sNormalMap);
            //FWString BumpTexturePath = GetMaterialTexturePath(pMtrl, FbxSurfaceMaterial::sBump);
            
        }
    }
}

bool FFbxLoader::FBXConvertScene()
{
    if (!Scene || !Manager)
    {
        OutputDebugStringA("Error: Scene or Manager is not initialized.\n");
        return false;
    }

    // 엔진의 목표 좌표계 정의 (Unreal Engine 기준)
    //    - Up: Z 축 (eZAxis)
    //    - Forward Axis Determination: Requires Parity based on Up & Handedness to achieve X-Forward.
    //    - Handedness: 왼손 좌표계 (eLeftHanded)
    FbxAxisSystem::EUpVector UpVector = (FbxAxisSystem::EUpVector)-FbxAxisSystem::eZAxis;
    FbxAxisSystem::EFrontVector FrontVector = (FbxAxisSystem::EFrontVector)-FbxAxisSystem::eParityEven; 
    FbxAxisSystem::ECoordSystem CoordSystem = FbxAxisSystem::eLeftHanded;
    FbxAxisSystem EngineAxisSystem(UpVector, FrontVector, CoordSystem);

    // FBX 파일의 원본 좌표계 가져오기
    FbxAxisSystem SourceAxisSystem = Scene->GetGlobalSettings().GetAxisSystem();

    FbxAMatrix& matrix = Scene->GetRootNode()->EvaluateGlobalTransform();

    //좌표계가 다른 경우에만 변환 수행
    if (SourceAxisSystem != EngineAxisSystem)
    {
        OutputDebugStringA("Info: Source coordinate system differs from engine. Converting scene...\n");

        //FbxRootNodeUtility::RemoveAllFbxRoots(Scene);

        FbxAxisSystem AxisSystem = Scene->GetGlobalSettings().GetAxisSystem();

        // FBX SDK를 사용하여 씬 전체의 좌표계를 변환합니다.
        // 이 함수는 노드 변환, 애니메이션 커브 등을 재귀적으로 수정합니다.
        EngineAxisSystem.ConvertScene(Scene);
        //EngineAxisSystem.DeepConvertScene(Scene);
  
    }
    else
    {
        OutputDebugStringA("Info: Source coordinate system already matches engine. No conversion needed.\n");
    }

    FbxAMatrix& aftermatrix = Scene->GetRootNode()->EvaluateGlobalTransform();


    // 애니메이션 평가기 리셋 (좌표계 변환 후 필요)
    // 변환으로 인해 노드/커브 데이터가 변경되었을 수 있으므로 평가기를 리셋합니다.
    Scene->GetAnimationEvaluator()->Reset();

    
    return true; 
}

FLinearColor FFbxLoader::GetMaterialColor(FbxSurfaceMaterial* Mtrl, const char* ColorName, const char* FactorName)
{
    FbxDouble3 vResult(0, 0, 0);
    double dFactor = 0;
    FbxProperty ColorPro = Mtrl->FindProperty(ColorName);
    FbxProperty FactorPro = Mtrl->FindProperty(FactorName);

    if (true == ColorPro.IsValid() && true == FactorPro.IsValid())
    {
        vResult = ColorPro.Get<FbxDouble3>();
        dFactor = FactorPro.Get<FbxDouble>();

        if (dFactor != 1)
        {
            vResult[0] *= dFactor;
            vResult[1] *= dFactor;
            vResult[2] *= dFactor;
        }
    }

    return FLinearColor((float)vResult[0], (float)vResult[1], (float)vResult[2], 1.0f);
}

float FFbxLoader::GetMaterialFactor(FbxSurfaceMaterial* Mtrl, const char* FactorName)
{
    double dFactor = 0;
    FbxProperty FactorPro = Mtrl->FindProperty(FactorName);

    if (true == FactorPro.IsValid())
    {
        dFactor = FactorPro.Get<FbxDouble>();
    }

    return (float)dFactor;
}

FWString FFbxLoader::GetMaterialTexturePath(FbxSurfaceMaterial* Mtrl, const char* TextureName)
{
    FbxProperty TexturePro = Mtrl->FindProperty(TextureName);

    FWString Str;
    if (true == TexturePro.IsValid())
    {
        FbxObject* pFileTex = TexturePro.GetFbxObject();

        int TexCount = TexturePro.GetSrcObjectCount<FbxFileTexture>();

        if (TexCount > 0)
        {
            FbxFileTexture* FileTex = TexturePro.GetSrcObject<FbxFileTexture>(0);

            if (nullptr != FileTex)
            {
                const char* src = FileTex->GetFileName();
                Str = std::wstring(src, src + std::strlen(src));
            }
        }
        else
        {
            return L"";
        }
    }
    else
    {
        return L"";
    }

    return Str;
}


void FFbxLoader::SetMatrialTexture(FbxSurfaceMaterial* Mtrl, const char* InTexturePath, EMaterialTextureSlots InSlotIdx,
    FObjMaterialInfo& OutFObjMaterialInfo)
{
    FWString TexturePath = GetMaterialTexturePath(Mtrl, InTexturePath);
    
    const uint32 SlotIdx = static_cast<uint32>(InSlotIdx);

    //TODO: 실제로 텍스쳐 있는 fbx로 테스트 해봐야함 
    //OutFObjMaterialInfo.TextureInfos[SlotIdx].TextureName = Line;

    //FWString InTexturePath = OutObjInfo.FilePath + OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].InTexturePath.ToWideString();
    if (FObjLoader::CreateTextureFromFile(TexturePath))
    {
        OutFObjMaterialInfo.TextureInfos[SlotIdx].TexturePath = TexturePath;
        OutFObjMaterialInfo.TextureInfos[SlotIdx].bIsSRGB = true;
        OutFObjMaterialInfo.TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Diffuse);
    }
}

std::unique_ptr<FSkeletalMeshRenderData> FFbxManager::LoadFbxSkeletalMeshAsset(const FWString& FilePath)
{
    std::unique_ptr<FSkeletalMeshRenderData> Data = std::make_unique<FSkeletalMeshRenderData>();

    // TODO: 여기에서 바이너리 파일 검색해서 찾으면 읽고 Data 채워서 리턴.

    FFbxLoader Loader;
    if (Loader.LoadFBX(FilePath, *Data))
    {
        return Data;
    }
    return nullptr;
}

USkeletalMesh* FFbxManager::CreateMesh(const FWString& FilePath)
{
    std::unique_ptr<FSkeletalMeshRenderData> SkeletalMeshRenderData = LoadFbxSkeletalMeshAsset(FilePath);

    if (SkeletalMeshRenderData == nullptr)
    {
        return nullptr;
    }

    USkeletalMesh* Mesh = FObjectFactory::ConstructObject<USkeletalMesh>(nullptr);
    if (Mesh)
    {
        Mesh->SetData(std::move(SkeletalMeshRenderData));
        SkeletalMeshMap.Add(FilePath, Mesh);
    }
    
    return Mesh;
}

USkeletalMesh* FFbxManager::GetSkeletalMesh(const FWString& FilePath)
{
    if (SkeletalMeshMap.Find(FilePath))
    {
        return SkeletalMeshMap[FilePath];
    }
    return CreateMesh(FilePath);
}
