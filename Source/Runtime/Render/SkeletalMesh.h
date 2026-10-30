#pragma once
#include "EnginePCH.h"
#include "Asset/RenderAsset.h"
#include "SkeletalMeshRenderData.h"

struct FMeshBoneInfo
{


};

struct FReferenceSkeleton
{
private:

	//원본 에셋(FBX)에 실제로 있는 본
	TArray<FMeshBoneInfo> RawRefBoneInfo;
	TArray<FTransform>RawRefBonePose;

	//Raw + 사용자가 추가한 가상 본
	TArray<FMeshBoneInfo> FinalRefBoneInfo;
	TArray<FTransform>    FinalRefBonePose;

};


class USkeletalMesh : public URenderAsset
{
public:
	USkeletalMesh() = default;
	~USkeletalMesh() = default;
	USkeletalMesh(const USkeletalMesh&) = delete;
	USkeletalMesh& operator=(const USkeletalMesh&) = delete;
private:
	TUniquePtr<FSkeletalMeshRenderData> SkeletalMeshRenderData;
	//fbx상의 skeleton
	TUniquePtr<FReferenceSkeleton> ReferenceSkeleton;





};