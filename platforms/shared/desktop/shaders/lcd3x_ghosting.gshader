[Preset]
Name=LCD 3x + Ghosting
Passes=2

[Pass0]
Path=lcd3x.glsl
ScaleType=Viewport
Filter=Nearest

[Pass1]
Path=ghosting.glsl
ScaleType=Viewport
Filter=Nearest
Feedback=true
History=true

[Parameters]
Trails=0.8
FrameHistory=3
brighten_scanlines=16.0
brighten_lcd=4.0

[Parameter.Trails]
Label=Trails
Min=0.0
Max=1.0
Step=0.01

[Parameter.FrameHistory]
Label=Frame History
Min=2.0
Max=8.0
Step=1.0

[Parameter.brighten_scanlines]
Label=Brighten Scanlines
Min=1.0
Max=32.0
Step=0.5

[Parameter.brighten_lcd]
Label=Brighten LCD
Min=1.0
Max=12.0
Step=0.1