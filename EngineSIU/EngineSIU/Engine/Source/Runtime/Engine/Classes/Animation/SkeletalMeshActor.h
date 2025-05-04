#pragma once
#include "GameFramework/Actor.h"

class USkeletalMeshComponent;

class ASkeletalMeshActor : public AActor
{
    DECLARE_CLASS(ASkeletalMeshActor, AActor)

public:
    ASkeletalMeshActor();
    virtual ~ASkeletalMeshActor() override = default;

protected:
    UPROPERTY
    (USkeletalMeshComponent*, SkeletalMeshComponent, = nullptr)
};
