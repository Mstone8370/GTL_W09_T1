#include "FbxLoader.h"

#include "UObject/ObjectFactory.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"

#include "Asset/SkeletalMeshAsset.h"
#include "Asset/StaticMeshAsset.h"
#include "AssetManager.h"
#include <fbxsdk/utils/fbxgeometryconverter.h>

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

    FbxGeometryConverter converter(Manager);
    // 씬 전체를 삼각형화 (원본 메시를 삼각형으로 대체)
    converter.Triangulate(Scene, /*replace=*/true);

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

    std::unordered_map<std::string, int> VertexMap;
    TArray<FFbxVertex> Vertices;
    TArray<uint32> Indices;

    int PolyCount = Mesh->GetPolygonCount();
    int PolyIndex = 0;
    for (int i = 0; i < PolyCount; ++i)
    {
        int PolySize = Mesh->GetPolygonSize(i);
        for (int j = 0; j < PolySize; ++j)
        {
            int ctrlPtIdx = Mesh->GetPolygonVertex(i, j);
            FbxVector4 p = Mesh->GetControlPointAt(ctrlPtIdx);
            FVector Position = FVector((float)p[0], (float)p[1], (float)p[2]);

            // UV 추출
            FVector2D UV(0, 0);
            if (Mesh->GetElementUVCount() > 0) {
                const FbxGeometryElementUV* UVs = Mesh->GetElementUV(0);
                if (UVs->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) {
                    FbxVector2 uv;
                    bool unmapped;
                    Mesh->GetPolygonVertexUV(i, j, UVs->GetName(), uv, unmapped);
                    UV = FVector2D((float)uv[0], 1.0 - (float)uv[1]);
                }
            }

            // 노멀 추출
            FVector Normal(0, 0, 1);
            if (Mesh->GetElementNormalCount() > 0) {
                const FbxGeometryElementNormal* Normals = Mesh->GetElementNormal(0);
                if (Normals->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) {
                    FbxVector4 n = Normals->GetDirectArray().GetAt(PolyIndex);
                    Normal = FVector((float)n[0], (float)n[1], (float)n[2]);
                }
            }

            // string 키 생성 (위치 인덱스/노멀/UV)
            std::string Key = std::to_string(ctrlPtIdx) + "/" +
                std::to_string(Normal.X) + "," + std::to_string(Normal.Y) + "," + std::to_string(Normal.Z) + "/" +
                std::to_string(UV.X) + "," + std::to_string(UV.Y);

            int VertIdx;
            auto it = VertexMap.find(Key);
            if (it != VertexMap.end()) {
                VertIdx = it->second;
            }
            else {
                FFbxVertex v;
                v.Position = Position;
                v.Normal = Normal;
                v.UV = UV;
                VertIdx = Vertices.Num();
                Vertices.Add(v);
                VertexMap[Key] = VertIdx;
            }
            Indices.Add(VertIdx);
            ++PolyIndex;
        }
    }

    // 머티리얼 Subset (Section) 파싱
    FbxGeometryElementMaterial* MatElem = Mesh->GetElementMaterial();
    if (MatElem)
    {
        auto mode = MatElem->GetMappingMode();
        if (mode == FbxGeometryElement::eByPolygon)
        {
            int PolyCount = Mesh->GetPolygonCount();
            TMap<int, FMaterialSubset> MatIndexToSubset;

            for (int i = 0; i < PolyCount; ++i)
            {
                int MatIndex = MatElem->GetIndexArray().GetAt(i);
                // 머티리얼 이름 얻기
                FString MaterialName;
                FbxNode* OwnerNode = Mesh->GetNode();
                if (OwnerNode && MatIndex < OwnerNode->GetMaterialCount())
                {
                    FbxSurfaceMaterial* FbxMat = OwnerNode->GetMaterial(MatIndex);
                    if (FbxMat)
                        MaterialName = FbxMat->GetName();
                }

                if (!MatIndexToSubset.Contains(MatIndex))
                {
                    FMaterialSubset Subset;
                    Subset.MaterialIndex = MatIndex;
                    Subset.IndexStart = i * 3; // 삼각형 기준
                    Subset.IndexCount = 0;
                    Subset.MaterialName = MaterialName;
                    MatIndexToSubset.Add(MatIndex, Subset);
                }
                MatIndexToSubset[MatIndex].IndexCount += 3;
            }
            for (auto& Elem : MatIndexToSubset)
            {
                OutMesh.MaterialSubsets.Add(Elem.Value);
            }
        }
        else if (mode == FbxGeometryElement::eAllSame)
        {
            int MatIndex = MatElem->GetIndexArray().GetAt(0);
            FString MaterialName;
            FbxNode* OwnerNode = Mesh->GetNode();
            if (OwnerNode && MatIndex < OwnerNode->GetMaterialCount())
            {
                FbxSurfaceMaterial* FbxMat = OwnerNode->GetMaterial(MatIndex);
                if (FbxMat)
                    MaterialName = FbxMat->GetName();
            }

            FMaterialSubset Subset;
            Subset.MaterialIndex = MatElem->GetIndexArray().GetAt(0);
            Subset.IndexStart = 0;
            Subset.IndexCount = Mesh->GetPolygonCount() * 3; // 삼각형 기준
            Subset.MaterialName = MaterialName;
            OutMesh.MaterialSubsets.Add(Subset);
        }
        // (필요시 다른 매핑 모드도 처리)
    }

    OutMesh.Vertices = Vertices;
    OutMesh.Indices = Indices;
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
            TexInfo.TexturePath = FString(Tex->GetRelativeFileName()).ToWideString();
            if (CreateTextureFromFile(TexInfo.TexturePath))
            {
                TexInfo.bIsSRGB = true;
                MatInfo.TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Diffuse);
            }
            MatInfo.TextureInfos.Add(TexInfo);
        }

        // (필요시 Specular, Normal, 기타 파라미터도 파싱)
        OutFbxInfo.Materials.Add(MatInfo);
        FFbxManager::CreateMaterial(MatInfo);
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

bool FFbxLoader::CreateTextureFromFile(const FWString& Filename, bool bIsSRGB)
{
    if (FEngineLoop::ResourceManager.GetTexture(Filename))
    {
        return true;
    }

    HRESULT hr = FEngineLoop::ResourceManager.LoadTextureFromFile(FEngineLoop::GraphicDevice.Device, Filename.c_str(), bIsSRGB);

    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

FFbxLoader FFbxManager::FbxLoader;

FStaticMeshRenderData* FFbxManager::LoadFbxStaticMeshAsset(const FString& PathFileName)
{
    UAssetManager* AssetManager = &UAssetManager::Get();
    FStaticMeshRenderData* NewStaticMesh = new FStaticMeshRenderData();

    if (const auto It = AssetManager->GetStaticMeshRenderData(PathFileName))
    {
        return It;
    }

    FFbxInfo NewFbxInfo;
    bool Result = FbxLoader.ParseFBX(PathFileName, NewFbxInfo);

    if (!Result)
    {
        delete NewStaticMesh;
        return nullptr;
    }

    ConvertRawToStaticMeshRenderData(NewFbxInfo, *NewStaticMesh);
    AssetManager->AddStaticMeshRenderData(PathFileName, NewStaticMesh);
    return NewStaticMesh;
}

FSkeletalMeshRenderData* FFbxManager::LoadFbxSkeletalMeshAsset(const FString& PathFileName)
{
    UAssetManager* AssetManager = &UAssetManager::Get();
    FSkeletalMeshRenderData* NewSkeletalMesh = new FSkeletalMeshRenderData();

    if (const auto It = AssetManager->GetSkeletalMeshRenderData(PathFileName))
    {
        return It;
    }

    FFbxInfo NewFbxInfo;
    bool Result = FbxLoader.ParseFBX(PathFileName, NewFbxInfo);

    if (!Result)
    {
        delete NewSkeletalMesh;
        return nullptr;
    }

    ConvertRawToSkeletalMeshRenderData(NewFbxInfo, *NewSkeletalMesh);
    AssetManager->AddSkeletalMeshRenderData(PathFileName, NewSkeletalMesh);
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

    UStaticMesh* StaticMesh = AssetManager->GetStaticMeshAsset(StaticMeshRenderData->ObjectName);
    if (StaticMesh != nullptr)
    {
        return StaticMesh;
    }

    StaticMesh = FObjectFactory::ConstructObject<UStaticMesh>(nullptr);
    StaticMesh->SetData(StaticMeshRenderData);

    AssetManager->AddStaticMeshAsset(StaticMeshRenderData->ObjectName, StaticMesh);
    return StaticMesh;
}

USkeletalMesh* FFbxManager::CreateSkeletalMesh(const FString& filePath)
{
    FSkeletalMeshRenderData* SkeletalMeshRenderData = FFbxManager::LoadFbxSkeletalMeshAsset(filePath);

    if (SkeletalMeshRenderData == nullptr) return nullptr;

    UAssetManager* AssetManager = &UAssetManager::Get();

    USkeletalMesh* SkeletalMesh = AssetManager->GetSkeletalMeshAsset(SkeletalMeshRenderData->ObjectName);
    if (SkeletalMesh != nullptr)
    {
        return SkeletalMesh;
    }

    SkeletalMesh = FObjectFactory::ConstructObject<USkeletalMesh>(nullptr);
    SkeletalMesh->SetData(SkeletalMeshRenderData);

    AssetManager->AddSkeletalMeshAsset(SkeletalMeshRenderData->ObjectName, SkeletalMesh);
    return SkeletalMesh;
}

void FFbxManager::ConvertRawToSkeletalMeshRenderData(const FFbxInfo& Raw, FSkeletalMeshRenderData& Cooked)
{
    Cooked.ObjectName = Raw.ObjectName;
    Cooked.DisplayName = Raw.DisplayName;

    UINT VertexBase = 0;
    UINT IndexBase = 0;

    int MaterialIndex = 0;
    for (const FFbxMesh& Mesh : Raw.Meshes)
    {
        // 1. 정점 추가
        for (const FFbxVertex& Vtx : Mesh.Vertices)
        {
            FSkeletalMeshVertex NewVtx;
            NewVtx.X = Vtx.Position.X;
            NewVtx.Y = Vtx.Position.Y;
            NewVtx.Z = Vtx.Position.Z;
            NewVtx.NormalX = Vtx.Normal.X;
            NewVtx.NormalY = Vtx.Normal.Y;
            NewVtx.NormalZ = Vtx.Normal.Z;
            NewVtx.U = Vtx.UV.X;
            NewVtx.V = Vtx.UV.Y;
            NewVtx.R = Vtx.Color.X;
            NewVtx.G = Vtx.Color.Y;
            NewVtx.B = Vtx.Color.Z;
            NewVtx.A = 1.0f; // 필요시 Vtx.Color.W
            Cooked.Vertices.Add(NewVtx);
        }

        // 2. 인덱스 추가 (정점 오프셋 적용)
        for (UINT idx : Mesh.Indices)
            Cooked.Indices.Add(idx + VertexBase);

        // 3. MaterialSubset(Section) 추가 (인덱스 오프셋 적용)
        for (const FMaterialSubset& Subset : Mesh.MaterialSubsets)
        {
            FMaterialSubset NewSubset = Subset;
            NewSubset.IndexStart += IndexBase;
            NewSubset.MaterialIndex = MaterialIndex++;
            Cooked.MaterialSubsets.Add(NewSubset);
        }

        VertexBase += Mesh.Vertices.Num();
        IndexBase += Mesh.Indices.Num();
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

    if (Cooked.Vertices.Num() > 0)
    {
        Cooked.BoundingBoxMin = FVector(Cooked.Vertices[0].X, Cooked.Vertices[0].Y, Cooked.Vertices[0].Z);
        Cooked.BoundingBoxMax = FVector(Cooked.Vertices[0].X, Cooked.Vertices[0].Y, Cooked.Vertices[0].Z);;
        for (const FSkeletalMeshVertex& Vtx : Cooked.Vertices)
        {
            FVector Pos(Vtx.X, Vtx.Y, Vtx.Z);
            Cooked.BoundingBoxMin.X = std::min(Cooked.BoundingBoxMin.X, Pos.X);
            Cooked.BoundingBoxMin.Y = std::min(Cooked.BoundingBoxMin.Y, Pos.Y);
            Cooked.BoundingBoxMin.Z = std::min(Cooked.BoundingBoxMin.Z, Pos.Z);
            Cooked.BoundingBoxMax.X = std::max(Cooked.BoundingBoxMax.X, Pos.X);
            Cooked.BoundingBoxMax.Y = std::max(Cooked.BoundingBoxMax.Y, Pos.Y);
            Cooked.BoundingBoxMax.Z = std::max(Cooked.BoundingBoxMax.Z, Pos.Z);
        }
    }
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

    if (Cooked.Vertices.Num() > 0)
    {
        Cooked.BoundingBoxMin = FVector(Cooked.Vertices[0].X, Cooked.Vertices[0].Y, Cooked.Vertices[0].Z);
        Cooked.BoundingBoxMax = FVector(Cooked.Vertices[0].X, Cooked.Vertices[0].Y, Cooked.Vertices[0].Z);;
        for (const FStaticMeshVertex& Vtx : Cooked.Vertices)
        {
            FVector Pos(Vtx.X, Vtx.Y, Vtx.Z);
            Cooked.BoundingBoxMin.X = std::min(Cooked.BoundingBoxMin.X, Pos.X);
            Cooked.BoundingBoxMin.Y = std::min(Cooked.BoundingBoxMin.Y, Pos.Y);
            Cooked.BoundingBoxMin.Z = std::min(Cooked.BoundingBoxMin.Z, Pos.Z);
            Cooked.BoundingBoxMax.X = std::max(Cooked.BoundingBoxMax.X, Pos.X);
            Cooked.BoundingBoxMax.Y = std::max(Cooked.BoundingBoxMax.Y, Pos.Y);
            Cooked.BoundingBoxMax.Z = std::max(Cooked.BoundingBoxMax.Z, Pos.Z);
        }
    }
}

UMaterial* FFbxManager::CreateMaterial(FMaterialInfo materialInfo)
{
    UAssetManager* AssetManager = &UAssetManager::Get();
    if (AssetManager->GetMaterial(materialInfo.MaterialName) != nullptr)
        return AssetManager->GetMaterial(materialInfo.MaterialName);

    UMaterial* newMaterial = FObjectFactory::ConstructObject<UMaterial>(nullptr); // Material은 Outer가 없이 따로 관리되는 객체이므로 Outer가 없음으로 설정. 추후 Garbage Collection이 추가되면 AssetManager를 생성해서 관리.
    newMaterial->SetMaterialInfo(materialInfo);
    AssetManager->GetMaterials().Add(materialInfo.MaterialName, newMaterial);
    return newMaterial;
}
