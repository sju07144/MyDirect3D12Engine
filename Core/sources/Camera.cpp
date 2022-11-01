#include "../includes/Camera.h"
using namespace DirectX;

Camera::Camera()
	: mPosition(XMFLOAT3(0.0f, 0.0f, 0.0f)),
	mRight(XMFLOAT3(1.0f, 0.0f, 0.0f)), mUp(XMFLOAT3(0.0f, 1.0f, 0.0f)), mLook(XMFLOAT3(0.0f, 0.0f, 1.0f))

{
	XMStoreFloat4x4(&mView, XMMatrixIdentity());
	XMStoreFloat4x4(&mProj, XMMatrixIdentity());
}

XMFLOAT3 Camera::GetPosition()
{
	return mPosition;
}
void Camera::SetPosition(const XMFLOAT3& position)
{
	mPosition = position;
}
void Camera::SetPosition(float x, float y, float z)
{
	mPosition = XMFLOAT3(x, y, z);
}

XMFLOAT3 Camera::GetRight()
{
	return mRight;
}
XMFLOAT3 Camera::GetUp()
{
	return mUp;
}
XMFLOAT3 Camera::GetLook()
{
	return mLook;
}

XMFLOAT4X4 Camera::GetView()
{
	return mView;
}
XMFLOAT4X4 Camera::GetProj()
{
	return mProj;
}

void Camera::LookAt(
	const XMFLOAT3& position, 
	const XMFLOAT3& target, 
	const XMFLOAT3& worldUp)
{
	XMVECTOR pos = XMLoadFloat3(&position);
	XMVECTOR tar = XMLoadFloat3(&target);

	XMVECTOR wUp = XMLoadFloat3(&worldUp);
	XMVECTOR look = XMVector3Normalize(XMVectorSubtract(tar, pos));
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(wUp, look));
	XMVECTOR up = XMVector3Cross(look, right);

	XMStoreFloat3(&mPosition, pos);
	XMStoreFloat3(&mRight, right);
	XMStoreFloat3(&mUp, up);
	XMStoreFloat3(&mLook, look);

	UpdateViewMatrix();
}
void Camera::LookAt(
	float posX, float posY, float posZ, 
	float targetX, float targetY, float targetZ, 
	float worldUpX, float worldUpY, float worldUpZ)
{
	Camera::LookAt(
		XMFLOAT3(posX, posY, posZ),
		XMFLOAT3(targetX, targetY, targetZ),
		XMFLOAT3(worldUpX, worldUpY, worldUpZ));
}

void Camera::SetLens(float fovAngleY, float aspectRatio, float zn, float zf)
{
	mFovAngleY = fovAngleY;
	mAspectRatio = aspectRatio;
	mNearZ = zn;
	mFarZ = zf;

	XMMATRIX proj = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, zn, zf);
	XMStoreFloat4x4(&mProj, proj);
}

void Camera::Strafe(float distance)
{
	XMVECTOR pos = XMLoadFloat3(&mPosition);
	XMVECTOR right = XMLoadFloat3(&mRight);

	pos = XMVectorMultiplyAdd(XMVectorReplicate(distance), right, pos);
	XMStoreFloat3(&mPosition, pos);

	UpdateViewMatrix();
}
void Camera::Walk(float distance)
{
	XMVECTOR pos = XMLoadFloat3(&mPosition);
	XMVECTOR look = XMLoadFloat3(&mLook);

	pos = XMVectorMultiplyAdd(XMVectorReplicate(distance), look, pos);
	XMStoreFloat3(&mPosition, pos);

	UpdateViewMatrix();
}

void Camera::Pitch(float angle)
{
	XMMATRIX rotationMatrix = XMMatrixRotationAxis(XMLoadFloat3(&mRight), angle);

	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), rotationMatrix));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), rotationMatrix));

	UpdateViewMatrix();
}
void Camera::RotateY(float angle)
{
	XMMATRIX rotationMatrix = XMMatrixRotationY(angle);

	XMStoreFloat3(&mRight, XMVector3TransformNormal(XMLoadFloat3(&mRight), rotationMatrix));
	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), rotationMatrix));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), rotationMatrix));

	UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{
	XMVECTOR pos = XMLoadFloat3(&mPosition);
	XMVECTOR right = XMLoadFloat3(&mRight);
	XMVECTOR up = XMLoadFloat3(&mUp);
	XMVECTOR look = XMLoadFloat3(&mLook);
	
	look = XMVector3Normalize(look);
	up = XMVector3Normalize(XMVector3Cross(look, right));
	right = XMVector3Cross(up, look);

	float x = -XMVectorGetX(XMVector3Dot(pos, right));
	float y = -XMVectorGetX(XMVector3Dot(pos, up));
	float z = -XMVectorGetX(XMVector3Dot(pos, look));

	XMStoreFloat3(&mRight, right);
	XMStoreFloat3(&mUp, up);
	XMStoreFloat3(&mLook, look);

	mView(0, 0) = mRight.x;
	mView(0, 1) = mUp.x;
	mView(0, 2) = mLook.x;
	mView(0, 3) = 0;

	mView(1, 0) = mRight.y;
	mView(1, 1) = mUp.y;
	mView(1, 2) = mLook.y;
	mView(1, 3) = 0;

	mView(2, 0) = mRight.z;
	mView(2, 1) = mUp.z;
	mView(2, 2) = mLook.z;
	mView(2, 3) = 0;

	mView(3, 0) = x;
	mView(3, 1) = y;
	mView(3, 2) = z;
	mView(3, 3) = 1;
}