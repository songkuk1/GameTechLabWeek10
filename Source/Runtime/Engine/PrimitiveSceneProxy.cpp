#include "EnginePCH.h"
#include "PrimitiveSceneProxy.h"
#include "Component/PrimitiveComponent.h"
#include "Render/Renderer.h"
#include "Engine/Scene.h"

void FPrimitiveSceneProxy::UpdateTransform()
{
    LocalToWorld = Component->GetWorldMatrix();
    WorldToLocal = LocalToWorld.Inverse();
    NormalMatrix = MakeNormalMatrix(LocalToWorld);
    Bounds = MakeWorldBounds(GetLocalBounds().GetWorldAABB(LocalToWorld));
}

void FPrimitiveSceneProxy::UpdateRenderState()
{
    bVisible = Component->IsVisible();
    bCastShadow = Component->CastsShadow();
    bReceiveShadow = Component->ReceivesShadow();
}

FBox FPrimitiveSceneProxy::GetLocalBounds() const
{
    return Component->CalcLocalBounds();
}
