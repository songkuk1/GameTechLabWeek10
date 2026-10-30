#include "EnginePCH.h"
#include "FBXImporter.h"

#include "Core/EngineLog.h"
#include "Render/StaticMeshData.h"
#include "Render/Vertex.h"
#include "Asset/StaticMeshBake.h"
#include "Containers/Map.h"
#include <algorithm>
#include <filesystem>

namespace
{
	// OpenGL 축(오른손, Y-Up)으로 맞춘 FBX → 엔진 (왼손, X-Forward, Y-Right, Z-Up)
	// ObjImporter의 ToEngine과 같은 규칙이다. 삼각형 winding도 함께 뒤집어야 한다
	FVector ToEngine(const FbxVector4& V)
	{
		return FVector(static_cast<float>(-V[2]), static_cast<float>(V[0]), static_cast<float>(V[1]));
	}

	// 노드를 돌면서 모은 머티리얼과 슬롯별 인덱스. 순회가 끝나면 슬롯 순서대로 이어 붙여 섹션을 만든다
	struct FFbxCookContext
	{
		FString FbxPath;
		TMap<FbxSurfaceMaterial*, uint32> MaterialToSlot; // 같은 머티리얼을 쓰는 노드끼리 슬롯을 공유한다
		TArray<TArray<uint32>> SlotIndices;               // [슬롯] = 그 슬롯을 쓰는 삼각형 인덱스
	};

	// FBX의 텍스처 경로는 만든 PC 기준 절대 경로인 경우가 많다 (예: C:\Projects\2D\crateColor.png)
	// FBX 파일 위치를 기준으로 실제 파일을 찾아 저장소 루트 기준 경로로 바꾼다
	FString ResolveTexturePath(const FString& FbxPath, const FbxFileTexture* Texture)
	{
		namespace fs = std::filesystem;

		const fs::path FbxDir = fs::path(FbxPath).parent_path();
		const fs::path Relative(Texture->GetRelativeFileName());
		const fs::path FileName = fs::path(Texture->GetFileName()).filename();

		TArray<fs::path> Candidates;
		if (!Relative.empty())
		{
			Candidates.Add(FbxDir / Relative);
		}
		if (!FileName.empty())
		{
			Candidates.Add(FbxDir / FileName);
			Candidates.Add(FbxDir / "textures" / FileName);
			Candidates.Add(FbxDir.parent_path() / "textures" / FileName);
		}
		for (const fs::path& Candidate : Candidates)
		{
			std::error_code Ec;
			if (fs::is_regular_file(Candidate, Ec))
			{
				return Candidate.lexically_normal().generic_string();
			}
		}

		// 마지막으로 FBX 상위 폴더 아래에서 같은 파일 이름을 찾는다
		const fs::path SearchRoot = FbxDir.parent_path();
		std::error_code Ec;
		if (!FileName.empty() && fs::is_directory(SearchRoot, Ec))
		{
			for (const fs::directory_entry& Entry : fs::recursive_directory_iterator(SearchRoot, Ec))
			{
				if (Entry.is_regular_file() && Entry.path().filename() == FileName)
				{
					return Entry.path().lexically_normal().generic_string();
				}
			}
		}

		HTR_LOG(Warning, "[FBX] {}: texture '{}' not found", FbxPath, Texture->GetFileName());
		return {};
	}

	// 머티리얼 프로퍼티(Diffuse, NormalMap 등)에 연결된 텍스처 파일 경로. 없으면 빈 문자열
	FString FindTexturePath(const FString& FbxPath, const FbxSurfaceMaterial* Material, const char* PropertyName)
	{
		const FbxProperty Property = Material->FindProperty(PropertyName);
		if (!Property.IsValid())
		{
			return {};
		}

		const FbxFileTexture* Texture = Property.GetSrcObject<FbxFileTexture>(0);
		return Texture ? ResolveTexturePath(FbxPath, Texture) : FString();
	}

	// FBX 머티리얼 -> 엔진 머티리얼 슬롯. OBJ의 Kd, Ks, Ns, map_Kd, map_Bump에 해당하는 값을 읽는다
	FStaticMaterialSlot MakeMaterialSlot(const FString& FbxPath, const FbxSurfaceMaterial* Material)
	{
		FStaticMaterialSlot Slot;
		Slot.Name = Material->GetName();

		// Phong도 Lambert를 상속하므로 Diffuse는 여기서 같이 읽힌다
		// 알파는 1로 둔다. TransparencyFactor는 툴마다 의미가 달라 잘못 읽으면 불투명한 메시가 반투명이 된다
		if (const FbxSurfaceLambert* Lambert = FbxCast<FbxSurfaceLambert>(Material))
		{
			const FbxDouble3 Diffuse = Lambert->Diffuse.Get();
			const double Factor = Lambert->DiffuseFactor.Get();
			Slot.BaseColor = FLinearColor(
				static_cast<float>(Diffuse[0] * Factor),
				static_cast<float>(Diffuse[1] * Factor),
				static_cast<float>(Diffuse[2] * Factor),
				1.0f);
		}
		if (const FbxSurfacePhong* Phong = FbxCast<FbxSurfacePhong>(Material))
		{
			const FbxDouble3 Specular = Phong->Specular.Get();
			const double Factor = Phong->SpecularFactor.Get();
			Slot.SpecularColor = FLinearColor(
				static_cast<float>(Specular[0] * Factor),
				static_cast<float>(Specular[1] * Factor),
				static_cast<float>(Specular[2] * Factor),
				1.0f);
			// 0은 pow(x, 0) = 1이 되어 면 전체가 하이라이트로 덮인다 (OBJ의 Ns와 같은 처리)
			Slot.Shininess = std::max(static_cast<float>(Phong->Shininess.Get()), 1.0f);
		}

		Slot.DiffuseTexturePath = FindTexturePath(FbxPath, Material, FbxSurfaceMaterial::sDiffuse);
		Slot.NormalTexturePath = FindTexturePath(FbxPath, Material, FbxSurfaceMaterial::sNormalMap);
		if (Slot.NormalTexturePath.empty())
		{
			// OBJ의 map_Bump처럼 Bump 슬롯에 노멀맵을 꽂는 툴도 있다
			Slot.NormalTexturePath = FindTexturePath(FbxPath, Material, FbxSurfaceMaterial::sBump);
		}

		// 엔진은 BaseColor × 텍스처로 그린다. Diffuse 텍스처가 있으면 색 값(툴 기본값 0.8 등)은 무시하고 텍스처 색 그대로 쓴다 (언리얼과 같은 처리)
		if (!Slot.DiffuseTexturePath.empty())
		{
			Slot.BaseColor = FLinearColor(1.0f, 1.0f, 1.0f, Slot.BaseColor.A);
		}
		return Slot;
	}

	// 머티리얼에 해당하는 슬롯 번호. 처음 보는 머티리얼이면 슬롯을 새로 만든다. 머티리얼이 없으면 0번(Default)
	uint32 FindOrAddSlot(FFbxCookContext& Context, FStaticMeshData& Out, FbxSurfaceMaterial* Material)
	{
		if (!Material)
		{
			return 0;
		}
		if (const uint32* Found = Context.MaterialToSlot.Find(Material))
		{
			return *Found;
		}

		const uint32 Slot = static_cast<uint32>(Out.MaterialSlots.Num());
		Out.MaterialSlots.Add(MakeMaterialSlot(Context.FbxPath, Material));
		Context.SlotIndices.Add(TArray<uint32>());
		Context.MaterialToSlot.Add(Material, Slot);
		return Slot;
	}

	// 폴리곤이 쓰는 머티리얼의 노드 내 번호 (Node->GetMaterial의 인덱스). 정보가 없으면 -1
	int32 GetPolygonMaterialIndex(FbxMesh* Mesh, int32 Poly)
	{
		const FbxGeometryElementMaterial* MaterialElement = Mesh->GetElementMaterial();
		if (!MaterialElement)
		{
			return -1;
		}

		const FbxLayerElementArrayTemplate<int>& IndexArray = MaterialElement->GetIndexArray();
		switch (MaterialElement->GetMappingMode())
		{
		case FbxGeometryElement::eAllSame:   // 메시 전체가 머티리얼 하나
			return IndexArray.GetCount() > 0 ? IndexArray.GetAt(0) : -1;
		case FbxGeometryElement::eByPolygon: // 폴리곤마다 머티리얼 번호
			return Poly < IndexArray.GetCount() ? IndexArray.GetAt(Poly) : -1;
		default:
			return -1;
		}
	}

	// 노드 트리를 돌면서 메시마다 월드 변환과 엔진 축 변환을 적용해 Out에 쌓는다
	// 정점은 Out.Vertices에 바로 쌓고, 인덱스는 머티리얼 슬롯별로 Context.SlotIndices에 모은다
	void CookNode(FbxNode* Node, FFbxCookContext& Context, FStaticMeshData& Out)
	{
		if (FbxMesh* Mesh = Node->GetMesh())
		{
			// ConvertScene은 정점이 아니라 노드 트랜스폼을 바꾼다. 월드 행렬을 곱해야 축 변환이 정점에 반영된다
			const FbxAMatrix World = Node->EvaluateGlobalTransform();

			// 노멀은 이동을 뺀 행렬의 역전치로 변환한다 (비균등 스케일에서도 면에 수직 유지)
			FbxAMatrix NormalMatrix = World;
			NormalMatrix.SetT(FbxVector4(0.0, 0.0, 0.0, 0.0));
			NormalMatrix = NormalMatrix.Inverse().Transpose();

			//ControlPoint : 메시의 고유한 위치 목록. 위치만 있다(로컬 위치임)
			const FbxVector4* ControlPoints = Mesh->GetControlPoints();

			FbxStringList UVSetNames;
			Mesh->GetUVSetNames(UVSetNames);
			// operator[]는 FbxString을 돌려준다. 삼항 연산자에 그대로 넣으면 임시 FbxString이 생겼다 사라져
			// 해제된 버퍼를 가리키게 되므로, 리스트 안의 원본 문자열 버퍼를 직접 꺼낸다
			const char* UVSetName = UVSetNames.GetCount() > 0 ? UVSetNames[0].Buffer() : nullptr;

			TArray<uint32> Corners; // 현재 폴리곤의 꼭짓점별 정점 인덱스
			for (int32 Poly = 0; Poly < Mesh->GetPolygonCount(); ++Poly)
			{
				Corners.Reset();
				//이 면의 꼭짓점 개수
				const int32 PolySize = Mesh->GetPolygonSize(Poly);

				// 이 면이 쓰는 머티리얼 → 엔진 슬롯. 번호는 노드마다 따로 매겨지므로 머티리얼 포인터로 전체 슬롯에 매핑한다
				const int32 MaterialIndex = GetPolygonMaterialIndex(Mesh, Poly);
				FbxSurfaceMaterial* Material = (MaterialIndex >= 0 && MaterialIndex < Node->GetMaterialCount())
					? Node->GetMaterial(MaterialIndex)
					: nullptr;
				const uint32 Slot = FindOrAddSlot(Context, Out, Material);

				for (int32 Corner = 0; Corner < PolySize; ++Corner)
				{
					const int32 ControlPointIndex = Mesh->GetPolygonVertex(Poly, Corner);

					FVertexPNCT Vertex;
					//MultT : 행렬로 점 하나를 변환하는 함수 로컬 -> 월드로 변환에서 버텍스에 넣어준다.
					Vertex.Position = ToEngine(World.MultT(ControlPoints[ControlPointIndex]));

					FbxVector4 Normal;
					if (Mesh->GetPolygonVertexNormal(Poly, Corner, Normal))
					{
						FbxVector4 WorldNormal = NormalMatrix.MultT(Normal);
						WorldNormal.Normalize();
						Vertex.Normal = ToEngine(WorldNormal);
					}

					FbxVector2 UV;
					bool bUnmapped = true;
					if (UVSetName && Mesh->GetPolygonVertexUV(Poly, Corner, UVSetName, UV, bUnmapped) && !bUnmapped)
					{
						// OBJ와 같이 V를 뒤집는다 (FBX는 아래가 0, 엔진은 위가 0)
						Vertex.UV = FVector2(static_cast<float>(UV[0]), 1.0f - static_cast<float>(UV[1]));
					}
					Vertex.Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

					Corners.Add(static_cast<uint32>(Out.Vertices.Num()));
					Out.Vertices.Add(std::move(Vertex));
				}

				// 삼각형 분할 (fan). FBX는 CCW, 엔진은 CW가 앞면이라 (0, k, k+1) → (0, k+1, k)
				// 슬롯별로 모아 두었다가 Cook에서 섹션으로 이어 붙인다
				TArray<uint32>& SlotIndices = Context.SlotIndices[Slot];
				for (int32 k = 1; k + 1 < Corners.Num(); ++k)
				{
					SlotIndices.Add(Corners[0]);
					SlotIndices.Add(Corners[k + 1]);
					SlotIndices.Add(Corners[k]);
				}


			}
		}

		for (int32 i = 0; i < Node->GetChildCount(); ++i)
		{
			CookNode(Node->GetChild(i), Context, Out);
		}
	}
}


TUniquePtr<FStaticMeshData> FFBXImporter::LoadStaticMeshData(const FString& Path)
{
	const FString BinPath = Path + ".bin";

	// .bin 파일 읽기
	if (TUniquePtr<FStaticMeshData> Baked = FStaticMeshBake::ReadBaked(BinPath))
	{
		return Baked;
	}

	//FBX매니저 생성 모든 SDK 객체는 Manager로부터 만들어진다
	FbxManager* Manager = FbxManager::Create();
	FbxScene* Scene = nullptr;
	TUniquePtr<FStaticMeshData> Data = MakeUnique<FStaticMeshData>();

	const bool bOk = ParseFBX(Manager, Path, Scene) && Cook(Path, Scene, *Data);

	// Cook에서 필요한 값을 전부 FStaticMeshData로 복사했으니 SDK 객체를 한 번에 해제한다
	Manager->Destroy();

	if (!bOk)
	{
		return nullptr;
	}

	FStaticMeshBake::WriteBaked(BinPath, *Data);
	return Data;
}

bool FFBXImporter::ParseFBX(FbxManager* Manager, const FString& Path, FbxScene*& OutScene)
{
    //임포트/익스포트 옵션 객체
    FbxIOSettings* IOS = FbxIOSettings::Create(Manager, IOSROOT);
    //만든 설정을 매니저에 등록
    Manager->SetIOSettings(IOS);
    //임포터 생성
    FbxImporter* Importer = FbxImporter::Create(Manager, "");
    //파일을 읽고 포맷을 판별,실패하면 false를 반환
    if (!Importer->Initialize(Path.c_str(), -1, Manager->GetIOSettings()))
    {
        HTR_LOG(Warning, "[FBX] {}: {}", Path, Importer->GetStatus().GetErrorString());
        Importer->Destroy();
        return false;
    }
    //읽을 데이터를 담을 빈 씬을 생성 (참조로 받았으므로 호출한 쪽 포인터가 바뀐다)
    OutScene = FbxScene::Create(Manager, "Scene");
    //담기
    const bool bImported = Importer->Import(OutScene);
    if (!bImported)
    {
        HTR_LOG(Warning, "[FBX] {}: {}", Path, Importer->GetStatus().GetErrorString());
    }
    //임포트 파괴
    Importer->Destroy();

    return bImported;
}

bool FFBXImporter::Cook(const FString& Path, FbxScene* Scene, FStaticMeshData& Out)
{
    FStaticMeshData Cooked;

    // 파일 축이 무엇이든 OBJ와 같은 OpenGL 축(오른손, Y-Up)으로 맞춘다
    // 왼손으로의 변환은 거울 반전이라 노드 트랜스폼에 넣으면 노멀,winding이 꼬인다. 그래서 ToEngine에서 한다 (언리얼도 SDK로는 오른손까지만 바꾼다)
    if (Scene->GetGlobalSettings().GetAxisSystem() != FbxAxisSystem::OpenGL)
    {
        FbxAxisSystem::OpenGL.ConvertScene(Scene);
    }

    // 엔진은 m 단위다. cm로 저장된 파일(Max, Maya, 많은 Blender 내보내기)은 100배 크게 들어오므로 m로 맞춘다
    // 축 변환과 마찬가지로 노드 트랜스폼에 스케일이 들어가므로 정점은 CookNode의 월드 행렬을 거쳐야 반영된다
    if (Scene->GetGlobalSettings().GetSystemUnit() != FbxSystemUnit::m)
    {
        FbxSystemUnit::m.ConvertScene(Scene);
    }

	// 오목 다각형도 올바르게 삼각형으로 바꾼다. 이후 모든 폴리곤은 꼭짓점이 3개다
	FbxGeometryConverter Converter(Scene->GetFbxManager());
	if (!Converter.Triangulate(Scene, true))
	{
		HTR_LOG(Warning, "[FBX] triangulate failed, using fan triangulation");
	}


    // 0번 슬롯은 항상 Default. 머티리얼이 없는 메시·폴리곤이 여기에 들어간다 (OBJ와 같은 규칙)
    FFbxCookContext Context;
    Context.FbxPath = Path;
    FStaticMaterialSlot DefaultSlot;
    DefaultSlot.Name = "Default";
    Cooked.MaterialSlots.Add(std::move(DefaultSlot));
    Context.SlotIndices.Add(TArray<uint32>());

    CookNode(Scene->GetRootNode(), Context, Cooked);

    if (Cooked.Vertices.IsEmpty())
    {
        HTR_LOG(Error, "[FBX] {}: no mesh", Path);
        return false;
    }

    // 슬롯별 인덱스를 슬롯 순서대로 이어 붙이고, 이어 붙인 구간마다 섹션을 하나 만든다
    // Validate가 섹션 인덱스 합 == 전체 인덱스 수, 슬롯 번호 < 슬롯 수를 검사한다
    for (uint32 Slot = 0; Slot < static_cast<uint32>(Context.SlotIndices.Num()); ++Slot)
    {
        const TArray<uint32>& SlotIndices = Context.SlotIndices[Slot];
        if (SlotIndices.IsEmpty())
        {
            continue;
        }

        FStaticMeshSection Section;
        Section.StartIndex = static_cast<uint32>(Cooked.Indices.Num());
        Section.IndexCount = static_cast<uint32>(SlotIndices.Num());
        Section.MaterialSlotIndex = Slot;
        Cooked.Indices.Append(SlotIndices);
        Cooked.Sections.Add(Section);
    }

    Cooked.CalculateTangents(Cooked.Vertices, Cooked.Indices);

    // AABB (변환이 끝난 Position 기준)
    FVector Min = Cooked.Vertices[0].Position;
    FVector Max = Min;
    for (const FVertexPNCT& Vertex : Cooked.Vertices)
    {
        Min.X = std::min(Min.X, Vertex.Position.X);
        Min.Y = std::min(Min.Y, Vertex.Position.Y);
        Min.Z = std::min(Min.Z, Vertex.Position.Z);
        Max.X = std::max(Max.X, Vertex.Position.X);
        Max.Y = std::max(Max.Y, Vertex.Position.Y);
        Max.Z = std::max(Max.Z, Vertex.Position.Z);
    }
    Cooked.AABB.Min = Min;
    Cooked.AABB.Max = Max;

    FString Error;
    if (!Cooked.Validate(Error))
    {
        HTR_LOG(Error, "[FBX] {}", Error);
        return false;
    }

    Out = std::move(Cooked);
    return true;
}
