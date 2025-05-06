#include "Define.h"

#include <format>

void FObjMaterialInfo::SetTextureForSlot(EMaterialTextureSlots Slot, const FWString& InTexturePath, const FWString& InTextureName)
{
    const uint32 SlotIdx = static_cast<uint32>(Slot);

        if (SlotIdx >= static_cast<uint32>(EMaterialTextureSlots::MTS_MAX))
        {
            OutputDebugStringA(std::format("Error: Invalid texture slot index {} in FObjMaterialInfo::SetTextureForSlot.\n", SlotIdx).c_str());
            return;
        }

        // TextureInfos 배열은 생성자에서 이미 크기가 할당되었다고 가정

        if (!InTexturePath.empty())
        {
            TextureInfos[SlotIdx].TexturePath = InTexturePath;
            TextureInfos[SlotIdx].TextureName = InTextureName;

            EMaterialTextureFlags FlagToSet = static_cast<EMaterialTextureFlags>(0);
            bool bIsSRGBDefault = true; // 기본 sRGB 설정

            switch (Slot)
            {
            case EMaterialTextureSlots::MTS_Diffuse:
                FlagToSet = EMaterialTextureFlags::MTF_Diffuse;
                bIsSRGBDefault = true;
                break;
            case EMaterialTextureSlots::MTS_Specular:
                FlagToSet = EMaterialTextureFlags::MTF_Specular;
                bIsSRGBDefault = false; // 선형
                break;
            case EMaterialTextureSlots::MTS_Normal:
                FlagToSet = EMaterialTextureFlags::MTF_Normal;
                bIsSRGBDefault = false; // 항상 선형
                break;
            case EMaterialTextureSlots::MTS_Emissive:
                FlagToSet = EMaterialTextureFlags::MTF_Emissive;
                bIsSRGBDefault = true;
                break;
            case EMaterialTextureSlots::MTS_Alpha:
                FlagToSet = EMaterialTextureFlags::MTF_Alpha;
                bIsSRGBDefault = false; // 선형
                break;
            case EMaterialTextureSlots::MTS_Ambient:
                FlagToSet = EMaterialTextureFlags::MTF_Ambient;
                bIsSRGBDefault = false; // 선형
                break;
            case EMaterialTextureSlots::MTS_Shininess:
                FlagToSet = EMaterialTextureFlags::MTF_Shininess;
                bIsSRGBDefault = false; // 선형
                break;
            case EMaterialTextureSlots::MTS_Metallic:
                FlagToSet = EMaterialTextureFlags::MTF_Metallic;
                bIsSRGBDefault = false; // 선형
                break;
            case EMaterialTextureSlots::MTS_Roughness:
                FlagToSet = EMaterialTextureFlags::MTF_Roughness;
                bIsSRGBDefault = false; // 선형
                break;
            default:
                OutputDebugStringA(std::format("Warning: Unhandled texture slot index {} in FObjMaterialInfo::SetTextureForSlot.\n", SlotIdx).c_str());
                break;
            }

            TextureInfos[SlotIdx].bIsSRGB = bIsSRGBDefault; // sRGB 설정
            if (static_cast<uint16>(FlagToSet) != 0)
            {
                TextureFlag |= static_cast<uint16>(FlagToSet); // 플래그 설정
            }
        }
        else
        {
            // 로드 실패 또는 경로 없음
            TextureInfos[SlotIdx].TexturePath = L"";
            TextureInfos[SlotIdx].TextureName = L"";
            // 해당 슬롯의 플래그는 변경하지 않거나, 필요시 제거하는 로직 추가 가능
            // 예: TextureFlag &= ~static_cast<uint16>(FlagToSet); // 만약 FlagToSet이 미리 계산되었다면
        }
    }
}
