#pragma once

#include <vector>
#include <glm/glm.hpp>

//#include <clipper/clipper.hpp>

//namespace p2t { struct Point; class Triangle; }

namespace math
{
	class Line;
	class Rectangle;
	class Triangle2;

	class Polygon : public std::vector<glm::vec2>
	{
	public:
		Polygon() { }
//			Polygon(const ClipperLib::Polygon &p);
		Polygon(const std::vector<glm::vec2> &l);
		Polygon(const std::initializer_list<glm::vec2> &s);
//			ClipperLib::Polygon toClipperPolygon() const;
//			Polygon(p2t::Triangle *t);


//			std::vector<p2t::Point*> toP2TPolygon() const;
		bool contains(glm::vec2 point) const;
		bool intersects(const Line& line) const;
		bool intersects(const Line& line, glm::vec2 &point, Line &collidedLine) const;
		bool intersects( const Line &line, std::vector<std::pair<glm::vec2, Line> >* collisions) const;

		bool isConvex();

		void add(const Triangle2 &triangle);
		void intersect(const Polygon& other);
			
		const Polygon expand(float amount) const;
				
		glm::vec2 getCenter() const;
			

		glm::vec2 normal(int index) const;

		//math::Rectangle getBoundingBox() const;

		glm::vec2 projectClosest(const glm::vec2 &position) const;
	};

}

