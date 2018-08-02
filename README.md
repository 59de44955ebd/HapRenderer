# HapRenderer
A simple DirectShow video renderer that renders HAP video (Hap1, Hap Alpha, Hap Q) as well as the corresponding raw DXT data types (DXT1, DXT5, DXTY).

For HAP AVIs it supports both the Renderheads flavor and the FFmpeg flavor.
Otherwise, if you use a Splitter filter like LAV Splitter, also containers MOV and MKV can be used.

The renderer creates a window with OpenGL context (using GLFW 3.3), and displays the video in this window. The window is transparent, so Hap Alpha video will be rendered directly over the Windows desktop. (TODO: add a property dialog and API that allows to switch to undecorated/chromesless window mode - at the moment the window always has a title bar and border).

The main purpose of this filter is to allow to explore and test Hap and DXT flavors in DirectShow editors like GraphEdit/GraphStudio/GraphStudioNext.

Here some GraphStudio screenshots of example graphs:

- HapRenderer rendering "sample-1080p30-Hap.avi" directly:


- HapRenderer rendering DXT1 frames, as provided by the Renderheads DirectShow codec:

