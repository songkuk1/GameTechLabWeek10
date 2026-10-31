#pragma once
#include "PrimitiveSceneProxy.h"

class USkeletalMeshComponent;
class USkeletalMesh;
class FSkeletalMeshObject;
class FVertexBuffer;
class FIndexBuffer;

// 스켈레탈 메시 컴포넌트의 프록시.
// 섹션·인덱스·머티리얼은 에셋(USkeletalMesh)에서, 정점은 컴포넌트의 스키닝 결과(FSkeletalMeshObject)에서 가져온다
// 같은 에셋을 쓰는 컴포넌트도 자세가 달라 정점이 다르기 때문이다
// 메시가 바뀌어도 프록시는 다시 만들지 않으므로, 메시는 생성자가 아니라 UpdateRenderState에서 컴포넌트로부터 받는다
class FSkeletalMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
	explicit FSkeletalMeshSceneProxy(USkeletalMeshComponent* InComponent);
	virtual ~FSkeletalMeshSceneProxy() override = default;

	FSkeletalMeshSceneProxy(const FSkeletalMeshSceneProxy&) = delete;
	FSkeletalMeshSceneProxy& operator=(const FSkeletalMeshSceneProxy&) = delete;

	virtual void UpdateRenderState() override;
	virtual bool HasRenderMesh() const override { return SkeletalMesh != nullptr && GetVertexBuffer() != nullptr; }

	USkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }
	// 섹션별 인덱스 범위와 실제로 적용될 머티리얼(컴포넌트 덮어쓰기 반영). 머티리얼이 없는 섹션은 빠진다
	const TArray<FCachedMeshSection>& GetSections() const { return Sections; }
	// 스키닝된 정점. 매 프레임 내용이 바뀌지만 버퍼 객체는 메시가 바뀔 때만 바뀐다
	FVertexBuffer* GetVertexBuffer() const;
	FIndexBuffer* GetIndexBuffer() const;

protected:
	// 바인드 포즈 AABB. 애니메이션으로 바운드를 벗어나는 만큼은 아직 반영하지 않는다
	virtual FBox GetLocalBounds() const override;

private:
	USkeletalMeshComponent* SkeletalMeshComponent = nullptr;
	USkeletalMesh* SkeletalMesh = nullptr;
	FSkeletalMeshObject* MeshObject = nullptr;
	TArray<FCachedMeshSection> Sections;
};
