#include "A1.hpp"
#include "cs488-framework/GlErrorCheck.hpp"

#include <iostream>

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;
using namespace std;

static const size_t DIM = 16;
static const size_t maxNumOfBars = 20;
static const float MAXDISTANCE = float(DIM)*2.0*M_SQRT1_2;

oneCellData::~oneCellData(){
	for ( int i = 0; i < 8; i++){
		delete [] vertex_data_buffer[i];
	}
	delete [] vertex_data_buffer;
}

oneCellData::oneCellData(int x, int z):x(x), z(z){
	vertex_data_buffer = new GLfloat*[8];
	for ( int i = 0; i < 8; i++ ){
		vertex_data_buffer[i] = new GLfloat[3];
	}
	this->resetData();
}

GLfloat** oneCellData::get_vertex_data(){
	return this->vertex_data_buffer;
}

void oneCellData::change_color( int newColorID ){
	colorID = newColorID;
}

void oneCellData::change_vertex_height_data(){
	GLfloat ** v = vertex_data_buffer;
	v[0][1] = height;
	v[1][1] = height;
	v[2][1] = height;
	v[3][1] = height;
}

void oneCellData::change_height( int n ){
	if ( n > 0 ){
		if ( height == maxNumOfBars ){
			cout << "Reach the maximum number of bars" << endl;
		}else{
			height++;
			change_vertex_height_data();
		}
	}else {
		if ( height == 0 ){
			cout << "Can not shrink anymore" << endl;
		}else{
			height--;
			change_vertex_height_data();
		}
	}
}

GLint oneCellData::get_current_height(){
	return height;
}

GLint oneCellData::get_current_color(){
	return colorID;
}

void oneCellData::copy_old_data( oneCellData * old ){
	GLfloat ** old_data = old->get_vertex_data();
	GLfloat ** v = vertex_data_buffer;
	height = old->get_current_height();
	colorID = old->get_current_color();

	change_vertex_height_data();
}

void oneCellData::resetData(){
	GLfloat ** v = vertex_data_buffer;
	v[0][0] = x; v[0][1] = 0; v[0][2] = z;
	v[1][0] = x; v[1][1] = 0; v[1][2] = z+1;
	v[2][0] = x + 1; v[2][1] = 0; v[2][2] = z+1;
	v[3][0] = x+1; v[3][1] = 0; v[3][2] = z;
	v[4][0] = x; v[4][1] = 0; v[4][2] = z;
	v[5][0] = x; v[5][1] = 0; v[5][2] = z+1;
	v[6][0] = x+1; v[6][1] = 0; v[6][2] = z+1;
	v[7][0] = x+1; v[7][1] = 0; v[7][2] = z;

	height = 0;
	colorID = 0;
}

//----------------------------------------------------------------------------------------
// Constructor
A1::A1()
	: current_col( 0 )
{
	// colour[0] = 0.0f;
	// colour[1] = 0.0f;
	// colour[2] = 0.0f;
	colour = new float*[8];
	for ( int i = 0; i < 8; i++){
		colour[i] = new float[3];
		for ( int j = 0; j < 3; j++){
			colour[i][j] = 0.0f;
		}
	}

	for ( int i=0; i < DIM; i++){
		for ( int j=0; j< DIM; j++){
			allCellData[i][j] = new oneCellData(i, j);
		}
	}
	current_x_position = DIM / 2;
	current_z_position = DIM / 2;
	active_cell_data_ptr = allCellData[current_x_position][current_z_position];

	current_idx_of_cube = 0;

	leftShiftPressed = 0;
	rightShiftPressed = 0;
	mouseClicking = 0;
	previous_xPos = 0.0;
	camera_xPos = 0.0f;
	quadrant = 1;
}

//----------------------------------------------------------------------------------------
// Destructor
A1::~A1()
{
	for ( int i = 0; i < DIM; i++){
		for ( int j = 0; j < DIM; j++){
			delete allCellData[i][j];
			cout << i*DIM + j << endl;
		}
	}
	for ( int i = 0; i < 8; i++){
		delete [] colour[i];
	}
}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void A1::init()
{
	// Set the background colour.
	glClearColor( 0.3, 0.5, 0.7, 1.0 );

	// Build the shader
	m_shader.generateProgramObject();
	m_shader.attachVertexShader(
		getAssetFilePath( "VertexShader.vs" ).c_str() );
	m_shader.attachFragmentShader(
		getAssetFilePath( "FragmentShader.fs" ).c_str() );
	m_shader.link();

	// Set up the uniforms
	P_uni = m_shader.getUniformLocation( "P" );
	V_uni = m_shader.getUniformLocation( "V" );
	M_uni = m_shader.getUniformLocation( "M" );
	col_uni = m_shader.getUniformLocation( "colour" );

	initGrid();

	// Set up initial view and projection matrices (need to do this here,
	// since it depends on the GLFW window being set up correctly).
	view = glm::lookAt( 
		glm::vec3( 0.0f, float(DIM)*2.0*M_SQRT1_2, float(DIM)*2.0*M_SQRT1_2 ),
		glm::vec3( 0.0f, 0.0f, 0.0f ),
		glm::vec3( 0.0f, 1.0f, 0.0f ) );
	proj = glm::perspective( 
		glm::radians( 45.0f ),
		float( m_framebufferWidth ) / float( m_framebufferHeight ),
		1.0f, 1000.0f );
}

void A1::initGrid()
{
	size_t sz = 3 * 2 * 2 * (DIM+3);

	float *verts = new float[ sz ];
	size_t ct = 0;
	for( int idx = 0; idx < DIM+3; ++idx ) {
		verts[ ct ] = -1;
		verts[ ct+1 ] = 0;
		verts[ ct+2 ] = idx-1;
		verts[ ct+3 ] = DIM+1;
		verts[ ct+4 ] = 0;
		verts[ ct+5 ] = idx-1;
		ct += 6;

		verts[ ct ] = idx-1;
		verts[ ct+1 ] = 0;
		verts[ ct+2 ] = -1;
		verts[ ct+3 ] = idx-1;
		verts[ ct+4 ] = 0;
		verts[ ct+5 ] = DIM+1;
		ct += 6;
	}

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays( 1, &m_grid_vao );
	glBindVertexArray( m_grid_vao );

	// Create the cube vertex buffer
	glGenBuffers( 1, &m_grid_vbo );
	glBindBuffer( GL_ARRAY_BUFFER, m_grid_vbo );
	glBufferData( GL_ARRAY_BUFFER, sz*sizeof(float),
		verts, GL_STATIC_DRAW );

	// Specify the means of extracting the position values properly.
	GLint posAttrib = m_shader.getAttribLocation( "position" );
	glEnableVertexAttribArray( posAttrib );
	glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr );

	// Reset state to prevent rogue code from messing with *my* 
	// stuff!
	glBindVertexArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	// OpenGL has the buffer now, there's no need for us to keep a copy.
	delete [] verts;

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A1::appLogic()
{
	// Place per frame, application logic here ...

	CHECK_GL_ERRORS;
}

void A1::load_one_vertex_data( GLfloat * current_vertex, GLfloat * new_cube ){
	for ( int i = 0; i < 3; i++ ){
		new_cube[current_idx_of_cube] = current_vertex[i];
		current_idx_of_cube++;
	}
}

void A1::push_vertex_to_buffer(GLint a, GLint b, GLint c, GLfloat * new_cube, GLfloat ** thisCellData ){
	
	load_one_vertex_data( thisCellData[a], new_cube );
	load_one_vertex_data( thisCellData[b], new_cube );
	load_one_vertex_data( thisCellData[c], new_cube );
}

void A1::push_data_to_buffer( GLfloat * data_set, int size, float * color ){
	GLuint tmp_vao;
	GLuint tmp_vbo;

	glGenVertexArrays( 1, &tmp_vao);
	glBindVertexArray( tmp_vao );

	glGenBuffers( 1, &tmp_vbo );
	glBindBuffer( GL_ARRAY_BUFFER, tmp_vbo );
	glBufferData( GL_ARRAY_BUFFER, 3*size*sizeof(float),
		data_set, GL_STATIC_DRAW);

	GLint posAttrib = m_shader.getAttribLocation( "position" );
	glEnableVertexAttribArray( posAttrib );
	glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr );

	glBindVertexArray( tmp_vao );
	glUniform3f( col_uni, color[0], color[1], color[2] );
	glDrawArrays( GL_TRIANGLES	, 0, size );
}

void A1::draw_cube(int x, int z){
	GLfloat *new_cube = new GLfloat[108]; 

	if ( allCellData[x][z]->get_current_height() ){
		current_idx_of_cube = 0;
		GLfloat ** thisCellData = allCellData[x][z]->get_vertex_data();

		push_vertex_to_buffer(0, 1, 2, new_cube, thisCellData);
		push_vertex_to_buffer(0, 2, 3, new_cube, thisCellData);	
		push_vertex_to_buffer(0, 1, 5, new_cube, thisCellData);
		push_vertex_to_buffer(0, 4, 5, new_cube, thisCellData);
		push_vertex_to_buffer(0, 4, 7, new_cube, thisCellData);
		push_vertex_to_buffer(0, 3, 7, new_cube, thisCellData);
		push_vertex_to_buffer(1, 5, 6, new_cube, thisCellData);
		push_vertex_to_buffer(1, 2, 6, new_cube, thisCellData);
		push_vertex_to_buffer(2, 3, 6, new_cube, thisCellData);
		push_vertex_to_buffer(3, 6, 7, new_cube, thisCellData);
		push_vertex_to_buffer(4, 5, 6, new_cube, thisCellData);
		push_vertex_to_buffer(4, 6, 7, new_cube, thisCellData);

		GLint colorID = allCellData[x][z]->get_current_color();
		
		push_data_to_buffer( new_cube, 36, colour[colorID] );
	}

	delete [] new_cube;
}

void A1::draw_active_indicator(){
	GLfloat * new_indicator = new GLfloat[72];
	current_idx_of_cube = 0;
	
	GLfloat ** v = new GLfloat*[6];
	for ( int i = 0; i < 6; i++ ){
		v[i] = new GLfloat[3];
	}

	float x = current_x_position ;
	float z = current_z_position ;
	int height = active_cell_data_ptr->get_current_height() + 3;

	v[0][0] = x + 0.25; v[0][1] = height; v[0][2] = z + 0.25 ;
	v[1][0] = x + 0.25; v[1][1] = height; v[1][2] = z + 0.75;
	v[2][0] = x + 0.75; v[2][1] = height; v[2][2] = z + 0.75;
	v[3][0] = x + 0.75; v[3][1] = height; v[3][2] = z + 0.25;
	v[4][0] = x + 0.5; v[4][1] = height - 1; v[4][2] = z + 0.5;
	v[5][0] = x + 0.5; v[5][1] = height + 1; v[5][2] = z + 0.5;

	push_vertex_to_buffer(0, 1, 4, new_indicator, v );
	push_vertex_to_buffer(1, 2, 4, new_indicator, v );
	push_vertex_to_buffer(2, 3, 4, new_indicator, v );
	push_vertex_to_buffer(0, 3, 4, new_indicator, v );
	push_vertex_to_buffer(0, 1, 5, new_indicator, v );
	push_vertex_to_buffer(0, 3, 5, new_indicator, v );
	push_vertex_to_buffer(1, 2, 5, new_indicator, v );
	push_vertex_to_buffer(2, 3, 5, new_indicator, v );

	float indicator_color[3] = { 0, 0, 0 };
	push_data_to_buffer( new_indicator, 24, indicator_color );

	delete [] new_indicator;
	for ( int i = 0; i < 6; i++){
		delete [] v[i];
	}
	delete [] v;
}

void A1::draw_additional_indicator(){
	GLfloat * new_buffer = new GLfloat[72];
	current_idx_of_cube = 0;

	GLfloat ** v = new GLfloat*[16];
	for ( int i = 0; i < 16; i++ ){
		v[i] = new GLfloat[3];
	}

	float x = current_x_position;
	float z = current_z_position;

	v[0][0] = -1; v[0][1] = 0; v[0][2] = z;
	v[1][0] = -1; v[1][1] = 0; v[1][2] = z+1;
	v[2][0] = 0; v[2][1] = 0; v[2][2] = z+1;
	v[3][0] = 0; v[3][1] = 0; v[3][2] = z;
	v[4][0] = x; v[4][1] = 0; v[4][2] = DIM;
	v[5][0] = x; v[5][1] = 0; v[5][2] = DIM + 1;
	v[6][0] = x+1; v[6][1] = 0; v[6][2] = DIM + 1;
	v[7][0] = x+1; v[7][1] = 0; v[7][2] = DIM;
	v[8][0] = DIM; v[8][1] = 0; v[8][2] = z;
	v[9][0] = DIM; v[9][1] = 0; v[9][2] = z + 1;
	v[10][0] = DIM + 1; v[10][1] = 0; v[10][2] = z + 1;
	v[11][0] = DIM + 1; v[11][1] = 0; v[11][2] = z;
	v[12][0] = x; v[12][1] = 0; v[12][2] = -1;	
	v[13][0] = x; v[13][1] = 0; v[13][2] = 0;
	v[14][0] = x + 1; v[14][1] = 0; v[14][2] = 0;
	v[15][0] = x + 1; v[15][1] = 0; v[15][2] = -1;

	push_vertex_to_buffer(0, 1, 2, new_buffer, v );
	push_vertex_to_buffer(0, 2, 3, new_buffer, v );
	push_vertex_to_buffer(4, 5, 6, new_buffer, v );
	push_vertex_to_buffer(4, 6, 7, new_buffer, v );
	push_vertex_to_buffer(8, 9, 10, new_buffer, v );
	push_vertex_to_buffer(8, 10, 11, new_buffer, v );
	push_vertex_to_buffer(12, 13, 14, new_buffer, v );
	push_vertex_to_buffer(12, 14, 15, new_buffer, v );

	float indicator_color[3] = { 1, 1, 1 };
	push_data_to_buffer( new_buffer, 24, indicator_color );

	delete [] new_buffer;
	for ( int i = 0; i < 16; i++){
		delete [] v[i];
	}
	delete [] v;
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A1::guiLogic()
{
	// We already know there's only going to be one window, so for 
	// simplicity we'll store button states in static local variables.
	// If there was ever a possibility of having multiple instances of
	// A1 running simultaneously, this would break; you'd want to make
	// this into instance fields of A1.
	static bool showTestWindow(false);
	static bool showDebugWindow(true);

	ImGuiWindowFlags windowFlags(ImGuiWindowFlags_AlwaysAutoResize);
	float opacity(0.5f);

	ImGui::Begin("Debug Window", &showDebugWindow, ImVec2(100,100), opacity, windowFlags);
		if( ImGui::Button( "Quit Application" ) ) {
			glfwSetWindowShouldClose(m_window, GL_TRUE);
		}
		if( ImGui::Button( "Reset Grid" ) ){
			resetGrid();
		}
		// Eventually you'll create multiple colour widgets with
		// radio buttons.  If you use PushID/PopID to give them all
		// unique IDs, then ImGui will be able to keep them separate.
		// This is unnecessary with a single colour selector and
		// radio button, but I'm leaving it in as an example.

		// Prefixing a widget name with "##" keeps it from being
		// displayed.

		for ( int i =0; i < 8; i++){
			ImGui::PushID( i );
			ImGui::ColorEdit3( "##Colour", colour[i] );
			ImGui::SameLine();
			if( ImGui::RadioButton( "##Col", &current_col, i ) ) {
				// Select this colour.
				active_cell_data_ptr->change_color(current_col);
			}
		}

		for ( int i = 0; i < 8; i++){
			ImGui::PopID();
		}
		// // For convenience, you can uncomment this to show ImGui's massive
		// // demonstration window right in your application.  Very handy for
		// // browsing around to get the widget you want.  Then look in 
		// // shared/imgui/imgui_demo.cpp to see how it's done.
		// if( ImGui::Button( "Test Window" ) ) {
		// 	showTestWindow = !showTestWindow;
		// }

		ImGui::Text( "Framerate: %.1f FPS", ImGui::GetIO().Framerate );

	ImGui::End();

	if( showTestWindow ) {
		ImGui::ShowTestWindow( &showTestWindow );
	}
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A1::draw()
{
	// Create a global transformation for the model (centre it).
	mat4 W;
	W = glm::translate( W, vec3( -float(DIM)/2.0f, 0, -float(DIM)/2.0f ) );

	m_shader.enable();
		glEnable( GL_DEPTH_TEST );

		glUniformMatrix4fv( P_uni, 1, GL_FALSE, value_ptr( proj ) );
		glUniformMatrix4fv( V_uni, 1, GL_FALSE, value_ptr( view ) );
		glUniformMatrix4fv( M_uni, 1, GL_FALSE, value_ptr( W ) );

		// Just draw the grid for now.
		glBindVertexArray( m_grid_vao );
		glUniform3f( col_uni, 1, 1, 1 );
		glDrawArrays( GL_LINES, 0, (3+DIM)*4 );

		// Draw each cell in the grid
		for ( int i = 0; i < DIM; i++){
			for ( int j = 0; j < DIM; j++ ){
				draw_cube( i , j );
			}
		}

		draw_additional_indicator();
		glDisable( GL_DEPTH_TEST );
		draw_active_indicator();

	m_shader.disable();

	// Restore defaults
	glBindVertexArray( 0 );

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
/*
 * Called when the reset button is pressed to reset the grid
 */
void A1::resetGrid(){

	for ( int i = 0; i < 8; i++){
		for ( int j = 0; j < 3; j++){
			colour[i][j] = 0.0f;
		}
	}
	for ( int i=0; i < DIM; i++){
		for ( int j=0; j< DIM; j++){
			allCellData[i][j]->resetData();
		}
	}
	current_x_position = 0;
	current_z_position = 0;
	active_cell_data_ptr = allCellData[current_x_position][current_z_position];

	current_idx_of_cube = 0;

	current_col = 0;
	leftShiftPressed = 0;
	rightShiftPressed = 0;
	quadrant = 1;
	camera_xPos = 0.0f;

	view = glm::lookAt( 
		glm::vec3( 0.0f, float(DIM)*2.0*M_SQRT1_2, float(DIM)*2.0*M_SQRT1_2 ),
		glm::vec3( 0.0f, 0.0f, 0.0f ),
		glm::vec3( 0.0f, 1.0f, 0.0f ) );
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void A1::cleanup()
{
	for ( int i = 0; i < DIM; i++){
		for ( int j = 0; j < DIM; j++){
			delete allCellData[i][j];
		}
	}
	for ( int i = 0; i < 8; i++){
		delete [] colour[i];
	}
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A1::cursorEnterWindowEvent (
		int entered
) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse cursor movement events.
 */
bool A1::mouseMoveEvent(double xPos, double yPos) 
{
	bool eventHandled(false);

	if (!ImGui::IsMouseHoveringAnyWindow()) {
		// Put some code here to handle rotations.  Probably need to
		// check whether we're *dragging*, not just moving the mouse.
		// Probably need some instance variables to track the current
		// rotation amount, and maybe the previous X position (so 
		// that you can rotate relative to the *change* in X.
		if ( mouseClicking ){
			float scaling = 0.06f;
			float x = xPos - previous_xPos;
			if ( quadrant ){
				camera_xPos += scaling * x;
				if ( camera_xPos > MAXDISTANCE ){
					camera_xPos = MAXDISTANCE - (camera_xPos - MAXDISTANCE);
					quadrant = 0;
				}
				if ( camera_xPos < -MAXDISTANCE ){
					camera_xPos = -MAXDISTANCE - ( camera_xPos + MAXDISTANCE);
					quadrant = 0;
				}
			}else {
				camera_xPos -= scaling * x;
				if ( camera_xPos > MAXDISTANCE ){
					camera_xPos = MAXDISTANCE - (camera_xPos - MAXDISTANCE);
					quadrant = 1;
				}
				if ( camera_xPos < -MAXDISTANCE ){
					camera_xPos = -MAXDISTANCE - ( camera_xPos + MAXDISTANCE);
					quadrant = 1;
				}
			}
			float z = sqrt( DIM*DIM*2 - camera_xPos * camera_xPos );
			z = quadrant ? z : -z;
			view = glm::lookAt( 
				glm::vec3( camera_xPos, float(DIM)*2.0*M_SQRT1_2, z ),
				glm::vec3( 0.0f, 0.0f, 0.0f ),
				glm::vec3( 0.0f, 1.0f, 0.0f ) );
		}
		previous_xPos = xPos;
		eventHandled = true;
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool A1::mouseButtonInputEvent(int button, int actions, int mods) {
	bool eventHandled(false);

	if (!ImGui::IsMouseHoveringAnyWindow()) {
		// The user clicked in the window.  If it's the left
		// mouse button, initiate a rotation.
		if ( button == GLFW_MOUSE_BUTTON_LEFT ){
			if ( actions == GLFW_PRESS ){
				mouseClicking = 1;
				eventHandled = true;
			}else if ( actions == GLFW_RELEASE ){
				mouseClicking = 0;
				eventHandled = true;
			}
		}
	}
	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A1::mouseScrollEvent(double xOffSet, double yOffSet) {
	bool eventHandled(false);

	// Zoom in or out.

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles window resize events.
 */
bool A1::windowResizeEvent(int width, int height) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool A1::keyInputEvent(int key, int action, int mods) {
	bool eventHandled(false);

	// Fill in with event handling code...
	if( action == GLFW_PRESS ) {
		// Respond to some key events.
		eventHandled = true;
		int shiftPressed = leftShiftPressed || rightShiftPressed;	//indicate that if shift keys are holded
		int updated_position = 0;
		int copy_data_from_old = 0;
		switch ( key ){
			case GLFW_KEY_Q:
				//quit the application
				glfwSetWindowShouldClose(m_window, GL_TRUE);
				break;
			case GLFW_KEY_R:
				//reset the grid
				resetGrid();
				break;
			case GLFW_KEY_SPACE:
				//This event will grow the bar in the active cell
				if ( !active_cell_data_ptr->get_current_height() ){
					active_cell_data_ptr->change_color(current_col);
				}
				active_cell_data_ptr->change_height( 1 );
				break;
			case GLFW_KEY_BACKSPACE:
				//shrink the bar by one unit
				active_cell_data_ptr->change_height( -1 );
				break;
			case GLFW_KEY_LEFT:
				//move the active cell left
				current_x_position--;
				if ( current_x_position < 0 ){
					current_x_position = DIM - 1 ;
				}
				if (shiftPressed){
					copy_data_from_old = 1;
				}
				updated_position = 1;
				break;
			case GLFW_KEY_RIGHT:
				//move the active cell right
				current_x_position++;
				if ( current_x_position >= DIM ){
					current_x_position = 0;
				}
				if (shiftPressed){
					copy_data_from_old = 1;
				}
				updated_position = 1;
				break;
			case GLFW_KEY_UP:
				//move the active cell up
				current_z_position--;
				if ( current_z_position <= 0 ){
					current_z_position = DIM - 1;
				}
				if (shiftPressed){
					copy_data_from_old = 1;
				}
				updated_position = 1;
				break;
			case GLFW_KEY_DOWN:
				//move the active cell down
				current_z_position++;
				if ( current_z_position >= DIM ){
					current_z_position = 0;
				}
				if (shiftPressed){
					copy_data_from_old = 1;
				}
				updated_position = 1;
				break;
			case GLFW_KEY_LEFT_SHIFT:
				leftShiftPressed = 1;
				break;
			case GLFW_KEY_RIGHT_SHIFT:
				rightShiftPressed = 1;
				break;
			default:
				eventHandled = false;
				break;
		}//switch over

		if ( updated_position ){
			oneCellData * old_cell_data = active_cell_data_ptr;
			active_cell_data_ptr = allCellData[current_x_position][current_z_position];
			if ( copy_data_from_old ){
				allCellData[current_x_position][current_z_position]->copy_old_data( old_cell_data );
			}
		}

	}
	if ( action == GLFW_RELEASE ){
		//Response to the shift key release event
		if ( key == GLFW_KEY_LEFT_SHIFT ){
			//set the left shift key flag
			cout << "Left Shift Released" << endl;
			leftShiftPressed = 0;
			eventHandled = true;
		}
		if ( key == GLFW_KEY_RIGHT_SHIFT ){
			//set the right shift key flag
			cout << "Right Shift Released" << endl;
			rightShiftPressed = 0;
			eventHandled = true;
		}
	}
	return eventHandled;
}
