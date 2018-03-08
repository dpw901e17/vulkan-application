#pragma once
#include "Vec4f.h"

class Camera
{
public:
	float FieldOfView() const { return m_FieldOfView; }
	float Near() const { return m_Near; }
	float Far() const { return m_Far; }
	Vec4f Position() const { return m_position; }
	Vec4f Target() const { return m_target; }
	Vec4f Up() const { return m_up; }
	void SetPosition(const Vec4f& position) { m_position = position; };
	void SetFar(float newfar) { m_Far = newfar; };
	static Camera Default() { return Camera(); }

private:
	Camera() = default;
	float m_FieldOfView = 45.0f*(3.141592f / 180.0f); // 45 degrees in radians
	float m_Near = 0.1f;
	float m_Far = 50.0f;
	Vec4f m_position = { 0.0f, 2.0f, -4.0f,	1.0f };
	Vec4f m_target = { 0.0f, 0.0f, 0.0f, 1.0f };
	Vec4f m_up = { 0.0f, 1.0f, 0.0f, 0.0f };
};