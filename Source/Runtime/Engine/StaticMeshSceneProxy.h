#pragma once
#include "PrimitiveSceneProxy.h"

class UStaticMesh;
class UStaticMeshComponent;

// 스태틱 메시 컴포넌트의 프록시. 에셋(UStaticMesh)의 정점을 그대로 그리므로 에셋을 참조하고,
// 섹션별 머티리얼과 LOD 정보를 RenderState로 캐시해 둔다
class FStaticMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
	explicit FStaticMeshSceneProxy(UStaticMeshComponent* InComponent);
	virtual ~FStaticMeshSceneProxy() override;

	FStaticMeshSceneProxy(const FStaticMeshSceneProxy&) = delete;
	FStaticMeshSceneProxy& operator=(const FStaticMeshSceneProxy&) = delete;

	virtual void UpdateTransform() override;
	virtual void UpdateRenderState() override;
	virtual bool HasRenderMesh() const override { return Mesh != nullptr; }
	virtual const FStaticMeshSceneProxy* AsStaticMeshProxy() const override { return this; }

	// UStaticMesh가 렌더 데이터를 바꾸거나 사라질 때 알려 준다
	void OnMeshRenderDataChanged();
	void OnMeshDestroyed();

	UStaticMesh* GetMesh() const { return Mesh; }
	const FLODSphere& GetLODSphere() const { return LODSphere; }
	uint8 GetLODCount() const { return RenderState ? RenderState->LODCount : 1; }
	const FMeshRenderState* GetRenderState() const { return RenderState.get(); }
	const float* GetLODThresholdsSq() const { return RenderState->LODThresholdSq; }
	const FCachedMeshLOD& GetLOD(uint32 Index) const { return RenderState->LODs[Index]; }
	const FCachedMeshSection& GetSection(uint32 Index) const { return RenderState->Sections[Index]; }

protected:
	virtual FBox GetLocalBounds() const override;

private:
	UStaticMeshComponent* StaticMeshComponent = nullptr;
	UStaticMesh* Mesh = nullptr;
	TSharedPtr<const FMeshRenderState> RenderState;
	FLODSphere LODSphere;
};
