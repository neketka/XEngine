#pragma once
#include <glm/glm.hpp>

class BoundingBox;
class BoundingFrustum;

class BoundingSphere 
{
public:
	BoundingSphere(glm::vec3 center, float radius);
private:
	glm::vec3 m_center;
	float m_radius;
};

class BoundingBox
{
public:
	BoundingBox(glm::vec3 min, glm::vec3 max);
private:
	glm::vec3 m_min;
	glm::vec3 m_max;
};

class BoundingFrustum
{
public:
	BoundingFrustum(glm::mat4 mvp);
private:

};