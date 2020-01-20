#pragma once
#include <glm/glm.hpp>

class IQuad2D;

class Quad2D // Represents a 2D plane space region
{
public:
	Quad2D() { }
	Quad2D(IQuad2D& quad);

	Quad2D(float x, float y, float w, float h) : Position(x, y), Size(w, h) { }
	Quad2D(glm::vec2 pos, glm::vec2 size) : Position(pos), Size(size) { }
	Quad2D(glm::vec4 quad) : Position(quad.x, quad.y), Size(quad.z, quad.w) { }
	glm::vec2 Position;
	glm::vec2 Size;
	inline Quad2D NoOffset() { return Quad2D(glm::vec2(0, 0), Size); }

	bool Intersects(Quad2D other) 
	{
		return Position.x < other.Position.x + other.Size.x && Position.x + Size.x > other.Position.x &&
			Position.y + Size.y > other.Position.y && Position.y < other.Position.y + other.Size.y;
	}

	bool Intersects(glm::vec2 pt)
	{
		return pt.x >= Position.x && pt.y >= Position.y && pt.x <= Position.x + Size.x
			&& pt.y <= Position.y + Size.y;
	}

	Quad2D Inset(float left, float right, float up, float down)
	{
		return Quad2D(Position.x + left, Position.y + down, Size.x - left - right, Size.y - up - down);
	}

	glm::vec2 GetCenter()
	{
		return Position + Size / 2.f;
	}

	bool operator==(const Quad2D& other) { return Position == other.Position && Size == other.Size; }
	bool operator!=(const Quad2D& other) { return Position != other.Position || Size != other.Size; }
};

class IQuad2D
{
public:
	IQuad2D() { }
	IQuad2D(Quad2D& quad) : Position(glm::floor(quad.Position)), Size(glm::ceil(quad.Size)) { }
	IQuad2D(int x, int y, int w, int h) : Position(glm::round(x), glm::round(y)), Size(glm::round(w), glm::round(h)) { }
	IQuad2D(glm::ivec2 pos, glm::ivec2 size) : Position(pos), Size(size) { }
	IQuad2D(glm::ivec4 quad) : Position(quad.x, quad.y), Size(quad.z, quad.w) { }
	glm::ivec2 Position;
	glm::ivec2 Size;
	inline IQuad2D NoOffset() { return IQuad2D(glm::ivec2(0, 0), Size); }
	inline IQuad2D Mipmapped(int level) { return IQuad2D(Position, glm::vec2(Size) / std::powf(2, float(level))); }

	bool Intersects(IQuad2D other) 
	{
		return Position.x < other.Position.x + other.Size.x && Position.x + Size.x > other.Position.x &&
			Position.y + Size.y > other.Position.y && Position.y < other.Position.y + other.Size.y;
	}

	glm::vec2 GetCenter()
	{
		return Position + Size / 2;
	}

	bool operator==(const IQuad2D& other) { return Position == other.Position && Size == other.Size; }
	bool operator!=(const IQuad2D& other) { return Position != other.Position || Size != other.Size; }
};