#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

#include "Container/Map.h"

enum class EAssetType : uint8
{
    StaticMesh,
    SkeletalMesh,
    Texture2D,
    Material,
};

class UStaticMesh;
class USkeletalMesh;
class UMaterial;
struct FStaticMeshRenderData;
struct FSkeletalMeshRenderData;

struct FAssetInfo
{
    FName AssetName;      // Asset의 이름
    FName PackagePath;    // Asset의 패키지 경로
    EAssetType AssetType; // Asset의 타입
    uint32 Size;          // Asset의 크기 (바이트 단위)
};

struct FAssetRegistry
{
    TMap<FName, FAssetInfo> PathNameToAssetInfo;
};

class UAssetManager : public UObject
{
    DECLARE_CLASS(UAssetManager, UObject)

private:
    std::unique_ptr<FAssetRegistry> AssetRegistry;

public:
    UAssetManager() = default;

    static bool IsInitialized();

    /** UAssetManager를 가져옵니다. */
    static UAssetManager& Get();

    /** UAssetManager가 존재하면 가져오고, 없으면 nullptr를 반환합니다. */
    static UAssetManager* GetIfInitialized();
    
    void InitAssetManager();

    const TMap<FName, FAssetInfo>& GetAssetRegistry();

    USkeletalMesh* GetSkeletalMesh(FWString name);
    UStaticMesh* GetStaticMesh(FWString name);

    static TMap<FWString, UStaticMesh*> StaticMeshAssetMap;
    static TMap<FWString, USkeletalMesh*> SkeletalMeshAssetMap;
    static TMap<FString, FStaticMeshRenderData*> StaticMeshRenderDataMap;
    static TMap<FString, FSkeletalMeshRenderData*> SkeletalMeshRenderDataMap;
    static TMap<FString, UMaterial*> MaterialAssetMap;
private:
    void LoadObjFiles();
};
