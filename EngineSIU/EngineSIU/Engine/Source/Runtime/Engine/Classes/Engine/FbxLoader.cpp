#include "FbxLoader.h"

#include "UObject/ObjectFactory.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"

#include "Asset/SkeletalMeshAsset.h"
#include "Asset/StaticMeshAsset.h"
#include "AssetManager.h"


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

bool FFbxLoader::LoadFBX(const FString& InFilePath)
{
    bool bRet = false;
    if (Importer->Initialize(*InFilePath, -1, Manager->GetIOSettings()))
    {
        bRet = Importer->Import(Scene);
    }

    return bRet;
}

bool FFbxLoader::ParseFBX(const FString& FbxFilePath, FFbxInfo& OutFbxInfo)
{
    OutFbxInfo.FilePath = FbxFilePath.ToWideString().substr(0, FbxFilePath.ToWideString().find_last_of(L"\\/") + 1);
    OutFbxInfo.ObjectName = FbxFilePath.ToWideString();
    // ObjectName은 wstring 타입이므로, 이를 string으로 변환 (간단한 ASCII 변환의 경우)
    std::wstring wideName = OutFbxInfo.ObjectName.substr(FbxFilePath.ToWideString().find_last_of(L"\\/") + 1);;
    std::string fileName(wideName.begin(), wideName.end());

    // 마지막 '.'을 찾아 확장자를 제거
    size_t dotPos = fileName.find_last_of('.');
    if (dotPos != std::string::npos)
    {
        OutFbxInfo.DisplayName = fileName.substr(0, dotPos);
    }
    else
    {
        OutFbxInfo.DisplayName = fileName;
    }

    /**
     * 블렌더 Export 설정
     *   > General
     *       Forward Axis:  Y
     *       Up Axis:       Z
     *   > Geometry
     *       ✅ Triangulated Mesh
     *   > Materials
     *       ✅ PBR Extensions
     *       Path Mode:     Strip
     */

    FbxNode* RootNode = Scene->GetRootNode();
    if (!RootNode) return false;

    ParseFbxNodeRecursive(RootNode, OutFbxInfo);

    return true;
}

void FFbxLoader::ParseFbxNodeRecursive(FbxNode* Node, FFbxInfo& OutFbxInfo)
{
    // 메시 파싱
    if (Node->GetMesh())
        ParseMesh(Node, OutFbxInfo);

    // 본 파싱
    if (Node->GetNodeAttribute() && Node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
        ParseBone(Node, OutFbxInfo);

    // 머티리얼 파싱
    ParseMaterial(Node, OutFbxInfo);

    // 애니메이션 파싱 (필요시)
    // ParseAnimation(Node, OutFbxInfo);

    // 자식 노드 재귀
    for (int i = 0; i < Node->GetChildCount(); ++i)
        ParseFbxNodeRecursive(Node->GetChild(i), OutFbxInfo);
}

void FFbxLoader::ParseMesh(FbxNode* Node, FFbxInfo& OutFbxInfo)
{
    FbxMesh* Mesh = Node->GetMesh();
    if (!Mesh) return;

    FFbxMesh OutMesh;
    OutMesh.Name = Node->GetName();

    // 정점 (Position)
    int VertexCount = Mesh->GetControlPointsCount();
    for (int i = 0; i < VertexCount; ++i)
    {
        FbxVector4 p = Mesh->GetControlPointAt(i);
        FFbxVertex v;
        v.Position = FVector((float)p[0], (float)p[1], (float)p[2]);
        OutMesh.Vertices.Add(v);
    }

    // 인덱스 (페이스)
    int PolyCount = Mesh->GetPolygonCount();
    for (int i = 0; i < PolyCount; ++i)
    {
        int PolySize = Mesh->GetPolygonSize(i);
        for (int j = 0; j < PolySize; ++j)
        {
            int idx = Mesh->GetPolygonVertex(i, j);
            OutMesh.Indices.Add(idx);
        }
    }

    // 노멀, UV, 컬러 등 (매핑 모드에 따라 파싱)
    // 예시: 첫 번째 UV 채널
    if (Mesh->GetElementUVCount() > 0)
    {
        const FbxGeometryElementUV* UVs = Mesh->GetElementUV(0);
        if (UVs->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
        {
            int PolyIndex = 0;
            for (int i = 0; i < PolyCount; ++i)
            {
                int PolySize = Mesh->GetPolygonSize(i);
                for (int j = 0; j < PolySize; ++j)
                {
                    int ctrlPtIdx = Mesh->GetPolygonVertex(i, j);
                    FbxVector2 uv;
                    bool unmapped;
                    Mesh->GetPolygonVertexUV(i, j, UVs->GetName(), uv, unmapped);
                    OutMesh.Vertices[ctrlPtIdx].UV = FVector2D((float)uv[0], (float)uv[1]);
                    ++PolyIndex;
                }
            }
        }
        // (다른 매핑 모드도 필요시 구현)
    }

    // 노멀 파싱 (eByPolygonVertex 예시)
    if (Mesh->GetElementNormalCount() > 0)
    {
        const FbxGeometryElementNormal* Normals = Mesh->GetElementNormal(0);
        if (Normals->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
        {
            int PolyIndex = 0;
            for (int i = 0; i < PolyCount; ++i)
            {
                int PolySize = Mesh->GetPolygonSize(i);
                for (int j = 0; j < PolySize; ++j)
                {
                    int ctrlPtIdx = Mesh->GetPolygonVertex(i, j);
                    FbxVector4 n = Normals->GetDirectArray().GetAt(PolyIndex);
                    OutMesh.Vertices[ctrlPtIdx].Normal = FVector((float)n[0], (float)n[1], (float)n[2]);
                    ++PolyIndex;
                }
            }
        }
        // (다른 매핑 모드도 필요시 구현)
    }

    // 머티리얼 Subset (Section) 파싱
    FbxGeometryElementMaterial* MatElem = Mesh->GetElementMaterial();
    if (MatElem && MatElem->GetMappingMode() == FbxGeometryElement::eByPolygon)
    {
        int PolyCount = Mesh->GetPolygonCount();
        TMap<int, FMaterialSubset> MatIndexToSubset;

        for (int i = 0; i < PolyCount; ++i)
        {
            int MatIndex = MatElem->GetIndexArray().GetAt(i);
            if (!MatIndexToSubset.Contains(MatIndex))
            {
                FMaterialSubset Subset;
                Subset.MaterialIndex = MatIndex;
                Subset.IndexStart = i * 3; // 삼각형 기준
                Subset.IndexCount = 0;
                MatIndexToSubset.Add(MatIndex, Subset);
            }
            MatIndexToSubset[MatIndex].IndexCount += 3;
        }
        for (auto& Elem : MatIndexToSubset)
        {
            OutMesh.MaterialSubsets.Add(Elem.Value);
        }
    }

    OutFbxInfo.Meshes.Add(OutMesh);
}

void FFbxLoader::ParseBone(FbxNode* Node, FFbxInfo& OutFbxInfo)
{
    FFbxBone Bone;
    Bone.Name = Node->GetName();
    Bone.SkinningMatrix = ConvertFbxMatrixToFMatrix(Node->EvaluateGlobalTransform());

    // 부모 인덱스 찾기 (부모가 스켈레톤 노드일 때만)
    FbxNode* Parent = Node->GetParent();
    Bone.ParentIndex = -1;
    if (Parent && Parent->GetNodeAttribute() && Parent->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
    {
        FString ParentName = Parent->GetName();
        for (int i = 0; i < OutFbxInfo.Bones.Num(); ++i)
        {
            if (OutFbxInfo.Bones[i].Name == ParentName)
            {
                Bone.ParentIndex = i;
                break;
            }
        }
    }

    OutFbxInfo.Bones.Add(Bone);
}

void FFbxLoader::ParseMaterial(FbxNode* Node, FFbxInfo& OutFbxInfo)
{
    int MaterialCount = Node->GetMaterialCount();
    for (int i = 0; i < MaterialCount; ++i)
    {
        FbxSurfaceMaterial* FbxMat = Node->GetMaterial(i);
        if (!FbxMat) continue;

        FMaterialInfo MatInfo;
        MatInfo.MaterialName = FbxMat->GetName();

        // Diffuse Color
        FbxProperty DiffuseProp = FbxMat->FindProperty(FbxSurfaceMaterial::sDiffuse);
        if (DiffuseProp.IsValid())
        {
            FbxDouble3 Diffuse = DiffuseProp.Get<FbxDouble3>();
            MatInfo.DiffuseColor = FVector((float)Diffuse[0], (float)Diffuse[1], (float)Diffuse[2]);
        }

        // Diffuse Texture
        int TexCount = DiffuseProp.GetSrcObjectCount<FbxFileTexture>();
        for (int t = 0; t < TexCount; ++t)
        {
            FbxFileTexture* Tex = DiffuseProp.GetSrcObject<FbxFileTexture>(t);
            FTextureInfo TexInfo;
            TexInfo.TextureName = Tex->GetName();
            TexInfo.TexturePath = FString(Tex->GetFileName()).ToWideString();
            TexInfo.bIsSRGB = true; // Diffuse는 보통 sRGB
            MatInfo.TextureInfos.Add(TexInfo);
        }

        // (필요시 Specular, Normal, 기타 파라미터도 파싱)
        OutFbxInfo.Materials.Add(MatInfo);
    }
}

FMatrix FFbxLoader::ConvertFbxMatrixToFMatrix(const FbxAMatrix& fbxMat)
{
    FMatrix result;
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            result.M[row][col] = static_cast<float>(fbxMat.Get(row, col));
    return result;
}


FFbxLoader FFbxManager::FbxLoader;

FStaticMeshRenderData* FFbxManager::LoadFbxStaticMeshAsset(const FString& PathFileName)
{
    UAssetManager* AssetManager = &UAssetManager::Get();
    FStaticMeshRenderData* NewStaticMesh = new FStaticMeshRenderData();

    if (const auto It = AssetManager->StaticMeshRenderDataMap.Find(PathFileName))
    {
        return *It;
    }

    FFbxInfo NewFbxInfo;
    bool Result = FbxLoader.ParseFBX(PathFileName, NewFbxInfo);

    if (!Result)
    {
        delete NewStaticMesh;
        return nullptr;
    }

    ConvertRawToStaticMeshRenderData(NewFbxInfo, *NewStaticMesh);
    AssetManager->StaticMeshRenderDataMap.Add(PathFileName, NewStaticMesh);
    return NewStaticMesh;
}

FSkeletalMeshRenderData* FFbxManager::LoadFbxSkeletalMeshAsset(const FString& PathFileName)
{
    UAssetManager* AssetManager = &UAssetManager::Get();
    FSkeletalMeshRenderData* NewSkeletalMesh = new FSkeletalMeshRenderData();

    if (const auto It = AssetManager->SkeletalMeshRenderDataMap.Find(PathFileName))
    {
        return *It;
    }

    FFbxInfo NewFbxInfo;
    bool Result = FbxLoader.ParseFBX(PathFileName, NewFbxInfo);

    if (!Result)
    {
        delete NewSkeletalMesh;
        return nullptr;
    }

    ConvertRawToSkeletalMeshRenderData(NewFbxInfo, *NewSkeletalMesh);
    AssetManager->SkeletalMeshRenderDataMap.Add(PathFileName, NewSkeletalMesh);
    return NewSkeletalMesh;
}

void FFbxManager::CreateMesh(const FString& filePath)
{
    if (!FbxLoader.LoadFBX(filePath))
    {
        // 에러 처리
        return;
    }

    // 2. 타입 판별 함수 호출
    if (IsFbxSkeletalMesh(FbxLoader.GetScene()))
    {
        CreateSkeletalMesh(filePath);
    }
    else
    {
        CreateStaticMesh(filePath);
    }
}

bool FFbxManager::IsFbxSkeletalMesh(FbxScene* Scene)
{
    bool hasSkeleton = false;
    bool hasSkin = false;

    std::function<void(FbxNode*)> Traverse;
    Traverse = [&](FbxNode* node)
        {
            if (node->GetNodeAttribute())
            {
                if (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
                    hasSkeleton = true;
                if (node->GetMesh() && node->GetMesh()->GetDeformerCount(FbxDeformer::eSkin) > 0)
                    hasSkin = true;
            }
            for (int i = 0; i < node->GetChildCount(); ++i)
                Traverse(node->GetChild(i));
        };

    Traverse(Scene->GetRootNode());
    return hasSkeleton && hasSkin;
}

UStaticMesh* FFbxManager::CreateStaticMesh(const FString& filePath)
{
    FStaticMeshRenderData* StaticMeshRenderData = FFbxManager::LoadFbxStaticMeshAsset(filePath);

    if (StaticMeshRenderData == nullptr) return nullptr;

    UAssetManager* AssetManager = &UAssetManager::Get();

    UStaticMesh* StaticMesh = AssetManager->GetStaticMesh(StaticMeshRenderData->ObjectName);
    if (StaticMesh != nullptr)
    {
        return StaticMesh;
    }

    StaticMesh = FObjectFactory::ConstructObject<UStaticMesh>(nullptr);
    StaticMesh->SetData(StaticMeshRenderData);

    AssetManager->StaticMeshAssetMap.Add(StaticMeshRenderData->ObjectName, StaticMesh);
    return StaticMesh;
}

USkeletalMesh* FFbxManager::CreateSkeletalMesh(const FString& filePath)
{
    FSkeletalMeshRenderData* SkeletalMeshRenderData = FFbxManager::LoadFbxSkeletalMeshAsset(filePath);

    if (SkeletalMeshRenderData == nullptr) return nullptr;

    UAssetManager* AssetManager = &UAssetManager::Get();

    USkeletalMesh* SkeletalMesh = AssetManager->GetSkeletalMesh(SkeletalMeshRenderData->ObjectName);
    if (SkeletalMesh != nullptr)
    {
        return SkeletalMesh;
    }

    SkeletalMesh = FObjectFactory::ConstructObject<USkeletalMesh>(nullptr);
    SkeletalMesh->SetData(SkeletalMeshRenderData);

    AssetManager->SkeletalMeshAssetMap.Add(SkeletalMeshRenderData->ObjectName, SkeletalMesh);
    return SkeletalMesh;
}

void FFbxManager::ConvertRawToSkeletalMeshRenderData(const FFbxInfo& Raw, FSkeletalMeshRenderData& Cooked)
{
    Cooked.ObjectName = Raw.ObjectName;
    Cooked.DisplayName = Raw.DisplayName;

    // 1. 정점/인덱스 복사 및 풀기(Flatten)
    for (const FFbxMesh& Mesh : Raw.Meshes)
    {
        for (const FFbxVertex& Vtx : Mesh.Vertices)
        {
            FSkeletalMeshVertex NewVtx;
            // 위치, 노멀, UV, 컬러, 탄젠트, 본 인덱스/가중치 등 복사
            // (필요시 정점 풀기/조합별 새 정점 생성)
            Cooked.Vertices.Add(NewVtx);
        }
        for (uint32 idx : Mesh.Indices)
            Cooked.Indices.Add(idx);
        for (const FMaterialSubset& Subset : Mesh.MaterialSubsets)
            Cooked.MaterialSubsets.Add(Subset);
    }

    // 2. 본 정보 복사
    for (const FFbxBone& bone : Raw.Bones)
    {
        Bone NewBone;
        NewBone.Name = bone.Name;
        NewBone.ParentIndex = bone.ParentIndex;
        NewBone.BindPose = bone.SkinningMatrix;
        Cooked.Bones.Add(NewBone);
    }

    // 3. 머티리얼 정보 복사
    for (const FMaterialInfo& Mat : Raw.Materials)
        Cooked.Materials.Add(Mat);

    // 4. 바운딩 박스 등 기타 정보
    // Cooked.BoundingBoxMin = ...
    // Cooked.BoundingBoxMax = ...
}

void FFbxManager::ConvertRawToStaticMeshRenderData(const FFbxInfo& Raw, FStaticMeshRenderData& Cooked)
{
    Cooked.ObjectName = Raw.ObjectName;
    Cooked.DisplayName = Raw.DisplayName;

    for (const FFbxMesh& Mesh : Raw.Meshes)
    {
        // 정점 변환
        for (const FFbxVertex& Vtx : Mesh.Vertices)
        {
            FStaticMeshVertex NewVertex;
            NewVertex.X = Vtx.Position.X;
            NewVertex.Y = Vtx.Position.Y;
            NewVertex.Z = Vtx.Position.Z;

            // 컬러
            NewVertex.R = Vtx.Color.X;
            NewVertex.G = Vtx.Color.Y;
            NewVertex.B = Vtx.Color.Z;
            NewVertex.A = 1.0f; // 필요시 Vtx.Color.W

            // 노멀
            NewVertex.NormalX = Vtx.Normal.X;
            NewVertex.NormalY = Vtx.Normal.Y;
            NewVertex.NormalZ = Vtx.Normal.Z;

            //// 탄젠트 (필요시)
            //NewVertex.TangentX = Vtx.Tangent.X;
            //NewVertex.TangentY = Vtx.Tangent.Y;
            //NewVertex.TangentZ = Vtx.Tangent.Z;
            //NewVertex.TangentW = 1.0f; // 필요시

            // UV
            NewVertex.U = Vtx.UV.X;
            NewVertex.V = Vtx.UV.Y;

            // 머티리얼 인덱스 (필요시)
            NewVertex.MaterialIndex = 0; // Section별로 다르게 넣으려면 후처리

            Cooked.Vertices.Add(NewVertex);
        }

        // 인덱스 변환
        for (uint32 idx : Mesh.Indices)
            Cooked.Indices.Add(idx);

        // MaterialSubsets(Section) 복사
        for (const FMaterialSubset& Subset : Mesh.MaterialSubsets)
            Cooked.MaterialSubsets.Add(Subset);
    }

    // 3. 머티리얼 정보 복사
    for (const FMaterialInfo& Mat : Raw.Materials)
        Cooked.Materials.Add(Mat);

    //// 4. 바운딩 박스 계산 (필요시)
    //if (Cooked.Vertices.Num() > 0)
    //{
    //    Cooked.BoundingBoxMin = Cooked.Vertices[0].GetPosition();
    //    Cooked.BoundingBoxMax = Cooked.Vertices[0].GetPosition();
    //    for (const FStaticMeshVertex& Vtx : Cooked.Vertices)
    //    {
    //        FVector Pos(Vtx.X, Vtx.Y, Vtx.Z);
    //        Cooked.BoundingBoxMin.X = std::min(Cooked.BoundingBoxMin.X, Pos.X);
    //        Cooked.BoundingBoxMin.Y = std::min(Cooked.BoundingBoxMin.Y, Pos.Y);
    //        Cooked.BoundingBoxMin.Z = std::min(Cooked.BoundingBoxMin.Z, Pos.Z);
    //        Cooked.BoundingBoxMax.X = std::max(Cooked.BoundingBoxMax.X, Pos.X);
    //        Cooked.BoundingBoxMax.Y = std::max(Cooked.BoundingBoxMax.Y, Pos.Y);
    //        Cooked.BoundingBoxMax.Z = std::max(Cooked.BoundingBoxMax.Z, Pos.Z);
    //    }
    //}
}
