#pragma once

#include <glm/glm.hpp>

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"

#include "grid.hpp"

class oneCellData{
	GLfloat **vertex_data_buffer;

	GLint x;
	GLint z;

	GLint height;
	GLint colorID;
public:
	oneCellData(int x, int z);
	~oneCellData();
	GLfloat** get_vertex_data();
	void change_height( int n );
	void change_color( int newColorID);
	void change_vertex_height_data();
	GLint get_current_color();
	GLint get_current_height();
	void copy_old_data( oneCellData * old );
	void resetData();
};


class A1 : public CS488Window {
public:
	A1();
	virtual ~A1();

protected:
	virtual void init() override;
	virtual void appLogic() override;
	virtual void guiLogic() override;
	virtual void draw() override;
	virtual void cleanup() override;

	virtual bool cursorEnterWindowEvent(int entered) override;
	virtual bool mouseMoveEvent(double xPos, double yPos) override;
	virtual bool mouseButtonInputEvent(int button, int actions, int mods) override;
	virtual bool mouseScrollEvent(double xOffSet, double yOffSet) override;
	virtual bool windowResizeEvent(int width, int height) override;
	virtual bool keyInputEvent(int key, int action, int mods) override;

private:
	void initGrid();

	void resetGrid();
	void draw_cube( int x, int z );
	void draw_active_indicator();
	void draw_additional_indicator();

	void load_one_vertex_data( GLfloat * current_vertex, GLfloat* new_cube );
	void push_vertex_to_buffer(int a, int b, int c, GLfloat* new_cube, GLfloat ** thisCellData);
	void push_data_to_buffer( GLfloat * data_set, int size, float * color );

	// Fields related to the shader and uniforms.
	ShaderProgram m_shader;
	GLint P_uni; // Uniform location for Projection matrix.
	GLint V_uni; // Uniform location for View matrix.
	GLint M_uni; // Uniform location for Model matrix.
	GLint col_uni;   // Uniform location for cube colour.

	// Fields related to grid geometry.
	GLuint m_grid_vao; // Vertex Array Object
	GLuint m_grid_vbo; // Vertex Buffer Object

	// Matrices controlling the camera and projection.
	glm::mat4 proj;
	glm::mat4 view;

	GLint current_x_position;
	GLint current_z_position;

	oneCellData * active_cell_data_ptr;

	float camera_xPos;
	double previous_xPos;
	float ** colour;
	int current_col;
	int current_idx_of_cube;
	int quadrant;

	oneCellData * allCellData[16][16];

	volatile int mouseClicking;
	volatile int leftShiftPressed;	//flag variable of leftShift
	volatile int rightShiftPressed;	//flag variable of rightShift
};
