#include "SiCKL.h"	
using namespace SiCKL;
#include "EasyBMP.h"
#include <math.h>

#if 0
KernelSource(MandelBrot)
{
	KernelMain(Float2 min, Float2 max, Buffer1D<Float3> color_map) -> KernelOutput(Float3 output)

	EndMain
};
#endif

// algorithm from: http://en.wikipedia.org/wiki/Mandelbrot_set
class Mandelbrot : public Source
{
public:
	Mandelbrot() : max_iterations(512) {}
	const int32_t max_iterations;

	BEGIN_SOURCE
		BEGIN_CONST_DATA
			CONST_DATA(Float2, min)
			CONST_DATA(Float2, max)
			CONST_DATA(Buffer1D<Float3>, color_map)
		END_CONST_DATA

		BEGIN_OUT_DATA
			OUT_DATA(Float3, output)
		END_OUT_DATA

		BEGIN_MAIN

			Float2 val0 = NormalizedIndex() * (max - min) + min;
			Float x0 = val0.X;
			Float y0 = val0.Y;

			Float x = 0;
			Float y = 0;

			Int iteration = 0;

			While(x*x + y*y < 4.0f && iteration < max_iterations)
				Float xtemp = x*x - y*y + x0;
				y = 2.0f*x*y + y0;

				x = xtemp;

				iteration = iteration + 1;
			EndWhile

			// log scale iteration count to 0,1
			Float norm_val = Log(((Float)iteration + 1.0f))/float(log(max_iterations + 1.0));


			// get color from lookup buffer
			output = color_map((Int)(norm_val * (float)(max_iterations - 1)));

		END_MAIN
	END_SOURCE
};

int main()
{
    if(OpenCLRuntime::Initialize() != SICKL_SUCCESS)
    {
        printf("Could not OpenCL Context\n");
        return -1;
    }

    Mandelbrot mbrot;
    
    mbrot.Parse();
  
    mbrot.GetRoot().Print();
    
    OpenCLProgram program;
    OpenCLCompiler::Build(mbrot, program);

#if 0

	// init GLEW/GLUT and other gl setup
    if(!OpenGLRuntime::Initialize())
    {
        printf("Could not create OpenGL Context\n");
        return -1;
    }

	OpenGLCompiler comp;

	Mandelbrot mbrot;
	mbrot.Parse();
	/// Prints the AST generated from the Mandelbrot source

	mbrot.GetRoot().Print();

	/// Compile our OpenGL program
	OpenGLProgram* program = comp.Build(mbrot);

	/// Print the generated GLSL source
	printf("%s\n", program->GetSource().c_str());

	
	const uint32_t width = 350 * 5;
	const uint32_t height = 200 * 5;
	const uint32_t colors = mbrot.max_iterations;

	/// Generate the color table (a nice gold)
	float* color_map_data = new float[3 * colors];
	for(uint32_t i = 0; i < colors; i++)
	{
		float x = i/(float)colors;
		color_map_data[3 * i + 0] = 191.0f / 255.0f * (1.0f - x);
		color_map_data[3 * i + 1] = 125.0f / 255.0f * (1.0f - x);
		color_map_data[3 * i + 2] = 37.0f / 255.0f * (1.0f - x);
	}

	/// put it int a 1d buffer
	OpenGLBuffer1D color_map(colors, ReturnType::Float3, color_map_data);

	/// our output buffer
	OpenGLBuffer2D result(width, height, ReturnType::Float3, nullptr);
	OpenGLBuffer2D copy(width, height, ReturnType::Float3, nullptr);

	/// initialize our program
	program->Initialize(width, height);

	/// get our binding locations for each of the program input and outputs
	input_t min_loc = program->GetInputHandle("min");
	input_t max_loc = program->GetInputHandle("max");
	input_t color_map_loc = program->GetInputHandle("color_map");

	output_t output_loc = program->GetOutputHandle("output");

	/// sets min values
	program->SetInput(min_loc, -2.5f, -1.0f);
	/// sets max values
	program->SetInput(max_loc, 1.0f, 1.0f);
	/// set the scaler
	program->SetInput(color_map_loc, color_map);

	/// sets the render location
	program->BindOutput(output_loc, result);

	/// Runs the program
	program->Run();

    /// We can copy our data to the second buffer
    copy.SetData(result);

    float* result_buffer = nullptr;
    /// We can either read result back from the texture
    copy.GetData(result_buffer);

	/// Or from the framebuffer (which is faster on nvidia hardware at least)
    program->GetOutput(output_loc, result_buffer);

	/// Finally, dump the image to a Bitmap to view
	BMP image;
	image.SetSize(width, height);

	for(uint32_t i = 0; i < height; i++)
	{
		for(uint32_t j = 0; j < width; j++)
		{
			float red = result_buffer[i * width * 3 + j * 3 + 0];
			float green = result_buffer[i * width * 3 + j * 3 + 1];
			float blue = result_buffer[i * width * 3 + j * 3 + 2];

			auto pixel = image(j,i);
			pixel->Red = (uint8_t)(red * 255);
			pixel->Green = (uint8_t)(green * 255);
			pixel->Blue = (uint8_t)(blue * 255);
		}
	}

	image.WriteToFile("result.bmp");

	/// Cleanup

	free(result_buffer);
	delete program;

    OpenGLRuntime::Finalize();
#endif
}
