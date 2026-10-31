#pragma once

#include "Math/Frustum.h"
#include "Render/MeshRenderData.h"

class FScene;
class UPrimitiveComponent;
class FStaticMeshSceneProxy;

// 렌더 스레드(렌더러) 쪽에서 본 프리미티브. Transform, 바운드, 가시성·그림자 플래그만 가진다
// 메시가 없는 프리미티브(Shape, Billboard, TextRender)는 이 클래스를 그대로 쓰고,
// 메시를 그리는 컴포넌트는 CreateSceneProxy()에서 자식 프록시를 만든다
class FPrimitiveSceneProxy
{
public:
    explicit FPrimitiveSceneProxy(UPrimitiveComponent* InComponent) : Component(InComponent) {}
    // Scene이 부모 포인터로 delete하므로 virtual이어야 자식 소멸자가 불린다
    virtual ~FPrimitiveSceneProxy() = default;

    virtual void UpdateTransform();
    // 컴포넌트의 가시성·그림자 플래그를 읽는다. 자식은 Super를 부른 뒤 메시 데이터를 갱신한다
    virtual void UpdateRenderState();

    // 그릴 메시가 있는 프록시인가 (그림자 Receiver 판정, 컬링 바운드 출처)
    virtual bool HasRenderMesh() const { return false; }
    // 스태틱 메시 전용 경로(LOD 선택, 캐시 섹션, 오클루더)가 쓰는 다운캐스트. 아니면 nullptr
    virtual const FStaticMeshSceneProxy* AsStaticMeshProxy() const { return nullptr; }

    UPrimitiveComponent* GetComponent() const { return Component; }
    const FAABB& GetBounds() const { return Bounds; }
    const FMatrix& GetLocalToWorld() const { return LocalToWorld; }
    const FMatrix& GetWorldToLocal() const { return WorldToLocal; }
    // 노멀 변환용 World 역전치. Transform이 바뀔 때만 다시 계산한다.
    const FMatrix& GetNormalMatrix() const { return NormalMatrix; }
    bool CastsShadow() const { return bCastShadow; }
    bool ReceivesShadow() const { return bReceiveShadow; }
    bool IsVisible() const { return bVisible; }
    FScene* GetScene() const { return Scene; }

protected:
    // 월드 바운드의 출처가 되는 로컬 바운드. 기본은 컴포넌트가 계산한 값
    virtual FBox GetLocalBounds() const;

private:
    friend class FScene;
    FScene* Scene = nullptr;
    UPrimitiveComponent* Component = nullptr;
    FMatrix LocalToWorld;
    FMatrix WorldToLocal;
    FMatrix NormalMatrix;
    FAABB Bounds;
    int32 PackedIndex = INDEX_NONE;
    bool bQueuedForUpdate = false;
    bool bRenderStateQueued = false;
    bool bVisible = true;
    bool bCastShadow = true;
    bool bReceiveShadow = true;
};
