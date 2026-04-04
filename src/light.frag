uniform sampler2D texture;    // Das normale Spielbild
uniform sampler2D lightMap;   // Unsere Licht-Textur (klein)

void main() {
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    vec4 light = texture2D(lightMap, gl_TexCoord[0].xy);

    gl_FragColor = pixel * light;
}