#include "Polygon.h"
//#include "Line.h"
//#include "Rectangle.h"
#include <limits>

//#include <blib/math/Triangle.h>
//#include <poly2tri/poly2tri.h>
//#include <clipper/clipper.hpp>

namespace math
{
	//bool Polygon::intersects(const Line &line) const
	//{
	//	for (size_t i = 0; i < size(); i++)
	//	{
	//		int ii = (i + 1) % size();
	//		Line l2(at(i), at(ii));

	//		if (line.intersects(l2))
	//			return true;
	//	}
	//	return false;
	//}
	//bool Polygon::intersects( const Line &line, glm::vec2 &point, Line &collidedLine) const
	//{
	//	for(size_t i = 0; i < size(); i++)
	//	{
	//		int ii = (i+1)%size();
	//		Line l2(at(i), at(ii));

	//		if(line.intersects(l2, point))
	//		{
	//			collidedLine.p1 = l2.p1;
	//			collidedLine.p2 = l2.p2;
	//			return true;
	//		}
	//	}
	//	return false;
	//}

	//bool Polygon::intersects( const Line &line, std::vector<std::pair<glm::vec2, Line> >* collisions) const
	//{
	//	glm::vec2 point;
	//	bool collision = false;
	//	for(size_t i = 0; i < size(); i++)
	//	{
	//		int ii = (i+1)%size();
	//		Line l2(at(i), at(ii));

	//		if(line.intersects(l2, point))
	//		{
	//			if(collisions != NULL)
	//				collisions->push_back(std::pair<glm::vec2, Line>(point, l2));
	//			collision = true;
	//		}
	//	}
	//	return collision;
	//}


	glm::vec2 Polygon::normal(int index) const
	{
		int index2 = (index+1)%size();
		return glm::normalize(glm::vec2(at(index2).y - at(index).y, -(at(index2).x - at(index).x)));
	}


	//ClipperLib::Polygon Polygon::toClipperPolygon() const
	//{
	//	ClipperLib::Polygon ret;
	//	for(size_t i = 0; i < size(); i++)
	//		ret.push_back(at(i));
	//	return ret;
	//}

	//Polygon::Polygon( const ClipperLib::Polygon &p )
	//{
	//	reserve(p.size());
	//	for(size_t i = 0; i < p.size(); i++)
	//		push_back(p.at(i));
	//}

	//blib::math::Rectangle Polygon::getBoundingBox() const
	//{
	//	blib::math::Rectangle ret(glm::vec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max()), glm::vec2(std::numeric_limits<float>::min(), std::numeric_limits<float>::min()));

	//	for(size_t i = 0; i < size(); i++)
	//	{
	//		ret.topleft = glm::min(ret.topleft, at(i));
	//		ret.bottomright = glm::max(ret.bottomright, at(i));
	//	}
	//	return ret;
	//}

	//Polygon::Polygon( p2t::Triangle *t)
	//{
	//	for(int i = 0; i < 3; i++)
	//		push_back(glm::vec2(t->GetPoint(i)->x, t->GetPoint(i)->y));
	//}

	Polygon::Polygon(const std::vector<glm::vec2> &l) : std::vector<glm::vec2>(l)
	{
			
	}

	Polygon::Polygon(const std::initializer_list<glm::vec2> &s) : std::vector<glm::vec2>(s)
	{

	}


	//std::vector<p2t::Point*> Polygon::toP2TPolygon() const
	//{
	//	std::vector<p2t::Point*> ret;
	//	for(size_t i = 0; i < size(); i++)
	//		ret.push_back(new p2t::Point(at(i)));
	//	return ret;		
	//}

	//TODO: unsure if this works for concave polygons properly. also check http://alienryderflex.com/polygon/
	bool Polygon::contains(glm::vec2 point) const
	{
		if (size() < 3)
			return false;
		//http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
		int i, j, c = 0;
		for (i = 0, j = (int)size() - 1; i < (int)size(); j = i++) {
			if ((((*this)[i].y>point.y) != ((*this)[j].y > point.y)) &&
				(point.x < ((*this)[j].x - (*this)[i].x) * (point.y - (*this)[i].y) / (float)((*this)[j].y - (*this)[i].y) + (*this)[i].x))
				c = !c;
		}
		return c != 0;
	}

	//glm::vec2 Polygon::projectClosest(const glm::vec2 &position) const
	//{
	//	float minDist = 9999;
	//	glm::vec2 closestPoint;

	//	for (size_t i = 0; i < size(); i++)
	//	{
	//		blib::math::Line line(at(i), at((i + 1) % size()));
	//		glm::vec2 p = line.project(position);
	//		float dist = glm::distance(p, position);
	//		if (dist < minDist)
	//		{
	//			minDist = dist;
	//			closestPoint = p;
	//		}
	//	}
	//	return closestPoint;
	//}

	bool Polygon::isConvex()
	{
		bool result;
		for (size_t i = 0; i < size(); i++)
		{
			int j = (i + 1) % size();
			int k = (i + 2) % size();

			float z = (at(j).x - at(i).x) * (at(k).y - at(j).y);
			z -= (at(j).y - at(i).y) * (at(k).x - at(j).x);;

			if (i == 0)
				result = std::signbit(z);
			if (std::signbit(z) != result)
				return false;
		}
		return true;
	}



	//void Polygon::add(const Triangle2 &triangle)
	//{//TODO : make this without clipper...
	//	ClipperLib::Clipper clipper;

	//	ClipperLib::Polygon thisPoly;
	//	for (size_t i = 0; i < size(); i++)
	//		thisPoly.push_back(at(i));
	//	clipper.AddPolygon(thisPoly, ClipperLib::ptSubject);

	//	ClipperLib::Polygon otherPoly;
	//	otherPoly.push_back(triangle.v1);
	//	otherPoly.push_back(triangle.v2);
	//	otherPoly.push_back(triangle.v3);
	//	clipper.AddPolygon(otherPoly, ClipperLib::ptClip);

	//	ClipperLib::Polygons res;
	//	clipper.Execute(ClipperLib::ctUnion, res);
	//	assert(res.size() == 1);

	//	clear();
	//	for (size_t i = 0; i < res[0].size(); i++)
	//		push_back(res[0][i]);
	//}


	//void Polygon::intersect(const Polygon& other)
	//{//TODO : make this without clipper...
	//	ClipperLib::Clipper clipper;

	//	ClipperLib::Polygon thisPoly;
	//	for (size_t i = 0; i < size(); i++)
	//		thisPoly.push_back(at(i));
	//	clipper.AddPolygon(thisPoly, ClipperLib::ptSubject);

	//	ClipperLib::Polygon otherPoly;
	//	for (size_t i = 0; i < other.size(); i++)
	//		otherPoly.push_back(other[i]);
	//	clipper.AddPolygon(otherPoly, ClipperLib::ptClip);

	//	ClipperLib::Polygons res;
	//	clipper.Execute(ClipperLib::ctIntersection, res);
	//	clear();

	//	if (res.size() == 1)
	//	{
	//		for (size_t i = 0; i < res[0].size(); i++)
	//			push_back(res[0][i]);
	//	}
	//}


	glm::vec2 Polygon::getCenter() const
	{
		glm::vec2 center;
		for (const glm::vec2 &v : *this)
			center += v;
		center /= (float)size();
		return center;
	}

	const Polygon Polygon::expand(float amount) const
	{
		glm::vec2 center = getCenter();
		Polygon ret;
		for (size_t i = 0; i < size(); i++)
			ret.push_back(at(i) + amount * glm::normalize(center - at(i)));
		return ret;
	}

}