#pragma once
#include <algorithm>
#include <utility>
#include <vector>

#include "ContainerAllocator.h"
#include "Serialization/Archive.h"


template <typename T, typename Allocator = FDefaultAllocator<T>>
class TArray
{
public:
    using SizeType = typename Allocator::SizeType;
    using ElementType = T;
    using ArrayType = std::vector<ElementType, Allocator>;

private:
    ArrayType ContainerPrivate;

public:
    // Iterator를 사용하기 위함
    auto begin() noexcept { return ContainerPrivate.begin(); }
    auto end() noexcept { return ContainerPrivate.end(); }
    auto begin() const noexcept { return ContainerPrivate.begin(); }
    auto end() const noexcept { return ContainerPrivate.end(); }
    auto rbegin() noexcept { return ContainerPrivate.rbegin(); }
    auto rend() noexcept { return ContainerPrivate.rend(); }
    auto rbegin() const noexcept { return ContainerPrivate.rbegin(); }
    auto rend() const noexcept { return ContainerPrivate.rend(); }

    T& operator[](SizeType Index);
    const T& operator[](SizeType Index) const;
    void operator+(const TArray& OtherArray);

public:
    ArrayType& GetContainerPrivate() { return ContainerPrivate; }
    const ArrayType& GetContainerPrivate() const { return ContainerPrivate; }

    TArray();
    ~TArray() = default;

    // 이니셜라이저 생성자
    TArray(std::initializer_list<T> InitList);

    // 복사 생성자
    TArray(const TArray& Other);

    // 이동 생성자
    TArray(TArray&& Other) noexcept;

    // 복사 할당 연산자
    TArray& operator=(const TArray& Other);

    // 이동 할당 연산자
    TArray& operator=(TArray&& Other) noexcept;

    /** Element를 Number개 만큼 초기화 합니다. */
    void Init(const T& Element, SizeType Number);
    SizeType Add(const T& Item);
    SizeType Add(T&& Item);
    SizeType AddUnique(const T& Item);

    template <typename... Args>
    SizeType Emplace(Args&&... Item);

    /** Array가 비어있는지 확인합니다. */
    bool IsEmpty() const;

    /** Array를 비웁니다 */
    void Empty();

    /** Item과 일치하는 모든 요소를 제거합니다. */
    SizeType Remove(const T& Item);

    /** 왼쪽부터 Item과 일치하는 요소를 1개 제거합니다. */
    bool RemoveSingle(const T& Item);

    /** 특정 위치에 있는 요소를 제거합니다. */
    void RemoveAt(SizeType Index);

    /** Predicate에 부합하는 모든 요소를 제거합니다. */
    template <typename Predicate>
        requires std::is_invocable_r_v<bool, Predicate, const T&>
    SizeType RemoveAll(const Predicate& Pred);

    T* GetData();
    const T* GetData() const;

    /**
     * Array에서 Item을 찾습니다.
     * @param Item 찾으려는 Item
     * @return Item의 인덱스, 찾을 수 없다면 -1
     */
    SizeType Find(const T& Item);
    bool Find(const T& Item, SizeType& Index);

    /**
     * Finds an item by predicate.
     *
     * @param Pred The predicate to match.
     * @returns Index to the first matching element, or INDEX_NONE if none is found.
     */
    template <typename Predicate>
    SizeType IndexOfByPredicate(Predicate Pred) const
    {
        /* 기존 언리얼 코드
        const ElementType* Start = GetData();
        for (const ElementType* Data = Start, DataEnd = (Start + Num()); Data != DataEnd; ++Data)
        {
            if (std::invoke(Pred, *Data))
            {
                return static_cast<SizeType>(Data - Start);
            }
        }
        */
        for (int32 i = 0; i < Num(); ++i)
        {
            const ElementType* Data = &ContainerPrivate[i];
            if (std::invoke(Pred, *Data))
            {
                return static_cast<SizeType>(i);
            }
        }
        return INDEX_NONE;
    }

    /** 요소가 존재하는지 확인합니다. */
    bool Contains(const T& Item) const;

    /** Array Size를 가져옵니다. */
    SizeType Num() const;

    /** Array의 Capacity를 가져옵니다. */
    SizeType Len() const;

    /** Array의 Size를 Number로 설정합니다. */
    void SetNum(SizeType Number);

    /** Array의 Capacity를 Number로 설정합니다. */
    void Reserve(SizeType Number);

    /** Count만큼 초기화되지 않은 공간을 확장합니다. */
    SizeType AddUninitialized(SizeType Count);

    void Sort();
    template <typename Compare>
        requires std::is_invocable_r_v<bool, Compare, const T&, const T&>
    void Sort(const Compare& CompFn);

    bool IsValidIndex(uint32 ElementIndex) const {
        if (ElementIndex < 0 || ElementIndex >= Len()) return false;

        return true;
    }

    ElementType Pop();

    
    /**
     * @brief 다른 TArray의 모든 요소를 이 배열의 끝에 추가합니다.
     * @param OtherArray 추가할 요소들을 포함하는 TArray.
     */
    void Append(const TArray& OtherArray);

    /**
     * @brief C-스타일 배열의 요소들을 이 배열의 끝에 추가합니다.
     * @param Ptr 추가할 요소들의 시작 포인터.
     * @param Count 추가할 요소들의 개수.
     */
    void Append(const T* Ptr, SizeType Count);

    // 선택 사항: std::vector를 직접 추가하는 오버로드
    /**
     * @brief std::vector의 모든 요소를 이 배열의 끝에 추가합니다.
     * @param OtherVector 추가할 요소들을 포함하는 std::vector.
     */
    void Append(const std::vector<T>& OtherVector);
};


template <typename T, typename Allocator>
T& TArray<T, Allocator>::operator[](SizeType Index)
{
    return ContainerPrivate[Index];
}

template <typename T, typename Allocator>
const T& TArray<T, Allocator>::operator[](SizeType Index) const
{
    return ContainerPrivate[Index];
}

template <typename T, typename Allocator>
void TArray<T, Allocator>::operator+(const TArray& OtherArray)
{
    ContainerPrivate.insert(end(), OtherArray.begin(), OtherArray.end());
}

template <typename T, typename Allocator>
TArray<T, Allocator>::TArray()
    : ContainerPrivate()
{
}

template <typename T, typename Allocator>
TArray<T, Allocator>::TArray(std::initializer_list<T> InitList)
    : ContainerPrivate(InitList)
{
}

template <typename T, typename Allocator>
TArray<T, Allocator>::TArray(const TArray& Other)
    : ContainerPrivate(Other.ContainerPrivate)
{
}

template <typename T, typename Allocator>
TArray<T, Allocator>::TArray(TArray&& Other) noexcept
    : ContainerPrivate(std::move(Other.ContainerPrivate))
{
}

template <typename T, typename Allocator>
TArray<T, Allocator>& TArray<T, Allocator>::operator=(const TArray& Other)
{
    if (this != &Other)
    {
        ContainerPrivate = Other.ContainerPrivate;
    }
    return *this;
}

template <typename T, typename Allocator>
TArray<T, Allocator>& TArray<T, Allocator>::operator=(TArray&& Other) noexcept
{
    if (this != &Other)
    {
        ContainerPrivate = std::move(Other.ContainerPrivate);
    }
    return *this;
}

template <typename T, typename Allocator>
void TArray<T, Allocator>::Init(const T& Element, SizeType Number)
{
    ContainerPrivate.assign(Number, Element);
}

template <typename T, typename Allocator>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::Add(const T& Item)
{
    return Emplace(Item);
}

template <typename T, typename Allocator>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::Add(T&& Item)
{
    return Emplace(std::move(Item));
}

template <typename T, typename Allocator>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::AddUnique(const T& Item)
{
    if (SizeType Index; Find(Item, Index))
    {
        return Index;
    }
    return Add(Item);
}

template <typename T, typename Allocator>
template <typename... Args>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::Emplace(Args&&... Item)
{
    ContainerPrivate.emplace_back(std::forward<Args>(Item)...);
    return Num()-1;
}

template <typename T, typename Allocator>
bool TArray<T, Allocator>::IsEmpty() const
{
    return ContainerPrivate.empty();
}

template <typename T, typename Allocator>
void TArray<T, Allocator>::Empty()
{
    ContainerPrivate.clear();
}

template <typename T, typename Allocator>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::Remove(const T& Item)
{
    auto oldSize = ContainerPrivate.size();
    ContainerPrivate.erase(std::remove(ContainerPrivate.begin(), ContainerPrivate.end(), Item), ContainerPrivate.end());
    return static_cast<SizeType>(oldSize - ContainerPrivate.size());
}

template <typename T, typename Allocator>
bool TArray<T, Allocator>::RemoveSingle(const T& Item)
{
    auto it = std::find(ContainerPrivate.begin(), ContainerPrivate.end(), Item);
    if (it != ContainerPrivate.end())
    {
        ContainerPrivate.erase(it);
        return true;
    }
    return false;
}

template <typename T, typename Allocator>
void TArray<T, Allocator>::RemoveAt(SizeType Index)
{
    if (Index >= 0 && static_cast<SizeType>(Index) < ContainerPrivate.size())
    {
        ContainerPrivate.erase(ContainerPrivate.begin() + Index);
    }
}

template <typename T, typename Allocator>
template <typename Predicate>
    requires std::is_invocable_r_v<bool, Predicate, const T&>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::RemoveAll(const Predicate& Pred)
{
    auto oldSize = ContainerPrivate.size();
    ContainerPrivate.erase(std::remove_if(ContainerPrivate.begin(), ContainerPrivate.end(), Pred), ContainerPrivate.end());
    return static_cast<SizeType>(oldSize - ContainerPrivate.size());
}

template <typename T, typename Allocator>
T* TArray<T, Allocator>::GetData()
{
    return ContainerPrivate.data();
}

template <typename T, typename Allocator>
const T* TArray<T, Allocator>::GetData() const
{
    return ContainerPrivate.data();
}

template <typename T, typename Allocator>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::Find(const T& Item)
{
    const auto it = std::find(ContainerPrivate.begin(), ContainerPrivate.end(), Item);
    return it != ContainerPrivate.end() ? std::distance(ContainerPrivate.begin(), it) : INDEX_NONE;
}

template <typename T, typename Allocator>
bool TArray<T, Allocator>::Find(const T& Item, SizeType& Index)
{
    Index = Find(Item);
    return (Index != INDEX_NONE);
}

template <typename T, typename Allocator>
bool TArray<T, Allocator>::Contains(const T& Item) const
{
    for (const T* Data = GetData(), *DataEnd = Data + Num(); Data != DataEnd; ++Data)
    {
        if (*Data == Item)
        {
            return true;
        }
    }
    return false;
}

template <typename T, typename Allocator>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::Num() const
{
    return ContainerPrivate.size();
}

template <typename T, typename Allocator>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::Len() const
{
    return ContainerPrivate.capacity();
}

template <typename T, typename Allocator>
void TArray<T, Allocator>::SetNum(SizeType Number)
{
    ContainerPrivate.resize(Number);
}

template <typename T, typename Allocator>
void TArray<T, Allocator>::Reserve(SizeType Number)
{
    ContainerPrivate.reserve(Number);
}

template <typename T, typename Allocator>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::AddUninitialized(SizeType Count)
{
    if (Count <= 0)
    {
        return ContainerPrivate.size();
    }

    // 기존 크기 저장
    SizeType OldSize = ContainerPrivate.size();

    // 메모리를 확장 (초기화하지 않음)
    ContainerPrivate.resize(OldSize + Count);

    // 새 크기를 반환
    return OldSize;
}

template <typename T, typename Allocator>
void TArray<T, Allocator>::Sort()
{
    std::sort(ContainerPrivate.begin(), ContainerPrivate.end());
}

template <typename T, typename Allocator>
template <typename Compare>
    requires std::is_invocable_r_v<bool, Compare, const T&, const T&>
void TArray<T, Allocator>::Sort(const Compare& CompFn)
{
    std::sort(ContainerPrivate.begin(), ContainerPrivate.end(), CompFn);
}

template <typename T, typename Allocator>
typename TArray<T, Allocator>::ElementType TArray<T, Allocator>::Pop()
{
    ElementType Element = ContainerPrivate.back();
    ContainerPrivate.pop_back();
    return Element;
}

template <typename T, typename Allocator>
void TArray<T, Allocator>::Append(const TArray& OtherArray)
{
    if (OtherArray.IsEmpty())
    {
        return; // 추가할 요소가 없으면 바로 반환
    }

    // 필요한 경우 메모리 재할당을 한 번에 하도록 예약
    Reserve(Num() + OtherArray.Num());

    // std::vector의 insert 기능을 사용하여 다른 배열의 요소들을 추가
    ContainerPrivate.insert(
        ContainerPrivate.end(), // 삽입 위치: 현재 배열의 끝
        OtherArray.ContainerPrivate.begin(), // 추가할 요소들의 시작 반복자
        OtherArray.ContainerPrivate.end()    // 추가할 요소들의 끝 반복자
    );
}

template <typename T, typename Allocator>
void TArray<T, Allocator>::Append(const T* Ptr, SizeType Count)
{
    if (Ptr == nullptr || Count <= 0)
    {
        return; // 유효하지 않은 입력이면 반환
    }

    // 필요한 경우 메모리 재할당을 한 번에 하도록 예약
    Reserve(Num() + Count);

    // std::vector의 insert 기능을 사용하여 C-스타일 배열의 요소들을 추가
    ContainerPrivate.insert(
        ContainerPrivate.end(), // 삽입 위치: 현재 배열의 끝
        Ptr,                    // 추가할 요소들의 시작 포인터
        Ptr + Count             // 추가할 요소들의 끝 포인터 (주의: Count가 SizeType이므로 포인터 연산 주의)
                                // 만약 SizeType이 int32이고 포인터 차이가 size_t를 넘을 수 있다면 문제 소지
                                // 여기서는 Count가 합리적인 범위 내라고 가정
    );
}

template <typename T, typename Allocator>
void TArray<T, Allocator>::Append(const std::vector<T>& OtherVector)
{
    if (OtherVector.empty())
    {
        return; // 추가할 요소가 없으면 바로 반환
    }

    // 필요한 경우 메모리 재할당을 한 번에 하도록 예약
    Reserve(Num() + static_cast<SizeType>(OtherVector.size())); // size_t -> SizeType 캐스팅

    // std::vector의 insert 기능을 사용하여 다른 벡터의 요소들을 추가
    ContainerPrivate.insert(
        ContainerPrivate.end(), // 삽입 위치: 현재 배열의 끝
        OtherVector.begin(),    // 추가할 요소들의 시작 반복자
        OtherVector.end()       // 추가할 요소들의 끝 반복자
    );
}


template <typename ElementType, typename Allocator>
FArchive& operator<<(FArchive& Ar, TArray<ElementType, Allocator>& Array)
{
    using SizeType = typename TArray<ElementType, Allocator>::SizeType;

    // 배열 크기 직렬화
    SizeType ArraySize = Array.Num();
    Ar << ArraySize;

    if (Ar.IsLoading())
    {
        // 로드 시 배열 크기 설정
        Array.SetNum(ArraySize);
    }

    // 배열 요소 직렬화
    for (SizeType Index = 0; Index < ArraySize; ++Index)
    {
        Ar << Array[Index];
    }

    return Ar;
}
