#include "Transform.h"

#include "Matrix.h"
#include "Axis.h"

const FTransform FTransform::Identity = FTransform();

FTransform::FTransform()
    : Translation(FVector::ZeroVector)
    , Rotation(FQuat::Identity)
    , Scale3D(FVector::OneVector)
{}

FTransform::FTransform(const FQuat& InRotation, const FVector& InTranslation, const FVector& InScale3D)
    : Translation(InTranslation)
    , Rotation(InRotation)
    , Scale3D(InScale3D)
{}

FTransform::FTransform(const FRotator& InRotation, const FVector& InTranslation, const FVector& InScale3D)
    : Translation(InTranslation)
    , Rotation(InRotation.Quaternion())
    , Scale3D(InScale3D)
{}

FTransform::FTransform(const FVector& InTranslation)
    : Translation(InTranslation)
    , Rotation(FQuat::Identity)
    , Scale3D(FVector::OneVector)
{}

FTransform::FTransform(const FQuat& InRotation)
    : Translation(FVector::ZeroVector)
    , Rotation(InRotation)
    , Scale3D(FVector::OneVector)
{}

FTransform::FTransform(const FRotator& InRotation)
    : Translation(FVector::ZeroVector)
    , Rotation(InRotation.Quaternion())
    , Scale3D(FVector::OneVector)
{}

FVector FTransform::TransformDirection(const FVector& V) const
{
    // 방향 벡터는 이동(Translation)의 영향을 받지 않고 회전과 크기 조정만 영향받음
    return Rotation.RotateVector(V);
}

FVector FTransform::InverseTransformDirection(const FVector& V) const
{
    // 방향 벡터의 역변환 - 회전의 역을 적용
    return Rotation.Inverse().RotateVector(V);
}

bool FTransform::IsUniform() const
{
    return FMath::IsNearlyEqual(Scale3D.X, Scale3D.Y)
        && FMath::IsNearlyEqual(Scale3D.Y, Scale3D.Z)
        && FMath::IsNearlyEqual(Scale3D.Z, Scale3D.X);
}

float FTransform::GetMaximumAxisScale() const
{
    return FMath::Max3(FMath::Abs(Scale3D.X), FMath::Abs(Scale3D.Y), FMath::Abs(Scale3D.Z));
}

float FTransform::GetMinimumAxisScale() const
{
    return FMath::Min3(FMath::Abs(Scale3D.X), FMath::Abs(Scale3D.Y), FMath::Abs(Scale3D.Z));
}

void FTransform::Accumulate(const FTransform& Other)
{
    // 두 변환을 합성
    *this = *this * Other;
}

void FTransform::MultiplyScale3D(const FVector& InMultiplyScale)
{
    // 기존 스케일에 새로운 스케일을 곱함
    Scale3D *= InMultiplyScale;
}

bool FTransform::IsValid() const
{
    if ( ContainsNaN() )
    {
        return false;
    }

    if ( !IsRotationNormalized() )
    {
        return false;
    }

    return true;
}

bool FTransform::IsRotationNormalized() const
{
    return Rotation.IsNormalized();
}

void FTransform::NormalizeRotation()
{
    // 회전 쿼터니언 정규화
    Rotation.Normalize();
}

void FTransform::Blend(const FTransform& Atom1, const FTransform& Atom2, float Alpha)
{
    // 두 변환 사이의 블렌딩 결과를 현재 변환에 저장
    Translation = FMath::Lerp(Atom1.Translation, Atom2.Translation, Alpha);
    Scale3D = FMath::Lerp(Atom1.Scale3D, Atom2.Scale3D, Alpha);
    
    // 회전은 Slerp를 사용하여 보간
    Rotation = FQuat::Slerp(Atom1.Rotation, Atom2.Rotation, Alpha);
    Rotation.Normalize();
}

FTransform FTransform::BlendWith(const FTransform& Other, float Alpha) const
{
    FTransform Result;
    Result.Blend(*this, Other, Alpha);
    return Result;
}

FTransform FTransform::LerpTransform(const FTransform& A, const FTransform& B, float Alpha)
{
    FTransform Result;
    Result.Blend(A, B, Alpha);
    return Result;
}

void FTransform::SetIdentity()
{
    // 항등 변환 설정: 이동 없음, 회전 없음, 균일 스케일 1
    Translation = FVector::ZeroVector;
    Rotation = FQuat::Identity;
    Scale3D = FVector::OneVector;
}

FMatrix FTransform::ToMatrixWithScale() const
{
    FMatrix OutMatrix;
    
    // 회전 행렬 생성
    const FMatrix RotationMatrix = Rotation.ToMatrix();
    
    // 스케일 적용
    OutMatrix.M[0][0] = RotationMatrix.M[0][0] * Scale3D.X;
    OutMatrix.M[0][1] = RotationMatrix.M[0][1] * Scale3D.X;
    OutMatrix.M[0][2] = RotationMatrix.M[0][2] * Scale3D.X;
    OutMatrix.M[0][3] = 0.0f;
    
    OutMatrix.M[1][0] = RotationMatrix.M[1][0] * Scale3D.Y;
    OutMatrix.M[1][1] = RotationMatrix.M[1][1] * Scale3D.Y;
    OutMatrix.M[1][2] = RotationMatrix.M[1][2] * Scale3D.Y;
    OutMatrix.M[1][3] = 0.0f;
    
    OutMatrix.M[2][0] = RotationMatrix.M[2][0] * Scale3D.Z;
    OutMatrix.M[2][1] = RotationMatrix.M[2][1] * Scale3D.Z;
    OutMatrix.M[2][2] = RotationMatrix.M[2][2] * Scale3D.Z;
    OutMatrix.M[2][3] = 0.0f;
    
    // 이동 설정
    OutMatrix.M[3][0] = Translation.X;
    OutMatrix.M[3][1] = Translation.Y;
    OutMatrix.M[3][2] = Translation.Z;
    OutMatrix.M[3][3] = 1.0f;
    
    return OutMatrix;
}

FMatrix FTransform::ToMatrixNoScale() const
{
    FMatrix OutMatrix;
    
    // 회전 행렬 생성
    const FMatrix RotationMatrix = Rotation.ToMatrix();
    
    // 스케일 없이 회전만 적용
    OutMatrix.M[0][0] = RotationMatrix.M[0][0];
    OutMatrix.M[0][1] = RotationMatrix.M[0][1];
    OutMatrix.M[0][2] = RotationMatrix.M[0][2];
    OutMatrix.M[0][3] = 0.0f;
    
    OutMatrix.M[1][0] = RotationMatrix.M[1][0];
    OutMatrix.M[1][1] = RotationMatrix.M[1][1];
    OutMatrix.M[1][2] = RotationMatrix.M[1][2];
    OutMatrix.M[1][3] = 0.0f;
    
    OutMatrix.M[2][0] = RotationMatrix.M[2][0];
    OutMatrix.M[2][1] = RotationMatrix.M[2][1];
    OutMatrix.M[2][2] = RotationMatrix.M[2][2];
    OutMatrix.M[2][3] = 0.0f;
    
    // 이동 설정
    OutMatrix.M[3][0] = Translation.X;
    OutMatrix.M[3][1] = Translation.Y;
    OutMatrix.M[3][2] = Translation.Z;
    OutMatrix.M[3][3] = 1.0f;
    
    return OutMatrix;
}

void FTransform::SetFromMatrix(const FMatrix& InMatrix)
{
    // 행렬 복사본 생성
    FMatrix M = InMatrix;
    
    // 스케일 추출
    Scale3D = M.ExtractScaling();
    
    // 음수 스케일링 처리
    if (InMatrix.Determinant() < 0.f)
    {
        // 음수 행렬식은 음수 스케일이 있다는 의미입니다.
        // X축을 따라 음수 스케일이 있다고 가정하고 변환을 수정합니다.
        // 어떤 축을 선택하든 '외관'은 동일합니다.
        Scale3D.X = -Scale3D.X;
        
        // X축 방향 반전
        M.SetAxis(0, -M.GetScaledAxis(EAxis::X));
    }
    
    // 스케일이 제거된 행렬에서 회전값 추출
    Rotation = FQuat(M);
    Rotation.Normalize();
    
    // 이동값 추출
    Translation = InMatrix.GetOrigin();
}

FTransform FTransform::GetRelativeTransform(const FTransform& Other) const
{
    // 현재 변환에 대한 다른 변환의 상대 변환 계산
    // Other^(-1) * This
    return Other.Inverse() * (*this);
}

FTransform FTransform::GetRelativeTransformReverse(const FTransform& Other) const
{
    // 다른 변환에 대한 현재 변환의 상대 변환 계산
    // This^(-1) * Other
    return Inverse() * Other;
}

FString FTransform::ToString() const
{
    return FString::Printf(TEXT("Translation=%s Rotation=%s Scale3D=%s"), 
                          *Translation.ToString(), 
                          *Rotation.ToString(), 
                          *Scale3D.ToString());
}

bool FTransform::Serialize(FArchive& Ar)
{
    Ar << Translation;
    Ar << Rotation;
    Ar << Scale3D;
    return true;
}

bool FTransform::ContainsNaN() const
{
    return Translation.ContainsNaN() || Rotation.ContainsNaN() || Scale3D.ContainsNaN();
}

FVector4 FTransform::TransformFVector4(const FVector4& V) const
{
    // FVector4 변환 - 위치, 회전, 스케일 모두 포함
    // W 성분은 포인트(1.0) 또는 방향(0.0)에 따라 다르게 처리
    if (V.W == 0.0f)
    {
        // 방향 벡터 (W=0)이면 이동 성분은 적용하지 않음
        return FVector4(Rotation.RotateVector(Scale3D * FVector(V)), 0.0f);
    }
    else
    {
        // 위치 벡터 (W=1)이면 이동 성분도 적용
        return FVector4(TransformPosition(FVector(V)), V.W);
    }
}

FVector4 FTransform::TransformFVector4NoScale(const FVector4& V) const
{
    // 스케일 변환 없이 FVector4 변환
    if (V.W == 0.0f)
    {
        // 방향 벡터 (W=0)
        return FVector4(Rotation.RotateVector(FVector(V)), 0.0f);
    }
    else
    {
        // 위치 벡터 (W=1)
        return FVector4(Rotation.RotateVector(FVector(V)) + Translation, V.W);
    }
}

void FTransform::ScaleTranslation(const FVector& InScale3D)
{
    // 이동 성분에만 스케일 적용
    Translation *= InScale3D;
}

void FTransform::RemoveScaling(float Tolerance)
{
    // 스케일을 1로 설정하되, 매우 작은 스케일 값은 0으로 설정
    Scale3D = FVector(
        FMath::Abs(Scale3D.X) < Tolerance ? 0.0f : 1.0f,
        FMath::Abs(Scale3D.Y) < Tolerance ? 0.0f : 1.0f,
        FMath::Abs(Scale3D.Z) < Tolerance ? 0.0f : 1.0f
    );
}

FVector FTransform::GetSafeScaleReciprocal(const FVector& InScale, float Tolerance) const
{
    // 0에 가까운 스케일 값의 역수를 안전하게 계산 (0 나누기 방지)
    return FVector(
        FMath::Abs(InScale.X) <= Tolerance ? 0.0f : 1.0f / InScale.X,
        FMath::Abs(InScale.Y) <= Tolerance ? 0.0f : 1.0f / InScale.Y,
        FMath::Abs(InScale.Z) <= Tolerance ? 0.0f : 1.0f / InScale.Z
    );
}

void FTransform::BlendFromIdentityAndAccumulate(FTransform& OutTransform, const FTransform& InTransform, float Alpha)
{
    // 항등 변환과 주어진 변환 사이에서 블렌딩한 후 결과를 OutTransform에 누적
    FTransform BlendedTransform;
    
    // 항등 변환과 InTransform 사이의 보간
    BlendedTransform.Translation = InTransform.Translation * Alpha;
    BlendedTransform.Scale3D = FMath::Lerp(FVector::OneVector, InTransform.Scale3D, Alpha);
    
    if (Alpha < 1.0f - SMALL_NUMBER)
    {
        BlendedTransform.Rotation = FQuat::Slerp(FQuat::Identity, InTransform.Rotation, Alpha);
    }
    else
    {
        BlendedTransform.Rotation = InTransform.Rotation;
    }
    
    // 결과를 OutTransform에 누적
    OutTransform = OutTransform * BlendedTransform;
}

FTransform FTransform::operator*(const FTransform& Other) const
{
    FTransform Result;
    Result.Scale3D = Scale3D * Other.Scale3D;
    Result.Rotation = Rotation * Other.Rotation;
    Result.Translation = Rotation.RotateVector(Scale3D * Other.Translation) + Translation;
    return Result;
}

FVector FTransform::TransformPosition(const FVector& V) const
{
    return Rotation.RotateVector(Scale3D * V) + Translation;
}

FVector FTransform::InverseTransformPosition(const FVector& V) const
{
    return (Rotation.Inverse().RotateVector(V - Translation)) / Scale3D;
}

FVector FTransform::TransformVector(const FVector& V) const
{
    return Rotation.RotateVector(Scale3D * V);
}

FVector FTransform::InverseTransformVector(const FVector& V) const
{
    return Rotation.Inverse().RotateVector(V) / Scale3D;
}

bool FTransform::IsIdentity() const
{
    return Translation.IsZero() && Rotation.IsIdentity() && Scale3D == FVector(1.0f, 1.0f, 1.0f);
}

FTransform FTransform::Inverse() const
{
    FTransform Result;
    Result.Scale3D = FVector(1.0f / Scale3D.X, 1.0f / Scale3D.Y, 1.0f / Scale3D.Z);
    Result.Rotation = Rotation.Inverse();
    Result.Translation = Result.Rotation.RotateVector((-Translation) / Scale3D);
    return Result;
}
