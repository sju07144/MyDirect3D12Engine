#pragma once
#include "Stdafx.h"

class Camera
{
public:
	Camera();
	
	DirectX::XMFLOAT3 GetPosition();
	void SetPosition(const DirectX::XMFLOAT3& position);
	void SetPosition(float x, float y, float z);

	DirectX::XMFLOAT3 GetRight();
	DirectX::XMFLOAT3 GetUp();
	DirectX::XMFLOAT3 GetLook();

	DirectX::XMFLOAT4X4 GetView();
	DirectX::XMFLOAT4X4 GetProj();

	void LookAt(
		const DirectX::XMFLOAT3& position,
		const DirectX::XMFLOAT3& target,
		const DirectX::XMFLOAT3& worldUp);
	void LookAt(
		float posX, float posY, float posZ,
		float targetX, float targetY, float targetZ,
		float worldUpX, float worldUpY, float worldUpZ);

	void SetLens(
		float fovAngleY,
		float aspectRatio,
		float zn,
		float zf);

	void Strafe(float distance);
	void Walk(float distance);

	void Pitch(float angle);
	void RotateY(float angle);
private:
	void UpdateViewMatrix();
private:
	DirectX::XMFLOAT3 mPosition;

	DirectX::XMFLOAT3 mRight;
	DirectX::XMFLOAT3 mUp;
	DirectX::XMFLOAT3 mLook;

	float mFovAngleY = 0.0f;
	float mAspectRatio = 0.0f;
	float mNearZ = 0.0f;
	float mFarZ = 0.0f;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;
};