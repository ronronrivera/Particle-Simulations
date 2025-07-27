#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include "OpenGL_Class.h"
#include <ctime>


#define pi 3.14159265359
#define WIDTH 1080
#define HEIGHT 800
#define PARTICLE_NR 12
GLFWwindow* window;


float deltaTime;

const char* vertexShaderSource = R"""(

#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 projection;
void main() {
	gl_Position = projection * vec4(aPos, 1.0);
}
)""";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 1.0, 1.1, 1.0); // orange-ish
}

)";

std::vector<float> vertices;

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
	glViewport(0, 0, width, height);
}

glm::mat4 GetOrthoProjection(){
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	float aspect = (float)width / (float)height;
	return glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
}

void DrawCircle(float centerX, float centerY, float radius, int segments){
	
	vertices.clear();
    
	vertices.push_back(centerX);
  vertices.push_back(centerY);
  vertices.push_back(0.0f);
    

	for(int i = 0; i<=segments; i++){

		float angle = 2.0f * pi * i/segments;
		float x = radius * cos(angle) + centerX;
		float y = radius * sin(angle) + centerY;


		vertices.push_back(x);
		vertices.push_back(y);
		vertices.push_back(0.0f);

	}

}

GLFWwindow* StartGLU(){
	if(!glfwInit()){
		std::cout << "Failed to initialize GLFW, PANIC!!" << std::endl;
		return nullptr;
	}

//	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Particle Sim", nullptr, nullptr);

	if(!window){
		std::cout << "Failed to initialize window" << std::endl;
	}
	glfwMakeContextCurrent(window);

  if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
       std::cout << "Failed to initialize GLAD" << std::endl;
    }


	return window;
}

bool CheckCollision(float x1, float y1, float x2, float y2, float r1, float r2){
	float dx = x2 - x1;
	float dy = y2 - y1;
	float distanceSquared = dx*dx + dy*dy;
	float radiusSum = r1 + r2;
	if (distanceSquared < radiusSum*radiusSum){
		return true;
	}
	return false;

}

void ResolveCollision(glm::vec2 &pos1, glm::vec2 &pos2, glm::vec2 &vel1, glm::vec2 &vel2, float mass1, float mass2) {
    glm::vec2 delta = pos1 - pos2;
    float dist = glm::length(delta);
    if(dist == 0.0f) return;
    
    glm::vec2 normal = delta / dist;
    glm::vec2 relativeVel = vel1 - vel2;
    float velAlongNormal = glm::dot(relativeVel, normal);
    
    // Only resolve if particles are moving toward each other
    if(velAlongNormal > 0.0f) return;
    
    float restitution = 0.5f; // Slightly bouncy
    float j = -(1.0f + restitution) * velAlongNormal;
    j /= (1.0f/mass1 + 1.0f/mass2);
    
    glm::vec2 impulse = j * normal;
    
    vel1 += impulse / mass1;
    vel2 -= impulse / mass2;
    
    // Small positional correction to prevent sticking
    float overlap = 0.5f * (0.0f - velAlongNormal) * deltaTime;
    pos1 += normal * overlap * (mass2/(mass1+mass2));
    pos2 -= normal * overlap * (mass1/(mass1+mass2));
}

void PositionalCorrection(glm::vec2 &pos1, glm::vec2 &pos2, float r1, float r2) {
    glm::vec2 delta = pos2 - pos1;
    float dist = glm::length(delta);
    if(dist == 0.0f || dist > r1 + r2) return;
    
    float overlap = (r1 + r2) - dist;
    glm::vec2 correction = (overlap * 0.5f) * delta / dist;
    
    pos1 -= correction;
    pos2 += correction;
}
struct Particle{

	glm::vec2 positions;
	glm::vec2 velocities;
	float mass;
	float radius;
	bool Initalizing = false;
	bool Launched = false;
	double phase;

};

std::vector<Particle> particles;

void initializeParticles(){

	
	srand(time(0));
	
	float max = 1.0f;
	float min = -1.0f;
for(int i = 0; i<PARTICLE_NR; i++){


		float randX = min + (rand() % (int)((max - min + 1) * 100)) / 100.0f;
		float randY = min + (rand() % (int)((max - min + 1) * 100)) / 100.0f;

		float dis = min + (rand() % (int)((max - min + 1) * 100)) / 100.0f;
		float massS = min + (rand() % (int)((max - min + 1) * 100)) / 100.0f;
		particles.push_back({
			.positions = glm::vec2(randX, randY),
			//initial velocities
			.velocities = glm::vec2(0.5f, 2.0f),
			.mass = 1.0f + massS, //+ dis(gen),
			.radius = 0.05f,
			.phase = dis * 2.0f * pi
			});

	}
}

int main(){
	StartGLU();

	// 1. Compile vertex shader
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	// Optional: check for errors
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
    	glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    	std::cout << "ERROR::VERTEX_SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	// 2. Compile fragment shader
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	// Optional: check for errors
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
    	glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    	std::cout << "ERROR::FRAGMENT_SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
}

	// 3. Link shaders into program
	unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	// Optional: check for errors
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER_PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}

	// Clean up shaders
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);


	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	unsigned int VAO, VBO;
		
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);	

	glBufferData(GL_ARRAY_BUFFER, 360 * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);


	float lastFrame = 0.0f;
	float radius = 0.1f;

	initializeParticles();

	while(!glfwWindowShouldClose(window)) {
    	float currentFrame = glfwGetTime();
    	deltaTime = currentFrame - lastFrame;
    	lastFrame = currentFrame;
    
    	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    	glClear(GL_COLOR_BUFFER_BIT);

    	glUseProgram(shaderProgram);

    	glm::mat4 projection = GetOrthoProjection();
    	unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
   	// Particle-particle collision detection and resolution
   		for(size_t i = 0; i < particles.size(); ++i) {
        	for(size_t j = i + 1; j < particles.size(); ++j) {
            	if(CheckCollision(particles[i].positions.x, particles[i].positions.y,
                             particles[j].positions.x, particles[j].positions.y,
                             particles[i].radius, particles[j].radius)) {
                ResolveCollision(particles[i].positions, particles[j].positions,
                                particles[i].velocities, particles[j].velocities,
                                particles[i].mass, particles[j].mass);
                
                PositionalCorrection(particles[i].positions, particles[j].positions,
                                   particles[i].radius, particles[j].radius);
            }
        }
    }
    	// Boundary collision checks
    	for(auto &particle : particles) {
        	// X-axis boundaries
					//
        	if(particle.positions.x - particle.radius <= -1.0f) {
            	particle.positions.x = -1.0f + particle.radius;
            	particle.velocities.x = -particle.velocities.x * 0.8f;
        	}
        	if(particle.positions.x + particle.radius >= 1.0f) {
            	particle.positions.x = 1.0f - particle.radius;
            	particle.velocities.x = -particle.velocities.x * 0.8f;
        	}

        	// Y-axis boundaries
        	if(particle.positions.y - particle.radius <= -1.0f) {
            	particle.positions.y = -1.0f + particle.radius;
            	particle.velocities.y = -particle.velocities.y * 0.8f;
        	}
        	if(particle.positions.y + particle.radius >= 1.0f) {
            	particle.positions.y = 1.0f - particle.radius;
            	particle.velocities.y = -particle.velocities.y * 0.8f;
        	}

			}
		
	for(auto &particle : particles) {
    	// Floating effect with individual phase offsets
    	float floatHeight = 0.5f;
    	float floatAmount = 0.05f;
			float gravity = -9.81f / 20.0f;
   // 	particle.positions.y = floatHeight + sin(glfwGetTime() * 1.0f + particle.phase) * floatAmount;
    
    	// Horizontal movement with damping
    	particle.positions.x += particle.velocities.x * deltaTime;
    	particle.velocities.x *= 1.0f; // Increased damping
			
			particle.positions.y += particle.velocities.y * deltaTime;
			//particle.velocities.y += gravity * deltaTime;
  }

   	// Draw all particles
    	for(auto &particle : particles) {
        	DrawCircle(particle.positions.x, particle.positions.y, particle.radius, 100);
        	glBindBuffer(GL_ARRAY_BUFFER, VBO);
        	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
        
        	glBindVertexArray(VAO);
        	glDrawArrays(GL_TRIANGLE_FAN, 0, vertices.size() / 3);
    	}

    	glfwSwapBuffers(window);
    	glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

