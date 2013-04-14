#include "SiCKL.h"
using namespace SiCKL;
#include "EasyBMP.h"

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
			// our sample location
			Float2 val0 = NormalizedIndex() * (max - min) + min;
			Float2 val(0.0f, 0.0f);

			Int iteration = 0;

			While(Dot(val, val) < 4.0f && iteration < max_iterations)
				Float xtemp = val.X*val.X - val.Y*val.Y + val0.X;
				val.Y = val.X * val.Y * 2.0f + val0.Y;
				val.X = xtemp;

				iteration += 1;
			EndWhile

			// log scale iteration count to 0,1
			Float norm_val = Log(iteration + 1.0f)/(float)std::logf(max_iterations + 1.0f);
			
			// get color from lookup buffer
			output = color_map((Int)(norm_val * (max_iterations - 1)));
		END_MAIN
	END_SOURCE
};

int main()
{
	// init GLEW/GLUT and other gl setup
	OpenGLRuntime::Initialize();
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
	for(int i = 0; i < colors; i++)
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

	/// Runs the pgoram
	program->Run();


	float* result_buffer = nullptr;
	/// We can either read result back from the texture
	//result.GetData(result_buffer);
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


	/// Keep program from exiting
	getc(stdin);
}