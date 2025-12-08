#version 330 core

in vec2 uvOut;

uniform sampler2D sampler;

out vec4 fragColor;

float sampleIntensity(ivec2 p)
{
    vec4 pixP = texelFetch(sampler, p, 0);
    float I = (pixP.x + pixP.y + pixP.z) / 3.;
    return I;
}

float bilateral(int ksize, float sigma_s, float sigma_r)
{
    int hksize = int(floor(ksize / 2));

    float twoSigmaS2 = 2.0 * sigma_s * sigma_s;
    float twoSigmaR2 = 2.0 * sigma_r * sigma_r;
    float acc = 0.0;
    float wsum = 0.0;

    ivec2 p = ivec2(gl_FragCoord.xy);

    float pixP = sampleIntensity(p);

    for (int dy = -hksize; dy <= hksize; ++dy)
    {
        for (int dx = -hksize; dx <= hksize; ++dx)
        {
            float w = exp(-(dx * dx + dy * dy) / twoSigmaS2);
            ivec2 q = p + ivec2(dx, dy);
            float pixQ = sampleIntensity(q);
            float pixDiff = abs(pixP - pixQ);
            pixDiff = exp(-(pixDiff * pixDiff) / twoSigmaR2);
            acc += w * pixDiff * pixQ;
            wsum += w * pixDiff;
        }
    }

    return acc / wsum;
}

void main()
{
    int dR = 6;
    float gamma = 0.7;

    vec4 linear = texture(sampler, uvOut);
    // fragColor = linear;
    float I = (linear.x + linear.y + linear.z) / 3.;
    vec4 C = vec4(linear.x / I, linear.y / I, linear.z / I, 1.);
    float E = log2(I);
    float B = log2(bilateral(9, 9, 0.4));

    float D = E - B;
    float B_max = 2.32, B_min = -3.32;
    float s = dR / (B_max - B_min);
    float B_p = (B - B_max) * s;
    float O = pow(2, B_p + D);
    vec4 hdr_p = O * C;
    fragColor = vec4(pow(hdr_p.x, gamma), pow(hdr_p.y, gamma), pow(hdr_p.z, gamma), 1.);
}
