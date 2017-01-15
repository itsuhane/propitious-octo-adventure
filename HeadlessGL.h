#pragma once

#include <Windows.h>

#pragma comment (lib, "opengl32.lib")

class HeadlessGL {
public:
    HeadlessGL() {
        HINSTANCE hInstance = GetModuleHandle(nullptr);
        WNDCLASS wc = { 0 };
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = creationHandler;
        wc.hInstance = hInstance;
        wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
        wc.lpszClassName = TEXT("Class_DummyWindowOfHeadessGL");
        if (SUCCEEDED(RegisterClass(&wc))) {
            m_hWnd = CreateWindow(wc.lpszClassName, TEXT("Class_DummyWindowOfHeadessGL"), 0, 0, 0, 640, 480, 0, 0, hInstance, 0);
            m_hDC = s_hDC();
            m_hGLRC = s_hGLRC();
        }
    }

    virtual ~HeadlessGL() {
        wglMakeCurrent(m_hDC, nullptr);
        wglDeleteContext(m_hGLRC);
        DestroyWindow(m_hWnd);
    }

    void make_current() {
        wglMakeCurrent(m_hDC, m_hGLRC);
    }

    void make_other() {
        wglMakeCurrent(m_hDC, nullptr);
    }

private:
    static HDC& s_hDC() {
        static HDC hDC;
        return hDC;
    }

    static HGLRC& s_hGLRC() {
        static HGLRC hGLRC;
        return hGLRC;
    }

    static LRESULT CALLBACK creationHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        if (message != WM_CREATE) {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }

        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
            PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
            32,                       //Colordepth of the framebuffer.
            0, 0, 0, 0, 0, 0,
            0,
            0,
            0,
            0, 0, 0, 0,
            24,                       //Number of bits for the depthbuffer
            8,                        //Number of bits for the stencilbuffer
            0,                        //Number of Aux buffers in the framebuffer.
            PFD_MAIN_PLANE,
            0,
            0, 0, 0
        };

        s_hDC() = GetDC(hWnd);
        SetPixelFormat(s_hDC(), ChoosePixelFormat(s_hDC(), &pfd), &pfd);
        s_hGLRC() = wglCreateContext(s_hDC());

        return 0;
    }

    HWND m_hWnd;
    HDC m_hDC;
    HGLRC m_hGLRC;
};
