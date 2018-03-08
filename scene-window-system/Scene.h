#pragma once
#include <vector>
#include "RenderObject.h"
#include "Camera.h"

class Scene
{
public:
	Scene(Camera camera, std::vector<RenderObject> render_objects) : m_Camera(camera), m_RenderObjects(render_objects) {  }
	Scene(Camera& camera, size_t dimensionCubeCount, double padding);
	~Scene();

	const Camera& camera() const { return m_Camera; }
	const std::vector<RenderObject>& renderObjects() const { return m_RenderObjects; }
private:
	Camera m_Camera;
	std::vector<RenderObject> m_RenderObjects;
};
