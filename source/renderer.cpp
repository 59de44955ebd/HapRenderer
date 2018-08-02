#include "renderer.h"

#include <initguid.h>
#include <stdio.h>
#include <ppl.h>

#include "opengl.h"
#include "defines.h"
#include "dbg.h"

#include "hap.h"

//######################################
// Globals
//######################################
COpenGL * g_openGL;

//######################################
// Setup data
//######################################

// {7DB57FC5-6810-449F-935B-BC9EA3631560}
DEFINE_GUID(CLSID_HapRenderer,
	0x7db57fc5, 0x6810, 0x449f, 0x93, 0x5b, 0xbc, 0x9e, 0xa3, 0x63, 0x15, 0x60);

const AMOVIESETUP_MEDIATYPE sudPinTypes = {
	&MEDIATYPE_Video,           // Major type
	&MEDIASUBTYPE_NULL          // Minor type
};

const AMOVIESETUP_PIN sudPins = {
	L"Input",                   // Name of the pin
	FALSE,                      // Is pin rendered
	FALSE,                      // Is an output pin
	FALSE,                      // Ok for no pins
	FALSE,                      // Allowed many
	&CLSID_NULL,                // Connects to filter
	L"Output",                  // Connects to pin
	1,                          // Number of pin types
	&sudPinTypes                // Details for pins
};

const AMOVIESETUP_FILTER sudDXTRenderer = {
	&CLSID_HapRenderer,      // Filter CLSID
	L"HapRenderer",          // Filter name
	MERIT_DO_NOT_USE,          // Filter merit
	1,                         // Number pins
	&sudPins                   // Pin details
};

//######################################
// Notify about errors
//######################################
void ErrorMessage (const char * msg) {
	// print to debug log
	OutputDebugStringA(msg);

	// optional: show blocking message box?
	MessageBoxA(NULL, msg, "Error", MB_OK);
}

//######################################
// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance
//######################################
CFactoryTemplate g_Templates[] = {
	{ L"HapRenderer"
	, &CLSID_HapRenderer
	, CVideoRenderer::CreateInstance
	, NULL
	, &sudDXTRenderer }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

//######################################
// CreateInstance
// This goes in the factory template table to create new filter instances
//######################################
CUnknown * WINAPI CVideoRenderer::CreateInstance (LPUNKNOWN pUnk, HRESULT *phr) {
	return new CVideoRenderer(NAME("HapRenderer"), pUnk, phr);
}

//######################################
// The new window/OpenGL thread
//######################################
DWORD WINAPI ThreadProc (void * data) {
	int exitCode = g_openGL->init();
	if (exitCode!=0) return exitCode;
	g_openGL->main();
	g_openGL->exit();
	return exitCode;
}

//######################################
// Constructor
//######################################
CVideoRenderer::CVideoRenderer (TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr) :
	CBaseVideoRenderer(CLSID_HapRenderer, pName, pUnk, phr),
	m_InputPin(NAME("Video Pin"), this, &m_InterfaceLock, phr, L"Input")
{
	// Store the video input pin
	m_pInputPin = &m_InputPin;
	g_openGL = new COpenGL(this);
}

//######################################
// Destructor
//######################################
CVideoRenderer::~CVideoRenderer () {
	m_pInputPin = NULL;
	g_openGL = NULL;
}

 //######################################
// CheckMediaType
// Check the proposed video media type
//######################################
HRESULT CVideoRenderer::CheckMediaType (const CMediaType *pMediaType) {

	// Does this have a VIDEOINFOHEADER format block
	const GUID *pFormatType = pMediaType->FormatType();
	if (*pFormatType != FORMAT_VideoInfo) {
		NOTE("Format GUID not a VIDEOINFOHEADER");
		return E_INVALIDARG;
	}
	ASSERT(pMediaType->Format());

	// Check the format looks reasonably ok
	ULONG Length = pMediaType->FormatLength();
	if (Length < SIZE_VIDEOHEADER) {
		NOTE("Format smaller than a VIDEOHEADER");
		return E_FAIL;
	}

	// Check if the media major type is MEDIATYPE_Video
	const GUID *pMajorType = pMediaType->Type();
	if (*pMajorType != MEDIATYPE_Video) {
		NOTE("Major type not MEDIATYPE_Video");
		return E_INVALIDARG;
	}

	// Check if the media subtype is a supported Hap/DXT type
	const GUID *pSubType = pMediaType->Subtype();
	if (
		   *pSubType == MEDIASUBTYPE_DXT1
		|| *pSubType == MEDIASUBTYPE_DXT5
		|| *pSubType == MEDIASUBTYPE_DXTY

		|| *pSubType == MEDIASUBTYPE_Hap1
		|| *pSubType == MEDIASUBTYPE_Hap5
		|| *pSubType == MEDIASUBTYPE_HapY

	) {
		return NOERROR;
	}

	return E_INVALIDARG;
}

//######################################
// GetPin
// We only support one input pin and it is numbered zero
//######################################
CBasePin *CVideoRenderer::GetPin (int n) {
	ASSERT(n == 0);
	if (n != 0) return NULL;

	// Assign the input pin if not already done so
	if (m_pInputPin == NULL) {
		m_pInputPin = &m_InputPin;
	}

	return m_pInputPin;
}

//######################################
//
//######################################
void HapMTDecode(HapDecodeWorkFunction function, void *info, unsigned int count, void * /*info*/) {
	concurrency::parallel_for((unsigned int)0, count, [&](unsigned int i) {
		function(info, i);
	});
}

//######################################
// DoRenderSample
// Render the current frame
//######################################
HRESULT CVideoRenderer::DoRenderSample (IMediaSample *pMediaSample) {
	CheckPointer(pMediaSample, E_POINTER);
	CAutoLock cInterfaceLock(&m_InterfaceLock);

	if (g_openGL->m_hwnd) {

		PBYTE pbData;
		HRESULT hr = pMediaSample->GetPointer(&pbData);
		if (FAILED(hr)) return hr;

		if (m_mtIn.subtype == MEDIASUBTYPE_Hap1 || m_mtIn.subtype == MEDIASUBTYPE_Hap5 || m_mtIn.subtype == MEDIASUBTYPE_HapY) { // Hap data

			size_t outputBufferDecodedSize;
			unsigned int outputBufferTextureFormat;

			unsigned int res = HapDecode(
				pbData,
				pMediaSample->GetActualDataLength(),
				0,
				HapMTDecode,
				nullptr,
				m_outputBuffer,
				(unsigned long)m_outputBufferSize,
				(unsigned long *)&outputBufferDecodedSize,
				&outputBufferTextureFormat
			);

			if (res != HapResult_No_Error) {
				return E_FAIL;
			}

			SendMessage(g_openGL->m_hwnd, WM_FRAMECHANGED, (WPARAM)m_outputBuffer, (LPARAM)outputBufferDecodedSize);

		}
		else { // raw DXT data
			SendMessage(g_openGL->m_hwnd, WM_FRAMECHANGED, (WPARAM)pbData, (LPARAM)pMediaSample->GetActualDataLength());
		}

	}

	return S_OK;
}

//######################################
// SetMediaType
// We store a copy of the media type used for the connection in the renderer
// because it is required by many different parts of the running renderer
// This can be called when we come to draw a media sample that has a format
// change with it. We normally delay type changes until they are really due
// for rendering otherwise we will change types too early if the source has
// allocated a queue of samples. In our case this isn't a problem because we
// only ever receive one sample at a time so it's safe to change immediately
//######################################
HRESULT CVideoRenderer::SetMediaType (const CMediaType *pMediaType) {
	CheckPointer(pMediaType, E_POINTER);
	HRESULT hr = NOERROR;
	CAutoLock cInterfaceLock(&m_InterfaceLock);
	CMediaType StoreFormat(m_mtIn);
	m_mtIn = *pMediaType;
	return NOERROR;
}

//######################################
// Active
//
// The auto show flag is used to have the window shown automatically when we
// change state. We do this only when moving to paused or running, when there
// is no outstanding EC_USERABORT set and when the window is not already up
// This can be changed through the IVideoWindow interface AutoShow property.
// If the window is not currently visible then we are showing it because of
// a state change to paused or running, in which case there is no point in
// the video window sending an EC_REPAINT as we're getting an image anyway
//######################################
HRESULT CVideoRenderer::Active() {
	NOTE("Active");

	//if (m_VideoText.IsAutoShowEnabled() == TRUE)
	if (g_openGL->m_hwnd) {
		if (m_bAbort == FALSE) {
			//if (IsWindowVisible(g_hwnd) == FALSE) {
				SetRepaintStatus(FALSE);

				//m_VideoText.PerformanceAlignWindow();
				//m_VideoText.DoShowWindow(SW_SHOWNORMAL);
				//m_VideoText.DoSetWindowForeground(TRUE);

				//g_openGL->show(TRUE);
				SendMessage(g_openGL->m_hwnd, WM_SHOW, 1, 0);
			//}
		}
	}

	return CBaseVideoRenderer::Active();
} 

//######################################
// BreakConnect
// This is called when a connection or an attempted connection is terminated
// and lets us to reset the connection flag held by the base class renderer
// The filter object may be hanging onto an image to use for refreshing the
// video window so that must be freed (the allocator decommit may be waiting
// for that image to return before completing) then we must also uninstall
// any palette we were using, reset anything set with the control interfaces
// then set our overall state back to disconnected ready for the next time
//######################################
HRESULT CVideoRenderer::BreakConnect () {
	NOTE("BreakConnect");

	CAutoLock cInterfaceLock(&m_InterfaceLock);

	// Check we are in a valid state
	HRESULT hr = CBaseVideoRenderer::BreakConnect();
	if (FAILED(hr)) return hr;

	// The window is not used when disconnected
	IPin *pPin = m_InputPin.GetConnected();
	if (pPin) SendNotifyWindow(pPin, NULL);

	// end thread, destroy window
	if (g_openGL->m_hwnd && m_thread) {
		SendMessage(g_openGL->m_hwnd, WM_DESTROY, 0, 0);
		WaitForSingleObject(m_thread, INFINITE);
		m_thread = NULL;
		g_openGL->m_hwnd = NULL;
	}

	if (m_outputBuffer) {
		free(m_outputBuffer);
		m_outputBuffer = NULL;
		m_outputBufferSize = 0;
	}

	return NOERROR;
}

//######################################
// CompleteConnect
// When we complete connection we need to see if the video has changed sizes
// If it has then we activate the window and reset the source and destination
// rectangles. If the video is the same size then we bomb out early. By doing
// this we make sure that temporary disconnections such as when we go into a
// fullscreen mode do not cause unnecessary property changes. The basic ethos
// is that all properties should be persistent across connections if possible
//######################################
HRESULT CVideoRenderer::CompleteConnect (IPin *pReceivePin) {
	NOTE("CompleteConnect");

	CAutoLock cInterfaceLock(&m_InterfaceLock);
	CBaseVideoRenderer::CompleteConnect(pReceivePin);

	VIDEOINFOHEADER *pVideoInfo = (VIDEOINFOHEADER *) m_mtIn.Format();
	g_openGL->setType(pVideoInfo, m_mtIn.subtype);

	// wen need a buffer for Hap types, but not for raw DXT types
	if (m_mtIn.subtype == MEDIASUBTYPE_Hap1 || m_mtIn.subtype == MEDIASUBTYPE_Hap5 || m_mtIn.subtype == MEDIASUBTYPE_HapY) {
		if (m_outputBuffer) {
			if (pVideoInfo->bmiHeader.biSizeImage>m_outputBufferSize) {
				m_outputBuffer = (PBYTE)realloc(m_outputBuffer, pVideoInfo->bmiHeader.biSizeImage);
				if (!m_outputBuffer) return E_OUTOFMEMORY;
			}
		}
		else {
			m_outputBuffer = (PBYTE)malloc(pVideoInfo->bmiHeader.biSizeImage);
			if (!m_outputBuffer) return E_OUTOFMEMORY;
		}
		m_outputBufferSize = pVideoInfo->bmiHeader.biSizeImage;
	}
	else {
		if (m_outputBuffer) {
			free(m_outputBuffer);
			m_outputBuffer = NULL;
		}

	}

	// this threads creates the video window and OpenGL context
	m_thread = CreateThread(0, 0, ThreadProc, NULL, 0, 0);

	// wait untill window was created
	while (!g_openGL->m_hwnd) Sleep(100);

	return NOERROR;
}

//######################################
// Constructor
//######################################
CVideoInputPin::CVideoInputPin (TCHAR *pObjectName,
		CVideoRenderer *pRenderer,
		CCritSec *pInterfaceLock,
		HRESULT *phr,
		LPCWSTR pPinName) :
	CRendererInputPin(pRenderer, phr, pPinName),
	m_pRenderer(pRenderer),
	m_pInterfaceLock(pInterfaceLock)
{
	ASSERT(m_pRenderer);
	ASSERT(pInterfaceLock);
}

////////////////////////////////////////////////////////////////////////
// Exported entry points for registration and unregistration
// (in this case they only call through to default implementations).
////////////////////////////////////////////////////////////////////////

//######################################
// DllRegisterSever
//######################################
STDAPI DllRegisterServer () {
	return AMovieDllRegisterServer2(TRUE);
}

//######################################
// DllUnregisterServer
//######################################
STDAPI DllUnregisterServer () {
	return AMovieDllRegisterServer2(FALSE);
}

//######################################
// DllEntryPoint
//######################################
extern "C" BOOL WINAPI DllEntryPoint (HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  dwReason, LPVOID lpReserved) {
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}
