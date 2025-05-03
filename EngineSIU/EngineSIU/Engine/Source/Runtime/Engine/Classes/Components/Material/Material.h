#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class UMaterial : public UObject {
    DECLARE_CLASS(UMaterial, UObject)

public:
    UMaterial() = default;

    virtual UObject* Duplicate(UObject* InOuter) override;

    FMaterialInfo& GetMaterialInfo() { return materialInfo; }
    void SetMaterialInfo(const FMaterialInfo& value) { materialInfo = value; }

    // 색상 및 재질 속성 설정자
    void SetDiffuse(const FVector& DiffuseIn) { materialInfo.DiffuseColor = DiffuseIn; }
    void SetSpecular(const FVector& SpecularIn) { materialInfo.SpecularColor = SpecularIn; }
    void SetAmbient(const FVector& AmbientIn) { materialInfo.AmbientColor = AmbientIn; }
    void SetEmissive(const FVector& EmissiveIn) { materialInfo.EmissiveColor = EmissiveIn; }

    // 스칼라 속성 설정자
    void SetSpecularPower(float SpecularPowerIn) { materialInfo.SpecularExponent = SpecularPowerIn; }
    void SetOpticalDensity(float DensityIn) { materialInfo.IOR = DensityIn; }
    void SetTransparency(float TransparencyIn) {
        materialInfo.Transparency = TransparencyIn;
        materialInfo.bTransparent = (TransparencyIn < 1.0f);
    }
private:
    FMaterialInfo materialInfo;
};
