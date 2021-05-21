#define _CRT_SECURE_NO_WARNINGS

#include <iostream>

#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <stdlib.h>
#include <map>

#include "include/GL/glew.h"		
#include "include/GLFW/glfw3.h" 
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "lib-vc2017/glew32.lib")
#pragma comment(lib, "lib-vc2017/glfw3.lib")

#define _CRT_SECURE_NO_WARNINGS

GLFWwindow* window;

using namespace glm;
using namespace std;

#define FOURCC_DXT1 0x31545844 // Equivalent to "DXT1" in ASCII
#define FOURCC_DXT3 0x33545844 // Equivalent to "DXT3" in ASCII
#define FOURCC_DXT5 0x35545844 // Equivalent to "DXT5" in ASCII

glm::mat4 ModelMatrix = glm::mat4(1.0);
glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;

glm::mat4 getModelMatrix() {
	return ModelMatrix;
}
glm::mat4 getViewMatrix() {
	return ViewMatrix;
}
glm::mat4 getProjectionMatrix() {
	return ProjectionMatrix;
}

// Initial position : on + Z
glm::vec3 position = glm::vec3(0, 10, 10);
glm::vec3 center = glm::vec3(0, 0, 0);
// Initial horizontal angle : toward -Z
float horizontalAngle = 3.14f;
// Initial vertical angle : none
float verticalAngle = 0.0f;
// Initial Field of View
float initialFoV = 45.0f;

float speed = 12.0f; // 12 units / second
float mouseSpeed = 0.005f;

//�༺�� ���� ���� ����
bool rot_angle = true;

void computeMatricesFromInputs() {

	// glfwGetTime is called only once, the first time this function is called
	static double lastTime = glfwGetTime();

	// Compute time difference between current and last frame
	double currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTime);

	// Get mouse position
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// Reset mouse position for next frame
	glfwSetCursorPos(window, 1024 / 2, 768 / 2);

	// Compute new orientation
	horizontalAngle += mouseSpeed * float(1024 / 2 - xpos);
	verticalAngle += mouseSpeed * float(768 / 2 - ypos);

	// Direction : Spherical coordinates to Cartesian coordinates conversion
	glm::vec3 direction(
		cos(verticalAngle) * sin(horizontalAngle),
		sin(verticalAngle),
		cos(verticalAngle) * cos(horizontalAngle)
	);

	// Right vector
	glm::vec3 right = glm::vec3(
		sin(horizontalAngle - 3.14f / 2.0f),
		0,
		cos(horizontalAngle - 3.14f / 2.0f)
	);

	// Up vector
	glm::vec3 up = glm::cross(right, direction);

	// Move forward
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		position += direction * deltaTime * speed;
	}
	// Move backward
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		position -= direction * deltaTime * speed;
	}
	// Strafe right
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		position += right * deltaTime * speed;
	}
	// Strafe left
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		position -= right * deltaTime * speed;
	}
	// �����̽� ������ �༺ ���/����
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		rot_angle = !rot_angle;
	}

	float FoV = initialFoV;// - 5 * glfwGetMouseWheel(); // Now GLFW 3 requires setting up a callback for this. It's a bit too complicated for this beginner's tutorial, so it's disabled instead.

	// Projection matrix : 45�� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	ProjectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, 0.1f, 1000.0f);

	// Camera matrix
	ViewMatrix = glm::lookAt(
		position,           // Camera is here
		position + direction, // and looks here : at the same position, plus "direction"
		up                 // Head is up (set to 0,-1,0 to look upside-down)
	);

	// For the next frame, the "last time" will be "now"
	lastTime = currentTime;
}

// Returns true iif v1 can be considered equal to v2
bool is_near(float v1, float v2) {
	return fabs(v1 - v2) < 0.01f;
}

// Searches through all already-exported vertices
// for a similar one.
// Similar = same position + same UVs + same normal
bool getSimilarVertexIndex(
	glm::vec3 & in_vertex,
	glm::vec2 & in_uv,
	glm::vec3 & in_normal,
	std::vector<glm::vec3> & out_vertices,
	std::vector<glm::vec2> & out_uvs,
	std::vector<glm::vec3> & out_normals,
	unsigned short & result
) {
	// Lame linear search
	for (unsigned int i = 0; i < out_vertices.size(); i++) {
		if (
			is_near(in_vertex.x, out_vertices[i].x) &&
			is_near(in_vertex.y, out_vertices[i].y) &&
			is_near(in_vertex.z, out_vertices[i].z) &&
			is_near(in_uv.x, out_uvs[i].x) &&
			is_near(in_uv.y, out_uvs[i].y) &&
			is_near(in_normal.x, out_normals[i].x) &&
			is_near(in_normal.y, out_normals[i].y) &&
			is_near(in_normal.z, out_normals[i].z)
			) {
			result = i;
			return true;
		}
	}
	// No other vertex could be used instead.
	// Looks like we'll have to add it to the VBO.
	return false;
}

void indexVBO_slow(
	std::vector<glm::vec3> & in_vertices,
	std::vector<glm::vec2> & in_uvs,
	std::vector<glm::vec3> & in_normals,

	std::vector<unsigned short> & out_indices,
	std::vector<glm::vec3> & out_vertices,
	std::vector<glm::vec2> & out_uvs,
	std::vector<glm::vec3> & out_normals
) {
	// For each input vertex
	for (unsigned int i = 0; i < in_vertices.size(); i++) {

		// Try to find a similar vertex in out_XXXX
		unsigned short index;
		bool found = getSimilarVertexIndex(in_vertices[i], in_uvs[i], in_normals[i], out_vertices, out_uvs, out_normals, index);

		if (found) { // A similar vertex is already in the VBO, use it instead !
			out_indices.push_back(index);
		}
		else { // If not, it needs to be added in the output data.
			out_vertices.push_back(in_vertices[i]);
			out_uvs.push_back(in_uvs[i]);
			out_normals.push_back(in_normals[i]);
			out_indices.push_back((unsigned short)out_vertices.size() - 1);
		}
	}
}

struct PackedVertex {
	glm::vec3 position;
	glm::vec2 uv;
	glm::vec3 normal;
	bool operator<(const PackedVertex that) const {
		return memcmp((void*)this, (void*)&that, sizeof(PackedVertex)) > 0;
	};
};

bool getSimilarVertexIndex_fast(
	PackedVertex & packed,
	std::map<PackedVertex, unsigned short> & VertexToOutIndex,
	unsigned short & result
) {
	std::map<PackedVertex, unsigned short>::iterator it = VertexToOutIndex.find(packed);
	if (it == VertexToOutIndex.end()) {
		return false;
	}
	else {
		result = it->second;
		return true;
	}
}

void indexVBO(
	std::vector<glm::vec3> & in_vertices,
	std::vector<glm::vec2> & in_uvs,
	std::vector<glm::vec3> & in_normals,

	std::vector<unsigned short> & out_indices,
	std::vector<glm::vec3> & out_vertices,
	std::vector<glm::vec2> & out_uvs,
	std::vector<glm::vec3> & out_normals
) {
	std::map<PackedVertex, unsigned short> VertexToOutIndex;

	// For each input vertex
	for (unsigned int i = 0; i < in_vertices.size(); i++) {

		PackedVertex packed = { in_vertices[i], in_uvs[i], in_normals[i] };


		// Try to find a similar vertex in out_XXXX
		unsigned short index;
		bool found = getSimilarVertexIndex_fast(packed, VertexToOutIndex, index);

		if (found) { // A similar vertex is already in the VBO, use it instead !
			out_indices.push_back(index);
		}
		else { // If not, it needs to be added in the output data.
			out_vertices.push_back(in_vertices[i]);
			out_uvs.push_back(in_uvs[i]);
			out_normals.push_back(in_normals[i]);
			unsigned short newindex = (unsigned short)out_vertices.size() - 1;
			out_indices.push_back(newindex);
			VertexToOutIndex[packed] = newindex;
		}
	}
}

void indexVBO_TBN(
	std::vector<glm::vec3> & in_vertices,
	std::vector<glm::vec2> & in_uvs,
	std::vector<glm::vec3> & in_normals,
	std::vector<glm::vec3> & in_tangents,
	std::vector<glm::vec3> & in_bitangents,

	std::vector<unsigned short> & out_indices,
	std::vector<glm::vec3> & out_vertices,
	std::vector<glm::vec2> & out_uvs,
	std::vector<glm::vec3> & out_normals,
	std::vector<glm::vec3> & out_tangents,
	std::vector<glm::vec3> & out_bitangents
) {
	// For each input vertex
	for (unsigned int i = 0; i < in_vertices.size(); i++) {

		// Try to find a similar vertex in out_XXXX
		unsigned short index;
		bool found = getSimilarVertexIndex(in_vertices[i], in_uvs[i], in_normals[i], out_vertices, out_uvs, out_normals, index);

		if (found) { // A similar vertex is already in the VBO, use it instead !
			out_indices.push_back(index);

			// Average the tangents and the bitangents
			out_tangents[index] += in_tangents[i];
			out_bitangents[index] += in_bitangents[i];
		}
		else { // If not, it needs to be added in the output data.
			out_vertices.push_back(in_vertices[i]);
			out_uvs.push_back(in_uvs[i]);
			out_normals.push_back(in_normals[i]);
			out_tangents.push_back(in_tangents[i]);
			out_bitangents.push_back(in_bitangents[i]);
			out_indices.push_back((unsigned short)out_vertices.size() - 1);
		}
	}
}

bool loadOBJ(		
	const char * path,
	std::vector<glm::vec3> & out_vertices,
	std::vector<glm::vec2> & out_uvs,
	std::vector<glm::vec3> & out_normals			//���� ����, ������ ����, �븻�� ����
	)
{
	printf("Loading OBJ file %s...\n", path);

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;


	FILE * file = fopen(path, "r");
	if (file == NULL) {
		printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
		getchar();
		return false;
	}

	while (1) {

		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.

				   // else : parse lineHeader

		if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);								//obj���� ���ؽ� ������ �о��
		}
		else if (strcmp(lineHeader, "vt") == 0) {											
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
			temp_uvs.push_back(uv);											//obj���� ������ ������ �о��
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);									//obj���� �븻�� ������ �о��
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9) {
				printf("File can't be read by our simple parser :-( Try exporting with other options\n");
				fclose(file);
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);						
		}
		else {
			// Probably a comment, eat up the rest of the line
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}

	}

	// For each vertex of each triangle
	for (unsigned int i = 0; i < vertexIndices.size(); i++) {

		// Get the indices of its attributes
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normalIndex = normalIndices[i];

		// Get the attributes thanks to the index
		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		glm::vec2 uv = temp_uvs[uvIndex - 1];
		glm::vec3 normal = temp_normals[normalIndex - 1];					//obj�� 1���� �����ϹǷ� 1�� �������ν� 0���� �����ϰ� ����

		// Put the attributes in buffers
		out_vertices.push_back(vertex); 
		out_uvs.push_back(uv);
		out_normals.push_back(normal);										//�������� ��������

	}
	fclose(file);
	return true;
}

GLuint loadDDS(const char * imagepath) {

	unsigned char header[124];

	FILE *fp;

	/* try to open the file */
	fopen_s(&fp, imagepath, "rb");
	if (fp == NULL) {
		printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", imagepath); getchar();
		return 0;
	}

	/* verify the type of file */
	char filecode[4];
	fread(filecode, 1, 4, fp);
	if (strncmp(filecode, "DDS ", 4) != 0) {
		fclose(fp);
		return 0;
	}

	/* get the surface desc */
	fread(&header, 124, 1, fp);

	unsigned int height = *(unsigned int*)&(header[8]);
	unsigned int width = *(unsigned int*)&(header[12]);
	unsigned int linearSize = *(unsigned int*)&(header[16]);
	unsigned int mipMapCount = *(unsigned int*)&(header[24]);
	unsigned int fourCC = *(unsigned int*)&(header[80]);


	unsigned char * buffer;
	unsigned int bufsize;
	/* how big is it going to be including all mipmaps? */
	bufsize = mipMapCount > 1 ? linearSize * 2 : linearSize;
	buffer = (unsigned char*)malloc(bufsize * sizeof(unsigned char));
	fread(buffer, 1, bufsize, fp);
	/* close the file pointer */
	fclose(fp);

	unsigned int components = (fourCC == FOURCC_DXT1) ? 3 : 4;
	unsigned int format;
	switch (fourCC)
	{
	case FOURCC_DXT1:
		format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		break;
	case FOURCC_DXT3:
		format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		break;
	case FOURCC_DXT5:
		format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		break;
	default:
		free(buffer);
		return 0;
	}

	// Create one OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
	unsigned int offset = 0;

	/* load the mipmaps */
	for (unsigned int level = 0; level < mipMapCount && (width || height); ++level)
	{
		unsigned int size = ((width + 3) / 4)*((height + 3) / 4)*blockSize;
		glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height,
			0, size, buffer + offset);

		offset += size;
		width /= 2;
		height /= 2;

		// Deal with Non-Power-Of-Two textures. This code is not included in the webpage to reduce clutter.
		if (width < 1) width = 1;
		if (height < 1) height = 1;

	}

	free(buffer);

	return textureID;


}

GLuint LoadShaders(const char * vertex_file_path, const char * fragment_file_path) {

	// ���̴��� ����
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// ���ؽ� ���̴� �ڵ带 ���Ͽ��� �б�
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if (VertexShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}
	else {
		printf("���� %s �� ���� �� ����. ��Ȯ�� ���丮�� ��� ���Դϱ� ? FAQ �� �켱 �о�� �� ���� ������!\n", vertex_file_path);
		getchar();
		return 0;
	}

	// �����׸�Ʈ ���̴� �ڵ带 ���Ͽ��� �б�
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if (FragmentShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;


	// ���ؽ� ���̴��� ������
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// ���ؽ� ���̴��� �˻�
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// �����׸�Ʈ ���̴��� ������
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// �����׸�Ʈ ���̴��� �˻�
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// ���α׷��� ��ũ
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// ���α׷� �˻�
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

int main(void)
{
	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "Final_Report", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// We would expect width and height to be 1024 and 768
	int windowWidth = 1024;
	int windowHeight = 768;
	// But on MacOS X with a retina screen it'll be 1024*2 and 768*2, so we get the actual framebuffer size:
	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	//���콺 ����� �� ������
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//���콺 ��ġ ���� ��ũ�� ���ͷ�.
	glfwPollEvents();
	glfwSetCursorPos(window, 1024 / 2, 768 / 2);

	//��� ����
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);
	//���� ���� ���
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders("������_20161642_vs.shader", "������_20161642_fs.shader");

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");

	// Our vertices. Tree consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
	// A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	bool res = loadOBJ("sphere.obj", vertices, uvs, normals);

	std::vector<unsigned short> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;
	indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);


	// Load the texture using any two methods
	GLuint Texture_sun = loadDDS("�¾�.DDS");
	GLuint Texture_earth = loadDDS("����.DDS");
	GLuint Texture_moon = loadDDS("��.DDS");
	GLuint Texture_Mercury = loadDDS("����.DDS");
	GLuint Texture_Venus = loadDDS("�ݼ�.DDS");
	GLuint Texture_Mars = loadDDS("ȭ��.DDS");
	GLuint Texture_Jupiter = loadDDS("��.DDS");
	GLuint Texture_Saturn = loadDDS("�伺.DDS");
	GLuint Texture_Uranus = loadDDS("õ�ռ�.DDS");
	GLuint Texture_Naptune = loadDDS("�ؿռ�.DDS");

	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");

	GLuint sphere_vertexbuffer;
	glGenBuffers(1, &sphere_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

	GLuint sphere_uvbuffer;
	glGenBuffers(1, &sphere_uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

	GLuint colorbuffer;
	glGenBuffers(1, &colorbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer;
	glGenBuffers(1, &elementbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);

	//���� OBJ����Ͽ� vertices, uvs, normal ���� //��ü(õü�� ������)

	//���α׷��� ��ID�� �־���
	glUseProgram(programID);
	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
	GLuint LightID2 = glGetUniformLocation(programID, "LightPosition_worldspace2");
	GLuint LightMatID = glGetUniformLocation(programID, "L");

	//���� ��ġ

	//==========================================================
	//�¾�� ũ��
	//�¾�	 ũ�� : 69�� 6300km
	//����	 ũ�� : 2440km			1 / 285
	//�ݼ�	 ũ�� : 6052km			1 / 115
	//����	 ũ�� : 6378km			1 / 109
	//ȭ��	 ũ�� : 3390km			1 / 205
	//��	 ũ�� : 6�� 9911km		1 / 10
	//�伺	 ũ�� : 5�� 8232km		1 / 12
	//õ�ռ� ũ�� : 2�� 5362km		1 / 27.5
	//�ؿռ� ũ�� : 2�� 4622km		1 / 28.3

	//ũ�Ⱑ �ʹ� �۾����� �Ⱥ��̴� ����� *10�� ����

	//�¾�� �Ÿ�					(���� �鸸 / ���� ����)
	//����   �Ÿ� : 58				Mercury_translate
	//�ݼ�   �Ÿ� : 108				Mercury_translate * 1.9
	//����   �Ÿ� : 149				Mercury_translate * 2.6
	//ȭ��   �Ÿ� : 227				Mercury_translate * 3.9
	//��   �Ÿ� : 778				Mercury_translate * 13.4
	//�伺   �Ÿ� : 1430			Mercury_translate * 24.6
	//õ�ռ� �Ÿ� : 2875			Mercury_translate * 49.5
	//�ؿռ� �Ÿ� : 4495			Mercury_translate * 77.5

	//������ �� ���� �Ÿ��� 38��km = a * (1 / 15.2)

	//==========================================================

	float Sun_scale = 1.0f;
	float min_scale = 10.0f;
	float Mercury_translate = 2.0f;

	//���� �Ÿ��� �� ũ��� ������ �� �Ⱥ��̴� ����� ���� ������

	//glm::mat4 model_Sun = glm::mat4(1.0f);
	//glm::mat4 model_Sun_scale = glm::mat4(1.0f);
	//model_Sun = glm::translate(model_Sun, vec3(0.0f, 0.0f, 0.0f));
	//model_Sun_scale = glm::scale(model_Sun_scale, vec3(Sun_scale));				// �¾� �𵨸�

	////============================================================================

	//glm::mat4 model_Mercury = glm::mat4(model_Sun);

	//model_Mercury = glm::translate(model_Mercury, vec3(Mercury_translate, 0.0f, 0.0f));

	//glm::mat4 model_Mercury_scale = glm::mat4(1.0f);
	//model_Mercury_scale = glm::scale(model_Mercury_scale, vec3(Sun_scale / 285 * min_scale));			// ���� �𵨸�

	////============================================================================

	//glm::mat4 model_Venus = glm::mat4(model_Sun);

	//model_Venus = glm::translate(model_Venus, vec3(Mercury_translate * 1.9f, 0.0f, 0.0f));

	//glm::mat4 model_Venus_scale = glm::mat4(1.0f);
	//model_Venus_scale = glm::scale(model_Venus_scale, vec3(Sun_scale / 115 * min_scale));			// �ݼ� �𵨸�

	////============================================================================

	//glm::mat4 model_Earth = glm::mat4(model_Sun);
	//glm::mat4 model_Moon = glm::mat4(model_Earth);

	//model_Earth = glm::translate(model_Earth, vec3(Mercury_translate * 2.6f, 0.0f, 0.0f));

	//glm::mat4 model_Earth_scale = glm::mat4(1.0f);
	//model_Earth_scale = glm::scale(model_Earth_scale, vec3(Sun_scale / 109 * min_scale));			// ���� �𵨸�

	////============================================================================

	//model_Moon = glm::translate(model_Moon, vec3(Mercury_translate * 2.6f / 15.2f, 0.0f, 0.0f));

	//glm::mat4 model_Moon_scale = glm::mat4(1.0f);
	//model_Moon_scale = glm::scale(model_Moon_scale, vec3(Sun_scale / 400 * min_scale));			// �� �𵨸�

	////============================================================================

	//glm::mat4 model_Mars = glm::mat4(model_Sun);

	//model_Mars = glm::translate(model_Mars, vec3(Mercury_translate * 3.9f, 0.0f, 0.0f));

	//glm::mat4 model_Mars_scale = glm::mat4(1.0f);
	//model_Mars_scale = glm::scale(model_Mars_scale, vec3(Sun_scale / 205 * min_scale));			// ȭ�� �𵨸�

	////============================================================================

	//glm::mat4 model_Jupiter = glm::mat4(model_Sun);

	//model_Jupiter = glm::translate(model_Jupiter, vec3(Mercury_translate * 13.4f, 0.0f, 0.0f));

	//glm::mat4 model_Jupiter_scale = glm::mat4(1.0f);
	//model_Jupiter_scale = glm::scale(model_Jupiter_scale, vec3(Sun_scale / 10 * min_scale));			// �� �𵨸�

	////============================================================================

	//glm::mat4 model_Saturn = glm::mat4(model_Sun);

	//model_Saturn = glm::translate(model_Saturn, vec3(Mercury_translate * 24.6f, 0.0f, 0.0f));

	//glm::mat4 model_Saturn_scale = glm::mat4(1.0f);
	//model_Saturn_scale = glm::scale(model_Saturn_scale, vec3(Sun_scale / 12 * min_scale));			// �伺 �𵨸�

	////============================================================================

	//glm::mat4 model_Uranus = glm::mat4(model_Sun);

	//model_Uranus = glm::translate(model_Uranus, vec3(Mercury_translate * 49.5f, 0.0f, 0.0f));

	//glm::mat4 model_Uranus_scale = glm::mat4(1.0f);
	//model_Uranus_scale = glm::scale(model_Uranus_scale, vec3(Sun_scale / 27.5f * min_scale));			// õ�ռ� �𵨸�

	////============================================================================

	//glm::mat4 model_Neptune = glm::mat4(model_Sun);

	//model_Neptune = glm::translate(model_Neptune, vec3(Mercury_translate * 77.5f, 0.0f, 0.0f));

	//glm::mat4 model_Neptune_scale = glm::mat4(1.0f);
	//model_Neptune_scale = glm::scale(model_Neptune_scale, vec3(Sun_scale / 28.3f * min_scale));			// �ؿռ� �𵨸�

	////============================================================================

	glm::mat4 model_Sun = glm::mat4(1.0f);
	glm::mat4 model_Sun_scale = glm::mat4(1.0f);
	model_Sun = glm::translate(model_Sun, vec3(0.0f, 0.0f, 0.0f));
	model_Sun_scale = glm::scale(model_Sun_scale, vec3(Sun_scale));				// �¾� �𵨸�

	//============================================================================

	glm::mat4 model_Mercury = glm::mat4(model_Sun);

	model_Mercury = glm::translate(model_Mercury, vec3(Mercury_translate * 1.2f, 0.0f, 0.0f));

	glm::mat4 model_Mercury_scale = glm::mat4(1.0f);
	model_Mercury_scale = glm::scale(model_Mercury_scale, vec3(Sun_scale / 27.5f * min_scale));			// ���� �𵨸�

	//============================================================================

	glm::mat4 model_Venus = glm::mat4(model_Sun);

	model_Venus = glm::translate(model_Venus, vec3(Mercury_translate * 2.0f, 0.0f, 0.0f));

	glm::mat4 model_Venus_scale = glm::mat4(1.0f);
	model_Venus_scale = glm::scale(model_Venus_scale, vec3(Sun_scale / 27.5f * min_scale));			// �ݼ� �𵨸�

	//============================================================================

	glm::mat4 model_Earth = glm::mat4(model_Sun);
	glm::mat4 model_Moon = glm::mat4(model_Earth);

	model_Earth = glm::translate(model_Earth, vec3(Mercury_translate * 3.0f, 0.0f, 0.0f));

	glm::mat4 model_Earth_scale = glm::mat4(1.0f);
	model_Earth_scale = glm::scale(model_Earth_scale, vec3(Sun_scale / 27.5f * min_scale));			// ���� �𵨸�

	//============================================================================

	model_Moon = glm::translate(model_Moon, vec3(Mercury_translate * 3.0f / 10.0f, 0.0f, 0.0f));

	glm::mat4 model_Moon_scale = glm::mat4(1.0f);
	model_Moon_scale = glm::scale(model_Moon_scale, vec3(Sun_scale / 100.0f * min_scale));			// �� �𵨸�

	//============================================================================

	glm::mat4 model_Mars = glm::mat4(model_Sun);

	model_Mars = glm::translate(model_Mars, vec3(Mercury_translate * 4.0f, 0.0f, 0.0f));

	glm::mat4 model_Mars_scale = glm::mat4(1.0f);
	model_Mars_scale = glm::scale(model_Mars_scale, vec3(Sun_scale / 27.5f * min_scale));			// ȭ�� �𵨸�

	//============================================================================

	glm::mat4 model_Jupiter = glm::mat4(model_Sun);

	model_Jupiter = glm::translate(model_Jupiter, vec3(Mercury_translate * 5.0f, 0.0f, 0.0f));

	glm::mat4 model_Jupiter_scale = glm::mat4(1.0f);
	model_Jupiter_scale = glm::scale(model_Jupiter_scale, vec3(Sun_scale / 27.5f * min_scale));			// �� �𵨸�

	//============================================================================

	glm::mat4 model_Saturn = glm::mat4(model_Sun);

	model_Saturn = glm::translate(model_Saturn, vec3(Mercury_translate * 6.0f, 0.0f, 0.0f));

	glm::mat4 model_Saturn_scale = glm::mat4(1.0f);
	model_Saturn_scale = glm::scale(model_Saturn_scale, vec3(Sun_scale / 27.5f * min_scale));			// �伺 �𵨸�

	//============================================================================

	glm::mat4 model_Uranus = glm::mat4(model_Sun);

	model_Uranus = glm::translate(model_Uranus, vec3(Mercury_translate * 7.0f, 0.0f, 0.0f));

	glm::mat4 model_Uranus_scale = glm::mat4(1.0f);
	model_Uranus_scale = glm::scale(model_Uranus_scale, vec3(Sun_scale / 27.5f * min_scale));			// õ�ռ� �𵨸�

	//============================================================================

	glm::mat4 model_Neptune = glm::mat4(model_Sun);

	model_Neptune = glm::translate(model_Neptune, vec3(Mercury_translate * 8.0f, 0.0f, 0.0f));

	glm::mat4 model_Neptune_scale = glm::mat4(1.0f);
	model_Neptune_scale = glm::scale(model_Neptune_scale, vec3(Sun_scale / 27.5f * min_scale));			// �ؿռ� �𵨸�

	//============================================================================

	//�¾�� ���� �ӵ�	| ���� �ֱ�		ȯ���(�ݿø�)
	//����	 48.87		| 1406.4�ð�	1.64 / 58.77
	//�ݼ�	 35			| 5832�ð�		1.18 / 243.71
	//����	 29.78		| 23.93�ð�
	//ȭ��	 24.13		| 24.62�ð�		0.81 / 1.02
	//��   13.076		| 9.93�ð�		0.44 / 0.41
	//�伺   9.69		| 10.7�ð�		0.33 / 0.45
	//õ�ռ� 6.81		| 17.2�ð�		0.23 / 0.72
	//�ؿռ� 5.43		| 16.1�ð�		0.18 / 0.67

	//�����ӵ��� �ʹ� ������ *10, ���� �ӵ� �ʹ� ������ ���� /10 �� ����

	float Earth_to_Sun = 29.78f / 10.0f;		//���� ���� �ӵ� / 10		> ������ ������ �и�� ���Ƿ� ���������� ���ϱ���
	float Earth_self   = 23.93f / 10.0f;		//���� ���� �ֱ� / 10

	glm::mat4 rot_Self = glm::mat4(1.0f);

	glm::mat4 rot_Earth_to_Sun = glm::mat4(model_Sun);					//���� ����	//������ ������ �������� ���
	glm::mat4 rot_Earth = glm::mat4(1.0f);								//���� ���� //������ ������ �������� ���

	glm::mat4 rot_Moon_to_Earth = glm::mat4(model_Earth);				//�� ����
	glm::mat4 rot_Moon = glm::mat4(1.0f);								//�� ����

	glm::mat4 rot_Mercury_to_Sun = glm::mat4(model_Sun);				//���� ����
	glm::mat4 rot_Venus_to_Sun = glm::mat4(model_Sun);					//�ݼ� ����
	glm::mat4 rot_Mars_to_Sun = glm::mat4(model_Sun);					//ȭ�� ����
	glm::mat4 rot_Jupiter_to_Sun = glm::mat4(model_Sun);				//�� ����
	glm::mat4 rot_Saturn_to_Sun = glm::mat4(model_Sun);					//�伺 ����
	glm::mat4 rot_Uranus_to_Sun = glm::mat4(model_Sun);					//õ�ռ� ����
	glm::mat4 rot_Neptune_to_Sun = glm::mat4(model_Sun);				//�ؿռ� ����

	glm::mat4 rot_Mercury = glm::mat4(1.0f);								//���� ����
	glm::mat4 rot_Venus = glm::mat4(1.0f);								//�ݼ� ����
	glm::mat4 rot_Mars = glm::mat4(1.0f);								//ȭ�� ����
	glm::mat4 rot_Jupiter = glm::mat4(1.0f);								//�� ����
	glm::mat4 rot_Saturn = glm::mat4(1.0f);								//�伺 ����
	glm::mat4 rot_Uranus = glm::mat4(1.0f);								//õ�ռ� ����
	glm::mat4 rot_Neptune = glm::mat4(1.0f);								//�ؿռ� ����

	//ȸ�� ����� ������

	float uv_offset = 0.0f;
	GLuint uvoffsetID = glGetUniformLocation(programID, "uvoffset");

	glm::vec3 lightPos = glm::vec3(0, -100, 0);
	glm::vec3 lightPos2 = glm::vec3(0, 8, 0);
	glm::mat4 lightMat(1.0);

	do {
		lightMat = glm::translate(lightMat, glm::vec3(5.0f, 5.0f, 5.0f));
		lightMat = glm::rotate(lightMat, glm::radians(2.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		//�𵨵� ��Ʈ���� ����
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		computeMatricesFromInputs();

		glUniformMatrix4fv(LightMatID, 1, GL_FALSE, &lightMat[0][0]);
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

		uv_offset += 0.00025f;
		glUniform1f(uvoffsetID, uv_offset);

		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);
		glUniform3f(LightID2, lightPos2.x, lightPos2.y, lightPos2.z);

		//=============================================================================================================================//

		//���� ������ �Ѱ���

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_sun);
		glUniform1i(TextureID, 0);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, sphere_vertexbuffer);
		glVertexAttribPointer(
			0,								  
			3,								  
			GL_FLOAT,						  
			GL_FALSE,						  
			0,								  
			(void*)0						  
		);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, sphere_uvbuffer);
		glVertexAttribPointer(
			1,                               
			2,                               
			GL_FLOAT,                        
			GL_FALSE,                        
			0,                               
			(void*)0                         
		);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glVertexAttribPointer(
			2,								 
			3,								 
			GL_FLOAT,						 
			GL_FALSE,						 
			0,								 
			(void*)0						 
		);

		//���� ������ �Ѱ���
		//�������� ȸ�� ��� ����

		rot_Self = glm::rotate(rot_Self, glm::radians(rot_angle * 0.1f), vec3(0.0f, 1.0f, 0.0f));		//���� �ӵ��� �ʹ� ���� *0.1

		MVP = ProjectionMatrix * ViewMatrix * rot_Self * model_Sun * model_Sun_scale;		//������ �۰� ���ְ� / ��ġ �� ���ְ� / ���� ������

		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		glDrawElements(
			GL_TRIANGLES,      
			indices.size(),    
			GL_UNSIGNED_SHORT, 
			(void*)0           
		);

		//=============================================================================================================================//

		//������ ������ �Ѱ���

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_Mercury);
		glUniform1i(TextureID, 0);

		//������ ������ �Ѱ���
		//�������� ȸ�� ��� ����

		rot_Mercury_to_Sun = glm::rotate(rot_Mercury_to_Sun, glm::radians(-rot_angle / (Earth_to_Sun * 1.64f)), vec3(0.0f, 1.0f, 0.0f));
		rot_Mercury = glm::rotate(rot_Mercury, glm::radians(rot_angle * (Earth_self * 58.77f)), vec3(0.0f, 1.0f, 0.0f));					

		MVP = ProjectionMatrix * ViewMatrix * rot_Mercury_to_Sun * model_Mercury * rot_Mercury * model_Mercury_scale;		

		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		glDrawElements(
			GL_TRIANGLES,     
			indices.size(),   
			GL_UNSIGNED_SHORT,
			(void*)0          
		);

		//=============================================================================================================================//

		//�ݼ��� ������ �Ѱ���

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_Venus);
		glUniform1i(TextureID, 0);

		//�ݼ��� ������ �Ѱ���
		//�������� ȸ�� ��� ����

		rot_Venus_to_Sun = glm::rotate(rot_Venus_to_Sun, glm::radians(-rot_angle / (Earth_to_Sun * 1.18f)), vec3(0.0f, 1.0f, 0.0f));
		rot_Venus = glm::rotate(rot_Venus, glm::radians(-rot_angle * (Earth_self * 243.71f)), vec3(0.0f, 1.0f, 0.0f));					//�ݼ��� ���� ������ �ݴ�	

		MVP = ProjectionMatrix * ViewMatrix * rot_Venus_to_Sun * model_Venus * rot_Venus * model_Venus_scale;			

		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		//=============================================================================================================================//

		//������ ������ �Ѱ���

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_earth);
		glUniform1i(TextureID, 0);

		//������ ������ �Ѱ���
		//�������� ȸ�� ��� ����

		rot_Earth_to_Sun = glm::rotate(rot_Earth_to_Sun, glm::radians(-rot_angle / Earth_to_Sun), vec3(0.0f, 1.0f, 0.0f));
		rot_Earth = glm::rotate(rot_Earth, glm::radians(rot_angle * Earth_self), vec3(0.0f, 1.0f, 0.0f));			

		MVP = ProjectionMatrix * ViewMatrix * rot_Earth_to_Sun * model_Earth * rot_Earth * model_Earth_scale;	

		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		//=============================================================================================================================//

		//���� ������ �Ѱ���

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_moon);
		glUniform1i(TextureID, 0);

		//���� ������ �Ѱ���
		//�������� ȸ�� ��� ����

		rot_Moon_to_Earth = glm::rotate(rot_Moon_to_Earth, glm::radians(-rot_angle / Earth_to_Sun), vec3(0.0f, 1.0f, 0.0f));				
		rot_Moon = glm::rotate(rot_Moon, glm::radians(rot_angle * Earth_self), vec3(0.0f, 1.0f, 0.0f));						//������ ���� ����/���� �ֱ�� ����					

		MVP = ProjectionMatrix * ViewMatrix * rot_Earth_to_Sun * rot_Moon_to_Earth * model_Moon * rot_Moon * model_Moon_scale;		

		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		//=============================================================================================================================//

		//ȭ���� ������ �Ѱ���

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_Mars);
		glUniform1i(TextureID, 0);

		//ȭ���� ������ �Ѱ���
		//�������� ȸ�� ��� ����

		rot_Mars_to_Sun = glm::rotate(rot_Mars_to_Sun, glm::radians(-rot_angle / (Earth_to_Sun * 0.81f)), vec3(0.0f, 1.0f, 0.0f));
		rot_Mars = glm::rotate(rot_Mars, glm::radians(rot_angle * (Earth_self * 1.02f)), vec3(0.0f, 1.0f, 0.0f));

		MVP = ProjectionMatrix * ViewMatrix * rot_Mars_to_Sun * model_Mars * rot_Mars * model_Mars_scale;

		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		//=============================================================================================================================//

		//���� ������ �Ѱ���

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_Jupiter);
		glUniform1i(TextureID, 0);

		//���� ������ �Ѱ���
		//�������� ȸ�� ��� ����

		rot_Jupiter_to_Sun = glm::rotate(rot_Jupiter_to_Sun, glm::radians(-rot_angle / (Earth_to_Sun * 0.44f)), vec3(0.0f, 1.0f, 0.0f));
		rot_Jupiter = glm::rotate(rot_Jupiter, glm::radians(rot_angle * (Earth_self * 0.41f)), vec3(0.0f, 1.0f, 0.0f));

		MVP = ProjectionMatrix * ViewMatrix * rot_Jupiter_to_Sun * model_Jupiter * rot_Jupiter * model_Jupiter_scale;

		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		//=============================================================================================================================//

		//�伺�� ������ �Ѱ���

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_Saturn);
		glUniform1i(TextureID, 0);

		//�伺�� ������ �Ѱ���
		//�������� ȸ�� ��� ����

		rot_Saturn_to_Sun = glm::rotate(rot_Saturn_to_Sun, glm::radians(-rot_angle / (Earth_to_Sun * 0.33f)), vec3(0.0f, 1.0f, 0.0f));
		rot_Saturn = glm::rotate(rot_Saturn, glm::radians(rot_angle * (Earth_self * 0.45f)), vec3(0.0f, 1.0f, 0.0f));

		MVP = ProjectionMatrix * ViewMatrix * rot_Saturn_to_Sun * model_Saturn * rot_Saturn * model_Saturn_scale;

		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		//=============================================================================================================================//

		//õ�ռ��� ������ �Ѱ���

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_Uranus);
		glUniform1i(TextureID, 0);

		//õ�ռ��� ������ �Ѱ���
		//�������� ȸ�� ��� ����

		rot_Uranus_to_Sun = glm::rotate(rot_Uranus_to_Sun, glm::radians(-rot_angle / (Earth_to_Sun * 0.23f)), vec3(0.0f, 1.0f, 0.0f));
		rot_Uranus = glm::rotate(rot_Uranus, glm::radians(rot_angle * (Earth_self * 0.72f)), vec3(0.5f, 0.0f, 0.0f));

		MVP = ProjectionMatrix * ViewMatrix * rot_Uranus_to_Sun * model_Uranus * rot_Uranus * model_Uranus_scale;

		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		//=============================================================================================================================//

		//�ؿռ��� ������ �Ѱ���

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture_Naptune);
		glUniform1i(TextureID, 0);

		//�ؿռ��� ������ �Ѱ���
		//�������� ȸ�� ��� ����

		rot_Neptune_to_Sun = glm::rotate(rot_Neptune_to_Sun, glm::radians(-rot_angle / (Earth_to_Sun * 0.18f)), vec3(0.0f, 1.0f, 0.0f));
		rot_Neptune = glm::rotate(rot_Neptune, glm::radians(rot_angle * (Earth_self * 0.67f)), vec3(0.0f, 1.0f, 0.0f));

		MVP = ProjectionMatrix * ViewMatrix * rot_Neptune_to_Sun * model_Neptune * rot_Neptune * model_Neptune_scale;

		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		//=============================================================================================================================//

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

	glDeleteBuffers(1, &sphere_vertexbuffer);
	glDeleteBuffers(1, &sphere_uvbuffer);
	glDeleteBuffers(1, &colorbuffer);

	glDeleteProgram(programID);
	glDeleteTextures(1, &Texture_sun);
	glDeleteTextures(1, &Texture_earth);
	glDeleteTextures(1, &Texture_moon);
	glDeleteTextures(1, &Texture_Mercury);
	glDeleteTextures(1, &Texture_Venus);
	glDeleteTextures(1, &Texture_Mars);
	glDeleteTextures(1, &Texture_Jupiter);
	glDeleteTextures(1, &Texture_Saturn);
	glDeleteTextures(1, &Texture_Uranus);
	glDeleteTextures(1, &Texture_Naptune);
	glDeleteVertexArrays(1, &VertexArrayID);

	glfwTerminate();

	return 0;
}