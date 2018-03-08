#pragma once
#ifdef TEST_USE_CUBE
const std::vector<uint16_t>HelloTriangleApplication::s_Indices = {
	4 * 0 + 0, 4 * 0 + 1, 4 * 0 + 2, 4 * 0 + 2, 4 * 0 + 3, 4 * 0 + 0, // Top
	4 * 1 + 0, 4 * 1 + 1, 4 * 1 + 2, 4 * 1 + 2, 4 * 1 + 3, 4 * 1 + 0, // Left
	4 * 2 + 0, 4 * 2 + 1, 4 * 2 + 2, 4 * 2 + 2, 4 * 2 + 3, 4 * 2 + 0, // Front
	4 * 3 + 0, 4 * 3 + 1, 4 * 3 + 2, 4 * 3 + 2, 4 * 3 + 3, 4 * 3 + 0, // Right
	4 * 4 + 0, 4 * 4 + 1, 4 * 4 + 2, 4 * 4 + 2, 4 * 4 + 3, 4 * 4 + 0, // Back
	4 * 5 + 0, 4 * 5 + 1, 4 * 5 + 2, 4 * 5 + 2, 4 * 5 + 3, 4 * 5 + 0  // Bottom
};
#endif
