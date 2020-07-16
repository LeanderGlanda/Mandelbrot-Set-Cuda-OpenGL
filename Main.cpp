#include "Main.h"
#include <iostream>
#include <Windows.h>

unsigned int width = 1280;
unsigned int height = 720;

int maxIteration = 10000;
double middlea = -1;
double middleb = 0;
double rangea = 5.25;
double rangeb = 3;

bool gpu = true;

// Frame counter used for key inputs
int count = 0;

unsigned int VAO, VBO, EBO, PBO, TEXTURE;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, "Apfelmännchen", NULL, NULL);;
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Print keys
    std::cout << "Press SPACE to switch between CPU and GPU calculation\n";

    // Set everything up
    setUp(VAO, VBO, EBO, PBO, TEXTURE);
    setUpCuda(PBO);

    calculate(window);

    //
    // render loop
    //

    Shader shader("vertex.shader", "fragment.shader");

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindTexture(GL_TEXTURE_2D, TEXTURE);

        shader.use();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
        count++;
        Sleep(1000 / 60);
    }

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &PBO);
	glDeleteTextures(1, &TEXTURE);

    glfwTerminate();
}

void framebuffer_size_callback(GLFWwindow* window, int l_width, int l_height)
{
    glViewport(0, 0, width, height);
    width = l_width;
    height = l_height;
}

void processInput(GLFWwindow* window)
{
    // The program only checks key inputs every 5 frames.
    if (count == 5)
    {
        count = 0;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            gpu = !gpu;
            std::cout << (gpu ? "Switched to GPU calculation" : "Switched to CPU calculation") << "\n";
            calculate(window);
        }
    }
}

void calculate(GLFWwindow* window)
{
    if (gpu)
    {
        launchCuda(PBO);
        updateTexture(TEXTURE, PBO);
    }
    else
    {
        // Create texture
        float* data = new float[(long long)width * height * 3];
        calculationCPU(data);
        // Upload the new texture
        uploadNewTexture(data, TEXTURE, PBO);
        delete[] data;
    }
    std::cout << "Finished calculation!\n";
}

// Mandelbrot calculation running on the CPU
void calculationCPU(float* mandelbrot)
{
    for (unsigned int y = 0; y < height; y++)
    {
        for (unsigned int x = 0; x < width; x++)
        {
            int ipos = width * y * 3 + x * 3;

            double x0 = (double)x / (double)width;
            double y0 = (double)y / (double)height;

            x0 = x0 * rangea + middlea - rangea / 2;
            y0 = y0 * rangeb + middleb - rangeb / 2;

            double real = 0;
            double imaginary = 0;

            int iteration = 0;
            while (real * real + imaginary * imaginary <= 4 && iteration < maxIteration)
            {
                double temp = real * real - imaginary * imaginary + x0;
                imaginary = 2 * real * imaginary + y0;
                real = temp;
                iteration++;
            }

            // Color algorithm from my brother (https://github.com/Julian-Wollersberger/Apfelmannchen)
            int runde;
            double fraction;

            if (iteration == maxIteration)
            {
                mandelbrot[ipos + 0] = 1.0f;
                mandelbrot[ipos + 1] = 1.0f;
                mandelbrot[ipos + 2] = 1.0f;
            }
            else
            {
                iteration += 8;
                runde = 15;
                while (iteration >= runde)
                    runde = (runde * 2) + 1;

                fraction = (iteration - runde / 2) / (double)(runde / 2);

                if (fraction < 0)
                {
                    mandelbrot[ipos + 0] = 1.0f;
                    mandelbrot[ipos + 1] = 1.0f;
                    mandelbrot[ipos + 2] = 1.0f;
                }
                else if (fraction < 1.0 / 6) { mandelbrot[ipos + 0] = 1.0f;  mandelbrot[ipos + 1] = 0.0f;  mandelbrot[ipos + 2] = fraction * 6.0f; }
                else if (fraction < 2.0 / 6) { mandelbrot[ipos + 0] = 1 - (fraction - 1.0 / 6) * 6;  mandelbrot[ipos + 1] = 0.0f;  mandelbrot[ipos + 2] = 1.0f; }
                else if (fraction < 3.0 / 6) { mandelbrot[ipos + 0] = 0.0f;  mandelbrot[ipos + 1] = (fraction - 2.0 / 6) * 6; mandelbrot[ipos + 2] = 1.0f; }
                else if (fraction < 4.0 / 6) { mandelbrot[ipos + 0] = 0.0f;  mandelbrot[ipos + 1] = 1.0f;  mandelbrot[ipos + 2] = 1 - (fraction - 3.0 / 6) * 6; }
                else if (fraction < 5.0 / 6) { mandelbrot[ipos + 0] = (fraction - 4.0 / 6) * 6;  mandelbrot[ipos + 1] = 1.0f;  mandelbrot[ipos + 2] = 0.0f; }
                else if (fraction <= 6.0 / 6) { mandelbrot[ipos + 0] = 1.0f;  mandelbrot[ipos + 1] = 1 - (fraction - 5.0 / 6) * 6;  mandelbrot[ipos + 2] = 0.0f; }
                else
                {
                    mandelbrot[ipos + 0] = 0.0f;
                    mandelbrot[ipos + 1] = 0.0f;
                    mandelbrot[ipos + 2] = 0.0f;
                }
            }
        }
    }
}