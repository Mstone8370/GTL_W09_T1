#pragma once

#include "EngineLoop.h"
#include "Container/Map.h"
#include "HAL/PlatformType.h"
#include "Serialization/Serializer.h"

class UStaticMesh;
struct FObjManager;

struct FStaticMeshVertex;
struct FStaticMeshRenderData;

struct FObjLoader
{
    // Obj Parsing (*.obj to FObjInfo)
    static bool ParseOBJ(const FString& ObjFilePath, FObjInfo& OutObjInfo);

    // Material Parsing (*.obj to MaterialInfo)
    static bool ParseMaterial(FObjInfo& OutObjInfo, FStaticMeshRenderData& OutFStaticMesh);

    // Convert the Raw data to Cooked data (FStaticMeshRenderData)
    static bool ConvertToStaticMesh(const FObjInfo& RawData, FStaticMeshRenderData& OutStaticMesh);

    static bool CreateTextureFromFile(const FWString& Filename, bool bIsSRGB = true);

    static void ComputeBoundingBox(const TArray<FStaticMeshVertex>& InVertices, FVector& OutMinVector, FVector& OutMaxVector);

private:
    static void CalculateTangent(FStaticMeshVertex& PivotVertex, const FStaticMeshVertex& Vertex1, const FStaticMeshVertex& Vertex2);
};

struct FObjManager
{
public:
    static FStaticMeshRenderData* LoadObjStaticMeshAsset(const FString& PathFileName);

    static void CombineMaterialIndex(FStaticMeshRenderData& OutFStaticMesh);

    static bool SaveStaticMeshToBinary(const FWString& FilePath, const FStaticMeshRenderData& StaticMesh);

    static bool LoadStaticMeshFromBinary(const FWString& FilePath, FStaticMeshRenderData& OutStaticMesh);

    static UMaterial* CreateMaterial(FMaterialInfo materialInfo);

    static TMap<FString, UMaterial*>& GetMaterials() { return MaterialMap; }

    static UMaterial* GetMaterial(FString name);

    static int GetMaterialNum() { return MaterialMap.Num(); }

    static UStaticMesh* CreateStaticMesh(const FString& filePath);

    static const TMap<FWString, UStaticMesh*>& GetStaticMeshes() { return StaticMeshMap; }

    static int GetStaticMeshNum() { return StaticMeshMap.Num(); }

private:
    inline static TMap<FString, FStaticMeshRenderData*> ObjStaticMeshMap;
    inline static TMap<FWString, UStaticMesh*> StaticMeshMap;
    inline static TMap<FString, UMaterial*> MaterialMap;
};
