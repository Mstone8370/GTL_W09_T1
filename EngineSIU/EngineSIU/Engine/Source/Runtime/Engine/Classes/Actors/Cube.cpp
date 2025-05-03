#include "Cube.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Engine/AssetManager.h"

#include "GameFramework/Actor.h"

ACube::ACube()
{
    StaticMeshComponent->SetStaticMesh(UAssetManager::Get().GetStaticMesh(L"Contents/Reference/Reference.obj"));

    
}

void ACube::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    //SetActorRotation(GetActorRotation() + FRotator(0, 0, 1));

}
