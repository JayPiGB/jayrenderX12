#pragma once

#include <std_includes.h>

using namespace DirectX;

enum CAMERA_MOVEMENT {
	UP,
	DOWN,
	FRONT,
	BACK,
	LEFT,
	RIGHT
};

const float YAW = 90.0f; //should face toward positive Z for LH
const float PITCH = 0.0f;
const float ROLL = 0.0f;
const float SPEED = 12.0f;
const float ZOOM = 0.0f;

class Camera
{
public:
	XMFLOAT3 mPosition;
	XMFLOAT3 mFocus;
	XMFLOAT3 mUp;

	float mYaw;
	float mPitch;
	float mRoll;

	float mMovementSpeed;
	float mMouseSensitivity;
	float mZoom;

	Camera() :
		mPosition(0.0f, 0.0f, -4.0f),
		mFocus(0.0f, 0.0f, 1.0f),
		mUp(0.0f, 1.0f, 0.0f),
		mYaw(YAW),
		mPitch(PITCH),
		mRoll(ROLL),
		mMovementSpeed(SPEED),
		mMouseSensitivity(0.1f),
		mZoom(ZOOM) { }

	XMMATRIX GetViewMatrix() const
	{
		auto position = XMLoadFloat3(&mPosition);
		auto focus = XMLoadFloat3(&mFocus);
		auto up = XMLoadFloat3(&mUp);
		return XMMatrixLookAtLH(position, focus, up);
	}

	void MoveCamera(CAMERA_MOVEMENT movement, float deltaTime)
	{
		float cameraSpeed = mMovementSpeed * deltaTime;

		auto camPosition = XMLoadFloat3(&mPosition);

		mFocus.x = cos(XMConvertToRadians(mYaw)) * cos(XMConvertToRadians(mPitch));
		mFocus.y = sin(XMConvertToRadians(mPitch));
		mFocus.z = sin(XMConvertToRadians(mYaw)) * cos(XMConvertToRadians(mPitch));

		auto camFocus = XMLoadFloat3(&mFocus);
		auto camUp = XMLoadFloat3(&mUp);

		XMVECTOR right = XMVector3Normalize((XMVector3Cross(camFocus, camUp)));;
		switch (movement)
		{
			case FRONT:
				camPosition += cameraSpeed * camFocus;
				break;

			case BACK:
				camPosition -= cameraSpeed * camFocus;
				break;

			case LEFT:
				camPosition -= cameraSpeed * right;
				//focus -= cameraSpeed * mov;
				break;

			case RIGHT:
				right = XMVector3Normalize((XMVector3Cross(camFocus, camUp)));
				camPosition += cameraSpeed * XMVector3Normalize((XMVector3Cross(camFocus, camUp)));
				//focus += cameraSpeed * mov;
				break;

			case UP:
				camUp = XMVector3Normalize(camUp);
				camPosition += cameraSpeed * camUp;
				//focus += cameraSpeed * mov;
				break;

			case DOWN:
				camUp = XMVector3Normalize(camUp);
				camPosition -= cameraSpeed * camUp;
				//focus -= cameraSpeed * mov;
				break;
		}

		XMStoreFloat3(&mPosition, camPosition);
		XMStoreFloat3(&mFocus, camFocus);
	}
};
