#pragma once
#include "GameFramework/Actor.h"
#include "Component/SkeletalMeshComponent.h"


class ASkeletalMeshActor :public AActor
{
	DECLARE_CLASS(ASkeletalMeshActor, AActor)
	// SetPrimitiveType은 enum 매개변수라 등록할 수 없다
	REFLECT_START(ClassName)
		FUNCTION(GetStaticMeshComponent)
		REFLECT_END()
public:
	ASkeletalMeshActor();
	virtual ~ASkeletalMeshActor() = default;
	virtual void BeginPlay() override;

	USkeletalMeshComponent* GetStaticMeshComponent() const { return SkeletalMeshComponent; }

private:
	USkeletalMeshComponent* SkeletalMeshComponent;
};