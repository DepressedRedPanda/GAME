#pragma once

#include "CoreMinimal.h"

class UStaticMesh;
class UMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Greybox helpers. Everything in Hollowreach can be played with engine basic shapes so
 * systems can be tested before art exists. Swap meshes/materials in Blueprint subclasses later.
 */
namespace HRGreybox
{
	HOLLOWREACH_API UStaticMesh* Cube();
	HOLLOWREACH_API UStaticMesh* Sphere();
	HOLLOWREACH_API UStaticMesh* Cylinder();
	HOLLOWREACH_API UStaticMesh* Cone();

	/** Applies a tinted dynamic instance of the engine's BasicShapeMaterial. */
	HOLLOWREACH_API UMaterialInstanceDynamic* Tint(UMeshComponent* Mesh, const FLinearColor& Color);
}
