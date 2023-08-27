#include "tspch.h"
#include "Model.h"
#include "Utils/Utility.h"

//#define AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS

namespace TS_ENGINE {

	Model::Model()
	{
		
	}

	Model::Model(const std::string& modelPath)
	{
		Utility::GetDirectory(modelPath, mModelDirectory);
		this->LoadModel(modelPath);
	}

	void Model::CopyFrom(Ref<Model> model)
	{
		mRendererID = model->mRendererID;
		mMaterial = model->mMaterial;
		mModelDirectory = model->mModelDirectory;
		mRootNode = model->mRootNode;
		mDefaultShader = model->mDefaultShader;
		mProcessedNodes = model->mProcessedNodes;
		mProcessedMeshes = model->mProcessedMeshes;
		mTexture = model->mTexture;
	}


	Model::~Model()
	{
		mRendererID = -1;
		mModelDirectory = "";
		mRootNode = nullptr;
		mDefaultShader = nullptr;
		mProcessedNodes.clear();
		mProcessedMeshes.clear();
		mTexture = nullptr;
	}

	void Model::LoadModel(const std::string& modelPath)
	{
		Assimp::Importer importer;
		mAssimpScene = importer.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_FlipUVs);

		if (!mAssimpScene || mAssimpScene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !mAssimpScene->mRootNode)
		{
			TS_CORE_ERROR("ERROR::ASSIMP:: {0}", importer.GetErrorString());
			return;
		}

		ProcessEmbeddedTextures();
		mRootNode = this->ProcessNode(mAssimpScene->mRootNode, mAssimpScene);
	}

	void Model::ProcessEmbeddedTextures()
	{
		for (unsigned int i = 0; i < mAssimpScene->mNumTextures; i++)
		{
			aiTexture* aiTex = mAssimpScene->mTextures[i];
			TS_CORE_INFO("Processing embedded aiTex named : {0}", aiTex->mFilename.C_Str());
			Ref<Texture2D> tex2D = Texture2D::Create(aiTex->mFilename.C_Str(), reinterpret_cast<unsigned char*>(aiTex->pcData), aiTex->mWidth);

			mProcessedEmbeddedTextures.insert(std::pair<std::string, Ref<Texture2D>>(aiTex->mFilename.C_Str(), tex2D));
		}
	}

	Ref<Node> Model::ProcessNode(aiNode* aiNode, const aiScene* scene)
	{	
		Ref<Node> node = CreateRef<Node>(aiNode->mName.C_Str());
		node->SetName(aiNode->mName.C_Str());
		
		aiVector3D pos;
		aiVector3D rot;
		aiVector3D scale;
		aiNode->mTransformation.Decompose(scale, rot, pos);
		
		node->SetPosition(Vector3(pos.x, pos.y, pos.z));
		node->SetEulerAngles(Vector3(rot.x, rot.y, rot.z));
		node->SetScale(Vector3(scale.x, scale.y, scale.z));

		for (GLuint i = 0; i < aiNode->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[aiNode->mMeshes[i]];
			Ref<Mesh> processedMesh = ProcessMesh(mesh, scene);
			mProcessedMeshes.push_back(processedMesh);
			node->AddMesh(processedMesh);
		}
		
		for (GLuint i = 0; i < aiNode->mNumChildren; i++)
		{
			node->AddChild(ProcessNode(aiNode->mChildren[i], scene).get());
		}

		mProcessedNodes.push_back(node);
		return node;
	}

	void Model::AddMaterialToDictionary(Ref<Material> material)
	{
		TS_CORE_INFO("Added material named {0} to dictionary", material->GetName().c_str());
		mProcessedMaterials.insert(std::pair<std::string, Ref<Material>>(material->GetName().c_str(), material));
	}

	Ref<Mesh> Model::ProcessMesh(aiMesh* aiMesh, const aiScene* scene)
	{	
		TS_CORE_TRACE("Processing mesh named: {0}", aiMesh->mName.C_Str());

		std::vector<Vertex> vertices;
		std::vector<GLuint> indices;
		std::vector<Texture> textures;
		
		for (GLuint i = 0; i < aiMesh->mNumVertices; i++)
		{
			Vertex vertex;
			
			Vector4 vector; 

			// Position
			vector.x = aiMesh->mVertices[i].x;
			vector.y = aiMesh->mVertices[i].y;
			vector.z = aiMesh->mVertices[i].z;
			vector.w = 1;
			vertex.position = vector;
			

			// Normal
			vector.x = aiMesh->mNormals[i].x;
			vector.y = aiMesh->mNormals[i].y;
			vector.z = aiMesh->mNormals[i].z;
			//vertex.normal = vector;
			

			if (aiMesh->mTextureCoords[0])
			{
				glm::vec2 vec;				
				vec.x = aiMesh->mTextureCoords[0][i].x;
				vec.y = aiMesh->mTextureCoords[0][i].y;
				vertex.texCoord = vec;
			}
			else
			{
				vertex.texCoord = glm::vec2(0.0f, 0.0f);
			}

			vertices.push_back(vertex);
		}
		
		for (GLuint i = 0; i < aiMesh->mNumFaces; i++)
		{
			aiFace face = aiMesh->mFaces[i];
			
			for (GLuint j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		// Process materials
		if (aiMesh->mMaterialIndex >= 0)
		{
			aiMaterial* aiMat = scene->mMaterials[aiMesh->mMaterialIndex];
			
			if (mProcessedMaterials[aiMat->GetName().C_Str()] != nullptr)
			{
				TS_CORE_INFO("Material {0} already processed", aiMat->GetName().C_Str());
			}
			else
			{
				ProcessMaterial(aiMat);
			}		
		}

		Ref<Mesh> mesh = CreateRef<Mesh>();
		mesh->SetName(aiMesh->mName.C_Str());
		mesh->SetVertices(vertices);
		mesh->SetIndices(indices);
		mesh->SetMaterial(mTsMaterial);
		mesh->Create();

		return mesh;
	}
	
	void Model::ProcessMaterial(aiMaterial* aiMat)
	{
		mDefaultShader = Shader::Create("DefaultShader", "HDRLighting.vert", "HDRLighting.frag");
		mTsMaterial = CreateRef<Material>(aiMat->GetName().C_Str(), mDefaultShader);

		aiMat->Get(AI_MATKEY_NAME, this->mMaterial.name);
		//aiMat->Get(AI_MATKEY_COLOR_AMBIENT, this->mMaterial.ambient);
		aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, this->mMaterial.diffuse);
		aiMat->Get(AI_MATKEY_COLOR_SPECULAR, this->mMaterial.specular);
		aiMat->Get(AI_MATKEY_OPACITY, this->mMaterial.opacity);
		aiMat->Get(AI_MATKEY_SHININESS, this->mMaterial.shininess);

		//TS_CORE_INFO("Material {0} has {1} diffuse textures", this->material.name.C_Str(), diffTexCount);

		//TODO: Add support for other type of textures;
		/*uint32_t ambientTexCount = material->GetTextureCount(aiTextureType_AMBIENT);		
		uint32_t heightMapTexCount = material->GetTextureCount(aiTextureType_HEIGHT);
		uint32_t opacityTexCount = material->GetTextureCount(aiTextureType_OPACITY);
		uint32_t displacementTexCount= material->GetTextureCount(aiTextureType_DISPLACEMENT);
		uint32_t shininessTexCount = material->GetTextureCount(aiTextureType_SHININESS);		
		uint32_t lightMapTexCount = material->GetTextureCount(aiTextureType_LIGHTMAP);
		uint32_t reflectionTexCount = material->GetTextureCount(aiTextureType_REFLECTION);*/

		mTsMaterial->SetAmbientColor(Vector4(this->mMaterial.ambient.r, this->mMaterial.ambient.g, this->mMaterial.ambient.b, 1));
		mTsMaterial->SetDiffuseColor(Vector4(this->mMaterial.diffuse.r, this->mMaterial.diffuse.g, this->mMaterial.diffuse.b, 1));
		mTsMaterial->SetSpecularColor(Vector4(this->mMaterial.specular.r, this->mMaterial.specular.g, this->mMaterial.specular.b, 1));
		mTsMaterial->SetShininess(this->mMaterial.shininess);

		uint32_t numDiffuseMaps = aiMat->GetTextureCount(aiTextureType_DIFFUSE);
		uint32_t numSpecularMaps = aiMat->GetTextureCount(aiTextureType_SPECULAR);
		uint32_t numNormalMaps = aiMat->GetTextureCount(aiTextureType_NORMALS);
		uint32_t numMetallicMaps = aiMat->GetTextureCount(aiTextureType_METALNESS);
		uint32_t numEmissiveMaps = aiMat->GetTextureCount(aiTextureType_EMISSIVE);

		for (uint32_t i = 0; i < numDiffuseMaps; i++)
		{
			aiString textureFile;

			if (aiMat->GetTexture(aiTextureType_DIFFUSE, i, &textureFile) == aiReturn_SUCCESS)
			{
				mTexture = nullptr;

				if (Ref<Texture2D> embeddedTex = mProcessedEmbeddedTextures[textureFile.C_Str()])
				{
					TS_CORE_INFO("Found diffuse map named {0} from already processed embedded textures", textureFile.C_Str());
					mTexture = embeddedTex;
				}
				else
				{
					TS_CORE_INFO("No embedded diffuse map named {0} found", textureFile.C_Str());
					mTexture = Texture2D::Create(mModelDirectory + "\\" + textureFile.C_Str());
				}

				mTsMaterial->SetDiffuseMap(mTexture);//Diffuse map
			}
		}

		for (uint32_t i = 0; i < numSpecularMaps; i++)
		{
			aiString textureFile;

			if (aiMat->GetTexture(aiTextureType_SPECULAR, i, &textureFile) == aiReturn_SUCCESS)
			{
				mTexture = nullptr;

				if (Ref<Texture2D> embeddedTex = mProcessedEmbeddedTextures[textureFile.C_Str()])
				{
					TS_CORE_INFO("Found specular map named {0} from already processed embedded textures", textureFile.C_Str());
					mTexture = embeddedTex;
				}
				else
				{
					TS_CORE_INFO("No embedded specular map named {0} found", textureFile.C_Str());
					mTexture = Texture2D::Create(mModelDirectory + "\\" + textureFile.C_Str());
				}

				mTsMaterial->SetSpecularMap(mTexture);//Specular map
			}
		}

		for (uint32_t i = 0; i < numNormalMaps; i++)
		{
			aiString textureFile;
			
			if (aiMat->GetTexture(aiTextureType_NORMALS, i, &textureFile) == aiReturn_SUCCESS)
			{
				mTexture = nullptr;

				if (Ref<Texture2D> embeddedTex = mProcessedEmbeddedTextures[textureFile.C_Str()])
				{
					TS_CORE_INFO("Found normal map named {0} from already processed embedded textures", textureFile.C_Str());
					mTexture = embeddedTex;
				}
				else
				{
					TS_CORE_INFO("No embedded normal map named {0} found", textureFile.C_Str());
					mTexture = Texture2D::Create(mModelDirectory + "\\" + textureFile.C_Str());
				}

				mTsMaterial->SetNormalMap(mTexture);//Normal map
			}
		}
		for (uint32_t i = 0; i < numMetallicMaps; i++)
		{
			aiString textureFile;

			if (aiMat->GetTexture(aiTextureType_METALNESS, i, &textureFile) == aiReturn_SUCCESS)
			{
				mTexture = nullptr;

				if (Ref<Texture2D> embeddedTex = mProcessedEmbeddedTextures[textureFile.C_Str()])
				{
					TS_CORE_INFO("Found metallic map named {0} from already processed embedded textures", textureFile.C_Str());
					mTexture = embeddedTex;
				}
				else
				{
					TS_CORE_INFO("No metallic normal map named {0} found", textureFile.C_Str());
					mTexture = Texture2D::Create(mModelDirectory + "\\" + textureFile.C_Str());
				}

				//mTsMaterial->SetMetallicMap(mTexture);// TODO: Add support for metallic map
			}
		}
		for (uint32_t i = 0; i < numEmissiveMaps; i++)
		{
			aiString textureFile;

			if (aiMat->GetTexture(aiTextureType_EMISSIVE, i, &textureFile) == aiReturn_SUCCESS)
			{
				mTexture = nullptr;

				if (Ref<Texture2D> embeddedTex = mProcessedEmbeddedTextures[textureFile.C_Str()])
				{
					TS_CORE_INFO("Found emissive map named {0} from already embedded textures", textureFile.C_Str());
					mTexture = embeddedTex;
				}
				else
				{
					TS_CORE_INFO("No emissive normal map named {0} found", textureFile.C_Str());
					mTexture = Texture2D::Create(mModelDirectory + "\\" + textureFile.C_Str());
				}

				//mTsMaterial->SetEmmisiveMap(mTexture);// TODO: Add support for emmisive map
			}
		}


		// Add processed material
		AddMaterialToDictionary(mTsMaterial);
	}
}