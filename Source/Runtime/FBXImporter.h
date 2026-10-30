#pragma once
#include "EnginePCH.h"
#include "FBXInfo.h"
#include <fbxsdk.h>

struct FStaticMeshData;

class FFBXImporter
{
public:
	FFBXImporter() = default;
	~FFBXImporter() = default;

	FFBXImporter(const FFBXImporter&) = delete;
	FFBXImporter& operator=(const FFBXImporter&) = delete;
private:
	// .fbx 파일 경로 -> FbxScene 변환
	static bool ParseFBX(const FString& Path, FbxScene* Out);
	// FbxScene -> FStaticMeshData 변환 (축 변환이 씬을 수정하므로 const가 아니다)
	static bool Cook(FbxScene* Scene, FStaticMeshData& Out);
};