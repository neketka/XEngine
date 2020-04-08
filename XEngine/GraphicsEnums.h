#pragma once

enum class ImageFormat
{
	R8G8B8
};

enum class ImageType
{
	Image2D, Image2DArray, ImageCubemap, ImageCubemapArray
};

enum class GraphicsQueueType
{
	Graphics, Compute, Transfer
};