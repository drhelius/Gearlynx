; Zfast LCD preset adapted from libretro slang-shaders handheld/zfast-lcd by Greg Hogan (SoltanGris42).
[Preset]
Name=Zfast LCD + Ghosting
Passes=2

[Pass0]
Path=zfast_lcd.glsl
ScaleType=Viewport
Filter=Linear

[Pass1]
Path=ghosting.glsl
ScaleType=Viewport
Filter=Nearest
Feedback=true
History=true

[Parameters]
Trails=0.8
FrameHistory=3
BORDERMULT=14.0
GBAGAMMA=1.0

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

[Parameter.BORDERMULT]
Label=Border Multiplier
Min=-40.0
Max=40.0
Step=1.0

[Parameter.GBAGAMMA]
Label=GBA Gamma Hack
Min=0.0
Max=1.0
Step=1.0