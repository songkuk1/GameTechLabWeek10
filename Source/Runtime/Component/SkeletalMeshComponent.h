#pragma once
#include "MeshComponent.h"
#include "Render/SkeletalMesh.h"

class USkeletalMeshComponent : public UMeshComponent
{
	DECLARE_CLASS(USkeletalMeshComponent, UMeshComponent)

	REFLECT_START(USkeletalMeshComponent)
		PROPERTY(SkeletalMesh)
		FUNCTION(GetSkeletalMesh)
		FUNCTION(SetSkeletalMesh)
		REFLECT_END()

public:
	USkeletalMeshComponent() = default;
	virtual ~USkeletalMeshComponent() = default;

	void SetSkeletalMesh(USkeletalMesh* InSkeletalMesh);
	USkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }

private:
	USkeletalMesh* SkeletalMesh = nullptr;
};