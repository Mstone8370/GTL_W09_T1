#pragma once

#include <string>
#include <algorithm> // std::transform 사용
#include <cwchar>    // wcscmp, wcsrchr 등 사용 (필요시)
#include <filesystem>

#include "Container/String.h"
#include "HAL/PlatformType.h"

class FPaths
{
public:

    // 헬퍼 함수: 와이드 문자열을 소문자로 변환
    static FORCEINLINE std::wstring ToLowerWide(const std::wstring& str)
    {
        std::wstring lowerStr = str;
        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
                       [](wchar_t c){ return std::tolower(c); });
        return lowerStr;
    }

    /**
     * @brief 주어진 파일 이름의 확장자가 지정된 확장자와 일치하는지 확인합니다. (대소문자 무시)
     *
     * @param filename 확인할 파일 이름 (예: L"MyTexture.png", L"path/to/archive.tar.gz").
     * @param extension 확인할 확장자 이름 (예: L"png", L"gz", L"TAR.GZ"). 점(.)은 포함하지 않습니다.
     * @return 파일 이름의 확장자가 주어진 확장자와 일치하면 true, 그렇지 않으면 false.
     *         filename이 null이거나 비어있거나, 확장자가 없거나, extension이 null이거나 비어있으면 false.
     */
    static bool HasExtension(const wchar_t* filename, const wchar_t* extension)
    {
        // 입력 유효성 검사
        if (filename == nullptr || filename[0] == L'\0' ||
            extension == nullptr || extension[0] == L'\0')
        {
            return false;
        }

        // 파일 이름에서 마지막 점(.)의 위치를 찾습니다.
        const wchar_t* dot = std::wcsrchr(filename, L'.');

        // 점이 없거나, 점이 파일 이름의 시작이거나 (예: ".hiddenfile"),
        // 점이 파일 이름의 마지막 문자이면 (예: "archive.") 확장자가 없는 것으로 간주합니다.
        if (dot == nullptr || dot == filename || *(dot + 1) == L'\0')
        {
            return false;
        }

        // 점(.) 다음부터가 실제 확장자입니다.
        std::wstring fileExtension(dot + 1);

        // 비교를 위해 파일 확장자와 주어진 확장자를 모두 소문자로 변환합니다.
        std::wstring lowerFileExt = ToLowerWide(fileExtension);
        std::wstring lowerGivenExt = ToLowerWide(std::wstring(extension)); // const wchar_t* -> std::wstring

        // 두 소문자 확장자를 비교합니다.
        return lowerFileExt == lowerGivenExt;
    }

    // Extension 포함
    static FString GetFileName(const FString& _Path)
    {
        std::filesystem::path ReturnPath = *_Path;
        return ReturnPath.filename().string();
    }

    static FString GetFileNameWithoutExtension(const FString& _Path)
    {
        std::filesystem::path ReturnPath = *_Path;
        return ReturnPath.stem().string();
    }

    static FString GetExtension(const FString& _Path)
    {
        std::filesystem::path ReturnPath = *_Path;
        return ReturnPath.extension().string();
    }
    
};
