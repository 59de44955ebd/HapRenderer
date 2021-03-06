# HapRenderer
A simple DirectShow video renderer that renders [HAP video](http://hap.video/) (Hap1, Hap Alpha, Hap Q) as well as the corresponding raw DXT data types (DXT1, DXT5, DXTY).

For HAP AVIs it supports both videos rendered with the [Renderheads VfW codec](https://github.com/Vidvox/hap-directshow) and with [FFmpeg](https://github.com/FFmpeg/FFmpeg). If you use a splitter filter like [LAV Splitter/LAV Splitter source filter](https://github.com/Nevcairiel/LAVFilters) also containers MOV and MKV can be used.

The renderer creates a window with OpenGL context (using [GLFW](https://github.com/glfw/glfw) 3.3), and displays the video in this window. The window is transparent, so HAP Alpha video will be rendered directly over the Windows desktop (TODO: add a property dialog and API that allows to switch to undecorated/chromeless window mode - at the moment the window always has a title bar and border).

The main purpose of this filter is to allow to explore and test HAP and DXT flavors in DirectShow editors like GraphEdit/GraphStudio/GraphStudioNext.

*Screenshots*

Here some GraphStudio screenshots of example graphs:

HapRenderer rendering "sample-1080p30-Hap.avi" directly:
![](screenshots/haprenderer_hap1.png)

HapRenderer rendering DXT1 frames, as provided by the Renderheads HAP DirectShow codec:
![](screenshots/haprenderer_dxt1.png)
