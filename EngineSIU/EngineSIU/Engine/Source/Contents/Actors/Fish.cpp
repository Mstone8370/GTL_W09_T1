
#include "Fish.h"

#include "GoalPlatformActor.h"
#include "ItemActor.h"
#include "PlatformActor.h"
#include "Components/SphereComponent.h"
#include "Contents/Components/FishTailComponent.h"
#include "Contents/Components/FishBodyComponent.h"
#include "Engine/FObjLoader.h"
#include "SoundManager.h"
#include "TriggerBox.h"
#include "Contents/Objects/DamageCameraShake.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/SpringArmComponent.h"
#include "World/World.h"

AFish::AFish()
    : JumpZVelocity(50.f)
    , Gravity(-9.8f * 10.f)
    , bShouldApplyGravity(true)
    , MeshPitchMax(5.f)
    , MeshPitch(MeshPitchMax)
    , MaxHealth(9999999) //시작전 안죽게 하려고 임시방편
    , Health(MaxHealth)
    , KillZ(-10.f)
    , Score(0)
{
    
}

void AFish::PostSpawnInitialize()
{
    Super::PostSpawnInitialize();
    
    SphereComponent = AddComponent<USphereComponent>(FName("SphereComponent_0"));
    SetRootComponent(SphereComponent);

    FishBody = AddComponent<UFishBodyComponent>(FName("FishBodyComponent_0"));
    FishBody->SetupAttachment(SphereComponent);
    
    FishTail = AddComponent<UFishTailComponent>(FName("FishTailComponent_0"));
    FishTail->SetupAttachment(FishBody);

    USpringArmComponent* SpringArmComp = AddComponent<USpringArmComponent>(FName("SpringArmComponent_0"));
    SpringArmComp->SetComponentRotation(FRotator(0.f, 0.f, 0.f));
    SpringArmComp->SetupAttachment(SphereComponent);

    UCameraComponent* CameraComp = AddComponent<UCameraComponent>(FName("CameraComponent_0"));
    CameraComp->SetupAttachment(SpringArmComp);
}

UObject* AFish::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));

    return NewActor;
}

void AFish::BeginPlay()
{
    APlayer::BeginPlay();

    Velocity = FVector(0.f, 0.f, JumpZVelocity);

    OnActorBeginOverlap.AddLambda(
        [this](AActor* OverlappedActor, AActor* OtherActor)
        {
            ActorBeginOverlap(OverlappedActor, OtherActor);
        }
    );

    OnHealthChanged.AddLambda(
        [this](int32 InCurrentHealth, int32 InMaxHealth)
        {
            const float HealthPercent = static_cast<float>(InCurrentHealth) / static_cast<float>(InMaxHealth);
            if (UFishTailComponent* TailComp = GetComponentByClass<UFishTailComponent>())
            {
                TailComp->SetCurrentYaw(TailComp->GetMaxYaw() * HealthPercent);
                TailComp->SetCurrentFrequency(TailComp->GetMaxFrequency() * HealthPercent);
            }

            MeshPitch = MeshPitchMax * HealthPercent;
        }
    );

    OnDied.AddLambda(
        [this](bool bZeroHealth)
        {
            if (bZeroHealth)
            {
                if (UFishBodyComponent* MeshComp = GetComponentByClass<UFishBodyComponent>())
                {
                    MeshComp->SetStaticMesh(FObjManager::GetStaticMesh(L"Contents/FishDish/FishDish.obj"));
                }
                
                if (UFishTailComponent* TailComp = GetComponentByClass<UFishTailComponent>())
                {
                    TailComp->SetRelativeScale3D(FVector(0.01f,0.01f,0.01f));
                }
            }
        }
    );
}

void AFish::Tick(float DeltaTime)
{
    APlayer::Tick(DeltaTime);

    if (bShouldApplyGravity)
    {
        Move(DeltaTime);
    }

    RotateMesh();
}

void AFish::SetHealth(int32 InHealth, bool bShouldNotify)
{
    Health = FMath::Max(0, FMath::Min(InHealth, MaxHealth));

    OnHealthChanged.Broadcast(GetHealth(), GetMaxHealth());

    if (IsDead() && bShouldNotify)
    {
        OnDied.Broadcast(true);
    }
}

void AFish::SetMaxHealth(int32 InMaxHealth)
{
    MaxHealth = InMaxHealth;

    SetHealth(GetHealth());
}

void AFish::Reset()
{
    SetMaxHealth(20);
    SetHealth(GetMaxHealth());
    bShouldApplyGravity = true;
    SetScore(0);
    Velocity.Z = JumpZVelocity;
    
    if (UFishTailComponent* TailComp = GetComponentByClass<UFishTailComponent>())
    {
        TailComp->SetRelativeScale3D(FVector(1.f,1.f,1.f));
    }
    
    if (UFishBodyComponent* MeshComp = GetComponentByClass<UFishBodyComponent>())
    {
        MeshComp->SetStaticMesh(FObjManager::GetStaticMesh(L"Contents/Fish/Fish_Front.obj"));
    }
    SetActorLocation(FVector(0, 0, 0));
    SetActorRotation(FRotator(0.0f, 0.0f, 0.0f));
}

void AFish::Move(float DeltaTime)
{
    FVector NextVelocity = Velocity;
    NextVelocity.Z += Gravity * DeltaTime;

    Velocity = NextVelocity;
    
    FVector NewLocation = GetActorLocation() + Velocity * DeltaTime;
    SetActorLocation(NewLocation);

    if (NewLocation.Z < KillZ)
    {
        SetHealth(0, false);
        OnDied.Broadcast(false);
        bShouldApplyGravity = false;
        GetWorld()->GetGameMode()->EndMatch(false);
    }
}

void AFish::RotateMesh()
{
    const float VelocityZ = Velocity.Z;

    float RotFactor = FMath::Clamp(VelocityZ, -MeshRotSpeed, MeshRotSpeed);
    
    if (UFishBodyComponent* MeshComp = GetComponentByClass<UFishBodyComponent>())
    {
        // 현재 PIE 모드에서 맴버 변수를 접근할 수 없기 때문에 이렇게 접근 함.
        FRotator CompRotation = MeshComp->GetRelativeRotation();
        CompRotation.Roll = RotFactor * MeshPitch * -1.f;
        
        MeshComp->SetRelativeRotation(CompRotation);
    }
}

void AFish::ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
    if (OtherActor->IsA<APlatformActor>())
    {
        FSoundManager::GetInstance().PlaySound("sizzle");

        GetWorld()->GetPlayerController()->ClientStartCameraShake(UDamageCameraShake::StaticClass());

        if (IsDead())
        {
            GetWorld()->GetGameMode()->EndMatch(false);
            Velocity.Z = 0.f;
            bShouldApplyGravity = false;
        }
        else
        {
            
            Velocity.Z = JumpZVelocity;
        }

        SetHealth(GetHealth() - 1);
        GetWorld()->GetPlayerController()->PlayerCameraManager->VignetteColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
        GetWorld()->GetPlayerController()->PlayerCameraManager->StartVignetteAnimation(1.0f, 0.0f, 0.3f);
        /* DEPRECATED
        if (GetWorld() && GetWorld()->GetMainCamera())
        {
            //GetWorld()->GetMainCamera()->CameraZ = GetActorLocation().Z;
        }
        */
    }
    else if (OtherActor->IsA<AItemActor>() && !OtherActor->IsHidden())
    {
        ++Score;
        OtherActor->SetHidden(true);
        
    }else if (OtherActor->IsA<AGoalPlatformActor>())
    {
        if (AGameMode* GameMode = GEngine->ActiveWorld->GetGameMode())
        {
            bShouldApplyGravity = false;
            GameMode->EndMatch(true);
        }
    }else if (OtherActor->IsA<ATriggerBox>())
    {
        
        // /** Camera does a simple linear interpolation. */
        // VTBlend_Linear
        // /** Camera has a slight ease in and ease out, but amount of ease cannot be tweaked. */
        // VTBlend_Cubic
        // /** Camera immediately accelerates, but smoothly decelerates into the target.  Ease amount controlled by BlendExp. */
        // VTBlend_EaseIn
        // /** Camera smoothly accelerates, but does not decelerate into the target.  Ease amount controlled by BlendExp. */
        // VTBlend_EaseOut
        // /** Camera smoothly accelerates and decelerates.  Ease amount controlled by BlendExp. */
        // VTBlend_EaseInOut

        FViewTargetTransitionParams Params;
        Params.BlendTime = 5.0f;
        Params.BlendFunction = VTBlend_EaseIn;
        Params.BlendExp = 3.f;

        FVector TargetLoc = OtherActor->GetActorLocation() + FVector(50, 0, 0);
        
        AActor* TargetActor = GEngine->ActiveWorld->SpawnActor<AActor>();
        TargetActor->SetActorLocation(TargetLoc);
        TargetActor->SetActorRotation(FRotator(90.f, 0.f, 0.f));
        GEngine->ActiveWorld->GetPlayerController()->SetViewTarget(TargetActor, Params);
    }
}
