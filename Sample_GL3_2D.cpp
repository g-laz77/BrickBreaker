#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <map>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
int game=0;
using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

GLuint programID;
struct COLOR {
    float r;
    float g;
    float b;
};
typedef struct COLOR COLOR;
int bucket;
struct Sprite {
    string name;
    COLOR color;
    float x,y;
    VAO* object;
    int status;
    float height,width;
    float x_speed,y_speed;
    float angle; //Current Angle (Actual rotated angle of the object)
    int inAir;
    float radius;
    int fixed;
    float friction; //Value from 0 to 1
    int health;
    int isRotating;
    int direction; //0 for clockwise and 1 for anticlockwise for animation
    float remAngle; //the remaining angle to finish animation
    int isMovingAnim;
    int dx;
    int dy;
    double last_refl_time;
    float weight;
};
typedef struct Sprite Sprite;
int score=0;
map <string, Sprite> bricks;
map <string, Sprite> gameover;
map <string, Sprite> buckets;
map <string, Sprite> lasers;
map <string, Sprite> mirrors;
map <string, Sprite> scores;
map <string, Sprite> penalties;


/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
//    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
float triangle_move_status = 0;
float rectangle_move_status = 0;
float cannon_rotation = 0;
float red_bucket_status=0;
float green_bucket_status=0;
float brick_speed=0;
int fire=0;
double last_fire_time, curr_time;
float bl=-4,br=4,bb=-4,bt=4;

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
     // Function is called first on GLFW_PRESS.
    double diff;
    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_W:
                rectangle_move_status = 0.1f;
                break;
            case GLFW_KEY_S:
                rectangle_move_status = -0.1f;
                break;
            case GLFW_KEY_A:
                cannon_rotation = 10;
                break;
            case GLFW_KEY_D:
                cannon_rotation = -10;
                break;    
            case GLFW_KEY_SPACE:
                curr_time = glfwGetTime();
                diff = curr_time - last_fire_time;
                if ( diff >= 1 ) 
                {
                    last_fire_time=glfwGetTime();
                    fire=1;
                }
                if(game==1)
                    game=0;
                break;
            case GLFW_KEY_LEFT_SHIFT:
                red_bucket_status=-0.05f;
                break;
            case GLFW_KEY_RIGHT_SHIFT:
                red_bucket_status=0.05f;
                break;
            case GLFW_KEY_LEFT_ALT:
                green_bucket_status=-0.05f;
                break;
            case GLFW_KEY_RIGHT_ALT:
                green_bucket_status=0.05f;
                break;
            case GLFW_KEY_N:
                brick_speed=-0.002f;
                break;
            case GLFW_KEY_M:
                brick_speed=0.002f;
                break;

            default:
                break;
        }
    }
    else {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            case GLFW_KEY_LEFT:
                {
                        if(bl>-4)
                                bl-=0.2, br-=0.2;
                        Matrices.projection = glm::ortho(bl, br, bb, bt, 0.1f, 500.0f);
                        break;
                }
                case GLFW_KEY_RIGHT:
                {
                        if(br<4)
                                br+=0.2, bl+=0.2;
                        Matrices.projection = glm::ortho(bl, br, bb, bt, 0.1f, 500.0f);
                        break;
                }
                case GLFW_KEY_UP:
                {
                        if(bt<4)
                                bt+=0.2, bb+=0.2;
                        Matrices.projection = glm::ortho(bl, br, bb, bt, 0.1f, 500.0f);
                        break;
                }
                case GLFW_KEY_DOWN:
                {
                        if(bb>-4)
                                bb-=0.2, bt-=0.2;
                        Matrices.projection = glm::ortho(bl, br, bb, bt, 0.1f, 500.0f);
                        break;
                }
            default:
                break;
        }
    }
}
/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		case 'X':
        case 'x':
        {
                if(bl>=-3.6)
                {
                        if(br<=3.6)
                                bl-=0.4, br+=0.4;
                        else if(bl!=-4)
                                bl-=0.8-4+br, br = 4;
                }
                else if(br!=4)
                        br+=0.8-4-bl, bl = -4;
                if(bb>=-3.6)
                {
                        if(bt<=3.6)
                                bb-=0.4, bt+=0.4;
                        else if(bb!=-4)
                                bb-=0.8-4+bt, bt = 4;
                }
                else if(bt!=4)
                        bt+=0.8-4-bb, bb = -4;
                Matrices.projection = glm::ortho(bl, br, bb, bt, 0.1f, 500.0f);
                break;
        }
        case 'Z':
        case 'z':
        {
                if(br-bl>4)
                        bl+=0.4, br-=0.4;
                if(bt-bb>4)
                        bb+=0.4, bt-=0.4;
                Matrices.projection = glm::ortho(bl, br, bb, bt, 0.1f, 500.0f);
                break;
        }
        default:
			break;
        
	}
}
int angle_flag=0;
int left_press=0;
int mouse_fire=0;
/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    double diff;
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_RELEASE)
             {
                left_press=0;
                mouse_fire=1;
                bucket=0;
                buckets["red"].status=0;
                buckets["scircle1"].status=0;
                buckets["green"].status=0;
                buckets["scircle2"].status=0;
                curr_time = glfwGetTime();
                diff = curr_time - last_fire_time;
                if ( diff >= 1 ) 
                {
                    last_fire_time=glfwGetTime();
                    fire=1;
                }
             }
             else if(action == GLFW_PRESS)
              {
                    left_press=1;
                    bucket=0;
              }
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_PRESS) {
                 angle_flag=1;
                // cout<<"hi"<<endl;
            }
            else if (action == GLFW_RELEASE) {
               // cout<<"0"<<endl;
                 angle_flag=0;
            }
            break;
        default:
            break;
    }
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
}
VAO *triangle,*laserbody,*canbody;
float somef(float a)
{
    int l=rand()%40;
    int m=rand()%3;
    int n=rand()%2;
    if(n==0)
    //printf("%f ",(a+(0.1*l)));
        return (a+(0.1*l));
    else 
        return (a+6+(0.1*m));
}
float cannon_angle=0;
float ang;
int penalty=15;
void createCircle (string name,COLOR color, float x, float y, float a,float b, int NoOfParts, string component, int fill)
{
    int parts = NoOfParts;
    float radius_a = a;
    float radius_b = b;

    GLfloat vertex_buffer_data[parts*9];
    GLfloat color_buffer_data[parts*9];
    int i,j;
    float angle=(2*M_PI/parts);
    float current_angle = 0;
    for(i=0;i<parts;i++){
        for(j=0;j<3;j++){
            color_buffer_data[i*9+j*3]=color.r;
            color_buffer_data[i*9+j*3+1]=color.g;
            color_buffer_data[i*9+j*3+2]=color.b;
        }
        vertex_buffer_data[i*9]=0;
        vertex_buffer_data[i*9+1]=0;
        vertex_buffer_data[i*9+2]=0;
        vertex_buffer_data[i*9+3]=radius_a*cos(current_angle);
        vertex_buffer_data[i*9+4]=radius_b*sin(current_angle);
        vertex_buffer_data[i*9+5]=0;
        vertex_buffer_data[i*9+6]=radius_a*cos(current_angle+angle);
        vertex_buffer_data[i*9+7]=radius_b*sin(current_angle+angle);
        vertex_buffer_data[i*9+8]=0;
        current_angle+=angle;
    }
    VAO* circle;
    if(fill==1)
        circle = create3DObject(GL_TRIANGLES, (parts*9)/3, vertex_buffer_data, color_buffer_data, GL_FILL);
    else
        circle = create3DObject(GL_TRIANGLES, (parts*9)/3, vertex_buffer_data, color_buffer_data, GL_LINE);
    Sprite suhansprite = {};
    suhansprite.color = color;
    suhansprite.name = name;
    suhansprite.object = circle;
    suhansprite.x=x;
    suhansprite.y=y;
    suhansprite.height=2*b; //Height of the sprite is 2*r
    suhansprite.width=2*a; //Width of the sprite is 2*r
    suhansprite.status=1;
    suhansprite.inAir=0;
    suhansprite.x_speed=0;
    suhansprite.y_speed=0;
    suhansprite.radius=a;
    suhansprite.fixed=0;
    suhansprite.friction=0.4;
    suhansprite.health=100;
    //suhansprite.weight=weight;
    if(component=="acylinder")
        buckets[name]=suhansprite;
}
// Creates the rectangle object used in this sample code
void createRectangles (float length, float breadth,COLOR color,float x, float y,string Component,string name)
{
  // GL3 accepts only Triangles. Quads are not supported
  //float 
  float h=length/2;
  float w=breadth/2;
  GLfloat vertex_buffer_data [] = {
    -h,-w,0, // vertex 1
    h,-w,0, // vertex 2
    h, w,0, // vertex 3

    h, w,0, // vertex 3
    -h, w,0, // vertex 4
    -h,-w,0  // vertex 1
  };

   GLfloat color_buffer_data [] = {
    color.r,color.g,color.b, // color 1
    color.r,color.g,color.b, // color 2
    color.r,color.g,color.b, // color 3

    color.r,color.g,color.b, // color 3
    color.r,color.g,color.b, // color 4
    color.r,color.g,color.b,  // color 1
  };
  // create3DObject creates and returns a handle to a VAO that can be used later
  VAO* rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  Sprite suhansprite = {};
  suhansprite.color = color;
  suhansprite.name = name;
  suhansprite.object = rectangle;
  suhansprite.x=x;
  suhansprite.y=y;
  suhansprite.height=length;
  suhansprite.width=breadth;
  
  if(Component=="brick")
  {
      suhansprite.y_speed=0.008;
      bricks[name]=suhansprite;
  }
  else if(Component=="bucket")
      buckets[name]=suhansprite;
  else if(Component=="laser")
  {
      suhansprite.x_speed=0.03;
      suhansprite.y_speed=0.02;
      if(angle_flag==1)
        suhansprite.angle=ang;
      else 
        suhansprite.angle=cannon_angle;
      suhansprite.status=0;
      suhansprite.inAir=0;
      suhansprite.last_refl_time=glfwGetTime();
      lasers[name]=suhansprite;
  }
  else if(Component=="mirror")
  {
      if(name=="mirror1")
        suhansprite.angle=135;
      else
        suhansprite.angle=45;
      //cout<<"angle is: "<<suhansprite.angle<<endl;
      mirrors[name]=suhansprite;
  }
  else if(Component=="score")
  {
      suhansprite.status=0;
      scores[name]=suhansprite;
  }
  else if(Component=="penalty")
  {
      suhansprite.status=0;
      penalties[name]=suhansprite;
  }
  else if(Component=="over")
  {
      suhansprite.status=1;
      gameover[name]=suhansprite;
  }
}
int laser_count=0;
string convert(int j)
{
    char s[10];
    int i=0,l;
    while(j!=0)
    {
        l=j%10;
        s[i++]=(char)('0'+l);
        j/=10;
    }
    s[i]='\0';
    //cout<<s<<"\n";
    return s;
}
float laser_y=0;

void createLaser()
{
    string name;
    COLOR color;
    laser_count++;
    name=convert(laser_count%10);
    color.r=1;
    color.b=1;
    color.g=1;
    createRectangles(0.2,0.06,color,-3.2,laser_y,"laser",name);
}
void createBricks(int uid)
{
    string name;
    int j, i;
    COLOR color;
    for(i=uid;i>uid-2;i-=1)
        {
            random();
            int col=rand()%3+1;
            //cout<<col<<" ";
            //convert j to string
            //j=i;
            name=convert(i);
            if(col==1)
            {
                color.r=1;
                color.g=0;
                color.b=0;
                createRectangles(0.2,0.25,color,somef(-2.5),3.7,"brick",name);
                //cout<<"Red\n";
            }
            else if (col==2)
            {
                color.r=0;
                color.g=1;
                color.b=0;
                createRectangles(0.2,0.25,color,somef(-2.5),3.7,"brick",name);
                //cout<<"Green\n";
            }
            else
            {
                color.r=0;
                color.g=0;
                color.b=1;
                createRectangles(0.2,0.25,color,somef(-2.5),3.7,"brick",name);
                //cout<<"White\n";
            }
        }
}
void createOver()
{

}
void createscore()
{
    int k,digits,r;
    for(map<string,Sprite>::iterator it=scores.begin();it!=scores.end();it++){
        string current = it->first; //The name of the current object
        scores[current].status=0;
    }
    if(score/10==0)
        digits=1;
    else
    {
        digits=2;
        r=score/10;
    }
    k=score%10;
    
    //cout<<k<<endl;
    if(digits==1||digits==2)
    {
        if(k==2||k==3||k==5||k==6||k==7||k==8||k==9||k==0)
            scores["box1"].status=1;
        if(k==1||k==4||k==5||k==6||k==8||k==9||k==0)
            scores["box2"].status=1;
        if(k==2||k==3||k==7||k==8||k==9||k==0||k==4)
            scores["box3"].status=1;
        if(k==2||k==3||k==5||k==6||k==8||k==9||k==4)
            scores["box4"].status=1;
        if(k==2||k==1||k==6||k==8||k==0)
            scores["box5"].status=1;
        if(k==3||k==4||k==5||k==6||k==7||k==8||k==9||k==0)
            scores["box6"].status=1;
        if(k==2||k==3||k==5||k==6||k==8||k==9||k==0)
            scores["box7"].status=1;
        
    }
    if(digits==2)
    {
        if(r==2||r==3||r==5||r==6||r==7||r==8||r==9||r==0)
            scores["box8"].status=1;
        if(r==1||r==4||r==5||r==6||r==8||r==9||r==0)
            scores["box9"].status=1;
        if(r==2||r==3||r==7||r==8||r==9||r==0||r==4)
            scores["box10"].status=1;
        if(r==2||r==3||r==5||r==6||r==8||r==9||r==4)
            scores["box11"].status=1;
        if(r==2||r==1||r==6||r==8||r==0)
            scores["box12"].status=1;
        if(r==3||r==4||r==5||r==6||r==7||r==8||r==9||r==0)
            scores["box13"].status=1;
        if(r==2||r==3||r==5||r==6||r==8||r==9||r==0)
            scores["box14"].status=1;
    }

}
void createpenalty()
{
    int k,digits,r;
    for(map<string,Sprite>::iterator it=penalties.begin();it!=penalties.end();it++){
        string current = it->first; //The name of the current object
        penalties[current].status=0;
    }
    if(penalty/10==0)
        digits=1;
    else
    {
        digits=2;
        k=penalty/10;
    }
    r=penalty%10;
    
    //cout<<k<<endl;
    if(digits==1||digits==2)
    {
        if(r==2||r==3||r==5||r==6||r==7||r==8||r==9||r==0)
            penalties["box8"].status=1;
        if(r==1||r==4||r==5||r==6||r==8||r==9||r==0)
            penalties["box9"].status=1;
        if(r==2||r==3||r==7||r==8||r==9||r==0||r==4)
            penalties["box10"].status=1;
        if(r==2||r==3||r==5||r==6||r==8||r==9||r==4)
            penalties["box11"].status=1;
        if(r==2||r==1||r==6||r==8||r==0)
            penalties["box12"].status=1;
        if(r==3||r==4||r==5||r==6||r==7||r==8||r==9||r==0)
            penalties["box13"].status=1;
        if(r==2||r==3||r==5||r==6||r==8||r==9||r==0)
            penalties["box14"].status=1;
    }
    if(digits==2)
    {
        if(k==2||k==3||k==5||k==6||k==7||k==8||k==9||k==0)
            penalties["box1"].status=1;
        if(k==1||k==4||k==5||k==6||k==8||k==9||k==0)
            penalties["box2"].status=1;
        if(k==2||k==3||k==7||k==8||k==9||k==0||k==4)
            penalties["box3"].status=1;
        if(k==2||k==3||k==5||k==6||k==8||k==9||k==4)
            penalties["box4"].status=1;
        if(k==2||k==1||k==6||k==8||k==0)
            penalties["box5"].status=1;
        if(k==3||k==4||k==5||k==6||k==7||k==8||k==9||k==0)
            penalties["box6"].status=1;
        if(k==2||k==3||k==5||k==6||k==8||k==9||k==0)
            penalties["box7"].status=1;   
    }

}
void laserBody ()
{
    int parts = 500;
    float radius_a = 0.4;
    float radius_b = 0.4;
    COLOR color;
    color.r=0.5;
    color.b=0.05;
    color.g=0.35;
    GLfloat vertex_buffer_data[parts*9];
    GLfloat color_buffer_data[parts*9];
    int i,j;
    float angle=(2*M_PI/parts);
    float current_angle = 0;
    for(i=0;i<parts;i++){
        for(j=0;j<3;j++){
            color_buffer_data[i*9+j*3]=color.r;
            color_buffer_data[i*9+j*3+1]=color.g;
            color_buffer_data[i*9+j*3+2]=color.b;
        }
        vertex_buffer_data[i*9]=0;
        vertex_buffer_data[i*9+1]=0;
        vertex_buffer_data[i*9+2]=0;
        vertex_buffer_data[i*9+3]=radius_a*cos(current_angle);
        vertex_buffer_data[i*9+4]=radius_b*sin(current_angle);
        vertex_buffer_data[i*9+5]=0;
        vertex_buffer_data[i*9+6]=radius_a*cos(current_angle+angle);
        vertex_buffer_data[i*9+7]=radius_b*sin(current_angle+angle);
        vertex_buffer_data[i*9+8]=0;
        current_angle+=angle;
    }
    VAO* circle;
        laserbody = create3DObject(GL_TRIANGLES, (parts*9)/3, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void cannonBody ()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.2,-0.1,0, // vertex 1
    0.2,-0.1,0, // vertex 2
    0.2, 0.1,0, // vertex 3

    0.2, 0.1,0, // vertex 3
    -0.2, 0.1,0, // vertex 4
    -0.2,-0.1,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    0.5f,0.35f,0.05f, // color 1
    0.5f,0.35f,0.05f, // color 2
    0.5f,0.35f,0.05f, // color 3

    0.5f,0.35f,0.05f, // color 3
    0.5f,0.35f,0.05f, // color 4
    0.5f,0.35f,0.05f  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  canbody = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
float camera_rotation_angle = 90;
float triangle_rotation = 0;

int checkCollisionRight(Sprite col_object, Sprite my_object){
    if(col_object.y+col_object.height/2>my_object.y-my_object.height/2 && col_object.y-col_object.height/2<my_object.y+my_object.height/2 && 
       col_object.x-col_object.width/2<my_object.x+my_object.width && col_object.x+col_object.width>my_object.x-my_object.width){
        return 1;
    }
    return 0;
}

int checkCollisionLeft(Sprite col_object, Sprite my_object){
    if(col_object.y+col_object.height/2>my_object.y-my_object.height/2 && col_object.y-col_object.height/2<my_object.y+my_object.height/2 && 
       col_object.x-col_object.width<my_object.x+my_object.width&& col_object.x+col_object.width/2>my_object.x-my_object.width/2){
        return 1;
    }
    return 0;
}

int checkCollisionTop(Sprite col_object, Sprite my_object){
    if(col_object.x+col_object.width/2>my_object.x-my_object.width/2 && col_object.x-col_object.width/2<my_object.x+my_object.width/2 && 
       col_object.y-col_object.height/2<my_object.y+my_object.height/2 && col_object.y+col_object.height/2>my_object.y-my_object.height/2){
        return 1;
    }
    return 0;
}

int checkCollisionBottom(Sprite col_object, Sprite my_object){
    if(col_object.x+col_object.width/2>my_object.x-my_object.width/2 && col_object.x-col_object.width/2<my_object.x+my_object.width/2 && 
       col_object.y+col_object.height/2>my_object.y -my_object.height/2&& col_object.y-col_object.height/2<my_object.y+my_object.height/2){
            //cout<<"hi";

        return 1;
    }
    return 0;
}
/* Render the scene with openGL */
/* Edit this function according to your assignment */
double nnx,nny,sx,sy;
void laser(glm::mat4 VP,GLFWwindow* window)
{
  glm::mat4 MVP;
  Matrices.model = glm::mat4(1.0f);
  //random();
  glfwGetCursorPos(window,&sx,&sy);
  sx=sx/100 - 4;
  sy=(-1*sy)/100 + 4;
  float temp;
  //rectangle_move_status=0;
  if(left_press==1&&(sx>(-4))&&(sx<-3.2)&&(sy>(laser_y-0.4))&&(sy<laser_y+0.4))
    laser_y=sy;
  else
      laser_y+=rectangle_move_status;

  glm::mat4 translateRectangle = glm::translate (glm::vec3(-3.6f, laser_y, 0));        // glTranslatef
  
  //glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle); //* rotateRectangle);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(laserbody);
}
void cannon(glm::mat4 VP,GLFWwindow* window)
{
  glm::mat4 MVP;
  glfwGetCursorPos(window,&nnx,&nny);
  nnx=nnx/100 - 4;
  nny=(-1*nny)/100 + 4;
  
  Matrices.model = glm::mat4(1.0f);
  //random();
  //laser_y+=rectangle_move_status;
  rectangle_move_status=0;
  //cout<<sx<<" "<<sy<<endl;
  float temp=nny/nnx,temp1;
  ang=atan(temp)*180/M_PI;
  cannon_angle+=cannon_rotation;
  if(angle_flag==1)
    temp1=ang;
  else 
    temp1=cannon_angle;
  //angle_flag=0
  //ang=0;
  //cannon_angle*=(M_PI/180);
  //ang+=cannon_angle;
  cannon_rotation=0;
  glm::mat4 rotateRectangle = glm::rotate((float)(temp1*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  glm::mat4 translateRectangle = glm::translate (glm::vec3(-3.25f, laser_y, 0));        // glTranslatef
  Matrices.model *= (translateRectangle * rotateRectangle);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(canbody);
}
void quit()
{
    //cout<<"yeaaaa"<<endl;
   // glClearColor(0.0f, 0.0f, 0.0f, 1.0f );
   // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glFlush();
}
void checkCollisions(string color)
{
    for(map<string,Sprite>::iterator it=bricks.begin();it!=bricks.end();){
        string current = it->first; 
        if(checkCollisionBottom(bricks[current],buckets[color])&&(bricks[current].color.r==buckets[color].color.r)&&buckets[color].name=="red")
        {    
            //score++;
            cout<<"red bucket. Score: "<<score<<endl;
            bricks.erase(it++);

        }
        else if(checkCollisionBottom(bricks[current],buckets[color])&&(bricks[current].color.g==buckets[color].color.g)&&buckets[color].name=="green")
        {
            //score++;
            cout<<"Green bucket. Score: "<<score<<endl;
            bricks.erase(it++);
        }
        else if(checkCollisionBottom(bricks[current],buckets[color])&&(bricks[current].color.b!=buckets[color].color.b))
        {
            bricks.erase(it++);
            game=1;
            score=0;
            penalty=15;
            createpenalty();
            createscore();
            //it++;
        }
        else if(checkCollisionBottom(bricks[current],buckets[color]))
        {   
            if(score>0)
                score--;
            createscore();
            it++;
        }
        else
            it++;
    }
}
void checkCollisionsLaser(string laser)
{
    for(map<string,Sprite>::iterator it=bricks.begin();it!=bricks.end();){
        string current = it->first; 
        if(checkCollisionLeft(bricks[current],lasers[laser])||checkCollisionBottom(bricks[current],lasers[laser])||
           checkCollisionTop(bricks[current],lasers[laser])||checkCollisionRight(bricks[current],lasers[laser]))
        {
            //cout<<"Killed"<<endl;
            if(bricks[current].color.b==1)
            {    
                 score++;
                 createscore();
                 cout<<"Blue killed. Score:"<<score<<endl;
                 bricks.erase(it++);
                 lasers[laser].status=1;
            }
            else
                {
                    penalty--;
                    if(penalty==0)
                        {
                            score=0;
                            penalty=15;
                            game=1;
                            createpenalty();
                            createscore();
                            
                        }
                    
                    //cout<<penalty<<endl;
                    else
                    {
                        createpenalty();
                        bricks.erase(it++);
                        lasers[laser].status=1;
                    }
                }
            //cout<<"hello\n";
            break;
        }
        else 
            it++;
    }
    for(map<string,Sprite>::iterator it=mirrors.begin();it!=mirrors.end();it++){
        string current = it->first; 
        float temp;
        if(checkCollisionLeft(mirrors[current],lasers[laser])||checkCollisionRight(mirrors[current],lasers[laser])||
           checkCollisionTop(mirrors[current],lasers[laser])||checkCollisionBottom(mirrors[current],lasers[laser]))
        {
            double temp = glfwGetTime();
            if((temp-lasers[laser].last_refl_time)>0.4)
            {
                temp=2*(mirrors[current].angle-lasers[laser].angle);
                lasers[laser].angle+=temp;
                lasers[laser].last_refl_time=glfwGetTime();
            }
        }
    }
}
void draw (GLFWwindow* window)
{
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model

  // Load identity to model matrix
  Matrices.model = glm::mat4(1.0f);
  checkCollisions("green");
  checkCollisions("red");
  for(map<string,Sprite>::iterator it=lasers.begin();it!=lasers.end();){
        string current = it->first; //The name of the current object
        checkCollisionsLaser(current);
        if(lasers[current].status==1)
            lasers.erase(it++);
        else 
            it++;
        //cout<<"BYe\n";
  } 
  /* Render your scene */
  laser(VP,window);
  cannon(VP,window);
  
  for(map<string,Sprite>::iterator it=bricks.begin();it!=bricks.end();){
        string current = it->first; 
        if(bricks[current].y<-3.8f)
            bricks.erase(it++);
        else 
            it++;
  }
  
  // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
  // glPopMatrix ();
//Draw the bricks
    for(map<string,Sprite>::iterator it=mirrors.begin();it!=mirrors.end();it++){
        string current = it->first; //The name of the current object
        //if(mirrors[current].status==0)
        //    continue;
        glm::mat4 MVP;	// MVP = Projection * View * Model
        //cout<<"Brick:"<<current<<"\n";
        Matrices.model = glm::mat4(1.0f);

        /* Render your scene */
        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(mirrors[current].x, mirrors[current].y, 0.0f)); // glTranslatef
        //mirrors[current].y-=mirrors[current].y_speed;
        string temp=mirrors[current].name;
        //if(mirrors[current].y<-5.0f)
          //  mirrors.erase(temp);
        double rad = ((mirrors[current].angle)*M_PI)/180.0f;
        //cout<<rad<<" "<<current<<endl;
        glm::mat4 rotateTriangle = glm::rotate((float)(rad), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
        //mirrors[current].angle=(coins[current].angle+1.0*time_delta);
        //if(mirrors[current].angle>=360.0)
        //    coins[current].angle=0.0;
        ObjectTransform=translateObject*rotateTriangle;
        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M

        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

        draw3DObject(mirrors[current].object);
        //glPopMatrix (); 
    }  
    for(map<string,Sprite>::iterator it=bricks.begin();it!=bricks.end();it++){
        string current = it->first; //The name of the current object
        //if(bricks[current].status==0)
        //    continue;
        glm::mat4 MVP;	// MVP = Projection * View * Model
        //cout<<"Brick:"<<current<<"\n";
        Matrices.model = glm::mat4(1.0f);


        /* Render your scene */
        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(bricks[current].x, bricks[current].y, 0.0f)); // glTranslatef
        bricks[current].y_speed+=brick_speed;
        bricks[current].y-=bricks[current].y_speed;
        string temp=bricks[current].name;
        //if(bricks[current].y<-5.0f)
          //  bricks.erase(temp);
       // glm::mat4 rotateTriangle = glm::rotate((float)((bricks[current].angle)*M_PI/180.0f), glm::vec3(0,1,0));  // rotate about vector (1,0,0)
        //bricks[current].angle=(coins[current].angle+1.0*time_delta);
        //if(bricks[current].angle>=360.0)
        //    coins[current].angle=0.0;
        ObjectTransform=translateObject;//*rotateTriangle;
        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M

        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

        draw3DObject(bricks[current].object);
        //glPopMatrix (); 
    }  
            brick_speed=0;
    double nx,ny;
    
    for(map<string,Sprite>::iterator it=buckets.begin();it!=buckets.end();it++){
        string current = it->first; //The name of the current object
        //if(bricks[current].status==0)
        //    continue;
        glm::mat4 MVP;	// MVP = Projection * View * Model
        //cout<<"Brick:"<<current<<"\n";
        Matrices.model = glm::mat4(1.0f);
        
        glm::mat4 ObjectTransform;
        double xx,yy;
        glfwGetCursorPos(window,&xx,&yy);
        xx=xx/100 - 4;
        yy=(-1*yy)/100 + 4;
        if(left_press==1&&(xx>(buckets[current].x-buckets[current].width))&&(xx<(buckets[current].x+buckets[current].width))&&
          (yy>(buckets[current].y-buckets[current].height))&&(yy-1<(buckets[current].y+buckets[current].height)))
        {
                //buckets[current].status=1;

               if((buckets[current].name=="red"||buckets[current].name=="scircle1")&& buckets["green"].status==0&&buckets["scircle1"].status==0)
               {
                    buckets["red"].status=1;
                    buckets["scircle1"].status=1;
               }
               else if( buckets["red"].status==0&&buckets["scircle2"].status==0&&(buckets[current].name=="green"||buckets[current].name=="scircle2"))
               {
                   buckets["green"].status=1;
                  buckets["scircle2"].status=1;
               }
               //cout<<"yooo";
               if(buckets[current].status==1)
                    buckets[current].x=xx;
               //buckets[current].y=yy; 
        }
        /* Render your scene */
        else if(buckets[current].name=="red"||buckets[current].name=="scircle1")
        {
            buckets[current].x+=red_bucket_status;
        }
        else if(buckets[current].name=="green"||buckets[current].name=="scircle2")
        {
            buckets[current].x+=green_bucket_status;
        }
        glm::mat4 translateObject = glm::translate (glm::vec3(buckets[current].x, buckets[current].y, 0.0f)); // glTranslatef
        //glm::mat4 rotateTriangle = glm::rotate((float)((0)*M_PI/180.0f), glm::vec3(0,1,0));  // rotate about vector (1,0,0)
        //bricks[current].angle=(coins[current].angle+1.0*time_delta);
        //if(bricks[current].angle>=360.0)
        //    coins[current].angle=0.0;
        ObjectTransform=translateObject;
        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M

        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

        draw3DObject(buckets[current].object);
        //glPopMatrix (); 
    }  
    red_bucket_status=0;
    green_bucket_status=0;

    for(map<string,Sprite>::iterator it=scores.begin();it!=scores.end();it++){
        string current = it->first; //The name of the current object
        //if(bricks[current].status==0)
        //    continue;
        glm::mat4 MVP;	// MVP = Projection * View * Model
        //cout<<"Brick:"<<current<<"\n";
        Matrices.model = glm::mat4(1.0f);

        /* Render your scene */
        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(scores[current].x, scores[current].y, 0.0f)); // glTranslatef
        //glm::mat4 rotateTriangle = glm::rotate((float)((0)*M_PI/180.0f), glm::vec3(0,1,0));  // rotate about vector (1,0,0)
        //bricks[current].angle=(coins[current].angle+1.0*time_delta);
        //if(bricks[current].angle>=360.0)
        //    coins[current].angle=0.0;
        ObjectTransform=translateObject;
        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M

        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        if(scores[current].status==1)
            draw3DObject(scores[current].object);
        //scores[current].status=0;
        //glPopMatrix (); 
    }
    for(map<string,Sprite>::iterator it=gameover.begin();it!=gameover.end();it++){
        string current = it->first; //The name of the current object
        //if(bricks[current].status==0)
        //    continue;
        glm::mat4 MVP;	// MVP = Projection * View * Model
        //cout<<"Brick:"<<current<<"\n";
        Matrices.model = glm::mat4(1.0f);

        /* Render your scene */
        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(gameover[current].x, gameover[current].y, 0.0f)); // glTranslatef
        //glm::mat4 rotateTriangle = glm::rotate((float)((0)*M_PI/180.0f), glm::vec3(0,1,0));  // rotate about vector (1,0,0)
        //bricks[current].angle=(coins[current].angle+1.0*time_delta);
        //if(bricks[current].angle>=360.0)
        //    coins[current].angle=0.0;
        ObjectTransform=translateObject;
        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M

        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        if(gameover[current].status==1&&game==1)
            draw3DObject(gameover[current].object);
        //scores[current].status=0;
        //glPopMatrix (); 
    }
    for(map<string,Sprite>::iterator it=penalties.begin();it!=penalties.end();it++){
        string current = it->first; //The name of the current object
        //if(bricks[current].status==0)
        //    continue;
        glm::mat4 MVP;	// MVP = Projection * View * Model
        //cout<<"Brick:"<<current<<"\n";
        Matrices.model = glm::mat4(1.0f);

        /* Render your scene */
        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(penalties[current].x, penalties[current].y, 0.0f)); // glTranslatef
        //glm::mat4 rotateTriangle = glm::rotate((float)((0)*M_PI/180.0f), glm::vec3(0,1,0));  // rotate about vector (1,0,0)
        //bricks[current].angle=(coins[current].angle+1.0*time_delta);
        //if(bricks[current].angle>=360.0)
        //    coins[current].angle=0.0;
        ObjectTransform=translateObject;
        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M

        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        if(penalties[current].status==1)
            draw3DObject(penalties[current].object);
        //scores[current].status=0;
        //glPopMatrix (); 
    }  
    float temp;
    for(map<string,Sprite>::iterator it=lasers.begin();it!=lasers.end();){
        string current = it->first; //The name of the current object
        if(lasers[current].x>3.75||lasers[current].y>3.75||lasers[current].y<-3.75)
        { 
               lasers.erase(it++);
               continue;
        }
        glm::mat4 MVP;  // MVP = Projection * View * Model
        glfwGetCursorPos(window,&nx,&ny);

        Matrices.model = glm::mat4(1.0f);
        /* Render your scene */
        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(lasers[current].x, lasers[current].y, 0.0f)); // glTranslatef
        float x_diff,y_diff,rad,xproj,yproj;
        //float ang=-atan(temp);
        rad = (lasers[current].angle)*M_PI/180.0f;
        x_diff = abs(3.2-lasers[current].x);
        y_diff = abs(laser_y-lasers[current].y);
        xproj = cos(rad) * 0.1;
        yproj = sin(rad) * 0.1 ;
        glm::mat4 rotateTriangle = glm::rotate((float)(rad), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
        glm::mat4 translateObject1 = glm::translate (glm::vec3(-x_diff, -y_diff, 0.0f)); // glTranslatef
        glm::mat4 translateObject2 = glm::translate (glm::vec3(x_diff, y_diff, 0.0f)); // glTranslatef
        ObjectTransform=translateObject*rotateTriangle;
        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M
        lasers[current].x+=xproj;
        lasers[current].y+=yproj;
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

        draw3DObject(lasers[current].object);
        it++;
        //glPopMatrix (); 
    }
  // Increment angles
  float increments = 1;

  //camera_rotation_angle++; // Simulating camera rotation
  //triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
  //rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
//        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Falling Bricks", NULL, NULL);

    if (!window) {
        glfwTerminate();
//        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
	// Create the models
	//createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
    laserBody ();
	cannonBody();
    createscore();
    createpenalty();
    string name="red";
    string name2="green";

    string mirror1="mirror1";
    string mirror2="mirror2";
    
    string circle="scircle1";
    string circle2="scircle2";

    string box1="box1";
    string box2="box2";
    string box3="box3";
    string box4="box4";
    string box5="box5";
    string box6="box6";
    string box7="box7";
    
    string end1="end1";
    string end2="end2";
    string end3="end3";
    string end4="end4";
    string end5="end5";
    string end6="end6";
    string end7="end7";
    
    string end8="end8";
    string end9="end9";
    string end10="end10";
    string end11="end11";
    string end12="end12";
    string end13="end13";
    string end14="end14";

    string end15="end15";
    string end16="end16";
    string end17="end17";
    string end18="end18";
    string end19="end19";
    string end20="end20";
    string end21="end21";

    string end22="end22";
    string end23="end23";
    string end24="end24";
    string end25="end25";
    string end26="end26";
    string end27="end27";
    string end28="end28";
    
    string end29="end29";
    string end30="end30";
    string end31="end31";
    string end32="end32";
    string end33="end33";
    string end34="end34";
    string end35="end35";

    string end36="end36";
    string end37="end37";
    string end38="end38";
    string end39="end39";
    string end40="end40";
    string end41="end41";
    string end42="end42";

    string end43="end43";
    string end44="end44";
    string end45="end45";
    string end46="end46";
    string end47="end47";
    string end48="end48";
    string end49="end49";
    
    string end50="end50";
    string end51="end51";
    string end52="end52";
    string end53="end53";
    string end54="end54";
    string end55="end55";
    string end56="end56";

    string end57="end57";
    string end58="end58";
    string end59="end59";
    string end60="end60";
    string end61="end61";
    string end62="end62";
    string end63="end63";
    
    string box8="box8";
    string box9="box9";
    string box10="box10";
    string box11="box11";
    string box12="box12";
    string box13="box13";
    string box14="box14";


    COLOR color,color1,color2,color3,color4;
    color.r=1;
    color.b=0;
    color.g=0;

    color1.r=0;
    color1.b=0.2;
    color1.g=0.8;
    
    color3.r=1;
    color3.b=0.3;
    color3.g=0;
    
    color4.r=0;
    color4.b=0.4;
    color4.g=0.7;
    
    color2.r=1;
    color2.b=1;
    color2.g=1;
    
    createCircle(circle2,color4,1.5,-3,0.5,0.3,100,"acylinder",1);
    createCircle(circle,color3,-1.5,-3,0.5,0.3,100,"acylinder",1);
    createRectangles(1,1,color,-1.5,-3.5,"bucket",name);
    createRectangles(1,1,color1,1.5,-3.5,"bucket",name2);
    

    createRectangles(0.9,0.08,color2,2.5,3.2,"mirror",mirror1);
    createRectangles(0.9,0.08,color2,2.5,-2.2,"mirror",mirror2);
    
    createRectangles(0.1,0.015,color2,-3.75,3.9,"penalty",box1);
    createRectangles(0.015,0.15,color2,-3.69,3.825,"penalty",box3);
    createRectangles(0.015,0.15,color2,-3.79,3.825,"penalty",box2);
    createRectangles(0.1,0.015,color2,-3.75,3.74,"penalty",box4);
    createRectangles(0.015,0.15,color2,-3.69,3.675,"penalty",box6);
    createRectangles(0.015,0.15,color2,-3.79,3.675,"penalty",box5);
    createRectangles(0.1,0.015,color2,-3.75,3.59,"penalty",box7);

    createRectangles(0.1,0.015,color2,-3.60,3.9,"penalty",box8);
    createRectangles(0.015,0.15,color2,-3.54,3.825,"penalty",box10);
    createRectangles(0.015,0.15,color2,-3.64,3.825,"penalty",box9);
    createRectangles(0.1,0.015,color2,-3.60,3.74,"penalty",box11);
    createRectangles(0.015,0.15,color2,-3.54,3.675,"penalty",box13);
    createRectangles(0.015,0.15,color2,-3.64,3.675,"penalty",box12);
    createRectangles(0.1,0.015,color2,-3.60,3.59,"penalty",box14);

    createRectangles(0.1,0.015,color2,3.75,3.9,"score",box1);
    createRectangles(0.015,0.15,color2,3.69,3.825,"score",box2);
    createRectangles(0.015,0.15,color2,3.79,3.825,"score",box3);
    createRectangles(0.1,0.015,color2,3.75,3.74,"score",box4);
    createRectangles(0.015,0.15,color2,3.69,3.675,"score",box5);
    createRectangles(0.015,0.15,color2,3.79,3.675,"score",box6);
    createRectangles(0.1,0.015,color2,3.75,3.59,"score",box7);

    createRectangles(0.1,0.015,color2,3.60,3.9,"score",box8);
    createRectangles(0.015,0.15,color2,3.54,3.825,"score",box9);
    createRectangles(0.015,0.15,color2,3.64,3.825,"score",box10);
    createRectangles(0.1,0.015,color2,3.60,3.74,"score",box11);
    createRectangles(0.015,0.15,color2,3.54,3.675,"score",box12);
    createRectangles(0.015,0.15,color2,3.64,3.675,"score",box13);
    createRectangles(0.1,0.015,color2,3.60,3.59,"score",box14);

    createRectangles(0.5,0.075,color2,-2.4,1.5,"over",end8);
    createRectangles(0.075,0.75,color2,-2.65,1.09,"over",end9);
    //createRectangles(0.075,0.75,color2,-2.15,1.09,"over",end10);
    //createRectangles(0.5,0.075,color2,-2.4,0.72,"over",end11);
    createRectangles(0.075,0.75,color2,-2.65,0.34,"over",end12);
    createRectangles(0.075,0.75,color2,-2.15,0.34,"over",end13);
    createRectangles(0.5,0.075,color2,-2.4,-0.04,"over",end14);

    createRectangles(0.5,0.075,color2,-1.8,1.5,"over",end1);
    createRectangles(0.075,0.75,color2,-2.05,1.09,"over",end2);
    createRectangles(0.075,0.75,color2,-1.55,1.09,"over",end3);
    createRectangles(0.5,0.075,color2,-1.8,0.72,"over",end4);
    createRectangles(0.075,0.75,color2,-2.05,0.34,"over",end5);
    createRectangles(0.075,0.75,color2,-1.55,0.34,"over",end6);
    //createRectangles(0.5,0.075,color2,-1.8,-0.04,"over",end7);

    createRectangles(0.5,0.075,color2,-1.2,1.5,"over",end15);
    createRectangles(0.075,0.75,color2,-1.45,1.09,"over",end16);
    createRectangles(0.075,0.75,color2,-0.95,1.09,"over",end17);
    //createRectangles(0.5,0.075,color2,-1.2,0.72,"over",end18);
    createRectangles(0.075,0.75,color2,-1.45,0.34,"over",end19);
    //createRectangles(0.075,0.75,color2,-0.95,0.34,"over",end20);
    //createRectangles(0.5,0.075,color2,-1.2,-0.04,"over",end21);

    createRectangles(0.5,0.075,color2,-0.6,1.5,"over",end22);
    createRectangles(0.075,0.75,color2,-0.85,1.09,"over",end23);
    createRectangles(0.075,0.75,color2,-0.35,1.09,"over",end24);
    //createRectangles(0.5,0.075,color2,-0.6,0.72,"over",end25);
    //createRectangles(0.075,0.75,color2,-0.85,0.34,"over",end26);
    createRectangles(0.075,0.75,color2,-0.35,0.34,"over",end27);
    //createRectangles(0.5,0.075,color2,-0.6,-0.04,"over",end28);

    createRectangles(0.5,0.075,color2,0,1.5,"over",end36);
    createRectangles(0.075,0.75,color2,-0.25,1.09,"over",end37);
    //createRectangles(0.075,0.75,color2,0.25,1.09,"over",end38);
    createRectangles(0.5,0.075,color2,0,0.72,"over",end39);
    createRectangles(0.075,0.75,color2,-0.25,0.34,"over",end40);
    //createRectangles(0.075,0.75,color2,0.25,0.34,"over",end41);
    createRectangles(0.5,0.075,color2,0,-0.04,"over",end42);

    createRectangles(0.5,0.075,color2,1.2,1.5,"over",end43);
    createRectangles(0.075,0.75,color2,1.45,1.09,"over",end44);
    createRectangles(0.075,0.75,color2,0.95,1.09,"over",end45);
    //createRectangles(0.5,0.075,color2,1.2,0.72,"over",end46);
    createRectangles(0.075,0.75,color2,1.45,0.34,"over",end47);
    createRectangles(0.075,0.75,color2,0.95,0.34,"over",end48);
    createRectangles(0.5,0.075,color2,1.2,-0.04,"over",end49);

    createRectangles(0.5,0.075,color2,2.4,1.5,"over",end29);
    createRectangles(0.075,0.75,color2,2.65,1.09,"over",end30);
    //createRectangles(0.075,0.75,color2,2.15,1.09,"over",end31);
    createRectangles(0.5,0.075,color2,2.4,0.72,"over",end32);
    createRectangles(0.075,0.75,color2,2.65,0.34,"over",end33);
    //createRectangles(0.075,0.75,color2,2.15,0.34,"over",end34);
    createRectangles(0.5,0.075,color2,2.4,-0.04,"over",end35);

    //createRectangles(0.5,0.075,color2,1.8,1.5,"over",end50);
    createRectangles(0.075,0.75,color2,2.05,1.09,"over",end51);
    createRectangles(0.075,0.75,color2,1.55,1.09,"over",end52);
    //createRectangles(0.5,0.075,color2,1.8,0.72,"over",end53);
    createRectangles(0.075,0.75,color2,2.05,0.34,"over",end54);
    createRectangles(0.075,0.75,color2,1.55,0.34,"over",end55);
    createRectangles(0.5,0.075,color2,1.8,-0.04,"over",end56);

    createRectangles(0.5,0.075,color2,3.0,1.5,"over",end57);
    createRectangles(0.075,0.75,color2,2.75,1.09,"over",end58);
    createRectangles(0.075,0.75,color2,3.25,1.09,"over",end59);
    createRectangles(0.5,0.075,color2,3.0,0.72,"over",end60);
    createRectangles(0.075,0.75,color2,2.75,0.34,"over",end61);
    createRectangles(0.075,0.75,color2,3.25,0.34,"over",end62);
    //createRectangles(0.5,0.075,color2,3.0,-0.04,"over",end63);

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

	
	reshapeWindow (window, width, height);

    // Background color of the scene
	glClearColor (0, 0, 0, 0); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
	int width = 800;
    std::flush(std::cout);

	int height = 800;
    int uid=2;
    GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

    double last_update_time = glfwGetTime(), current_time,diff;
    //float i = 4.0f;
    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {

        // OpenGL Draw commands
        draw(window);
        if(mouse_fire==1)
        {
            mouse_fire=0;
            curr_time = glfwGetTime();
                diff = curr_time - last_fire_time;
                if ( diff >= 1 ) 
                {
                    last_fire_time=glfwGetTime();
                    fire=1;
                }
        }
        if (fire==1)
        { 
               createLaser();
               fire=0;
        }
        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 2) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            createBricks(uid);
            uid+=2;
            last_update_time = current_time;
            //draw(8,i);
        }
    }

    glfwTerminate();
//    exit(EXIT_SUCCESS);
}
