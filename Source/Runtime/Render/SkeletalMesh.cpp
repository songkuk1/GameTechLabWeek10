#include "EnginePCH.h"
#include "SkeletalMesh.h"

void FReferenceSkeleton::Build(const TArray<FMeshBoneInfo>& Bones, const TArray<FMatrix>& BindPoses)
{
	RawRefBoneInfo = Bones;
	RawRefBonePose.Reset();
	RawRefBonePose.Reserve(static_cast<uint32>(Bones.Num()));

	for (int32 i = 0; i < static_cast<int32>(Bones.Num()); ++i)
	{
		// 행 벡터라 월드 = 로컬 * 부모월드. 로컬 = 월드 * 부모월드^-1
		const int32 Parent = Bones[i].ParentIndex;
		const FMatrix Local = Parent >= 0 ? BindPoses[i] * BindPoses[Parent].Inverse() : BindPoses[i];

		// FTransform::GetLocalMatrix의 역순으로 분해한다. 행 0~2 = 회전 행 * 스케일, 행 3 = 위치
		FVector Scale(
			FVector(Local.M[0][0], Local.M[0][1], Local.M[0][2]).Length(),
			FVector(Local.M[1][0], Local.M[1][1], Local.M[1][2]).Length(),
			FVector(Local.M[2][0], Local.M[2][1], Local.M[2][2]).Length());

		FMatrix Rotation = Local;
		const float Scales[3] = { Scale.X, Scale.Y, Scale.Z };
		for (int32 Row = 0; Row < 3; ++Row)
		{
			if (Scales[Row] > 0.0f)
			{
				for (int32 Col = 0; Col < 3; ++Col)
				{
					Rotation.M[Row][Col] /= Scales[Row];
				}
			}
		}

		const FVector Location(Local.M[3][0], Local.M[3][1], Local.M[3][2]);
		RawRefBonePose.Add(FTransform(MatrixToRotator(Rotation), Location, Scale));
	}
}

int32 FReferenceSkeleton::FindBoneIndex(const FName& BoneName) const
{
	for (int32 i = 0; i < static_cast<int32>(RawRefBoneInfo.Num()); ++i)
	{
		if (RawRefBoneInfo[i].Name == BoneName)
		{
			return i;
		}
	}
	return -1;
}

void USkeletalMesh::SetSkeletalMeshData(TUniquePtr<FSkeletalMeshData> InData)
{
	SkeletalMeshData = std::move(InData);
	ReferenceSkeleton = MakeUnique<FReferenceSkeleton>();
	if (SkeletalMeshData)
	{
		ReferenceSkeleton->Build(SkeletalMeshData->Bones, SkeletalMeshData->BindPoses);
	}
}

UMaterial* USkeletalMesh::GetMaterial(uint32 SlotIndex) const
{
	return SlotIndex < static_cast<uint32>(Materials.Num()) ? Materials[SlotIndex] : nullptr;
}
