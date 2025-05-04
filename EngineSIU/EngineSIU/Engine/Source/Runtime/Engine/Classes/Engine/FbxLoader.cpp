
#include "FbxLoader.h"

#include <format>

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

    // Read FBX
    /*
    int32 UpSign = 1;
    FbxAxisSystem::EUpVector UpVector = FbxAxisSystem::eZAxis;

    int32 FrontSign = 1;
    FbxAxisSystem::EFrontVector FrontVector = FbxAxisSystem::eParityEven;

    FbxAxisSystem::ECoordSystem CoordSystem = FbxAxisSystem::eLeftHanded;

    FbxAxisSystem DesiredAxisSystem(UpVector, FrontVector, CoordSystem);
    
    // DesiredAxisSystem.ConvertScene(Scene); // 언리얼 엔진 방식 좌표축
    //FbxAxisSystem::Max.ConvertScene(Scene); // 언리얼 엔진 방식 좌표축
    
    */

    const FbxGlobalSettings& GlobalSettings = Scene->GetGlobalSettings();
    FbxSystemUnit SystemUnit = GlobalSettings.GetSystemUnit();
    const double ScaleFactor = SystemUnit.GetScaleFactor();
    OutputDebugStringA(std::format("### FBX ###\nScene Scale: {} cm\n", ScaleFactor).c_str());

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
