#include <Windows.h>
#include <stdint.h>

#define global static;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

struct Dimension
{
	int width;
	int height;
};

struct OffscreenBuffer
{
	void* memory;
	BITMAPINFO bitmapInfo;
	int height;
	int width;
	int pitch;
};

global bool globalRunning;
global OffscreenBuffer buffer;

Dimension getWindowDimension(HWND window)
{
	Dimension windowDimensions;

	RECT clientRect;
	GetClientRect(window, &clientRect);
	windowDimensions.width = clientRect.right - clientRect.left;
	windowDimensions.height = clientRect.bottom - clientRect.top;

	return windowDimensions;
}

// Allocates the back buffer.
void initBuffer(OffscreenBuffer* buffer, int height, int width)
{
	// free the memory if there is any
	if (buffer->memory)
	{
		VirtualFree(buffer->memory, 0, MEM_RELEASE);
	}

	// set the width and height of the buffer
	buffer->height = height;
	buffer->width = width;
	int bytesPerPixel = 4;

	// fill out the bitmap info
	buffer->bitmapInfo.bmiHeader.biSize = sizeof(buffer->bitmapInfo.bmiHeader);
	buffer->bitmapInfo.bmiHeader.biWidth = buffer->width;
	buffer->bitmapInfo.bmiHeader.biHeight = -buffer->height;
	buffer->bitmapInfo.bmiHeader.biPlanes = 1;
	buffer->bitmapInfo.bmiHeader.biBitCount = 32;
	buffer->bitmapInfo.bmiHeader.biCompression = BI_RGB;

	// set aside memory for the buffer's pixel data
	int bitmapMemorySize = buffer->width * buffer->height * bytesPerPixel;
	buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	buffer->pitch = buffer->width * bytesPerPixel;
}

void renderGradient(OffscreenBuffer *buffer, int r, int g, int b)
{
	int width = buffer->width;
	int height = buffer->height;

	uint8 *row = (uint8 *) buffer->memory;
	for (int y = 0; y < height; y++)
	{
		uint32 *pixel = (uint32 *)row;
		for (int x = 0; x < width; x++)
		{
			// r g b xx
			// b g r xx
			uint8 blue = b + x;
			uint8 green = g + y;

			uint32 color = blue | (green << 8) | (r << 16);
			*pixel++ = color;
		}

		row += buffer->pitch;
	}

}

void drawBufferToScreen(HDC deviceContext, OffscreenBuffer* buffer, int windowWidth, int windowHeight)
{
	StretchDIBits(
		deviceContext,
		0,
		0,
		windowWidth,
		windowHeight,
		0,
		0,
		buffer->width,
		buffer->height,
		buffer->memory,
		&buffer->bitmapInfo,
		DIB_RGB_COLORS,
		SRCCOPY);
}

LRESULT CALLBACK MessageHandler(HWND window,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	LRESULT result = 0;

	switch (message)
	{

	case WM_CLOSE:
	{
		globalRunning = false;
	}break;

	case WM_DESTROY:
	{
		globalRunning = false;
	}break;

	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC deviceContext = BeginPaint(window, &paint);
		Dimension windowDim = getWindowDimension(window);
		drawBufferToScreen(deviceContext, &buffer, windowDim.width, windowDim.height);
		EndPaint(window, &paint);

	}break;
	
	default:
	{
		result = DefWindowProc(window, message, wParam, lParam);
	}break;
	}

	return result;
}

int CALLBACK
WinMain(HINSTANCE instance,
	HINSTANCE prevInstance,
	LPSTR     cmdLine,
	int       showCode)
{
	WNDCLASS windowClass = {};

	// makes the entire client rect part of the window redraw whenever
	// moved horizontally or veritcally
	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = MessageHandler;
	windowClass.hInstance = instance;
	windowClass.lpszClassName = L"Gradient Renderer";

	if (RegisterClass(&windowClass))
	{
		HWND window = CreateWindowEx(
			0,
			windowClass.lpszClassName,
			L"GradientRenderer",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE, //window with border and title. also shows up visible by default
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			instance,
			0);

		if (window)
		{
			HDC deviceContext = GetDC(window);
			uint8 xOffset = 0;
			uint8 yOffset = 0;

			initBuffer(&buffer, 1280, 720);

			globalRunning = true;
			while (globalRunning)
			{
				MSG message;
				while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
				{
					if (message.message == WM_QUIT)
					{
						globalRunning = false;
					}

					TranslateMessage(&message);
					DispatchMessage(&message);
				}

				renderGradient(&buffer, 255, xOffset, yOffset);
				xOffset++;
				yOffset++;


				Dimension windowDimension = getWindowDimension(window);
				drawBufferToScreen(deviceContext, &buffer, windowDimension.width, windowDimension.height);
			}


		}
	}

	return 0;
}