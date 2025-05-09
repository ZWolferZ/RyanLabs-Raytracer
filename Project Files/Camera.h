#pragma once

#pragma region Includes
//Include{s}
#include <DirectXMath.h>
#include <windows.h>
#include <windowsx.h>
#include  <vector>
using namespace DirectX;
#pragma endregion

// I have no clue why resharper sometimes chooses between the different summery comments styles,
// but I am going to roll with it.

/// <summary>
/// The User Interactable Camera class. Right Click to look around. WASDQE to move.
/// </summary>
class Camera
{
public:
#pragma region Constructors

	/// The Camera constructor, which takes in the position, look direction and up vector.
	/// @param posIn The starting position of the camera.
	/// @param lookDirIn The direction the camera is looking at.
	/// @param upIn The up vector of the camera.
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

#pragma endregion

#pragma region Getters and Setters
	/// <summary>
	/// Gets the position of the camera.
	/// </summary>
	/// <returns>The position of the camera as a XMFLOAT3</returns>
	XMFLOAT3 GetPosition() { return position; }

	/// Sets the position of the camera.
	/// @param newPosition The new position of the camera it will be set to.
	void SetPosition(XMFLOAT3 newPosition) { position = newPosition; }
	void SetPosition(XMVECTOR newPosition) { XMStoreFloat3(&position, newPosition); }

	/// Gets the current view matrix.
	/// @return The current view matrix as a XMMATRIX.
	XMMATRIX GetViewMatrix() const
	{
		UpdateViewMatrix();
		return XMLoadFloat4x4(&viewMatrix);
	}
#pragma endregion

#pragma region Camera Movement
	/// Will move the camera forward in the direction it is looking.
	/// @param distance The distance the camera will move.
	void MoveForward(float distance)
	{
		// Get the normalized forward vector (camera's look direction)
		XMVECTOR forwardVec = XMVector3Normalize(XMLoadFloat3(&lookDir));
		XMVECTOR posVec = XMLoadFloat3(&position);

		// Move in the direction the camera is facing
		XMStoreFloat3(&position, XMVectorMultiplyAdd(XMVectorReplicate(distance), forwardVec, posVec));
	}

	/// <summary>
	/// Will strafe the camera left by the given distance.
	/// </summary>
	/// <param name="distance">The distance the camera will move.</param>
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

	/// <summary>
	/// Will move the camera backward in the direction it is looking.
	/// </summary>
	/// <param name="distance">The distance the camera will move.</param>
	void MoveBackward(float distance)
	{
		// Call MoveForward with negative distance to move backward
		MoveForward(-distance);
	}

	/// <summary>
	/// Will strafe the camera right by the given distance.
	/// </summary>
	/// <param name="distance">The distance the camera will move.</param>
	void StrafeRight(float distance)
	{
		// Call StrafeLeft with negative distance to move right
		StrafeLeft(-distance);
	}

	/// <summary>
	/// Will move the camera up by the given distance.
	/// </summary>
	/// <param name="distance">The distance the camera will move.</param>
	void MoveUp(float distance) {
		XMVECTOR upVec = XMLoadFloat3(&up);
		XMVECTOR posVec = XMLoadFloat3(&position);

		XMStoreFloat3(&position, XMVectorMultiplyAdd(XMVectorReplicate(distance), upVec, posVec));
	}

	/// Will move the camera down by the given distance.
	/// @param distance The distance the camera will move.
	void MoveDown(float distance) {
		MoveUp(-distance);
	}

	/// <summary>
	/// Rotates the camera around the Y-axis (yaw).
	/// </summary>
	/// <param name="angle">The angle the camera will rotate.</param>
	void RotateYaw(float angle) {
		XMVECTOR upVec = XMLoadFloat3(&up);
		XMVECTOR lookDirVec = XMLoadFloat3(&lookDir);

		lookDirVec = XMVector3Transform(lookDirVec, XMMatrixRotationAxis(upVec, angle));
		lookDirVec = XMVector3Normalize(lookDirVec);

		XMStoreFloat3(&lookDir, lookDirVec);
	}

	/// <summary>
	/// Rotates the camera around the Z-axis (roll).
	/// </summary>
	/// <param name="angle">The angle the camera will rotate.</param>
	void RotateRoll(float angle) {
		XMVECTOR lookDirVec = XMLoadFloat3(&lookDir);
		XMVECTOR upVec = XMLoadFloat3(&up);

		upVec = XMVector3Transform(upVec, XMMatrixRotationAxis(lookDirVec, angle));
		upVec = XMVector3Normalize(upVec);

		XMStoreFloat3(&up, upVec);
	}

	/// <summary>
	/// Rotates the camera around the X-axis (pitch).
	/// </summary>
	/// <param name="angle">The angle the camera will rotate.</param>
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

	/// <summary>
	/// Resets the camera to its original position, look direction, and up vector.
	/// </summary>
	void Reset()
	{
		position = originalPosition;
		lookDir = originalLookDir;
		up = originalUp;
	}

#pragma endregion

#pragma region Spline Animation
	/// <summary>
	/// The Catmull-Rom spline function.
	/// </summary>
	/// <param name="p0">The initial velocity.</param>
	/// <param name="p1">The first point.</param>
	/// <param name="p2">The second point.</param>
	/// <param name="p3">The final velocity</param>
	/// <param name="t">The time.</param>
	/// <returns>The updated position at the time specified</returns>
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

	/// <summary>
	/// Animates the camera along a Catmull-Rom spline.
	/// </summary>
	/// <param name="deltaTime">The change in time.</param>
	/// <param name="controlPoints">The points along the spline.</param>
	/// <param name="duration">The length of the spline animation.</param>
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
#pragma endregion

#pragma region Update
	/// <summary>
	/// Updates the camera's look direction based on mouse movement.
	/// </summary>
	/// <param name="delta">Change in mouse movement</param>
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

	/// <summary>
	/// Updates the view matrix.
	/// </summary>
	void Update() { UpdateViewMatrix(); }

private:

	/// <summary>
	/// Updates the view matrix.
	/// </summary>
	void UpdateViewMatrix() const
	{
		// Calculate the look-at point based on the position and look direction
		XMVECTOR posVec = XMLoadFloat3(&position);
		XMVECTOR lookDirVec = XMLoadFloat3(&lookDir);
		XMVECTOR lookAtPoint = posVec + lookDirVec; // This is the new look-at point

		// Update the view matrix to look from the camera's position to the look-at point
		XMStoreFloat4x4(&viewMatrix, XMMatrixLookAtLH(posVec, lookAtPoint, XMLoadFloat3(&up)));
	}

#pragma endregion

#pragma region Private Variables

	XMFLOAT3 position;
	XMFLOAT3 lookDir;
	XMFLOAT3 up;

	XMFLOAT3 originalPosition;
	XMFLOAT3 originalLookDir;
	XMFLOAT3 originalUp;
	mutable XMFLOAT4X4 viewMatrix;
#pragma endregion
};
