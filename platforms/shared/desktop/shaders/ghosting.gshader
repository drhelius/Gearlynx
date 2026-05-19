[Preset]
Name=Ghosting
Passes=1

[Pass0]
Path=ghosting.glsl
ScaleType=Viewport
Filter=Nearest
Feedback=true
History=true

[Parameters]
Trails=0.8
FrameHistory=3

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
