#include "SiCKL.h"
using namespace SiCKL;
#include "EasyBMP.h"

class Mandelbrot : public Source
{
public:
	Mandelbrot() : max_iterations(50) {}
	const int32_t max_iterations;

	BEGIN_SOURCE
		BEGIN_CONST_DATA

		END_CONST_DATA

		BEGIN_OUT_DATA
			OUT_DATA(Float, output)
		END_OUT_DATA

		BEGIN_MAIN
			Float2 val0 = NormalizedIndex() * Float2(3.5f, 2.0f) - Float2(2.5f, 1.0f);
			Float2 val(0.0f, 0.0f);

			Int iteration = 0;

			While(Dot(val, val) < 4.0f && iteration < max_iterations)
				Float xtemp = val.X*val.X - val.Y*val.Y + val0.X;
				val.Y = val.X * val.Y * 2.0f + val0.Y;
				val.X = xtemp;

				iteration += 1;
			EndWhile

			Float norm_val = (Float)(Log(iteration + 1.0f)/(float)std::logf(max_iterations + 1.0f));

			output = (Float)1.0 - norm_val;
			output = Sqrt(output);
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

	const uint32_t width = 350 * 10;
	const uint32_t height = 200 * 10;

	program->Initialize(width, height);

	OpenGLBuffer2D result(width, height, ReturnType::Float, nullptr);

	program->BindOutput(0, result);
	program->Run();
	float* result_buffer = nullptr;
	/// We can either read result back from the texture
	//result.GetData(result_buffer);
	/// Or from the framebuffer (which is faster on nvidia hardware at least)
	program->GetOutput(0, result_buffer);

	/// Finally, dump the image to a Bitmap to view
	BMP image;
	image.SetSize(width, height);

	for(uint32_t i = 0; i < height; i++)
	{
		for(uint32_t j = 0; j < width; j++)
		{
			float red = result_buffer[i * width + j];
			float green = 0.0f;
			float blue = 0.0f;

			auto pixel = image(j,i);
			pixel->Red = (uint8_t)(red * 255);
			pixel->Green = (uint8_t)(green * 255);
			pixel->Blue = (uint8_t)(red * 255);
		}
	}

	image.WriteToFile("result.bmp");

	/// Cleanup

	free(result_buffer);
	delete program;


	/// Keep program from exiting
	getc(stdin);
}