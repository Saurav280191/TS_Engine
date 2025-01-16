#include "Bone.h"
#include "Bone.h"
#include "Renderer/MaterialManager.h"
#include "Core/Factory.h"
#include "Renderer/RenderCommand.h"

namespace TS_ENGINE {
	
	Bone::Bone() :
		mId(0),
		mOffsetMatrix(Matrix4(1)),
		mNode(nullptr),
		mJointGuiNode(nullptr),
		mBoneGuiNodes({})
	{

	}

	void Bone::SetParams(int _id, const Matrix4& _offsetMatrix)
	{
		mId = _id;		
		mOffsetMatrix = _offsetMatrix;
	}

	void Bone::SetNode(Ref<Node> _node)
	{
		mNode = _node;
	}

	Ref<Node> Bone::GetNode()
	{
		return mNode; 
	}

	int Bone::GetId()
	{
		return mId;
	}
	
	void Bone::Initialize(const std::string& _name)
	{
		// Create sphere Gui to show joint
		mJointGuiNode = Factory::GetInstance()->InstantiateSphere(_name + "-SphereGui", nullptr);

		// Set mesh color to Orange
		mJointGuiNode->GetMesh()->GetMaterial()->SetAmbientColor(Vector4(1.0f, 0.647f, 0.0f, 1.0f));

		// Create bone between node and it's children
		for (auto& child : mNode->GetChildren())
		{
			// Create bone Gui
			Ref<Node> boneGuiNode = Factory::GetInstance()->InstantiateBone(_name + "-BoneGui", nullptr);
			// Set mesh color to Orange
			boneGuiNode->GetMesh()->GetMaterial()->SetAmbientColor(Vector4(1.0f, 0.647f, 0.0f, 1.0f));
			
			mBoneGuiNodes.push_back(boneGuiNode);
		}
	}

	void Bone::Update(Ref<Shader> _shader)
	{
		mBoneTransformMatrix = mNode->GetTransform()->GetWorldTransformationMatrix() * mOffsetMatrix;
		_shader->SetMat4(std::string("finalBonesMatrices[" + std::to_string(mId) + "]").c_str(), mBoneTransformMatrix);
	}

	void Bone::UpdateBoneGui(Ref<Node> _rootNode)
	{
		Matrix4 jointWorldTransform = _rootNode->GetTransform()->GetWorldTransformationMatrix() * mNode->GetTransform()->GetWorldTransformationMatrix();
		mJointGuiNode->mTransform->SetWorldTransformationMatrix(jointWorldTransform);
		
		for (int i = 0; i < mNode->GetChildCount(); i++)
		{
			Vector3 point1 = mNode->GetTransform()->GetPosition();
			Vector3 point2 = mNode->GetChildAt(i)->GetTransform()->GetPosition();
			glm::vec3 direction = glm::normalize(point1 - point2);
			glm::quat rotation = glm::rotation(glm::vec3(1, 0, 0), direction);
			float boneLength = glm::distance(point1, point2);

			mBoneGuiNodes[i]->GetTransform()->SetLocalPosition((point1 + point2) * 0.5f);
			mBoneGuiNodes[i]->GetTransform()->SetLocalRotation(rotation);
			mBoneGuiNodes[i]->GetTransform()->SetLocalScale(glm::vec3(boneLength, 0.1f, 0.1f));

			mBoneGuiNodes[i]->GetTransform()->ComputeTransformationMatrix(nullptr);
			mBoneGuiNodes[i]->GetTransform()->SetWorldTransformationMatrix(_rootNode->GetTransform()->GetWorldTransformationMatrix() * mBoneGuiNodes[i]->GetTransform()->GetWorldTransformationMatrix());
		}
	}

	void Bone::Render(Ref<Shader> _shader)
	{
		// Make sure bone is never rendered in wireframe
		RenderCommand::EnableWireframe(false);

		// Render mJointGuiNode 
		_shader->SetMat4("u_Model", mJointGuiNode->mTransform->GetWorldTransformationMatrix());
#ifdef TS_ENGINE_EDITOR
		mJointGuiNode->GetMesh()->Render(mJointGuiNode->GetEntity()->GetEntityID(), false);
#else
		mJointGuiNode->GetMesh()->Render(false);
#endif

		// Render all boneGuiNodes 
		for(auto& boneGuiNode : mBoneGuiNodes)
		{
			_shader->SetMat4("u_Model", boneGuiNode->mTransform->GetWorldTransformationMatrix());
#ifdef TS_ENGINE_EDITOR		
			boneGuiNode->GetMesh()->Render(boneGuiNode->GetEntity()->GetEntityID(), false);
#else		
			boneGuiNode->GetMesh()->Render(false);
#endif
		}

		// If wireframe mode is enabled, re-enable it for other meshes
		if (Application::GetInstance().IsWireframeModeEnabled())
		{
			RenderCommand::EnableWireframe(true);
		}
	}

	bool Bone::PickNode(int _entityId)
	{
		// If JointGuiNode's entity Id matches
		if(mJointGuiNode->GetEntity()->GetEntityID() == _entityId)
		{
			return true;
		}

		// If boneGuiNode's entity Id matches
		for (auto& boneGuiNode : mBoneGuiNodes)
		{
			if (boneGuiNode->GetEntity()->GetEntityID() == _entityId)
			{
				return true;
			}
		}

		return false;
	}
}