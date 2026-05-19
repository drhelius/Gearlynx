in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D Source;
uniform sampler2D PassFeedback0;
uniform sampler2D SourceHistory0;
uniform sampler2D SourceHistory1;
uniform sampler2D SourceHistory2;
uniform sampler2D SourceHistory3;
uniform sampler2D SourceHistory4;
uniform sampler2D SourceHistory5;
uniform sampler2D SourceHistory6;
uniform int SourceHistoryCount;
uniform float Trails;
uniform float FrameHistory;

void accumulate_frame(vec3 sample_color, inout vec3 frame_sum)
{
    frame_sum += sample_color;
}

void main()
{
    vec3 current = texture(Source, vTexCoord).rgb;
    vec3 frame_sum = current;
    int frame_history = int(clamp(floor(FrameHistory + 0.5), 2.0, 8.0));

    if (SourceHistoryCount == 0)
    {
        FragColor = vec4(current, 1.0);
        return;
    }

    if (frame_history > 1 && SourceHistoryCount > 0)
        accumulate_frame(texture(SourceHistory0, vTexCoord).rgb, frame_sum);

    if (frame_history > 2 && SourceHistoryCount > 1)
        accumulate_frame(texture(SourceHistory1, vTexCoord).rgb, frame_sum);

    if (frame_history > 3 && SourceHistoryCount > 2)
        accumulate_frame(texture(SourceHistory2, vTexCoord).rgb, frame_sum);

    if (frame_history > 4 && SourceHistoryCount > 3)
        accumulate_frame(texture(SourceHistory3, vTexCoord).rgb, frame_sum);

    if (frame_history > 5 && SourceHistoryCount > 4)
        accumulate_frame(texture(SourceHistory4, vTexCoord).rgb, frame_sum);

    if (frame_history > 6 && SourceHistoryCount > 5)
        accumulate_frame(texture(SourceHistory5, vTexCoord).rgb, frame_sum);

    if (frame_history > 7 && SourceHistoryCount > 6)
        accumulate_frame(texture(SourceHistory6, vTexCoord).rgb, frame_sum);

    float trails = clamp(Trails, 0.0, 1.0);
    vec3 combined = frame_sum * ((1.0 - trails) / float(frame_history));

    vec3 previous = texture(PassFeedback0, vTexCoord).rgb;
    vec3 color = previous * trails + combined;
    FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
