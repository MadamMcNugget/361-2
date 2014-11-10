/*
CMPT 361 Assignment 1 - FruitTetris implementation Sample Skeleton Code

- This is ONLY a skeleton code showing:
How to use multiple buffers to store different objects
An efficient scheme to represent the grids and blocks

- Compile and Run:
Type make in terminal, then type ./FruitTetris

This code is extracted from Connor MacLeod's (crmacleo@sfu.ca) assignment submission
by Rui Ma (ruim@sfu.ca) on 2014-03-04.

Modified in Sep 2014 by Honghua Li (honghual@sfu.ca).
*/

#include "include/Angel.h"
#include <cstdlib>
#include <iostream>
#include <stdlib.h>

using namespace std;


// xsize and ysize represent the window size - updated if window is reshaped to prevent stretching of the game
int xsize = 400;
int ysize = 720;

// current tile
vec2 tile[4]; // An array of 4 2d vectors representing displacement from a 'center' piece of the tile, on the grid
vec2 tilepos = vec2(5, 19); // The position of the current tile using grid coordinates ((0,0) is the bottom left corner)

int rotationType; // one of the 4 rotations
int shapeType; // which shape -> 0 = S, 1 = I, 2 = L
int shuffleType;

vec4 tileColours[4];
vec4 newcolours[144];

// Arrays storing all possible orientations of all possible tiles
// The 'tile' array will always be some element [i][j] of this array (an array of vec2)
vec2 allRotationsLshape[4][4] =
    {{vec2(-1, -1), vec2(-1,0), vec2(0, 0), vec2(1, 0)},
     {vec2(1, -1), vec2(0, -1), vec2(0, 0), vec2(0, 1)},
     {vec2(1, 1), vec2(1,0), vec2(0, 0), vec2(-1, 0)},
     {vec2(-1,1), vec2(0, 1), vec2(0, 0), vec2(0, -1)}};

vec2 allRotationsIshape[4][4] =
    {{vec2(-2, 0), vec2(-1, 0), vec2(0, 0), vec2(1,0)},
     {vec2(0, -2), vec2(0, -1), vec2(0, 0), vec2(0, 1)},
     {vec2(2, 0), vec2(1, 0), vec2(0, 0), vec2(-1, 0)},
     {vec2(0, 2), vec2(0, 1), vec2(0, 0), vec2(0, -1)}};

vec2 allRotationsSshape[4][4] =
    {{vec2(-1, -1), vec2(0, -1), vec2(0, 0), vec2(1,0)},
     {vec2(1, -1), vec2(1, 0), vec2(0, 0), vec2(0, 1)},
     {vec2(1, 1), vec2(0, 1), vec2(0, 0), vec2(-1, 0)},
     {vec2(-1, 1), vec2(-1, 0), vec2(0, 0), vec2(0, -1)}};


// colours
vec4 red = vec4(1.0, 0.0, 0.0, 1.0);
vec4 orange = vec4(1.0, 0.5, 0.0, 1.0);
vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);
vec4 green = vec4(0.0, 1.0, 0.0, 1.0);
vec4 purple = vec4(0.5137, 0.4353, 1.0, 1.0);
vec4 white = vec4(1.0, 1.0, 1.0, 1.0);
vec4 black = vec4(0.0, 0.0, 0.0, 1.0);
vec4 blue = vec4(0.0, 0.56, 1.0, 1.0);
vec4 gray = vec4(0.6, 0.6, 0.6, 1.0);

//board[x][y] represents whether the cell (x,y) is occupied
bool board[10][20];

//An array containing the colour of each of the 10*20*2*3 vertices that make up the board
//Initially, all will be set to black. As tiles are placed, sets of 6 vertices (2 triangles; 1 square)
//will be set to the appropriate colour in this array before updating the corresponding VBO
vec4 boardcolours[7200];  //*********7200

// location of vertex attributes in the shader program
GLuint vPosition;
GLuint vColor;

// locations of uniform variables in shader program
GLuint locxsize;
GLuint locysize;

// VAO and VBO
GLuint vaoIDs[6]; // One VAO for each object: the grid, the board, the current piece, roboarm
GLuint vboIDs[12]; // Two Vertex Buffer Objects for each VAO (specifying vertex positions and colours, respectively)

// timer
int timer = 100; // in milliseconds

// level related things
int level = 1;
int deleted = 0;

// game options
bool fallTiles = true;
bool checkRows = false;
bool findThrees = true;

// game over
bool gameOver = false;
bool falling = true;
bool place = true;
bool shuff = false;

typedef Angel::vec4  color4;
typedef Angel::vec4  point4;

const int NumVertices = 36; //(6 faces)(2 triangles/face)(3 vertices/triangle)
point4 points[NumVertices];
color4 colors[NumVertices];

// Viewing transformation parameters
GLfloat radius = 1.0;
GLfloat theta = 20.0;
GLfloat phi = 1.0;

const GLfloat  dr = 5.0 * DegreesToRadians;

GLuint  model_view;  // model-view matrix uniform shader variable location
mat4  mv;

// Projection transformation parameters

GLfloat  cleft = -xsize/2, cright = xsize/2;
GLfloat  bottom = -ysize/2, top = ysize/2;
GLfloat  zNear = 10.0, zFar = 2500.0;

GLuint  projection; // projection matrix uniform shader variable location

// Robot Arm params
GLfloat larmTheta = 10.0;
GLfloat uarmTheta = 270.0;

GLfloat baseheight = 33.0;
GLfloat basewidth = 100.0;
GLfloat larmheight = 400.0;
GLfloat larmwidth = 20.0;
GLfloat uarmheight = 375.0;
GLfloat uarmwidth = 10.0;

color4 robocolor[36];

vec4 hand = vec4(0,0,0,0);  // For getting coordinates of end of robot arm


//----------------------------------------------------------------------------


bool collision()
{
    bool outOfBounds = false;

    // check bounds
    for (int i=0 ; i<4 ; i++) {
        int xpos = tilepos.x + tile[i].x;
        int ypos = tilepos.y + tile[i].y;
        if (tilepos.x + tile[i].x < 0 || tilepos.x + tile[i].x > 9)  // if walls exist
            outOfBounds = true;
        if (tilepos.y + tile[i].y < 0 || tilepos.y + tile[i].y > 19)  // if walls exist
            outOfBounds = true;
        if (board[xpos][ypos]) // if square already occupied
            outOfBounds = true;
    }

    return outOfBounds;
}

// When the current tile is moved or rotated (or created), update the VBO containing its vertex position data
void updatetile()
{
    if (!falling) {
        int rx = hand.x/33 - 1;
        int ry = hand.y/33 - 1;
        tilepos = vec2(rx, ry);  }

    // Bind the VBO containing current tile vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, vboIDs[4]);
    // For each of the 4 'cells' of the tile,
    for (int i = 0; i < 4; i++)
    {
        // Calculate the grid coordinates of the cell
        GLfloat x = tilepos.x + tile[i].x;
        GLfloat y = tilepos.y + tile[i].y;
        GLfloat z = 33.0;

        // Create the 8 corners of the cube - these vertices are using location in pixels
        // These vertices are later converted by the vertex shader
        vec4 p1 = vec4(33.0 + (x * 33.0), 33.0 + (y * 33.0), 0, 1);
        vec4 p2 = vec4(33.0 + (x * 33.0), 66.0 + (y * 33.0), 0, 1);
        vec4 p3 = vec4(66.0 + (x * 33.0), 33.0 + (y * 33.0), 0, 1);
        vec4 p4 = vec4(66.0 + (x * 33.0), 66.0 + (y * 33.0), 0, 1);

        vec4 p5 = vec4(33.0 + (x * 33.0), 33.0 + (y * 33.0), z, 1);
        vec4 p6 = vec4(33.0 + (x * 33.0), 66.0 + (y * 33.0), z, 1);
        vec4 p7 = vec4(66.0 + (x * 33.0), 33.0 + (y * 33.0), z, 1);
        vec4 p8 = vec4(66.0 + (x * 33.0), 66.0 + (y * 33.0), z, 1);

        // Two points are used by two triangles each
        vec4 newpoints[36] = {p5, p1, p7, p1, p7, p3,  // bottom
                              p5, p6, p7, p6, p7, p8,  // back
                              p5, p6, p1, p6, p1, p2,  // left -> missing
                              p1, p2, p3, p2, p3, p4,  // front
                              p7, p8, p3, p8, p3, p4,  // right
                              p6, p2, p8, p2, p8, p4   // top
                          };

        // Put new data in the VBO
        glBufferSubData(GL_ARRAY_BUFFER, i*36*sizeof(vec4), 36*sizeof(vec4), newpoints);
    }

    if (collision())
    {
        color4 grayarray[144];
        for (int i=0 ; i<144 ; i++)
            grayarray[i] = gray; 
        glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]); // Bind the VBO containing current tile vertex colours
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(grayarray), grayarray); // Put the colour data in the VBO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        place = false;
    }

    else
    {   
        glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]); // Bind the VBO containing current tile vertex colours
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(newcolours), newcolours); // Put the colour data in the VBO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        place = true;
    }

    glBindVertexArray(0);
}

//-------------------------------------------------------------------------------------------------------------------


// Called at the start of play and every time a tile is placed
void newtile()
{
    falling = false;

    shapeType = rand() % 3; // Choose one of the 3 shapes we have
    rotationType = rand() % 4; // there are 4 positions for each shape
    shuffleType = 0;

    // Update the geometry VBO of current tile
    for (int i = 0; i < 4; i++) {
        if (shapeType == 0)
            tile[i] = allRotationsSshape[rotationType][i];
        else if (shapeType == 1)
         tile[i] = allRotationsIshape[rotationType][i];
        else
            tile[i] = allRotationsLshape[rotationType][i]; }

    // start at robot hand
    int rx = hand.x/33 - 1;
    int ry = hand.y/33 - 1;
    tilepos = vec2(rx, ry);

    // Update the color VBO of current tile

    for (int j = 0; j < 144; j++)
    {
        int randColour;

        if (remainder(j, 36) == 0) // Assign a new colour every 36 vertices
            randColour = rand() % 5;

        switch(randColour)
        {
            case 0:
                newcolours[j] = red;
                break;
            case 1:
                newcolours[j] = orange;
                break;
            case 2:
                newcolours[j] = yellow;
                break;
            case 3:
                newcolours[j] = green;
                break;
            case 4:
                newcolours[j] = purple;
                break;
        }
    }
    //storing the colours
    tileColours[0] = newcolours[1];
    tileColours[1] = newcolours[37];
    tileColours[2] = newcolours[73];
    tileColours[3] = newcolours[109];

    // if new piece cannot fit on board
    /*for (int i=0 ; i<4 ; i++) {
        int xpos = tilepos.x + tile[i].x;
        int ypos = tilepos.y + tile[i].y;
        if (board[xpos][ypos]) { // if square already occupied
            gameOver = true;
            cout<<"Game Over\n";
            exit(EXIT_SUCCESS);}
    }*/
  
    updatetile();

}

//-------------------------------------------------------------------------------------------------------------------
void robobase()
{
    // robot base coords
    point4 p1 = point4( -basewidth/2+basewidth,  0.0,         -basewidth/2+basewidth, 1.0 );
    point4 p2 = point4( -basewidth/2+basewidth,  baseheight,  -basewidth/2+basewidth, 1.0 );
    point4 p3 = point4( -basewidth/2,            0.0,         -basewidth/2+basewidth, 1.0 );
    point4 p4 = point4( -basewidth/2,            baseheight,  -basewidth/2+basewidth, 1.0 );
    point4 p5 = point4( -basewidth/2+basewidth,  0.0,         -basewidth/2,           1.0 );
    point4 p6 = point4( -basewidth/2+basewidth,  baseheight,  -basewidth/2,           1.0 );
    point4 p7 = point4( -basewidth/2,            0.0,         -basewidth/2,           1.0 );
    point4 p8 = point4( -basewidth/2,            baseheight,  -basewidth/2,           1.0 );
    
    point4 robobasepoints[36] = {p1, p2, p3, p2, p3, p4,  //front
                          p6, p2, p8, p2, p8, p4,   //top
                          p7, p8, p3, p8, p3, p4,  //right
                          p5, p6, p7, p6, p7, p8,  //back
                          p5, p6, p1, p6, p1, p2,  //left
                          p5, p1, p7, p1, p7, p3   //bottom
                            };


    for (int i = 0 ; i<36 ; i++)
        robocolor[i] = blue;

        // Set up first VAO (representing grid lines)
    glBindVertexArray(vaoIDs[3]); // Bind the first VAO
    glGenBuffers(2, &vboIDs[6]); // Create two Vertex Buffer Objects for this VAO (positions, colours)

    mv *= (Translate(-80.0-basewidth/2+33.0, 33.0, 33.0/2) ) ;
    glUniformMatrix4fv( model_view , 1, GL_TRUE,  mv );

    // Grid vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, vboIDs[6]); // Bind the first robo VBO (vertex positions)
    glBufferData(GL_ARRAY_BUFFER, 36*sizeof(vec4), robobasepoints, GL_DYNAMIC_DRAW); // Put the robo points in the VBO
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vPosition); // Enable the attribute

    // Grid vertex colours
    glBindBuffer(GL_ARRAY_BUFFER, vboIDs[7]); // Bind the second robo VBO (vertex colours)
    glBufferData(GL_ARRAY_BUFFER, 36*sizeof(vec4), robocolor, GL_DYNAMIC_DRAW); // Put the robo colours in the VBO
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vColor); // Enable the attribute*   

    //glBindVertexArray(vaoIDs[3]); // VAO for robo arm
    glDrawArrays(GL_TRIANGLES, 0, 36); // Draw the base (12 triangles)
}

void roboLowArm()
{
    // robot low arm coords
    point4 p1 = point4( -larmwidth/2,            0.0,        -larmwidth/2+larmwidth, 1.0 );
    point4 p2 = point4( -larmwidth/2,            larmheight, -larmwidth/2+larmwidth, 1.0 );
    point4 p3 = point4( -larmwidth/2+larmwidth,  0.0,        -larmwidth/2+larmwidth, 1.0 );
    point4 p4 = point4( -larmwidth/2+larmwidth,  larmheight, -larmwidth/2+larmwidth, 1.0 );
    point4 p5 = point4( -larmwidth/2,            0.0,        -larmwidth/2,           1.0 );
    point4 p6 = point4( -larmwidth/2,            larmheight, -larmwidth/2,           1.0 );
    point4 p7 = point4( -larmwidth/2+larmwidth,  0.0,        -larmwidth/2,           1.0 );
    point4 p8 = point4( -larmwidth/2+larmwidth,  larmheight, -larmwidth/2,           1.0 );

    point4 robolarmpoints[36] = {p1, p2, p3, p2, p3, p4,  //front
                          p6, p2, p8, p2, p8, p4,  //top
                          p7, p8, p3, p8, p3, p4,  //right
                          p5, p6, p7, p6, p7, p8,  //back
                          p5, p6, p1, p6, p1, p2,  //left
                          p5, p1, p7, p1, p7, p3   //bottom
                            };

        // Set up first VAO (representing grid lines)
    glBindVertexArray(vaoIDs[4]); // Bind the first VAO
    glGenBuffers(2, &vboIDs[8]); // Create two Vertex Buffer Objects for this VAO (positions, colours)

    mv *= (Translate( 0.0, baseheight, 0.0) *
            RotateZ(larmTheta) ) ;
    glUniformMatrix4fv( model_view , 1, GL_TRUE,  mv );

    // Grid vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, vboIDs[8]); // Bind the first robo VBO (vertex positions)
    glBufferData(GL_ARRAY_BUFFER, 36*sizeof(vec4), robolarmpoints, GL_DYNAMIC_DRAW); // Put the robo points in the VBO
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vPosition); // Enable the attribute

    // Grid vertex colours
    glBindBuffer(GL_ARRAY_BUFFER, vboIDs[9]); // Bind the second robo VBO (vertex colours)
    glBufferData(GL_ARRAY_BUFFER, 36*sizeof(vec4), robocolor, GL_DYNAMIC_DRAW); // Put the robo colours in the VBO
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vColor); // Enable the attribute*

    //glBindVertexArray(vaoIDs[4]);
    glDrawArrays( GL_TRIANGLES, 0, 36 );
}

void roboUpArm()
{
    // robot low arm coords
    point4 p1 = point4( -uarmwidth/2,            0.0,        -uarmwidth/2+uarmwidth, 1.0 );
    point4 p2 = point4( -uarmwidth/2,            uarmheight, -uarmwidth/2+uarmwidth, 1.0 );
    point4 p3 = point4( -uarmwidth/2+uarmwidth,  0.0,        -uarmwidth/2+uarmwidth, 1.0 );
    point4 p4 = point4( -uarmwidth/2+uarmwidth,  uarmheight, -uarmwidth/2+uarmwidth, 1.0 );
    point4 p5 = point4( -uarmwidth/2,            0.0,        -uarmwidth/2,           1.0 );
    point4 p6 = point4( -uarmwidth/2,            uarmheight, -uarmwidth/2,           1.0 );
    point4 p7 = point4( -uarmwidth/2+uarmwidth,  0.0,        -uarmwidth/2,           1.0 );
    point4 p8 = point4( -uarmwidth/2+uarmwidth,  uarmheight, -uarmwidth/2,           1.0 );

    point4 robouarmpoints[36] = {p1, p2, p3, p2, p3, p4,  //front
                          p6, p2, p8, p2, p8, p4,  //top
                          p7, p8, p3, p8, p3, p4,  //right
                          p5, p6, p7, p6, p7, p8,  //back
                          p5, p6, p1, p6, p1, p2,  //left
                          p5, p1, p7, p1, p7, p3   //bottom
                        };

    // Set up first VAO (representing grid lines)
    glBindVertexArray(vaoIDs[5]); // Bind the first VAO
    glGenBuffers(2, &vboIDs[10]); // Create two Vertex Buffer Objects for this VAO (positions, colours)

    mv *= (Translate(0.0, larmheight, 0.0) *
            RotateZ(uarmTheta) );
    glUniformMatrix4fv( model_view , 1, GL_TRUE,  mv );


    // Grid vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, vboIDs[10]); // Bind the first robo VBO (vertex positions)
    glBufferData(GL_ARRAY_BUFFER, 36*sizeof(vec4), robouarmpoints, GL_DYNAMIC_DRAW); // Put the robo points in the VBO
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vPosition); // Enable the attribute

    // Grid vertex colours
    glBindBuffer(GL_ARRAY_BUFFER, vboIDs[11]); // Bind the second robo VBO (vertex colours)
    glBufferData(GL_ARRAY_BUFFER, 36*sizeof(vec4), robocolor, GL_DYNAMIC_DRAW); // Put the robo colours in the VBO
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vColor); // Enable the attribute*

    //glBindVertexArray(vaoIDs[5]);
    glDrawArrays( GL_TRIANGLES, 0, 36 );
}

//-------------------------------------------------------------------------------------------------------------------

void initGrid()
{
    // ***Generate geometry data   ***590
    vec4 gridpoints[590]; // Array containing the 64 points of the 32 total lines to be later put in the VBO
    vec4 gridcolours[590]; // One colour per vertex

    // Vertical lines
    for (int i = 0; i < 11; i++){
        gridpoints[2*i] = vec4((33.0 + (33.0 * i)), 33.0, 0, 1);
        gridpoints[2*i + 1] = vec4((33.0 + (33.0 * i)), 693.0, 0, 1);
    }

    // Horizontal lines
    for (int i = 0; i < 21; i++){
        gridpoints[22 + 2*i] = vec4(33.0, (33.0 + (33.0 * i)), 0, 1);
        gridpoints[22 + 2*i + 1] = vec4(363.0, (33.0 + (33.0 * i)), 0, 1);
    }

    // Vertical lines + z
    for (int i = 0; i < 11; i++){
        gridpoints[64 + 2*i] = vec4((33.0 + (33.0 * i)), 33.0, 33.0, 1);
        gridpoints[64 + 2*i + 1] = vec4((33.0 + (33.0 * i)), 693.0, 33.0, 1);
    }

    // Horizontal lines + z
    for (int i = 0; i < 21; i++){
        gridpoints[86 + 2*i] = vec4(33.0, (33.0 + (33.0 * i)), 33.0, 1);
        gridpoints[86 + 2*i + 1] = vec4(363.0, (33.0 + (33.0 * i)), 33.0, 1);
    }

    // Lines in between
    for (int i = 0; i < 11; i++){  //x
        for (int j = 0 ; j < 21 ; j++) {  //y
            gridpoints[128 + i*42 + 2*j] = vec4((33.0+(33.0*i)), (33.0+(33.0*j)), 0, 1);
            gridpoints[128 + i*42 + 2*j + 1] = vec4((33.0+(33.0*i)), (33.0+(33.0*j)), 33.0, 1);
        }
    }

    // Make all grid lines white
    for (int i = 0; i < 590; i++)
        gridcolours[i] = white;


    // *** set up buffer objects
    // Set up first VAO (representing grid lines)
    glBindVertexArray(vaoIDs[0]); // Bind the first VAO
    glGenBuffers(2, vboIDs); // Create two Vertex Buffer Objects for this VAO (positions, colours)



    // Grid vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, vboIDs[0]); // Bind the first grid VBO (vertex positions)
    glBufferData(GL_ARRAY_BUFFER, 590*sizeof(vec4), gridpoints, GL_STATIC_DRAW); // Put the grid points in the VBO
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vPosition); // Enable the attribute

    // Grid vertex colours
    glBindBuffer(GL_ARRAY_BUFFER, vboIDs[1]); // Bind the second grid VBO (vertex colours)
    glBufferData(GL_ARRAY_BUFFER, 590*sizeof(vec4), gridcolours, GL_STATIC_DRAW); // Put the grid colours in the VBO
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vColor); // Enable the attribute
}


void initBoard()
{
    // *** Generate the geometric data
    vec4 boardpoints[7200];
    for (int i = 0; i < 7200; i++)
        boardcolours[i] = black; // Let the empty cells on the board be black

    // Each cell is a cube (2 triangles with 6 vertices)
        for (int i = 0; i < 20; i++){
            for (int j = 0; j < 10; j++)
            {
                vec4 p1 = vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), 0, 1);
                vec4 p2 = vec4(33.0 + (j * 33.0), 66.0 + (i * 33.0), 0, 1);
                vec4 p3 = vec4(66.0 + (j * 33.0), 33.0 + (i * 33.0), 0, 1);
                vec4 p4 = vec4(66.0 + (j * 33.0), 66.0 + (i * 33.0), 0, 1);
                vec4 p5 = vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), 33.0, 1);
                vec4 p6 = vec4(33.0 + (j * 33.0), 66.0 + (i * 33.0), 33.0, 1);
                vec4 p7 = vec4(66.0 + (j * 33.0), 33.0 + (i * 33.0), 33.0, 1);
                vec4 p8 = vec4(66.0 + (j * 33.0), 66.0 + (i * 33.0), 33.0, 1);
                // Two points are reused
                boardpoints[36*(10*i + j) ] = p1;
                boardpoints[36*(10*i + j) + 1] = p2;
                boardpoints[36*(10*i + j) + 2] = p3;
                boardpoints[36*(10*i + j) + 3] = p2;
                boardpoints[36*(10*i + j) + 4] = p3;
                boardpoints[36*(10*i + j) + 5] = p4;

                boardpoints[36*(10*i + j) + 6] = p7;   // right
                boardpoints[36*(10*i + j) + 7] = p8;
                boardpoints[36*(10*i + j) + 8] = p3;
                boardpoints[36*(10*i + j) + 9] = p8;
                boardpoints[36*(10*i + j) + 10] = p3;
                boardpoints[36*(10*i + j) + 11] = p4;

                boardpoints[36*(10*i + j) + 12] = p5;
                boardpoints[36*(10*i + j) + 13] = p6;
                boardpoints[36*(10*i + j) + 14] = p7;
                boardpoints[36*(10*i + j) + 15] = p6;
                boardpoints[36*(10*i + j) + 16] = p7;
                boardpoints[36*(10*i + j) + 17] = p8;

                boardpoints[36*(10*i + j) + 18] = p5;  // left
                boardpoints[36*(10*i + j) + 19] = p6;
                boardpoints[36*(10*i + j) + 20] = p1;
                boardpoints[36*(10*i + j) + 21] = p6;
                boardpoints[36*(10*i + j) + 22] = p1;
                boardpoints[36*(10*i + j) + 23] = p2;     

                boardpoints[36*(10*i + j) + 24] = p5;
                boardpoints[36*(10*i + j) + 25] = p1;
                boardpoints[36*(10*i + j) + 26] = p7;
                boardpoints[36*(10*i + j) + 27] = p1;
                boardpoints[36*(10*i + j) + 28] = p7;
                boardpoints[36*(10*i + j) + 29] = p3;

                boardpoints[36*(10*i + j) + 30] = p6;
                boardpoints[36*(10*i + j) + 31] = p2;
                boardpoints[36*(10*i + j) + 32] = p8;
                boardpoints[36*(10*i + j) + 33] = p2;
                boardpoints[36*(10*i + j) + 34] = p8;
                boardpoints[36*(10*i + j) + 35] = p4;

            }
        }

    // Initially no cell is occupied
    for (int i = 0; i < 10; i++)
        for (int j = 0; j < 20; j++)
            board[i][j] = false;

    // *** set up buffer objects
    glBindVertexArray(vaoIDs[1]);
    glGenBuffers(2, &vboIDs[2]);

    // Grid cell vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, vboIDs[2]);
    glBufferData(GL_ARRAY_BUFFER, 7200*sizeof(vec4), boardpoints, GL_STATIC_DRAW);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vPosition);

    // Grid cell vertex colours
    glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
    glBufferData(GL_ARRAY_BUFFER, 7200*sizeof(vec4), boardcolours, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vColor);
}


// No geometry for current tile initially
void initCurrentTile()
{
    glBindVertexArray(vaoIDs[2]);
    glGenBuffers(2, &vboIDs[4]);

    // Current tile vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, vboIDs[4]);
    glBufferData(GL_ARRAY_BUFFER, 144*sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vPosition);

    // Current tile vertex colours
    glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]);
    glBufferData(GL_ARRAY_BUFFER, 144*sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vColor);
}

void init()
{
    // Load shaders and use the shader program
    GLuint program = InitShader("vshader.glsl", "fshader.glsl");
    glUseProgram(program);

    // Get the location of the attributes (for glVertexAttribPointer() calls)
    vPosition = glGetAttribLocation(program, "vPosition");
    vColor = glGetAttribLocation(program, "vColor");

    // Create 3 Vertex Array Objects, each representing one 'object'. Store the names in array vaoIDs
    glGenVertexArrays(3, &vaoIDs[0]);

    // Initialize the grid, the board, and the current tile
    initGrid();
    initBoard();
    initCurrentTile();

    // The location of the uniform variables in the shader program
    locxsize = glGetUniformLocation(program, "xsize");
    locysize = glGetUniformLocation(program, "ysize");

    model_view = glGetUniformLocation( program, "model_view" );
    projection = glGetUniformLocation( program, "projection" );

    // Game initialization
    hand = vec4(0,0,0,0);
    hand = hand + vec4(0.0, uarmheight, 0.0, 0.0);
    hand = RotateZ(uarmTheta) * hand;
    hand = hand + vec4(0.0, larmheight, 0.0, 0.0);
    hand = RotateZ(larmTheta) * hand;
    hand = hand + vec4(-80.0-basewidth/2+33.0, 33.0+baseheight, 0,0);
    newtile(); // create new next tile

    // set to default
    glBindVertexArray(0);
    glClearColor(0, 0, 0, 0);

    glEnable(GL_DEPTH_TEST);
}

//-------------------------------------------------------------------------------------------------------------------

// Shuffle colour order of tile
void shuffleTile()
{
    vec2 extra;

    extra = tile[3];
    tile[3] = tile[2];
    tile[2] = tile[1];
    tile[1] = tile[0];
    tile[0] = extra;

    if (shuff) {
    	shuffleType++;
    	 }

    if (shuffleType >= 4)
    	shuffleType = 0;
    updatetile();
}

//-------------------------------------------------------------------------------------------------------------------

// Rotates the current tile, if there is room
void rotate()
{
    //bool outOfBounds = false;

    rotationType++;
    if (rotationType == 4)
        rotationType = 0;

    if (shapeType == 0) { // S shape
        for (int i = 0; i < 4; i++)
            tile[i] = allRotationsSshape[rotationType][i]; }
    else if (shapeType == 1) { // I shape
        for (int i = 0; i < 4; i++)
            tile[i] = allRotationsIshape[rotationType][i]; }
    else {
        for (int i = 0; i < 4; i++)  // L shape
            tile[i] = allRotationsLshape[rotationType][i]; }

    if (shuffleType > 0) {
    	for (int i=0 ; i<shuffleType ; i++)
    		shuffleTile();    
    }

    updatetile();
}

//-------------------------------------------------------------------------------------------------------------------

// Checks if the specified row (0 is the bottom 19 the top) is full
// If every cell in the row is occupied, it will clear that cell and everything above it will shift down one row
void checkfullrow()
{
    bool isFull = true;

    while (isFull) \
    {
    	isFull = false;

        for (int j = 0 ; j < 19 ; j++)
        {
            int c = 0;

            for (int i = 0 ; i < 10 ; i++)    // i = left/right x
            {
                if (board[i][j] == true) {      //***changed j to 0 
                    c++; }
             }

            if (c==10)
                isFull = true;

            if (isFull) // if row is full, delete row
            {
                for (int k = j ; k < 19 ; k++)    // k = up/down y    ***change j to 0
                {
                    for (int l = 0 ; l < 10 ; l++) {    // l = left/right x
                        int a = k * 360 + l * 36;
                        int b = a+360;
                        for (int c = 0 ; c<36 ; c++)
                            boardcolours[a+c] = boardcolours[b+c];   // change boardcolour vertices colours
                        board[l][k] = board[l][k+1]; }
                }

                for (int m=0 ; m<10 ; m++)
                    board[m][19] = false;
                for (int c=6840 ; c<7200 ; c++)
                    boardcolours[c] = black;
            }
        }    
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
    glBufferData(GL_ARRAY_BUFFER, 7200*sizeof(vec4), boardcolours, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vColor);

}

//-------------------------------------------------------------------------------------------------------------------

// Game over when any of the top row is true
/*void checkGameOver()
{
    for (int i = 0 ; i< 10 ; i++) {
        if (board[i][19]) {
            gameOver = true;
            cout<<"Game Over\n";
        }
    }

}*/

//-------------------------------------------------------------------------------------------------------------------

// falling tiles
void fallTile()
{
    for (int i=0 ; i<10 ; i++)   // i = x
    {
        int spacenum = 0;       // number of spaces from bottom to tile above
        int spaceb = 20;        // bottom of spaces
        bool space = false;     // check for space in column
        bool space2 = false;    // check for block above space in column
        int j=0;                // counter
        
        while(j<20) {  // j = y    // checks whether needs to fall, if yes, space2 = true
            if (board[i][j]) {
                if (space) {
                    space2 = true;
                    break; }
                j++; }
            else {  //block not occupied
                if (spaceb > j)
                    spaceb = j;
                space = true;
                spacenum++;
                j++;
            }
        }
            
        if (space2)
        {
            while (spaceb+spacenum < 20)
            {
                int pos = spaceb * 360 + i * 36;
                int pos2 = pos + (spacenum*360);
                board[i][spaceb] = board[i][spaceb+spacenum];    // update occupancy
                for (int c=0 ; c<36 ; c++)
                    boardcolours[pos+c] = boardcolours[pos2+c];  // update colours
                spaceb++;
            }
            while (spaceb < 20)  // filling whatevers left on top with blanks
            {
                int pos = spaceb * 360 + i * 36;
                board[i][spaceb] = false;
                for (int c=0 ; c<36 ; c++)
                    boardcolours[pos+c] = black;
                spaceb++;
            }
        }
        
    }

}

//-------------------------------------------------------------------------------------------------------------------

// Check for rows/columns to see if the have three of the same fruits. no combos
// called when set
void three()
{
    bool del = true;

    while (del)
    {
        del = false;

        for (int y=0 ; y<20 ; y++)  // y
        {
            for (int x=0 ; x<8 ; x++)  // check row
            {
                int tcol = y * 360 + x * 36;
                {

                    if ((boardcolours[tcol].x == boardcolours[tcol+36].x) &&     // since (boardcolours[tcol] == boardcolours[tcol+6]
                        (boardcolours[tcol].y == boardcolours[tcol+36].y) &&     // does not seem to work
                        (boardcolours[tcol].z == boardcolours[tcol+36].z) &&
                         board[x][y] && board[x+1][y]  &&
                        (boardcolours[tcol].x == boardcolours[tcol+72].x) &&
                        (boardcolours[tcol].y == boardcolours[tcol+72].y) &&
                        (boardcolours[tcol].z == boardcolours[tcol+72].z) &&
                         board[x][y] && board[x+2][y])   // row has three of same colour
                    { // if begins

                        if (fallTiles)
                        {
                            for (int yy=y ; yy<19 ; yy++)
                            {
                                int tcoll = yy * 360 + x * 36;
                                board[x][yy] = board[x][yy+1];                           // update board occupancy
                                for (int c=0 ; c<36 ; c++)                                // update colours
                                    boardcolours[tcoll+c] = boardcolours[tcoll+360+c];
                                board[x+1][yy] = board[x+1][yy+1];
                                for (int c=0 ; c<36 ; c++)
                                    boardcolours[tcoll+c+36] = boardcolours[tcoll+396+c];
                                board[x+2][yy] = board[x+2][yy+1];
                                for (int c=0 ; c<36 ; c++)
                                    boardcolours[tcoll+c+72] = boardcolours[tcoll+432+c];

                            }
                        }
                        else
                        {
                            int tcoll = y * 360 + x * 36;
                            board[x][y] = false;                           // update board occupancy
                            for (int c=0 ; c<36 ; c++)                                // update colours
                                boardcolours[tcoll+c] = black;
                            board[x+1][y] = false;
                            for (int c=0 ; c<36 ; c++)
                                boardcolours[tcoll+c+36] = black;
                            board[x+2][y] = false;
                            for (int c=0 ; c<36 ; c++)
                                boardcolours[tcoll+c+72] = black;
                        }

                        del = true;
                        deleted++;    // level up if deleted enough lines
                       /* if (deleted == 10)
                        {
                            level++;
                            timer = timer/level;
                            cout<<"Leveled Up! Now level "<<level<<"\n";
                            deleted = 0;
                        }*/

                    }  // end if
                }
            }
        }

        for (int y=0 ; y<18 ; y++)
        {

            for (int x=0 ; x<10 ; x++)   // check column
            {
                int tcol = y * 360 + x * 36;

                if ((boardcolours[tcol].x == boardcolours[tcol+360].x) &&     // since (boardcolours[tcol] == boardcolours[tcol+6]
                    (boardcolours[tcol].y == boardcolours[tcol+360].y) &&     // does not seem to work
                    (boardcolours[tcol].z == boardcolours[tcol+360].z) &&     // column has three of same colour
                     board[x][y] && board[x][y+1]  &&
                    (boardcolours[tcol].x == boardcolours[tcol+720].x) &&
                    (boardcolours[tcol].y == boardcolours[tcol+720].y) &&
                    (boardcolours[tcol].z == boardcolours[tcol+720].z) &&
                     board[x][y] && board[x][y+2])
                {  // if begins

                    if (fallTiles)
                    {
                        for (int yy=y ; yy<20 ; yy++)                 //if column has three of a kind
                        {
                            if (yy<17)                      // if 17 or higher, just delete
                            {
                            int tcoll = yy * 360 + x * 36;
                            board[x][yy] = board[x][yy+3];                           // update board occupancy
                            for (int c=0 ; c<36 ; c++)                                // update colours
                                boardcolours[tcoll+c] = boardcolours[tcoll+1080+c];
                            }
                            else
                            {
                                board[x][yy] = false;
                                boardcolours[yy*360+x+36] = black;
                            }
                        }
                    }
                    else
                    {
                        int tcoll = y * 360 + x * 36;
                        board[x][y] = false;                           // update board occupancy
                        for (int c=0 ; c<36 ; c++)                                // update colours
                            boardcolours[tcoll+c] = black;
                        board[x][y+1] = false;
                        for (int c=0 ; c<36 ; c++)
                            boardcolours[tcoll+c+360] = black;
                        board[x][y+2] = false;
                        for (int c=0 ; c<36 ; c++)
                            boardcolours[tcoll+c+720] = black;
                    }

                    del = true;
                    deleted++;   // level up
                    /*if (deleted == 10)
                    {
                        level++;
                        timer = 750/level;
                        cout<<"Leveled Up! Now level "<<level<<"\n";
                        deleted = 0;
                    }*/

                }  // end if

            }
        }
    }
}

//-------------------------------------------------------------------------------------------------------------------

// Places the current tile - update the board vertex colour VBO and the array maintaining occupied cells
void settile()
{
    bool set = false;

    // check to see if tile needs to set
    for (int i=0 ; i<4 ; i++)
    {

        int xpos = tilepos.x + tile[i].x;
        int ypos = tilepos.y + tile[i].y;

        if (tilepos.y + tile[i].y == 0)  // bottom line
            set = true;

        if (board[xpos][ypos] == true) { // square is occupied
            tilepos.y++;
            set = true;  }
    }


    if (set) {
        for (int i = 0 ; i<4 ; i++)
        {
            int xpos = tilepos.x + tile[i].x;
            int ypos = tilepos.y + tile[i].y;
            int pos = ypos * 360 + xpos * 36;
                        
            board[xpos][ypos] = true;
            for (int j = pos; j<pos+36 ; j++)
                boardcolours[j] = tileColours[i];
            
        }



        if (fallTiles)
            fallTile();
        if (findThrees)
            three();
        if (checkRows)
            checkfullrow();

        //checkGameOver();
        if (!gameOver)
            newtile();



        glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
        glBufferData(GL_ARRAY_BUFFER, 7200*sizeof(vec4), boardcolours, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(vColor);

       /* int low = tilepos.y;
        for (int i = 0 ; i<4 ; i++) {
            if (tilepos.y + tile[i].y < low)
                low = tilepos.y + tile[i].y;
        } */
    }
}


//-------------------------------------------------------------------------------------------------------------------
/*
bool movetile(vec2 direction)
{
    tilepos = tilepos + direction;

    updatetile();
    settile();
    return true; 
} */

//-------------------------------------------------------------------------------------------------------------------

// Makes the current tile fall every [timer] seconds
void fall(int i)
{
    glutTimerFunc(timer, fall, 1);
    if (!gameOver) 
    {
        if (falling) 
        {
            tilepos = tilepos + vec2(0, -1);
            updatetile();
            settile();  
        } 
    }

}


//-------------------------------------------------------------------------------------------------------------------

// Starts the game over - empties the board, creates new tiles, resets line counters
void restart()
{
    //glutTimerFunc(timer, fall, 1);
    //timer = 100;
    gameOver = false;
    level = 1;
    deleted = 0;
    init();
}

//-------------------------------------------------------------------------------------------------------------------

// Draws the game
void display()
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    point4  eye( 363.0/2+500.0*radius*cos(theta), 
                 600.0,
                 500.0*radius*sin(theta),
                 1.0 );
    point4  at( xsize/2, ysize/2, 0.0, 1.0 );
    vec4    up( 0.0, 1.0, 0.0, 0.0 );

    vec4 cam(  (100.0+363.0*radius*cos(theta) - xsize/2 ),
                800.0, 
                363.0*radius*sin(theta),
                1.0 );

    point4  all( radius*sin(theta)*cos(phi),
         radius*sin(theta)*sin(phi),
         radius*cos(theta),
         1.0 );

    mv = LookAt( eye, at, up );
    glUniformMatrix4fv( model_view, 1, GL_TRUE, mv );

    mat4  p = Ortho( cleft, cright, bottom, top, zNear, zFar );
    glUniformMatrix4fv( projection, 1, GL_TRUE, p );

    glUniform1i(locxsize, xsize); // x and y sizes are passed to the shader program to maintain shape of the vertices on screen
    glUniform1i(locysize, ysize);

    glBindVertexArray(vaoIDs[2]); // Bind the VAO representing the current tile (to be drawn on top of the board)
    glDrawArrays(GL_TRIANGLES, 0, 144); // Draw the current tile (8 triangles)

    glBindVertexArray(vaoIDs[0]); // Bind the VAO representing the grid lines (to be drawn on top of everything else)
    glDrawArrays(GL_LINES, 0, 590); // Draw the grid lines (21+11 = 32 lines)

    glBindVertexArray(vaoIDs[1]); // Bind the VAO representing the grid cells (to be drawn first)
    glDrawArrays(GL_TRIANGLES, 0, 7200); // Draw the board (10*20*2 = 400 triangles)

    robobase();
    roboLowArm();
    roboUpArm();

    //char text = '3';
    //printText2D(text, 10, 500, 60);

    //glDrawArrays( GL_TRIANGLES, 0, NumVertices );

    glutPostRedisplay();
    glutSwapBuffers();

}

//-------------------------------------------------------------------------------------------------------------------

// Reshape callback will simply change xsize and ysize variables, which are passed to the vertex shader
// to keep the game the same from stretching if the window is stretched
void reshape(GLsizei w, GLsizei h)
{
    xsize = w;
    ysize = h;
    glViewport(0, 0, w, h);
}

//-------------------------------------------------------------------------------------------------------------------

// Handle arrow key keypresses
void special(int key, int x, int y)
{
    switch(key)
    {
        case 101: //up arrow    ** 102 = right, 100 = left, 103 = down
        	shuff = false;
            rotate();
            break;
    }

    if (glutGetModifiers() == GLUT_ACTIVE_CTRL && key == 100) {
            theta += dr; }

    if (glutGetModifiers() == GLUT_ACTIVE_CTRL && key == 102) {
            theta -= dr; }
}

//-------------------------------------------------------------------------------------------------------------------

// Handles standard keypresses
void keyboard(unsigned char key, int x, int y)
{
    switch(key)
    {
        case 033: // Both escape key and 'q' cause the game to exit
            exit(EXIT_SUCCESS);
            break;
        case 'q':
            exit (EXIT_SUCCESS);
            break;
        case 'r': // 'r' key restarts the game
            restart();
            break;
        case 'e': // space bar -> shuffles tile order
        	shuff = true;
            shuffleTile();
            break;
        case 'o': theta += dr; break;
        case 'p': theta -= dr; break;   
        case 'x': cleft *= 1.1; cright *= 1.1; bottom *= 1.1; top *= 1.1;break;
        case 'z': cleft *= 0.9; cright *= 0.9; bottom *= 0.9; top *= 0.9;break;
        case 'k': phi += dr; break;
        case 'l': phi -= dr; break;
        case 'a': 
            larmTheta += dr*25; 
            hand = vec4(0,0,0,0);
            hand = hand + vec4(0.0, uarmheight, 0.0, 0.0);
            hand = RotateZ(uarmTheta) * hand;
            hand = hand + vec4(0.0, larmheight, 0.0, 0.0);
            hand = RotateZ(larmTheta) * hand;
            hand = hand + vec4(-80.0-basewidth/2+33.0, 33.0+baseheight, 0,0);
            updatetile();
            break;
        case 'd': 
            larmTheta -= dr*25; 
            hand = vec4(0,0,0,0);
            hand = hand + vec4(0.0, uarmheight, 0.0, 0.0);
            hand = RotateZ(uarmTheta) * hand;
            hand = hand + vec4(0.0, larmheight, 0.0, 0.0);
            hand = RotateZ(larmTheta) * hand;
            hand = hand + vec4(-80.0-basewidth/2+33.0, 33.0+baseheight, 0,0);
            updatetile();
            break;
        case 's': 
            uarmTheta -= dr*25;
            hand = vec4(0,0,0,0); 
            hand = hand + vec4(0.0, uarmheight, 0.0, 0.0);
            hand = RotateZ(uarmTheta) * hand;
            hand = hand + vec4(0.0, larmheight, 0.0, 0.0);
            hand = RotateZ(larmTheta) * hand;
            hand = hand + vec4(-80.0-basewidth/2+33.0, 33.0+baseheight, 0,0);
            updatetile();    
            break;
        case 'w': 
            uarmTheta += dr*25; 
            hand = vec4(0,0,0,0);
            hand = hand + vec4(0.0, uarmheight, 0.0, 0.0);
            hand = RotateZ(uarmTheta) * hand;
            hand = hand + vec4(0.0, larmheight, 0.0, 0.0);
            hand = RotateZ(larmTheta) * hand;
            hand = hand + vec4(-80.0-basewidth/2+33.0, 33.0+baseheight, 0,0);
            updatetile();
            break;
        case ' ': //cout<<"hand.x: "<<hand.x<<"  hand.y: "<<hand.y<<"\n";break;
            if (place)
                falling = true;
            break;
    }

    glutPostRedisplay();
}

//-------------------------------------------------------------------------------------------------------------------

void idle(void)
{
    if (!gameOver)
        glutPostRedisplay();
}

void menu(int op)
{
    switch (op)
    {
        case 1:
            restart();
            break;
        case 2:
            if (checkRows)
                checkRows = false;
            else
                checkRows = true;
            break;
        case 3:
            if (fallTiles)
                fallTiles = false;
            else
                checkRows = true;
            break;
        case 4:
            exit(EXIT_SUCCESS);
        case 5:
            restart();
            break;
        case 6:
            restart();
            level = 3;
            timer = 750/level;
            break;
        case 7:
            restart();
            level = 5;
            timer = 750/level;
            break;
        case 8:
            restart();
            level = 7;
            timer = 750/level;
            break;
    }
}

//-------------------------------------------------------------------------------------------------------------------

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(xsize, ysize);
    glutInitWindowPosition(680, 178); // Center the game window (well, on a 1920x1080 display)
    glutCreateWindow("Fruit Tetris");
    glewInit();
    init();

    // Menu functions
    glutCreateMenu(menu);
    glutAddMenuEntry("Restart", 1);
    //glutAddSubMenu("Skip to level ", subMenu);
    glutAddMenuEntry("Disable/Enable row deletion", 2);
    glutAddMenuEntry("Disable/Enable falling tiles", 3);
    glutAddMenuEntry("Quit", 4);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    // Callback functions
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(special);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle);
    glutTimerFunc(timer, fall, 1);

    glutMainLoop(); // Start main loop
    return 0;
}
