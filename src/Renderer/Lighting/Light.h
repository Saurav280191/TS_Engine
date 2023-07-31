#pragma once
#include "Core/tspch.h"
#include "Core/Object.h"
#include "Renderer/Shader.h"

namespace TS_ENGINE {

	enum class LightType {
		DIRECTIONAL,
		POINT,
		SPOT
	};

	/*struct DirectionalLightProperties
	{
		Vector3 position;
		Vector3 direction;

		Vector3 ambient;
		Vector3 diffuse;
		Vector3 specular;
	};

	struct PointLightProperties
	{
		Vector3 position;
		Vector3 direction;

		Vector3 ambient;
		Vector3 diffuse;
		Vector3 specular;

		float constant;
		float linear;
		float quadratic;
	};

	struct SpotLightProperties
	{
		Vector3 position;
		Vector3 direction;

		Vector3 ambient;
		Vector3 diffuse;
		Vector3 specular;

		float constant;
		float linear;
		float quadratic;

		float cutOff;
		float outerCutOff;
	};*/

	class Light : public Object
	{
	public:
		Light();
		~Light();		

		// Inherited via Object
		virtual void Initialize() override;
		virtual void SetName(const std::string& name) override;
		virtual void Update(float deltaTime) override;

		void SetCommonParams(const Ref<Shader>& shader, const Vector3& position, const Vector3& direction, const Vector3& ambient, const Vector3& diffuse, const Vector3& specular);
	protected:
		LightType mType;
		Ref<Shader> mShader;
		
		//Common params
		Vector3 position;
		Vector3 direction;
		Vector3 ambient;
		Vector3 diffuse;
		Vector3 specular;
	};
}
