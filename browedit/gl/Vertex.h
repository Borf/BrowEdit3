#pragma once

#include <glm/glm.hpp>

	// A simple structure to store vertices. Can store positions, normals, colors and texture coordinats
	struct Vertex
	{
	public:
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec4 color;
		glm::vec2 texcoord;

		// Creates a vertex with a position
		static Vertex P(const glm::vec3& position) {
			return { position, glm::vec3(0,1,0), glm::vec4(1,1,1,1), glm::vec2(0,0) };
		}

		// Creates a vertex with a position and a color
		static Vertex PC(const glm::vec3& position, const glm::vec4& color) {
			return { position, glm::vec3(0,1,0), color, glm::vec2(0,0) };
		}

		// Creates a vertex with a position and a texture coordinat
		static Vertex PT(const glm::vec3& position, const glm::vec2& texcoord) {
			return { position, glm::vec3(0,1,0), glm::vec4(1,1,1,1), texcoord };
		}

		// Creates a vertex with a position and a normal
		static Vertex PN(const glm::vec3& position, const glm::vec3& normal) {
			return { position, normal, glm::vec4(1,1,1,1), glm::vec2(0,0) };
		}

		// Creates a vertex with a position, a texture coordinat and a color
		static Vertex PTC(const glm::vec3& position, const glm::vec2& texcoord, const glm::vec4& color) {
			return { position, glm::vec3(0,1,0), color, texcoord };
		}


		// Creates a vertex with a position, color and normal
		static Vertex PCN(const glm::vec3& position, const glm::vec4& color, const glm::vec3& normal) {
			return { position, normal, color, glm::vec2(0,0) };
		}

		// Creates a vertex with a position, texture coordinat and normal
		static Vertex PTN(const glm::vec3& position, const glm::vec2& texcoord, const glm::vec3& normal) {
			return { position, normal, glm::vec4(1,1,1,1), texcoord };
		}

		// Creates a vertex with a position, color, texture coordinat and normal
		static Vertex PCTN(const glm::vec3& position, const glm::vec4& color, const glm::vec2& texcoord, const glm::vec3& normal) {
			return { position, normal, color, texcoord };
		}
	};

	template<int N>
	class Vert
	{
	public:
		float data[N];
	protected:
		void set(const float& v, int& pos)
		{
			data[pos++] = v;
		}
		void set(const glm::vec2& v, int& pos)
		{
			data[pos++] = v.x;
			data[pos++] = v.y;
		}
		void set(const glm::vec3& v, int& pos)
		{
			data[pos++] = v.x;
			data[pos++] = v.y;
			data[pos++] = v.z;
		}
		void set(const glm::vec4& v, int& pos)
		{
			data[pos++] = v.x;
			data[pos++] = v.y;
			data[pos++] = v.z;
			data[pos++] = v.w;
		}
	};

	class VertexP3T2T2T2N3 : public Vert<3+2+2+2+3>
	{
	public:
		VertexP3T2T2T2N3(const glm::vec3& pos, const glm::vec2& t1, const glm::vec2& t2, const glm::vec2& t3, const glm::vec3& n)
		{
			int index = 0;
			set(pos, index);
			set(t1, index);
			set(t2, index);
			set(t3, index);
			set(n, index);
		}
	};

	class VertexP3T2T2C4N3 : public Vert<3 + 2 + 2 + 4 + 3>
	{
	public:
		VertexP3T2T2C4N3(const glm::vec3& pos, const glm::vec2& t1, const glm::vec2& t2, const glm::vec4& c1, const glm::vec3& n)
		{
			int index = 0;
			set(pos, index);
			set(t1, index);
			set(t2, index);
			set(c1, index);
			set(n, index);
		}
	};


	class VertexP3T2N3 : public Vert<3 + 2 + 3>
	{
	public:
		VertexP3T2N3(const glm::vec3& pos, const glm::vec2& t, const glm::vec3& n)
		{
			int index = 0;
			set(pos, index);
			set(t, index);
			set(n, index);
		}
		VertexP3T2N3(const glm::vec3& pos, const glm::vec2& t)
		{
			int index = 0;
			set(pos, index);
			set(t, index);
		}
		VertexP3T2N3(const glm::vec3& pos, const glm::vec3& n)
		{
			int index = 0;
			set(pos, index);
			index += 2;
			set(n, index);
		}
		VertexP3T2N3(const glm::vec3& pos)
		{
			int index = 0;
			set(pos, index);
		}
	};

	class VertexP3T2N3C1 : public Vert<3 + 2 + 3 + 1>
	{
	public:
		VertexP3T2N3C1(const glm::vec3& pos, const glm::vec2& t, const glm::vec3& n, float twoSide)
		{
			int index = 0;
			set(pos, index);
			set(t, index);
			set(n, index);
			set(twoSide, index);
		}
	};


	class VertexP3T2 : public Vert<3 + 2>
	{
	public:
		VertexP3T2(const glm::vec3& pos, const glm::vec2& t)
		{
			int index = 0;
			set(pos, index);
			set(t, index);
		}
	};

	class VertexP2T2A1 : public Vert<3 + 2 + 1>
	{
	public:
		VertexP2T2A1(const glm::vec2& pos, const glm::vec2& t, float alpha)
		{
			int index = 0;
			set(pos, index);
			set(t, index);
			set(alpha, index);
		}
	};