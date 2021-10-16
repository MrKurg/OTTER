#include "ObjLoader.h"

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

// Borrowed from https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
#pragma region String Trimming

// trim from start (in place)
static inline void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
		}));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
		}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string& s) {
	ltrim(s);
	rtrim(s);
}

#pragma endregion 

VertexArrayObject::Sptr ObjLoader::LoadFromFile(const std::string& filename)
{
	// Open our file in binary mode
	std::ifstream file;
	file.open(filename, std::ios::binary);

	// If our file fails to open, we will throw an error
	if (!file) {
		throw std::runtime_error("Failed to open file");
	}

	std::string line;
	
	// TODO: Load data from file
	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> textureCoords;
	std::vector<glm::vec3> normals;
	std::vector<glm::ivec3> vertices;

	glm::vec3 vecData;	//Stores Vertex position
	glm::vec2 vtData;	//Stores vertex texture coords
	glm::vec3 vnData;	//Stores vertex normals
	glm::ivec3 vertexIndices;

	// Read and process the entire file
	while (file.peek() != EOF) {
		// Read in the first part of the line (ex: f, v, vn, etc...)
		std::string command;
		file >> command;

		// We will ignore the rest of the line for comment lines
		if (command == "#") {
			std::getline(file, line);
		}
		// The v command defines a vertex's position
		else if (command == "v") {
			// Read in and store a position
			file >> vecData.x >> vecData.y >> vecData.z;
			positions.push_back(vecData);
		}
		// TODO: handle normals and textures
		// The vt command defines a vertex's texture coordinates
		else if (command == "vt") {
			// Read in and store a position
			file >> vtData.x >> vtData.y;
			textureCoords.push_back(vtData);
		}
		// The vn command defines a vertex's normals
		else if (command == "vn") {
			file >> vnData.x >> vnData.y >> vnData.z;
			normals.push_back(vnData);
		}
		
		// The f command defines a polygon in the mesh
		// NOTE: make sure you triangulate in blender, otherwise it will
		// output quads instead of triangles
		else if (command == "f") {
			// Read the rest of the line from the file
			std::getline(file, line);

			// Trim whitespace from either end of the line
			trim(line);

			// Create a string stream so we can use streaming operators on it
			std::stringstream stream = std::stringstream(line);
			
			for (int ix = 0; ix < 3; ix++) {
				// Read in the 3 attributes (position, UV, normal)
				char separator;
				stream >> vertexIndices.x >> separator >> vertexIndices.y >> separator >> vertexIndices.z;

				// OBJ format uses 1-based indices
				vertexIndices -= glm::ivec3(1);

				// add the vertex indices to the list
				// NOTE: This will create duplicate vertices!
				vertices.push_back(vertexIndices);
			}
		}

	}

	// TODO: Generate mesh from the data we loaded
	std::vector<VertexPosNormTexCol> vertexData;

	for (int ix = 0; ix < vertices.size(); ix++) {
		glm::ivec3 attribs = vertices[ix];
		glm::ivec3 attribs2 = vertices[ix];
		glm::ivec3 attribs3 = vertices[ix];

		// Extract attributes from lists (except color)
		glm::vec3 position = positions[attribs.x];
		glm::vec3 normal = normals[attribs2.x];
		glm::vec2 uv = textureCoords[attribs3.x];
		glm::vec4 color = glm::vec4(1.0f);

		// Add the vertex to the mesh
		vertexData.push_back(VertexPosNormTexCol(position, normal, uv, color));
	}

	// Create a vertex buffer and load all our vertex data
	VertexBuffer::Sptr vertexBuffer = VertexBuffer::Create();
	vertexBuffer->LoadData(vertexData.data(), vertexData.size());

	// Create the VAO, and add the vertices
	VertexArrayObject::Sptr result = VertexArrayObject::Create();
	result->AddVertexBuffer(vertexBuffer, VertexPosNormTexCol::V_DECL);

	return result;
	//return VertexArrayObject::Create();
}
