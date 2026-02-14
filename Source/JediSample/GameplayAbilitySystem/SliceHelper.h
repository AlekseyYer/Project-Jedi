#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProceduralMeshComponent.h"
#include "SliceHelper.generated.h"

UCLASS()
class JEDISAMPLE_API USliceHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Slice")
	static void CopySkeletalMeshToProceduralMesh(
		USkeletalMeshComponent* SkeletalMesh,
		UProceduralMeshComponent* ProcMesh
	);

	UFUNCTION(BlueprintCallable, Category = "Slice")
	static void UpdateProceduralMeshPositions(
		USkeletalMeshComponent* SkeletalMesh,
		UProceduralMeshComponent* ProcMesh
	);
	
};
