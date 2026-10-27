#include "Core/HRGreybox.h"

#include "Components/MeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	UStaticMesh* LoadShape(TWeakObjectPtr<UStaticMesh>& Cache, const TCHAR* Path)
	{
		if (!Cache.IsValid())
		{
			Cache = LoadObject<UStaticMesh>(nullptr, Path);
		}
		return Cache.Get();
	}
}

UStaticMesh* HRGreybox::Cube()
{
	static TWeakObjectPtr<UStaticMesh> Cache;
	return LoadShape(Cache, TEXT("/Engine/BasicShapes/Cube.Cube"));
}

UStaticMesh* HRGreybox::Sphere()
{
	static TWeakObjectPtr<UStaticMesh> Cache;
	return LoadShape(Cache, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
}

UStaticMesh* HRGreybox::Cylinder()
{
	static TWeakObjectPtr<UStaticMesh> Cache;
	return LoadShape(Cache, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
}

UStaticMesh* HRGreybox::Cone()
{
	static TWeakObjectPtr<UStaticMesh> Cache;
	return LoadShape(Cache, TEXT("/Engine/BasicShapes/Cone.Cone"));
}

UMaterialInstanceDynamic* HRGreybox::Tint(UMeshComponent* Mesh, const FLinearColor& Color)
{
	if (!Mesh)
	{
		return nullptr;
	}

	static TWeakObjectPtr<UMaterialInterface> BaseMaterial;
	if (!BaseMaterial.IsValid())
	{
		BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}

	UMaterialInterface* Base = BaseMaterial.IsValid() ? BaseMaterial.Get() : Mesh->GetMaterial(0);
	if (!Base)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Mesh);
	MID->SetVectorParameterValue(TEXT("Color"), Color);
	const int32 NumSlots = FMath::Max(1, Mesh->GetNumMaterials());
	for (int32 Slot = 0; Slot < NumSlots; ++Slot)
	{
		Mesh->SetMaterial(Slot, MID);
	}
	return MID;
}
