#include <iostream>
#include <string>
#include <algorithm>
#include <list>
#include <stdlib.h>

//Include matrix libraries
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/ext.hpp"
// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// Other includes
#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include <SOIL.h>

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void do_movement(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void spawnMen(Model& Man, Model& Spencer, Shader& ourShader);

// Window dimensions
const GLuint WIDTH = 1650, HEIGHT = 900;

//Camera
Camera camera(glm::vec3(0.0f, 3.0f, 0.0f));

bool keys[1024];
GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;

GLfloat deltaTime = 0.0f;	// Time between current frame and last frame
GLfloat lastFrame = 0.0f;  	// Time of last frame

list<Model> Bullets;

//Model Bullet;
glm::mat4 bulPos;
bool bulletActive = false;
bool setup = true;

vector<glm::mat4> badGuyPositions;
bool dead[10];

int spencerHitCount = 0;
bool spencerActivated = false;
glm::mat4 spencerPos;

float redColour = 0.5;
float otherColour = 0.5;
bool fadeColour = false;
bool fadeIn = false;
int wave = 1;

int main()
{
	// Init GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); //OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create a window
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Thomas' Retribution", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Set the required callback functions
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetKeyCallback(window, key_callback);

	// GLFW Options
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
	glewExperimental = GL_TRUE;
	// Initialize GLEW to setup the OpenGL Function pointers
	glewInit();

	// Define the viewport dimensions
	glViewport(0, 0, WIDTH, HEIGHT);

	// Enable z-buffer (allows opengl when to draw over a pixel and when not to)
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// Build and compile our shader program
	Shader ourShader("shader.vert", "shader.frag");

	// Load models
	Model Sun("objects/oldMan.obj");
	Model Track("objects/rail.obj");
	Model Platform("objects/thomas.obj");
	Model Environment("objects/environment.obj");
	Model Bullet("objects/coal.obj");
	Model Man("objects/oldMan.obj");
	Model Spencer("objects/spencer.obj");
	Model Smoke("objects/smoke.obj");
	Model Explosion("objects/fire.obj");

	glm::mat4 camPos;
	glm::mat4 badGuys; // = glm::translate(badGuys, glm::vec3(10.0f, 0.0f, 0.0f));

	camPos = glm::scale(camPos, glm::vec3(0.09, 0.09, 0.09));
	glm::mat4 camEmpty;
	glm::vec4 vecEmpty;

	glm::vec4 badPos;
	glm::vec4 spencerHitBox;
	glm::vec4 coalPos;
	glm::vec4 origin = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	vector<glm::mat4> smokePosList;
	vector<glm::mat4> explosionList;
	int smokeCounter = 0;
	int explosionCounter = 0;

	float expCurve = 1;
	float bulletZ = 0;
	float bulletX = 0;
	
	bool collision = false;
	bool spacePressed = false;
	float speed = 0.1;

	bool demise = false;

	// Set all enemies to dead first
	for (size_t i = 0; i < 10; i++)
	{
		dead[i] = true;
	}

	// Game loop
	while (!glfwWindowShouldClose(window)&&!demise)
	{
		// Calculate deltatime of current frame
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();

		// do_movement(window);

		// Render
		// Clear the colorbuffer
		glClearColor(redColour, otherColour, otherColour, 1.0f); //background color
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		ourShader.Use();


		//Lighting Shit
		glUniform3f(glGetUniformLocation(ourShader.Program, "lightColor"), 1.0f, 1.0f, 1.0f);
		glUniform3f(glGetUniformLocation(ourShader.Program, "lightPos"), 20.0f, 20.0f, 20.0f);
		glUniform3f(glGetUniformLocation(ourShader.Program, "viewPos"), camera.Position[0], camera.Position[1], camera.Position[2]);
		// Fog
		glUniform4f(glGetUniformLocation(ourShader.Program, "fogColor"), redColour, otherColour, otherColour, 1.0);


		// Transformation matrices
		glm::mat4 projection = glm::perspective(camera.Zoom, (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));

		spawnMen(Man, Spencer, ourShader);

		coalPos = camPos*origin;

		// Collision Check and Player Death
		for (int i = 0; i<badGuyPositions.size(); i++)
		{
			badPos = badGuyPositions[i] * origin;

			float upperX = badPos[0] + 1;
			float lowerX = badPos[0] - 1;
			float upperZ = badPos[2] + 1;
			float lowerZ = badPos[2] - 1;

			if (coalPos[0] < upperX && coalPos[0] > lowerX && coalPos[2] < upperZ && coalPos[2] > lowerZ && coalPos[1] < 5)
			{
				if (!dead[i])
				{
					collision = true;
					dead[i] = true;
				}
			}

			if (badPos[0] < 1 && badPos[0] > -1 && badPos[2] < 1 && badPos[2] > -1) demise = true;

			upperX = 0;
			lowerX = 0;
			upperZ = 0;
			lowerZ = 0;

		}

		if (spencerActivated) {
			spencerHitBox = spencerPos * origin;
			float upperX = spencerHitBox[0] + 6;
			float lowerX = spencerHitBox[0] - 6;
			float upperY = spencerHitBox[1] + 4;
			float upperZ = spencerHitBox[2] + 2;
			float lowerZ = spencerHitBox[2] - 2;

			if (coalPos[0] < upperX && coalPos[1] < upperY && coalPos[2] < upperZ && coalPos[0] > lowerX && coalPos[2] > lowerZ) 
			{
				spencerHitCount++;
				collision = true;
				cout << "Spencer Hit Count = " << spencerHitCount << endl;
			}

			if (spencerHitBox[0] < 8 && spencerHitBox[0] > -8) demise = true;
			upperX = 0;
			lowerX = 0;
			upperY = 0;
			upperZ = 0;
			lowerZ = 0;
		}

		// Bullet Physics
		if (bulletActive)
		{
			// Calculate translation since last iteration of while loop
			expCurve -= 0.015; // Bullet Drop
			bulletX *= 0.992;	//Direction in x
			bulletZ *= 0.992;	//Direction in z
			camPos = glm::translate(camPos, glm::vec3(bulletX, expCurve, bulletZ)); 
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(camPos));
			Bullet.Draw(ourShader);

			smokeCounter++;

			if (smokeCounter % 5 == 0) {
				float randNumX, randNumZ;
				int smokeSize;
				randNumX = ((float)(rand() % 100)) / 100;
				randNumZ = ((float)(rand() % 100)) / 100;
				glm::mat4 smokePos;
				smokePos = glm::translate(camPos, glm::vec3(randNumX, 0, randNumZ));
				smokePos = glm::scale(smokePos, glm::vec3(1.5 + randNumX, 1.5 + randNumX, 1.5 + randNumX));
				smokePos = glm::rotate(smokePos, 180 + (-glm::radians(camera.Yaw)), glm::vec3(0.0f, 1.0f, 0.0f));
				smokePos = glm::rotate(smokePos, glm::radians(308.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(smokePos));
				smokePosList.push_back(smokePos);
			}

			for (int i = 0; i < smokePosList.size(); i++)
			{
				float x, z;
				int leftRight;
				leftRight = rand() % 2;
				if (leftRight == 0)
					leftRight = -1;
				
				smokePosList[i] = glm::translate(smokePosList[i], glm::vec3(0.2*leftRight, 0.2, 0.2*leftRight));
				smokePosList[i] = glm::scale(smokePosList[i], glm::vec3(0.97, 0.97, 0.97));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(smokePosList[i]));
				Smoke.Draw(ourShader);
			}

			coalPos = camPos*origin;
			if (coalPos[1] < 0)
			{
				bulletActive = false;
				camPos = camEmpty;
				camPos = glm::scale(camPos, glm::vec3(0.09, 0.09, 0.09));
			}

			if (collision)
			{
				bool waveClear = true;
				for (int i = 0; i < 10; i++)
				{
					if (!dead[i]) waveClear = false;
				}

				if (waveClear)
				{
					setup = true;
					wave++;
				}
				
				for (int i = 0; i < 3; i++)
				{
					float randNumX, randNumZ;
					int smokeSize;
					randNumX = ((float)(rand() % 100)) / 100;
					randNumZ = ((float)(rand() % 100)) / 100;
					glm::mat4 smokePos = camPos;
					smokePos = glm::translate(smokePos, glm::vec3(randNumX, 0, randNumZ));
					smokePos = glm::scale(smokePos, glm::vec3(15 + randNumX, 15 + randNumX, 15 + randNumX));
					smokePos = glm::rotate(smokePos, 180 + (-glm::radians(camera.Yaw)), glm::vec3(0.0f, 1.0f, 0.0f));
					smokePos = glm::rotate(smokePos, glm::radians(308.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(smokePos));
					explosionList.push_back(smokePos);
				}
				bulletActive = false;
				collision = false;
				camPos = camEmpty;
				camPos = glm::scale(camPos, glm::vec3(0.09, 0.09, 0.09));
			}

		}
		if (!bulletActive) {
			expCurve = 1.2;
			bulletZ = camera.Front[2];
			bulletX = camera.Front[0];
			smokeCounter = 0;
			smokePosList.clear();
		}

		if (fadeColour) {
			redColour += 0.00025;
			otherColour -= 0.0005;
			if (redColour > 0.75) redColour = 0.75;
			if (otherColour < 0) otherColour = 0;
			if (redColour == 0.75 && otherColour == 0) fadeColour = false;
		}

		if (fadeIn) {
			otherColour += 0.001;
			if (otherColour > 0.75) 
			{
				otherColour = 0.75;
				fadeIn = false;
			}
			
		}

		if (!bulletActive &&  glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		{
			speed += 0.03;
			expCurve += 0.05;
			spacePressed = true;
		}

		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE && spacePressed)
		{

			if (speed > 5) speed = 5;
			if (!bulletActive) {
				camPos = glm::translate(camPos, glm::vec3(camera.Position[0], camera.Position[1] + 20, camera.Position[2]));
				// X is speed in camera.front * X 
				bulletZ = camera.Front[2] * speed;
				bulletX = camera.Front[0] * speed;
			}
			bulletActive = true;
			spacePressed = false;
			// Velocity = speed;
			speed = 0.1;
		}


		if (explosionList.size() > 0)
		{		

			for (size_t i = 0; i < explosionList.size(); i++)
			{

				if (explosionCounter > 400)
				{
					explosionList.clear();
					explosionCounter = 0;
				}
				else 
				{
					float x, z;
					int leftRight;
					leftRight = rand() % 2;
					if (leftRight == 0)
						leftRight = -1;

					explosionList[i] = glm::translate(explosionList[i], glm::vec3(0, 0.1, 0));
					explosionList[i] = glm::scale(explosionList[i], glm::vec3(0.97, 0.97, 0.97));
					glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(explosionList[i]));
					Explosion.Draw(ourShader);
					explosionCounter++;
				}

			}
		}



		// Track Placement and Environment Placed
		glm::mat4 model;
		glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		Track.Draw(ourShader);
		Environment.Draw(ourShader);

		glm::mat4 thomPos;
		thomPos = glm::rotate(thomPos, 180 + (-glm::radians(camera.Yaw)), glm::vec3(0.0f, 1.0f, 0.0f));
		thomPos = glm::rotate(thomPos, glm::radians(308.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(thomPos));
		Platform.Draw(ourShader);

		glm::mat4 sunPos;
		sunPos = glm::translate(sunPos, glm::vec3(20.0f, 20.0f, 20.0f));
		sunPos = glm::scale(sunPos, glm::vec3(0.2, 0.2, 0.2));
		glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(sunPos));
		Sun.Draw(ourShader);

		model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		Track.Draw(ourShader);
		model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		Track.Draw(ourShader);
		model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		Track.Draw(ourShader);
		model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		Track.Draw(ourShader);
		model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		Track.Draw(ourShader);
		model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		Track.Draw(ourShader);
		model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		Track.Draw(ourShader);

		// Swap the screen buffers
		glfwSwapBuffers(window);
	}
	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
	return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

void do_movement(GLFWwindow *window)
{
	// Camera controls
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	GLfloat xoffset = xpos - lastX;
	GLfloat yoffset = lastY - ypos;  // Reversed since y-coordinates go from bottom to left

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void spawnMen(Model& Man, Model& Spencer, Shader& ourShader)
{
	// Waves of Enemies
	if (wave == 1)
	{
		if (setup)
		{
			glm::mat4 badGuy1;
			glm::mat4 badGuy2;

			// 1st Enemy
			badGuy1 = glm::translate(badGuy1, glm::vec3(20.0f, 0.0f, 0.0f));
			badGuy1 = glm::rotate(badGuy1, glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy1);
			dead[0] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[0]));
			Man.Draw(ourShader);

			// 2nd Enemy
			badGuy2 = glm::translate(badGuy2, glm::vec3(0.0f, 0.0f, 20.0f));
			badGuy2 = glm::rotate(badGuy2, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy2);
			dead[1] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[1]));
			Man.Draw(ourShader);

			setup = false;
		}
		else
		{
			if (!dead[0])
			{
				// Draw First Man with Movement Translation
				badGuyPositions[0] = glm::translate(badGuyPositions[0], glm::vec3(0.0f, 0.0f, 0.01f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[0]));
				Man.Draw(ourShader);
			}
			if (!dead[1])
			{
				// Draw Second Man with Movement Translation
				badGuyPositions[1] = glm::translate(badGuyPositions[1], glm::vec3(0.0f, 0.0f, 0.01f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[1]));
				Man.Draw(ourShader);
			}
		}
	}
	else if (wave == 2)
	{
		if (setup)
		{
			badGuyPositions.clear();
			glm::mat4 badGuy;
			glm::mat4 empty;

			// 1st Enemy
			badGuy = glm::translate(badGuy, glm::vec3(20.0f, 0.0f, 0.0f));
			badGuy = glm::rotate(badGuy, glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy);
			dead[0] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[0]));
			Man.Draw(ourShader);


			// 2nd Enemy
			badGuy = empty;
			badGuy = glm::translate(badGuy, glm::vec3(0.0f, 0.0f, 20.0f));
			badGuy = glm::rotate(badGuy, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy);
			dead[1] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[1]));
			Man.Draw(ourShader);

			// 3rd Enemy
			badGuy = empty;
			badGuy = glm::translate(badGuy, glm::vec3(20.0f, 0.0f, 20.0f));
			badGuy = glm::rotate(badGuy, glm::radians(220.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy);
			dead[2] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[2]));
			Man.Draw(ourShader);

			// 4th Enemy
			badGuy = empty;
			badGuy = glm::translate(badGuy, glm::vec3(-20.0f, 0.0f, -20.0f));
			badGuy = glm::rotate(badGuy, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy);
			dead[3] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[3]));
			Man.Draw(ourShader);


			setup = false;
		}
		else
		{
			if (!dead[0])
			{
				// Draw First Man with Movement Translation
				badGuyPositions[0] = glm::translate(badGuyPositions[0], glm::vec3(0.0f, 0.0f, 0.01f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[0]));
				Man.Draw(ourShader);
			}

			if (!dead[1])
			{
				// Draw Second Man with Movement Translation
				badGuyPositions[1] = glm::translate(badGuyPositions[1], glm::vec3(0.0f, 0.0f, 0.01f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[1]));
				Man.Draw(ourShader);
			}

			if (!dead[2])
			{
				// Draw Third Man with Movement Translation
				badGuyPositions[2] = glm::translate(badGuyPositions[2], glm::vec3(0.001f, 0.0f, 0.011f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[2]));
				Man.Draw(ourShader);
			}
			if (!dead[3])
			{
				// Draw Fourth Man with Movement Translation
				badGuyPositions[3] = glm::translate(badGuyPositions[3], glm::vec3(0.00008f, 0.0f, 0.011f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[3]));
				Man.Draw(ourShader);
			}
		}
	}
	else if (wave == 3)
	{
		if (setup)
		{
			badGuyPositions.clear();
			glm::mat4 badGuy;
			glm::mat4 empty;

			// 1st Enemy
			badGuy = glm::translate(badGuy, glm::vec3(20.0f, 0.0f, -20.0f));
			badGuy = glm::rotate(badGuy, glm::radians(315.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy);
			dead[0] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[0]));
			Man.Draw(ourShader);

			// 2nd Enemy
			badGuy = empty;
			badGuy = glm::translate(badGuy, glm::vec3(-20.0f, 0.0f, 20.0f));
			badGuy = glm::rotate(badGuy, glm::radians(135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy);
			dead[1] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[1]));
			Man.Draw(ourShader);

			// 3rd Enemy
			badGuy = empty;
			badGuy = glm::translate(badGuy, glm::vec3(0.0f, 0.0f, -20.0f));
			badGuy = glm::rotate(badGuy, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy);
			dead[2] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[2]));
			Man.Draw(ourShader);

			// 4th Enemy
			badGuy = empty;
			badGuy = glm::translate(badGuy, glm::vec3(-20.0f, 0.0f, 0.0f));
			badGuy = glm::rotate(badGuy, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy);
			dead[3] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[3]));
			Man.Draw(ourShader);

			setup = false;
		} else
		{
			if (!dead[0])
			{
				// Draw First Man with Movement Translation
				badGuyPositions[0] = glm::translate(badGuyPositions[0], glm::vec3(0.00008f, 0.0f, 0.011f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[0]));
				Man.Draw(ourShader);
			}

			if (!dead[1])
			{
				// Draw Second Man with Movement Translation
				badGuyPositions[1] = glm::translate(badGuyPositions[1], glm::vec3(0.00008f, 0.0f, 0.011f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[1]));
				Man.Draw(ourShader);
			}

			if (!dead[2])
			{
				// Draw Third Man with Movement Translation
				badGuyPositions[2] = glm::translate(badGuyPositions[2], glm::vec3(0.0f, 0.0f, 0.01f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[2]));
				Man.Draw(ourShader);
			}
			if (!dead[3])
			{
				// Draw Fourth Man with Movement Translation
				badGuyPositions[3] = glm::translate(badGuyPositions[3], glm::vec3(0.0f, 0.0f, 0.01f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[3]));
				Man.Draw(ourShader);
			}
		}
	}
	else if (wave == 4)
	{
		if (setup)
		{
			badGuyPositions.clear();
			glm::mat4 badGuy;
			glm::mat4 empty;

			// 1st Enemy
			badGuy = glm::translate(badGuy, glm::vec3(20.0f, 0.0f, 0.0f));
			badGuy = glm::rotate(badGuy, glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy);
			dead[0] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[0]));
			Man.Draw(ourShader);


			// 2nd Enemy
			badGuy = empty;
			badGuy = glm::translate(badGuy, glm::vec3(0.0f, 0.0f, 20.0f));
			badGuy = glm::rotate(badGuy, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy);
			dead[1] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[1]));
			Man.Draw(ourShader);

			// 3rd Enemy
			badGuy = empty;
			badGuy = glm::translate(badGuy, glm::vec3(20.0f, 0.0f, 20.0f));
			badGuy = glm::rotate(badGuy, glm::radians(220.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy);
			dead[2] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[2]));
			Man.Draw(ourShader);

			// 4th Enemy
			badGuy = empty;
			badGuy = glm::translate(badGuy, glm::vec3(-20.0f, 0.0f, -20.0f));
			badGuy = glm::rotate(badGuy, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy);
			dead[3] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[3]));
			Man.Draw(ourShader);

			// 5th Enemy
			badGuy = empty;
			badGuy = glm::translate(badGuy, glm::vec3(20.0f, 0.0f, -20.0f));
			badGuy = glm::rotate(badGuy, glm::radians(315.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy);
			dead[4] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[4]));
			Man.Draw(ourShader);

			// 6th Enemy
			badGuy = empty;
			badGuy = glm::translate(badGuy, glm::vec3(-20.0f, 0.0f, 20.0f));
			badGuy = glm::rotate(badGuy, glm::radians(135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy);
			dead[5] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[5]));
			Man.Draw(ourShader);

			// 7th Enemy
			badGuy = empty;
			badGuy = glm::translate(badGuy, glm::vec3(0.0f, 0.0f, -20.0f));
			badGuy = glm::rotate(badGuy, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy);
			dead[6] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[6]));
			Man.Draw(ourShader);

			// 8th Enemy
			badGuy = empty;
			badGuy = glm::translate(badGuy, glm::vec3(-20.0f, 0.0f, 0.0f));
			badGuy = glm::rotate(badGuy, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			badGuyPositions.push_back(badGuy);
			dead[7] = false;
			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[7]));
			Man.Draw(ourShader);

			setup = false;
		}
		else
		{
			if (!dead[0])
			{
				// Draw First Man with Movement Translation
				badGuyPositions[0] = glm::translate(badGuyPositions[0], glm::vec3(0.0f, 0.0f, 0.005f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[0]));
				Man.Draw(ourShader);
			}

			if (!dead[1])
			{
				// Draw Second Man with Movement Translation
				badGuyPositions[1] = glm::translate(badGuyPositions[1], glm::vec3(0.0f, 0.0f, 0.005f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[1]));
				Man.Draw(ourShader);
			}

			if (!dead[2])
			{
				// Draw Third Man with Movement Translation
				badGuyPositions[2] = glm::translate(badGuyPositions[2], glm::vec3(0.001f, 0.0f, 0.011f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[2]));
				Man.Draw(ourShader);
			}
			if (!dead[3])
			{
				// Draw Fourth Man with Movement Translation
				badGuyPositions[3] = glm::translate(badGuyPositions[3], glm::vec3(0.00008f, 0.0f, 0.011f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[3]));
				Man.Draw(ourShader);
			}

			if (!dead[4])
			{
				// Draw First Man with Movement Translation
				badGuyPositions[4] = glm::translate(badGuyPositions[4], glm::vec3(0.00008f, 0.0f, 0.011f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[4]));
				Man.Draw(ourShader);
			}

			if (!dead[5])
			{
				// Draw Second Man with Movement Translation
				badGuyPositions[5] = glm::translate(badGuyPositions[5], glm::vec3(0.00008f, 0.0f, 0.011f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[5]));
				Man.Draw(ourShader);
			}

			if (!dead[6])
			{
				// Draw Third Man with Movement Translation
				badGuyPositions[6] = glm::translate(badGuyPositions[6], glm::vec3(0.0f, 0.0f, 0.005f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[6]));
				Man.Draw(ourShader);
			}
			if (!dead[7])
			{
				// Draw Fourth Man with Movement Translation
				badGuyPositions[7] = glm::translate(badGuyPositions[7], glm::vec3(0.0f, 0.0f, 0.005f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(badGuyPositions[7]));
				Man.Draw(ourShader);
			}
		}
	}
	else if (wave>4 && spencerHitCount < 6)
	{
		if (!spencerActivated)
		{
			spencerPos = glm::translate(spencerPos, glm::vec3(35.0f, 0.0f, 0.0f));
			spencerPos = glm::rotate(spencerPos, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

			glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(spencerPos));
			Spencer.Draw(ourShader);
			fadeColour = true;
			spencerActivated = true;
		}
		else
		{
			if (spencerActivated) {
				spencerPos = glm::translate(spencerPos, glm::vec3(0.0f, 0.0f, -0.01f));
				glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(spencerPos));
				Spencer.Draw(ourShader);
			}
		}
	}
	else
	{
		fadeIn = true;
		wave = 1;
	}
}