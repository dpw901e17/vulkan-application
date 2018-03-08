#include "Scene.h"

Scene::Scene(Camera& camera, size_t dimensionCubeCount, double paddingFactor) : m_Camera(camera)
{
	//make oringin (0,0,0) the center of the "cube of cubes" by subtracting an offset:
	auto offset = (dimensionCubeCount + (dimensionCubeCount - 1) * paddingFactor) / 2;
	for(auto x = 0; x < dimensionCubeCount; ++x){
		for (auto y = 0; y < dimensionCubeCount; ++y) {
			for (auto z = 0; z < dimensionCubeCount; ++z) {
				m_RenderObjects.push_back(
					RenderObject(
						x + x * paddingFactor - offset, 
						y + y * paddingFactor - offset, 
						z + z * paddingFactor - offset
					)
				);
			}
		}
	}
}

Scene::~Scene()
{
	/*
	if (m_RenderObjects.size() > 0) {
		for (auto& ro : m_RenderObjects) {
			delete &ro;
		}
	}
	*/
}
