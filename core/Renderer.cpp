#include "Renderer.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include<stb_image.h>
#include<stb_image_write.h>

unsigned int Renderer::GetTextureRGB32F(int w, int h) {
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, w, h, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

Renderer::Renderer(std::shared_ptr<Integrator> inte, std::shared_ptr<PostProcessing> p) {
	width = inte->width;
	height = inte->height;
	integrator = inte;
	post = p;
	frameCounter = 0;

#pragma region InitOpenGL
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(width, height, "DreamRender", NULL, NULL);
	if (window == NULL) {
		std::cout << "Init Window failed!" << std::endl;
		glfwTerminate();

		assert(0);
	}
	glfwMakeContextCurrent(window);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Init glad failed!" << std::endl;

		assert(0);
	}

	glViewport(0, 0, width, height);
#pragma endregion

#pragma region PipelineConfiguration
	Shader shader1("../shader/VertexShader.vert", "../shader/MixFrameShader.frag");
	Shader shader2("../shader/VertexShader.vert", "../shader/LastFrameShader.frag");
	Shader shader3("../shader/VertexShader.vert", "../shader/OutputShader.frag");

	pass1.program = shader1.ID;
	pass1.width = width;
	pass1.height = height;
	pass1.colorAttachments.push_back(GetTextureRGB32F(pass1.width, pass1.height));
	pass1.BindData();

	pass2.program = shader2.ID;
	pass2.width = width;
	pass2.height = height;
	lastFrame = GetTextureRGB32F(pass2.width, pass2.height);
	pass2.colorAttachments.push_back(lastFrame);
	pass2.BindData();

	pass3.program = shader3.ID;
	pass3.width = width;
	pass3.height = height;
	pass3.BindData(true);
#pragma endregion
}

Renderer::~Renderer() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

#pragma pack(2)//Ӱ���ˡ����롱������ʵ��ǰ�� sizeof(BITMAPFILEHEADER) �Ĳ��

struct RGBColor
{
	char B;		//��
	char G;		//��
	char R;		//��
};

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef long    LONG;

//BMP�ļ�ͷ��
struct BITMAPFILEHEADER
{
	WORD  bfType;		//�ļ����ͱ�ʶ������ΪASCII�롰BM��
	DWORD bfSize;		//�ļ��ĳߴ磬��byteΪ��λ
	WORD  bfReserved1;	//�����֣�����Ϊ0
	WORD  bfReserved2;	//�����֣�����Ϊ0
	DWORD bfOffBits;	//һ����byteΪ��λ��ƫ�ƣ���BITMAPFILEHEADER�ṹ�忪ʼ��λͼ����
};

//BMP��Ϣͷ��
struct BITMAPINFOHEADER
{
	DWORD biSize;			//����ṹ��ĳߴ�
	LONG  biWidth;			//λͼ�Ŀ��
	LONG  biHeight;			//λͼ�ĳ���
	WORD  biPlanes;			//The number of planes for the target device. This value must be set to 1.
	WORD  biBitCount;		//һ�������м�λ
	DWORD biCompression;    //0����ѹ����1��RLE8��2��RLE4
	DWORD biSizeImage;      //4�ֽڶ����ͼ�����ݴ�С
	LONG  biXPelsPerMeter;  //������/�ױ�ʾ��ˮƽ�ֱ���
	LONG  biYPelsPerMeter;  //������/�ױ�ʾ�Ĵ�ֱ�ֱ���
	DWORD biClrUsed;        //ʵ��ʹ�õĵ�ɫ����������0��ʹ�����еĵ�ɫ������
	DWORD biClrImportant;	//��Ҫ�ĵ�ɫ����������0�����еĵ�ɫ����������Ҫ
};

void WriteBMP(const char* FileName, RGBColor* ColorBuffer, int ImageWidth, int ImageHeight)
{
	//��ɫ�����ܳߴ磺
	const int ColorBufferSize = ImageHeight * ImageWidth * sizeof(RGBColor);

	//�ļ�ͷ
	BITMAPFILEHEADER fileHeader;
	fileHeader.bfType = 0x4D42;	//0x42��'B'��0x4D��'M'
	fileHeader.bfReserved1 = 0;
	fileHeader.bfReserved2 = 0;
	fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + ColorBufferSize;
	fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	//��Ϣͷ
	BITMAPINFOHEADER bitmapHeader = { 0 };
	bitmapHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapHeader.biHeight = ImageHeight;
	bitmapHeader.biWidth = ImageWidth;
	bitmapHeader.biPlanes = 1;
	bitmapHeader.biBitCount = 24;
	bitmapHeader.biSizeImage = ColorBufferSize;
	bitmapHeader.biCompression = 0; //BI_RGB


	FILE* fp;//�ļ�ָ��

	//���ļ���û���򴴽���
	fopen_s(&fp, FileName, "wb");

	//д���ļ�ͷ����Ϣͷ
	fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp);
	fwrite(&bitmapHeader, sizeof(BITMAPINFOHEADER), 1, fp);
	//д����ɫ����
	fwrite(ColorBuffer, ColorBufferSize, 1, fp);

	fclose(fp);
}


void Renderer::Run() {
	RGBSpectrum* nowTexture = new RGBSpectrum[width * height];
	nowFrame = GetTextureRGB32F(width, height);

	while (!glfwWindowShouldClose(window)) {
		t2 = clock();
		dt = (double)(t2 - t1) / CLOCKS_PER_SEC;
		fps = 1.0 / dt;
		std::cout << "\r";
		std::cout << std::fixed << std::setprecision(2) << "FPS : " << fps << "    FrameCounter: " << frameCounter;
		t1 = t2;

		integrator->RenderImage(post, nowTexture);

		glBindTexture(GL_TEXTURE_2D, nowFrame);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nowTexture);

		glUseProgram(pass1.program);

		glUniform1ui(glGetUniformLocation(pass1.program, "frameCounter"), frameCounter++);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, nowFrame);
		glUniform1i(glGetUniformLocation(pass1.program, "nowFrame"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, lastFrame);
		glUniform1i(glGetUniformLocation(pass1.program, "lastFrame"), 1);

		pass1.Draw();
		pass2.Draw(pass1.colorAttachments);
		pass3.Draw(pass2.colorAttachments);

		RGBColor* ColorBuffer = new RGBColor[width * height];

		//��ȡ���أ�ע������ĸ�ʽ�� BGR��
		glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, ColorBuffer);

		//������д���ļ�
		WriteBMP("output.bmp", ColorBuffer, width, height);

		delete[] ColorBuffer;
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	if (nowTexture != NULL) {
		delete[] nowTexture;
		nowTexture = NULL;
	}
}

std::shared_ptr<Renderer> Renderer::Create(const RendererParams& params) {
	return std::make_shared<Renderer>(params.integrator, params.post);
}
