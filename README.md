# Mandelbrot Set 
This implementation uses Cuda to calculate the mandelbrot set, and OpenGL with glfw to show it onscreen.

The pipeline is calculation on gpu -> gpu writes the colors to an opengl buffer object -> the buffer object is used as an PBO and gives the information to a texture -> the texture gets rendered every frame
