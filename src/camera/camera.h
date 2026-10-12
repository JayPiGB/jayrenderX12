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

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float ROW = 0.0f;
const float SPEED = 12.0f;
const float ZOOM = 0.0f;

class Camera
{
public:
	XMFLOAT3 mPosition;
	XMFLOAT3 mFocus; //review changing this to mFocus
	XMFLOAT3 mUp;

	float mYaw;
	float mPitch;
	float mRow;

	float mMovementSpeed;
	float mMouseSensitivity;
	float mZoom;

	Camera() :
		mPosition(0.0f, 0.0f, -4.0f),
		mFocus(0.0f, 0.0f, 1.0f),
		mUp(0.0f, 1.0f, 0.0f),
		mYaw(YAW),
		mPitch(PITCH),
		mRow(ROW),
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
		//XMFLOAT3 movementFront = mFocus;
		//movementFront.y = 0.0f;

		auto position = XMLoadFloat3(&mPosition);
		//mFocus.y = 0.0f;
		auto focus = XMLoadFloat3(&mFocus);
		auto up = XMLoadFloat3(&mUp);

		XMVECTOR mov{};
		switch (movement)
		{
			case FRONT:
				position += cameraSpeed * focus;
				break;

			case BACK:
				position -= cameraSpeed * focus;
				break;

			case LEFT:
				mov = XMVector3Normalize((XMVector3Cross(focus, up)));
				position -= cameraSpeed * mov;
				focus -= cameraSpeed * mov;
				break;

			case RIGHT:
				mov = XMVector3Normalize((XMVector3Cross(focus, up)));
				position += cameraSpeed * XMVector3Normalize((XMVector3Cross(focus, up)));
				focus += cameraSpeed * mov;
				break;

			case UP:
				mov = XMVector3Normalize(up);
				position += cameraSpeed * mov;
				focus += cameraSpeed * mov;
				break;

			case DOWN:
				mov = XMVector3Normalize(up);
				position -= cameraSpeed * mov;
				focus -= cameraSpeed * mov;
				break;
		}

		XMStoreFloat3(&mPosition, position);
		XMStoreFloat3(&mFocus, focus);
	}
};
