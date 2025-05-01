#pragma once

#pragma region Includes
//Include{s}
#include <DirectXMath.h>
#include <windows.h>
#include <windowsx.h>
#include  <vector>
using namespace DirectX;
#pragma endregion

class Camera
{
public:

	Camera(XMFLOAT3 posIn, XMFLOAT3 lookDirIn, XMFLOAT3 upIn)
	{
		position = posIn;
		lookDir = lookDirIn;
		up = upIn;

		originalPosition = posIn;
		originalLookDir = lookDirIn;
		originalUp = upIn;

		XMStoreFloat4x4(&viewMatrix, XMMatrixIdentity());
	}

	XMFLOAT3 GetPosition() { return position; }
	void SetPosition(XMFLOAT3 newPosition) { position = newPosition; }
	void SetPosition(XMVECTOR newPosition) { XMStoreFloat3(&position, newPosition); }

	void MoveForward(float distance)
	{
		// Get the normalized forward vector (camera's look direction)
		XMVECTOR forwardVec = XMVector3Normalize(XMLoadFloat3(&lookDir));
		XMVECTOR posVec = XMLoadFloat3(&position);

		// Move in the direction the camera is facing
		XMStoreFloat3(&position, XMVectorMultiplyAdd(XMVectorReplicate(distance), forwardVec, posVec));
	}

	void StrafeLeft(float distance)
	{
		// Get the current look direction and up vector
		XMVECTOR lookDirVec = XMLoadFloat3(&lookDir);
		XMVECTOR upVec = XMLoadFloat3(&up);

		// Calculate the right vector (side vector of the camera)
		XMVECTOR rightVec = XMVector3Normalize(XMVector3Cross(upVec, lookDirVec));
		XMVECTOR posVec = XMLoadFloat3(&position);

		// Move left by moving opposite to the right vector
		XMStoreFloat3(&position, XMVectorMultiplyAdd(XMVectorReplicate(-distance), rightVec, posVec));
	}

	void MoveBackward(float distance)
	{
		// Call MoveForward with negative distance to move backward
		MoveForward(-distance);
	}

	void StrafeRight(float distance)
	{
		// Call StrafeLeft with negative distance to move right
		StrafeLeft(-distance);
	}

	void MoveUp(float distance) {
		XMVECTOR upVec = XMLoadFloat3(&up);
		XMVECTOR posVec = XMLoadFloat3(&position);

		XMStoreFloat3(&position, XMVectorMultiplyAdd(XMVectorReplicate(distance), upVec, posVec));
	}

	void MoveDown(float distance) {
		MoveUp(-distance);
	}

	void RotateYaw(float angle) {
		XMVECTOR upVec = XMLoadFloat3(&up);
		XMVECTOR lookDirVec = XMLoadFloat3(&lookDir);

		lookDirVec = XMVector3Transform(lookDirVec, XMMatrixRotationAxis(upVec, angle));
		lookDirVec = XMVector3Normalize(lookDirVec);

		XMStoreFloat3(&lookDir, lookDirVec);
	}

	void RotateRoll(float angle) {
		XMVECTOR lookDirVec = XMLoadFloat3(&lookDir);
		XMVECTOR upVec = XMLoadFloat3(&up);

		upVec = XMVector3Transform(upVec, XMMatrixRotationAxis(lookDirVec, angle));
		upVec = XMVector3Normalize(upVec);

		XMStoreFloat3(&up, upVec);
	}

	void RotatePitch(float angle) {
		//ROTATE AROUND THE RIGHT VECTOR YOU FOOL

		XMVECTOR lookDirVec = XMLoadFloat3(&lookDir);
		XMVECTOR upVec = XMLoadFloat3(&up);
		XMVECTOR rightVec = XMVector3Cross(upVec, lookDirVec);
		rightVec = XMVector3Normalize(rightVec);

		lookDirVec = XMVector3Transform(lookDirVec, XMMatrixRotationAxis(rightVec, angle));
		lookDirVec = XMVector3Normalize(lookDirVec);

		upVec = XMVector3Cross(lookDirVec, rightVec);
		upVec = XMVector3Normalize(upVec);

		XMStoreFloat3(&lookDir, lookDirVec);
		XMStoreFloat3(&up, upVec);
	}

	void Reset()
	{
		position = originalPosition;
		lookDir = originalLookDir;
		up = originalUp;
	}

	XMVECTOR CatmullRom(XMVECTOR p0, XMVECTOR p1, XMVECTOR p2, XMVECTOR p3, float t)
	{
		float t2 = t * t;
		float t3 = t2 * t;
		return 0.5f * (
			(2.0f * p1) +
			(-p0 + p2) * t +
			(2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
			(-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
			);
	}

	float m_splineTransition = 0.0f;

	void CameraSplineAnimation(float deltaTime, std::vector<XMVECTOR> controlPoints, float duration)
	{
		m_splineTransition += deltaTime / duration;

		if (m_splineTransition > 1.0f) // BETWEEN 0 AND 1
		{
			m_splineTransition = 0.0f;
		}

		float smoothTransition;

		// Blatant Theft from the lecture slides
		if (m_splineTransition < 0.5f)
		{
			smoothTransition = 2.0f * m_splineTransition * m_splineTransition;
		}
		else
		{
			smoothTransition = 1.0f - (pow(-2.0f * m_splineTransition + 2.0f, 2) / 2.0f);
		}

		int sections = controlPoints.size() - 3; // Why 3 and not 2, I have no clue

		// Super fucked way to find the which part of the spline we are on
		float currentSplinePosition = smoothTransition * static_cast<float>(sections);
		int sectionIndex = static_cast<int>(floor(currentSplinePosition));

		// Uhh, don't go out of bounds forehead
		if (sectionIndex >= sections)
		{
			sectionIndex = sections - 1;
		}

		float currentSectionTime = currentSplinePosition - static_cast<float>(sectionIndex);

		XMVECTOR intialVelocity = controlPoints[sectionIndex];
		XMVECTOR p1 = controlPoints[sectionIndex + 1];
		XMVECTOR p2 = controlPoints[sectionIndex + 2];
		XMVECTOR endingVelocity = controlPoints[sectionIndex + 3];

		XMVECTOR newCameraPosition = CatmullRom(intialVelocity, p1, p2, endingVelocity, currentSectionTime);

		SetPosition(newCameraPosition);

		// No clue how do reconstruct the view matrix without it crashing

		/*XMVECTOR futureCameraPosition = CatmullRom(intialVelocity, p1, p2, endingVelocity, currentSectionTime + deltaTime);

		XMVECTOR splineForward = XMVector3Normalize(futureCameraPosition - newCameraPosition);

		// Define a world-up vector (assumes Y-up world)
		XMVECTOR splineUp = XMVectorSet(0, 1, 0, 0);

		// Compute right vector using cross product (ensuring perpendicularity)
		XMVECTOR splineRight = XMVector3Normalize(XMVector3Cross(splineUp, splineForward));

		// Recompute up vector to ensure it remains perpendicular
		splineUp = XMVector3Normalize(XMVector3Cross(splineForward, splineRight));

		// Construct view matrix
		XMMATRIX splineViewMatrix = XMMatrixLookToLH(newCameraPosition, splineForward, splineUp);

		XMStoreFloat4x4(&viewMatrix, splineViewMatrix); */
	}

	void UpdateLookAt(POINTS delta)
	{
		// Sensitivity factor for mouse movement
		const float sensitivity = 0.001f;

		// Apply sensitivity
		float dx = delta.x * sensitivity; // Yaw change
		float dy = delta.y * sensitivity; // Pitch change

		// Get the current look direction and up vector
		XMVECTOR lookDirVec = XMLoadFloat3(&lookDir);
		lookDirVec = XMVector3Normalize(lookDirVec);
		XMVECTOR upVec = XMLoadFloat3(&originalUp);
		upVec = XMVector3Normalize(upVec);

		// Calculate the camera's right vector
		XMVECTOR rightVec = XMVector3Cross(upVec, lookDirVec);
		rightVec = XMVector3Normalize(rightVec);

		// Rotate the lookDir vector left or right based on the yaw
		lookDirVec = XMVector3Transform(lookDirVec, XMMatrixRotationAxis(upVec, dx));
		lookDirVec = XMVector3Normalize(lookDirVec);

		// Rotate the lookDir vector up or down based on the pitch
		lookDirVec = XMVector3Transform(lookDirVec, XMMatrixRotationAxis(rightVec, dy));
		lookDirVec = XMVector3Normalize(lookDirVec);

		// Re-calculate the right vector after the yaw rotation
		rightVec = XMVector3Cross(upVec, lookDirVec);
		rightVec = XMVector3Normalize(rightVec);

		// Re-orthogonalize the up vector to be perpendicular to the look direction and right vector
		upVec = XMVector3Cross(lookDirVec, rightVec);
		upVec = XMVector3Normalize(upVec);

		// Store the updated vectors back to the class members
		XMStoreFloat3(&lookDir, lookDirVec);
		XMStoreFloat3(&up, upVec);
	}

	void Update() { UpdateViewMatrix(); }

	XMMATRIX GetViewMatrix() const
	{
		UpdateViewMatrix();
		return XMLoadFloat4x4(&viewMatrix);
	}

private:

	void UpdateViewMatrix() const
	{
		// Calculate the look-at point based on the position and look direction
		XMVECTOR posVec = XMLoadFloat3(&position);
		XMVECTOR lookDirVec = XMLoadFloat3(&lookDir);
		XMVECTOR lookAtPoint = posVec + lookDirVec; // This is the new look-at point

		// Update the view matrix to look from the camera's position to the look-at point
		XMStoreFloat4x4(&viewMatrix, XMMatrixLookAtLH(posVec, lookAtPoint, XMLoadFloat3(&up)));
	}

	XMFLOAT3 position;
	XMFLOAT3 lookDir;
	XMFLOAT3 up;

	XMFLOAT3 originalPosition;
	XMFLOAT3 originalLookDir;
	XMFLOAT3 originalUp;
	mutable XMFLOAT4X4 viewMatrix;
};
