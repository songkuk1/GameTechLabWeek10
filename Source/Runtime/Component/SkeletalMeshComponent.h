#pragma once
#include "MeshComponent.h"
#include "Render/SkeletalMesh.h"
#include "Render/SkeletalMeshObject.h"

class USkeletalMeshComponent : public UMeshComponent
{
	DECLARE_CLASS(USkeletalMeshComponent, UMeshComponent)

	REFLECT_START(USkeletalMeshComponent)
		PROPERTY(SkeletalMesh)
		FUNCTION(GetSkeletalMesh)
		FUNCTION(SetSkeletalMesh)
	REFLECT_END()

public:
	USkeletalMeshComponent();
	virtual ~USkeletalMeshComponent() = default;

	USkeletalMeshComponent(const USkeletalMeshComponent&) = delete;
	USkeletalMeshComponent& operator=(const USkeletalMeshComponent&) = delete;

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual void PostEditChangeProperty(const FProperty& Property) override;

	// 메시가 바뀌면 슬롯 구성이 달라지므로 덮어쓰기를 비운다
	void SetSkeletalMesh(USkeletalMesh* InSkeletalMesh);
	USkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }
	// 이 컴포넌트의 스키닝 결과. 프록시가 여기서 정점 버퍼를 가져간다
	FSkeletalMeshObject* GetMeshObject() const { return MeshObject.get(); }

	// 바인드 포즈 정점 기준. CalcLocalBounds(컬링 바운드)도 이 데이터의 AABB를 쓴다
	virtual const FStaticMeshData* GetMeshData() const override;

	virtual int32 GetNumMaterials() const override;
	virtual FString GetMaterialSlotName(int32 SlotIndex) const override;
	virtual UMaterial* GetDefaultMaterial(int32 SlotIndex) const override;

private:
	USkeletalMesh* SkeletalMesh = nullptr;
	TUniquePtr<FSkeletalMeshObject> MeshObject;
};
