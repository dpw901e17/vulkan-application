#pragma once

struct QueueFamilyIndices {
	int graphicsFamily = -1; //<-- "not found"
	int presentFamily = -1;

	bool isComplete() const
	{
		return graphicsFamily >= 0 && presentFamily >= 0;
	}
};