#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <cstdlib>
#include <vector>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include <iostream>

using namespace std;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Entity.h"

SDL_Window* displayWindow;
bool gameIsRunning = true;

ShaderProgram program;
ShaderProgram ground;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

struct GameState {
	Entity player;
	Entity ground;
	Entity Title;
	Entity Wall;
	Entity LandingZone;
	Entity rock1;
	Entity rock2;
	Entity Pass;
	Entity Fail;
};
GLuint fontTextureID;
GameState state;

float ranY[10];
bool wonVar=false;
bool failVar = false;

GLuint LoadTexture(const char* filePath) {
	int w, h, n;
	unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	stbi_image_free(image);
	return textureID;
}

void Initialize() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Lunar Landing", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 480);

	program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");

	state.Pass.textureID = LoadTexture("passed.png");
	state.Fail.textureID = LoadTexture("failed.png");

	state.player.textureID = LoadTexture("me.png");
	state.player.position.x = 0;
	state.player.position.y = 4;

	state.rock1.textureID = LoadTexture("rock.png");
	state.rock1.position.y = -6.25;
	state.rock1.position = glm::vec3(-3.5f,-6.25f, 0.0f);
	state.rock2.textureID = LoadTexture("rock.png");
	state.rock2.position =glm::vec3(4.0f,-6.25f,0.0f);

	state.LandingZone.textureID = LoadTexture("landingZone.png");
	//	x = (condition) ? (value_if_true) : (value_if_false);
	int EoO = (rand()%4+1  < 2) ? -1 : 1;
	state.LandingZone.position = glm::vec3(rand() % 10,-7.25,0);

 
/*	state.Title.textureID = LoadTexture("george_0.png");
	state.Title.speed = 2;
	state.Title.cols = 4;
	state.Title.rows = 4;
	state.Title.animIndices = new int[4]{ 3, 7, 11, 15 };
	state.Title.animFrames = 4;
*/
 


	for (int i = 0; i < sizeof(ground); i++) {
		state.ground.textureID = LoadTexture("tile.png");
	}
	for (int i = 0; i <	30; i++) {
		state.Wall.textureID = LoadTexture("brick.png");
	}
	

	viewMatrix = glm::mat4(1.0f);
	modelMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-10.0f, 10.0f, -7.50f, 7.50f, -1.0f, 1.0f);

	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);
	program.SetColor(1.0f, 1.0f, 1.0f, 1.0f);

	glUseProgram(program.programID);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	state.Pass.isText = true;
	state.Fail.isText = true;

	glClearColor(0.2f, 0.2f, 0.8f, 1.0f);
}

void ProcessInput() {


	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			gameIsRunning = false;
		}
	}
	// Check for pressed/held keys below
	const Uint8* keys = SDL_GetKeyboardState(NULL);
	//Player 1
	if (keys[SDL_SCANCODE_A])
	{
		state.player.movement.x = -1;
	}
	else if (keys[SDL_SCANCODE_D])
	{
		state.player.movement.x = 1;
	}
}

void fail() {
	glClearColor(0.7f, 0.4f, 0.4f, 1.0f);
	failVar = true;
}

void won() {

	glClearColor(0.5f, 0.5f, 0.7f, 1.0f);
	wonVar = true;
}

void didWin() {
	if (state.player.position.x >= state.LandingZone.position.x - 0.5 && state.player.position.x <= state.LandingZone.position.x + 0.5) {
		won();
	}
	else {
		fail();
	}
}

void didCollide() {

	//Did my player collide with the ground
	if (state.player.position.y<= state.ground.position.y+1) {
		state.player.position.y = state.ground.position.y + 1.f;
		state.player.speed = 0;
		didWin();
	}
	else {
		state.player.movement.y -= 0.0004f;
	}

	//Did player collide with walls
	if (state.player.position.x <= -9 || state.player.position.x >= 9) {
		state.player.speed = 0;
		fail();
	}
	 //Did player hit rock
		if (state.player.position.y <= -5.50) {
			 
			if (state.player.position.x >= state.rock1.position.x - 0.5 && state.player.position.x <= state.rock1.position.x + 0.5) {
				state.player.speed = 0;
				glClearColor(0.5f, 0.3f, 0.3f, 0.3f);

			}
			if (state.player.position.x >= state.rock2.position.x - 0.5 && state.player.position.x <= state.rock2.position.x + 0.5) {
				state.player.speed = 0;
				glClearColor(0.5f, 0.3f, 0.3f, 0.3f);

			}
		}
		

	


}

#define FIXED_TIMESTEP 0.0166666f
float lastTicks = 0;
float accumulator = 0.0f;

void Update() {
	float ticks = (float)SDL_GetTicks() / 1000.0f;
	float deltaTime = ticks - lastTicks;
	lastTicks = ticks;

 

	deltaTime += accumulator;
	if (deltaTime < FIXED_TIMESTEP) {
		accumulator = deltaTime;
		return;
	}

	while (deltaTime >= FIXED_TIMESTEP) {
		// Update. Notice it's FIXED_TIMESTEP. Not deltaTime
		state.player.Update(FIXED_TIMESTEP);

		deltaTime -= FIXED_TIMESTEP;
	}

	 
	accumulator = deltaTime;
	state.Title.Update(deltaTime);
	state.player.Update(FIXED_TIMESTEP);
}

int start = 0;
 
int x1 = -3;
int x2 = 4;
bool once = true;
void Render() {
	glClear(GL_COLOR_BUFFER_BIT);
	//	state.player.position.y -= 0.0004f;

	didCollide();



	//state.Title.Render(&program);
	state.player.Render(&program);
	state.ground.position.y = -7.25;
	state.rock1.Render(&program);
	state.rock2.Render(&program);




	for (int i = 0; i <22; i++) {
 
		state.ground.position.x = -10 + i;
  
		state.ground.Render(&program);

	}
	if (wonVar) {
		state.Pass.Render(&program);
	}
	if (failVar) {
		state.Fail.Render(&program);
	}
	int offset = -15;
	for (int i = 0; i < 30; i++) {
		if(i<15){
			state.Wall.position.x = -10;
			state.Wall.position.y = -6.25 + i;
			state.Wall.Render(&program);
		}
		else {
			state.Wall.position.x = 10;
			state.Wall.position.y = -6.25 + offset+i;
			state.Wall.Render(&program);
		}

	}
	state.LandingZone.Render(&program);
	SDL_GL_SwapWindow(displayWindow);
}


void Shutdown() {
	SDL_Quit();
}

int main(int argc, char* argv[]) {
	Initialize();

	while (gameIsRunning) {
		ProcessInput();
		Update();
		Render();
	}

	Shutdown();
	return 0;
}