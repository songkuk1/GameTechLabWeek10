#pragma once
#include "EnginePCH.h"

constexpr int MAX_BONE_INFLUENCES = 4;
struct FSkinWeightInfo
{
	// 영향 주는 본 인덱스 (스켈레톤 기준)
	uint8  BoneIndices[MAX_BONE_INFLUENCES] = {};
	// 각 본의 가중치, 합 = 1.0. 가중치가 큰 순서로 채우고 빈 칸은 0
	float  BoneWeights[MAX_BONE_INFLUENCES] = {};
};
