#include <QtWidgets/QApplication>
#include <QOpenGLWidget>
#include <QSurfaceFormat>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLContext>

#include <string>
#include <iostream>
#include <fstream>

#define ASSERT(x) if(!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
	x;\
	ASSERT(GLLogCall(#x, __FILE__, __LINE__))

void GLClearError() {
	QOpenGLFunctions_3_3_Core *f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();
	while (f->glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, int line) {
	QOpenGLFunctions_3_3_Core *f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();
	if (GLenum error = f->glGetError()) {
		std::cout << "[OpenGL Error] (" << error << ") : " << function << ", "
			<< file << " : " << line << std::endl;
		return false;
	}

	return true;
}

std::string LoadShader(const std::string& filepath) {
	std::ifstream stream(filepath);

	std::string whole, line;
	while (getline(stream, line)) {
		whole = whole + line + '\n';
	}

	return whole;
}

unsigned int CompileShader(unsigned int type, const std::string& source) {
	QOpenGLFunctions_3_3_Core *f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();

	GLCall(unsigned int id = f->glCreateShader(type));
	const char* src = source.c_str();
	GLCall(f->glShaderSource(id, 1, &src, 0));
	GLCall(f->glCompileShader(id));

	int result;
	GLCall(f->glGetShaderiv(id, GL_COMPILE_STATUS, &result));
	if (result == GL_FALSE) {
		int length;
		GLCall(f->glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));

		char *message = new char[length * sizeof(char)];
		GLCall(f->glGetShaderInfoLog(id, length, &length, message));

		std::cout << "Failed to compile " <<
			(type == GL_VERTEX_SHADER ? "vertex " : "fragment ") <<
			"shader!" << std::endl;
		std::cout << message << std::endl;

		delete message;

		return 0;
	}

	return id;
}

unsigned int CreateShaderProgram(const std::string& vertexShader, const std::string& fragmentShader) {
	QOpenGLFunctions_3_3_Core *f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();

	GLCall(unsigned int program = f->glCreateProgram());
	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

	GLCall(f->glAttachShader(program, vs));
	GLCall(f->glAttachShader(program, fs));
	GLCall(f->glLinkProgram(program));
	GLCall(f->glValidateProgram(program));

	GLCall(f->glDeleteShader(vs));
	GLCall(f->glDeleteShader(fs));

	return program;
}

class OpenGL : public QOpenGLWidget {
private:
	float positions[8] = {
		-0.5f, -0.5f,
		 0.5f, -0.5f,
		 0.5f,  0.5f,
		-0.5f,  0.5f
	};

	unsigned int indices[6] = {
		0, 1, 2,
		2, 3, 0
	};

	unsigned int vao;
	unsigned int vbo;
	unsigned int ibo;
	unsigned int program;
	int location;

	bool initialized;
	
public:
	OpenGL() : initialized(false) {	}
	~OpenGL() {
		if (initialized) {
			QOpenGLFunctions_3_3_Core *f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();
			
			GLCall(f->glDeleteVertexArrays(1, &vao));
			GLCall(f->glDeleteBuffers(1, &vbo));
			GLCall(f->glDeleteBuffers(1, &ibo));
			GLCall(f->glDeleteProgram(program));
		}
	}

private:
	void initializeGL() {
		QOpenGLFunctions_3_3_Core *f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();

		GLCall(std::cout << f->glGetString(GL_VERSION) << std::endl);

		std::string vertexShader = LoadShader("res/shaders/Basic.vertex");
		std::string fragmentShader = LoadShader("res/shaders/Basic.fragment");

		program = CreateShaderProgram(vertexShader, fragmentShader);
		
		GLCall(f->glUseProgram(program));
		GLCall(location = f->glGetUniformLocation(program, "u_Color"));
		ASSERT(location != -1);

		GLCall(f->glGenVertexArrays(1, &vao));
		GLCall(f->glBindVertexArray(vao));
		
		GLCall(f->glGenBuffers(1, &vbo));
		GLCall(f->glBindBuffer(GL_ARRAY_BUFFER, vbo));
		GLCall(f->glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), positions, GL_STATIC_DRAW));

		GLCall(f->glEnableVertexAttribArray(0));
		GLCall(f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0));

		GLCall(f->glGenBuffers(1, &ibo));
		GLCall(f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
		GLCall(f->glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(float), indices, GL_STATIC_DRAW));
	}

	void paintGL() {
		QOpenGLFunctions_3_3_Core *f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();

		GLCall(f->glClear(GL_COLOR_BUFFER_BIT));

		static float r = 0.0f;
		static float increment = 0.05f;

		r += increment;
		if (r > 1.0f) {
			r = 1.0f;
			increment = -0.05f;
		}
		else if (r < 0.0f) {
			r = 0.0f;
			increment = 0.05f;
		}
			
		std::cout << "r : " << r << std::endl;
		GLCall(f->glUseProgram(program));
		GLCall(f->glUniform4f(location, r, 0.3f, 0.8f, 1.0f));

		GLCall(f->glBindVertexArray(vao));
		GLCall(f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
		GLCall(f->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
	}
};

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);

	QSurfaceFormat format;
	format.setVersion(3, 3);
	format.setProfile(QSurfaceFormat::CoreProfile);

	OpenGL openGL;
	openGL.setFormat(format);
	openGL.show();

	return a.exec();
}
