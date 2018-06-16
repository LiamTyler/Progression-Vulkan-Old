#include "include/lights.h"

Light::Light(const glm::vec3& a, const glm::vec3& d, const glm::vec3& s) :
	Ia(a),
	Id(d),
	Is(s)
{
}

DirectionalLight::DirectionalLight(const glm::vec3& dir) :
	direction(dir)
{
}

DirectionalLight::DirectionalLight(const glm::vec3& dir, const glm::vec3& a, const glm::vec3& d, const glm::vec3& s) :
	Light(a, d, s),
	direction(glm::normalize(dir))
{
}

PointLight::PointLight(const glm::vec3& pos) :
	position(pos)
{
}

PointLight::PointLight(const glm::vec3& pos, const glm::vec3& a, const glm::vec3& d, const glm::vec3& s) :
	Light(a, d, s),
	position(pos)
{
}
