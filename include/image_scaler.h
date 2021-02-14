#pragma once

namespace img {
	void downscale(
		unsigned char* buffer,
		unsigned int width, unsigned int height,
		unsigned char* new_buffer,
		unsigned int new_width, unsigned int new_height);
}
