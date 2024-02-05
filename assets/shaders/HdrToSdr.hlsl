#include "Common.hlsli"

struct VertToPix
{
    float4 Position : SV_Position;
    float2 TC : TEXCOORD;
};

struct PixOut
{
    float4 Color : SV_Target0;
};

VertToPix MainVS(uint vtxId : SV_VertexID)
{
    VertToPix OUT = (VertToPix)0;

    if (vtxId == 0)
    {
        OUT.TC = float2(-1.0f, -1.0f);
        OUT.Position = float4(OUT.TC, 0.0f, 1.0f);
    }
    else if (vtxId == 1)
    {
        OUT.TC = float2(-1.0f, 3.0f);
        OUT.Position = float4(OUT.TC, 0.0f, 1.0f);
    }
    else if (vtxId == 2)
    {
        OUT.TC = float2(3.0f, -1.0f);
        OUT.Position = float4(OUT.TC, 0.0f, 1.0f);
    }

    return OUT;
}

static const float A = 0.15f;
static const float B = 0.50f;
static const float C = 0.10f;
static const float D = 0.20f;
static const float E = 0.02f;
static const float F = 0.30f;
static const float W = 11.2f;

float3 Uncharted2_tonemap(float3 value) {
    return ((value * (A * value + C * B) + D * E) / (value * (A * value + B) + D * F)) - E / F;
}

float3 Tonemap(float3 color)
{
    float exposure = 1.75f;
    float gamma = 2.2f;

    float3 result = Uncharted2_tonemap(color * exposure);
    result /= Uncharted2_tonemap(W);
    return pow(result, 1.0f / gamma);
}

struct DrawData
{
    Texture hdrTexture;
    Sampler hdrSampler;
    Texture bloomTexture;
    Sampler bloomSampler;
};

PUSH_CONSTANTS(DrawData, drawData);

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
static const float3x3 ACESInputMat =
{
    {0.59719, 0.35458, 0.04823},
    {0.07600, 0.90834, 0.01566},
    {0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367},
    {-0.10208,  1.10813, -0.00605},
    {-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 ACESFittedApproximated(float3 color)
{
    color = mul(ACESInputMat, color);
    color = RRTAndODTFit(color);
    color = mul(ACESOutputMat, color);

    return color;
}

float3 GammaCorrection(float3 input)
{
    return pow(input, 1.0f / 2.4f);
}

static float3x3 SRGBtoAP0_BlueCorrect = float3x3(
    0.41420905289197057f, 0.36705342236474414f, 0.16810061604328555f,
    0.066330365032269356f, 0.66695482548766416f, 0.076184550080066041f,
    0.066411226478631652f, 0.27396635261484203f, 0.90078958910652707f
);

static float3x3 SRGBtoAP1_BlueCorrect = float3x3(
    0.577179061216496430f, 0.32703801295684265f, 0.0451460171266615810f,
    0.046924630206983384f, 0.75303338971673572f, 0.0095117206762808798f,
    0.079803051920395413f, 0.28537532421905765f, 0.8759887920605471400f
);

static float3x3 SRGBtoAP1 = float3x3(
    0.613097406148404580f, 0.33952313882246637f, 0.047379455029129658f,
    0.070193736609379262f, 0.91635386688554310f, 0.013452396505077169f,
    0.020615600584029631f, 0.10956972117366949f, 0.869814678242301190f
);

static float3x3 AP1toSRGB = float3x3(
     1.705050997849740100f, -0.62179212130162886f, -0.083258876548112426f,
    -0.130256445729053340f,  1.14080475996513610f, -0.010548314236082178f,
    -0.024003375005876577f, -0.12896889906550266f,  1.152972274071379300f
);

static float3x3 AP1toXYZ = float3x3(
     0.66245420313861858f,   0.13400421044655456f,   0.15618769321229287f,
     0.27222873775640632f,   0.67408174554290545f,   0.053689516700688222f,
    -0.0055746609768594652f, 0.0040607417178481625f, 1.0103390853508929f
);

static float3x3 XYZtoAP1 = float3x3(
     1.6410233343579648f,   -0.32480330174139527f,   -0.23642470163833421f,
    -0.66366291334624439f,   1.6153316547472050f,     0.016756358233453519f,
     0.011721918507054247f, -0.0082844591992188316f,  0.98839487027473794f
);

static float3x3 blueCorrect = float3x3(
     0.9404372683, 0.0083786969,  0.0005471261,
    -0.0183068787, 0.8286599939, -0.0008833746,
     0.0778696104, 0.1629613092,  1.0003362486
);

// "Glow" module constants
static const float RRT_GLOW_GAIN = 0.05f;
static const float RRT_GLOW_MID = 0.08f;

// Red modifier constants
static const float RRT_RED_SCALE = 0.82f;
static const float RRT_RED_PIVOT = 0.03f;
static const float RRT_RED_HUE = 0.0f;
static const float RRT_RED_WIDTH = 135.0f;

// Desaturation contants
static const float RRT_SAT_FACTOR = 0.96;
static float3x3 RRT_SAT_MAT = float3x3(
    0.97088914951025618,   0.010889149510256263,  0.010889149510256263,
    0.026963269821716242,  0.98696326982171623,   0.026963269821716242,
    0.0021475806680275309, 0.0021475806680275309, 0.96214758066802752
);

static float3x3 ODT_SAT_MAT = float3x3(
    0.94905601164294850f,   0.019056011642948428f,  0.019056011642948428f,
    0.047185722188003348f,  0.97718572218800337f,   0.047185722188003348f,
    0.0037582661690481732f, 0.0037582661690481732f, 0.93375826616904822f
);

// Gamma compensation factor
static const float DIM_SURROUND_GAMMA = 0.9811f;

// Target white and black points for cinema system tonescale
static const float CINEMA_WHITE = 48.0;
static const float CINEMA_BLACK = pow(10.0f, log10(0.02f)); // CINEMA_WHITE / 2400.

float rgb_2_hue(float3 rgb)
{
    // Returns a geometric hue angle in degrees (0-360) based on RGB values.
    // For neutral colors, hue is undefined and the function will return a quiet NaN value.
    float hue;
    if (rgb.r == rgb.g && rgb.g == rgb.b)
    {
        hue = 0.0f / 0.0f; // RGB triplets where RGB are equal have an undefined hue
    }
    else
    {
        hue = (180.0f / PI) * atan2(sqrt(3) * (rgb.g - rgb.b), 2.0f * rgb.r - rgb.g - rgb.b);
    }

    if (hue < 0.0f)
    {
        hue = hue + 360.0f;
    }

    return hue;
}

float rgb_2_saturation(float3 rgb)
{
    return (max(max_float3(rgb), 1e-10f) - max(min_float3(rgb), 1e-10f)) / max(max_float3(rgb), 1e-2f);
}

float rgb_2_yc(float3 rgb, float ycRadiusWeight = 1.75f)
{
    // Converts RGB to a luminance proxy, here called YC
    // YC is ~ Y + K * Chroma
    // Constant YC is a cone-shaped surface in RGB space, with the tip on the
    // neutral axis, towards white.
    // YC is normalized: RGB 1 1 1 maps to YC = 1
    //
    // ycRadiusWeight defaults to 1.75, although can be overridden in function
    // call to rgb_2_yc
    // ycRadiusWeight = 1 -> YC for pure cyan, magenta, yellow == YC for neutral
    // of same value
    // ycRadiusWeight = 2 -> YC for pure red, green, blue  == YC for  neutral of
    // same value.

    float r = rgb.r;
    float g = rgb.g;
    float b = rgb.b;

    float chroma = sqrt(b * (b - g) + g * (g - r) + r * (r - b));

    return (b + g + r + ycRadiusWeight * chroma) / 3.0f;
}

// ------- Glow module functions

float glow_fwd(float ycIn, float glowGainIn, float glowMid)
{
    if (ycIn <= 2.0f / 3.0f * glowMid)
    {
        return glowGainIn;
    }
    else if (ycIn >= 2.0f * glowMid)
    {
        return 0.0f;
    }
    else
    {
        return glowGainIn * (glowMid / ycIn - 1.0f / 2.0f);
    }
}

float sigmoid_shaper(float x)
{
    // Sigmoid function in the range 0 to 1 spanning -2 to +2.

    float t = max(1.f - abs(x / 2.0f), 0.0f);
    float y = 1.0f + sign(x) * (1.0f - t * t);

    return y / 2.0f;
}

// ------- Red modifier functions

// w - full base width of the shaper function (in degrees)
float cubic_basis_shaper(float x, float w)
{
    float4x4 M = { -1.0f / 6.0f,  3.0f / 6.0f, -3.0f / 6.0f,  1.0f / 6.0f,
                    3.0f / 6.0f, -6.0f / 6.0f,  3.0f / 6.0f,  0.0f / 6.0f,
                   -3.0f / 6.0f,  0.0f / 6.0f,  3.0f / 6.0f,  0.0f / 6.0f,
                    1.0f / 6.0f,  4.0f / 6.0f,  1.0f / 6.0f,  0.0f / 6.0f };

    float knots[5] = { -w / 2.0f, -w / 4.0f, 0.0f, w / 4.0f, w / 2.0f };

    float y = 0;
    if ((x > knots[0]) && (x < knots[4]))
    {
        float knot_coord = (x - knots[0]) * 4./w;
        int j = knot_coord;
        float t = knot_coord - j;

        float monomials[4] = { t * t * t, t * t, t, 1.0f };

        if (j == 3)
        {
            y = monomials[0] * M[0][0] + monomials[1] * M[1][0] + monomials[2] * M[2][0] + monomials[3] * M[3][0];
        }
        else if (j == 2)
        {
            y = monomials[0] * M[0][1] + monomials[1] * M[1][1] + monomials[2] * M[2][1] + monomials[3] * M[3][1];
        }
        else if (j == 1)
        {
            y = monomials[0] * M[0][2] + monomials[1] * M[1][2] + monomials[2] * M[2][2] + monomials[3] * M[3][2];
        }
        else if (j == 0)
        {
            y = monomials[0] * M[0][3] + monomials[1] * M[1][3] + monomials[2] * M[2][3] + monomials[3] * M[3][3];
        }
        else
        {
            y = 0.0;
        }
    }

    return y * 3.0f / 2.0f;
}

float center_hue(float hue, float centerH)
{
    float hueCentered = hue - centerH;
    if (hueCentered < -180.0f)
    {
        return hueCentered + 360.0f;
    }
    else if (hueCentered > 180.0f)
    {
        return hueCentered - 360.0f;
    }

    return hueCentered;
}

struct SplineMapPoint
{
    float x;
    float y;
};

struct SegmentedSplineParams_c5
{
    float coefsLow[6];    // coefs for B-spline between minPoint and midPoint (units of log luminance)
    float coefsHigh[6];   // coefs for B-spline between midPoint and maxPoint (units of log luminance)
    SplineMapPoint minPoint; // {luminance, luminance} linear extension below this
    SplineMapPoint midPoint; // {luminance, luminance}
    SplineMapPoint maxPoint; // {luminance, luminance} linear extension above this
    float slopeLow;       // log-log slope of low linear extension
    float slopeHigh;      // log-log slope of high linear extension
};

static const SegmentedSplineParams_c5 RRT_PARAMS =
{
    { -4.0000000000f, -4.0000000000f, -3.1573765773f, -0.4852499958f, 1.8477324706f, 1.8477324706f }, // coefsLow[6]
    { -0.7185482425f,  2.0810307172f,  3.6681241237f,  4.0000000000f, 4.0000000000f, 4.0000000000f },    // coefsHigh[6]
    { 0.18f * pow(2.0f, -15), 0.0001f }, // minPoint
    { 0.18f, 4.8f },                     // midPoint
    { 0.18f * pow(2.0f, 18), 10000.0f }, // maxPoint
    0.0f, // slopeLow
    0.0f  // slopeHigh
};

// Textbook monomial to basis-function conversion matrix.
static const float3x3 M = float3x3(
     0.5, -1.0, 0.5,
    -1.0,  1.0, 0.5,
     0.5,  0.0, 0.0
);

float segmented_spline_c5_fwd(float x, SegmentedSplineParams_c5 C = RRT_PARAMS)
{
    const int N_KNOTS_LOW = 4;
    const int N_KNOTS_HIGH = 4;

    // Check for negatives or zero before taking the log. If negative or zero,
    // set to HALF_MIN.
    float logx = log10(max(x, 0x1.0p-126f));

    float logy;

    if (logx <= log10(C.minPoint.x))
    {
        logy = logx * C.slopeLow + (log10(C.minPoint.y) - C.slopeLow * log10(C.minPoint.x));
    }
    else if ((logx > log10(C.minPoint.x)) && (logx < log10(C.midPoint.x)))
    {
        float knot_coord = (N_KNOTS_LOW - 1) * (logx - log10(C.minPoint.x)) / (log10(C.midPoint.x) - log10(C.minPoint.x));
        int j = knot_coord;
        float t = knot_coord - j;

        float3 cf = { C.coefsLow[j], C.coefsLow[j + 1], C.coefsLow[j + 2]};

        float3 monomials = { t * t, t, 1.0f };
        logy = dot(monomials, mul(cf, M));
    }
    else if ((logx >= log10(C.midPoint.x)) && (logx < log10(C.maxPoint.x)))
    {
        float knot_coord = (N_KNOTS_HIGH-1) * (logx-log10(C.midPoint.x))/(log10(C.maxPoint.x)-log10(C.midPoint.x));
        int j = knot_coord;
        float t = knot_coord - j;

        float3 cf = { C.coefsHigh[j], C.coefsHigh[j + 1], C.coefsHigh[j + 2]};

        float3 monomials = { t * t, t, 1.0f };
        logy = dot(monomials, mul(cf, M));
    }
    else //if (logIn >= log10(C.maxPoint.x)) {
    {
        logy = logx * C.slopeHigh + (log10(C.maxPoint.y) - C.slopeHigh * log10(C.maxPoint.x));
    }

    return pow(10.0f, logy);
}

struct SegmentedSplineParams_c9
{
    float coefsLow[10];    // coefs for B-spline between minPoint and midPoint (units of log luminance)
    float coefsHigh[10];   // coefs for B-spline between midPoint and maxPoint (units of log luminance)
    SplineMapPoint minPoint; // {luminance, luminance} linear extension below this
    SplineMapPoint midPoint; // {luminance, luminance}
    SplineMapPoint maxPoint; // {luminance, luminance} linear extension above this
    float slopeLow;       // log-log slope of low linear extension
    float slopeHigh;      // log-log slope of high linear extension
};

static const SegmentedSplineParams_c9 ODT_48nits =
{
    { -1.6989700043f, -1.6989700043f, -1.4779f, -1.2291f, -0.8648f, -0.448f, 0.00518f, 0.4511080334f, 0.9113744414f, 0.9113744414f }, // coefsLow[10]
    { 0.5154386965f, 0.8470437783f, 1.1358f, 1.3802f, 1.5197f, 1.5985f, 1.6467f, 1.6746091357f, 1.687873339f, 1.687873339f },         // coefsHigh[10]
    { segmented_spline_c5_fwd(0.18f * pow(2.0f, -6.5f)), 0.02f }, // minPoint
    { segmented_spline_c5_fwd(0.18f), 4.8f },                     // midPoint
    { segmented_spline_c5_fwd(0.18f * pow(2.0f,  6.5f)), 48.0f }, // maxPoint
    0.0f, // slopeLow
    0.04f // slopeHigh
};

float segmented_spline_c9_fwd(float x, SegmentedSplineParams_c9 C = ODT_48nits)
{
    const int N_KNOTS_LOW = 8;
    const int N_KNOTS_HIGH = 8;

    // Check for negatives or zero before taking the log. If negative or zero,
    // set to HALF_MIN.
    float logx = log10(max(x, 0x1.0p-126f));

    float logy;

    if (logx <= log10(C.minPoint.x))
    {
        logy = logx * C.slopeLow + (log10(C.minPoint.y) - C.slopeLow * log10(C.minPoint.x));
    }
    else if ((logx > log10(C.minPoint.x)) && (logx < log10(C.midPoint.x)))
    {
        float knot_coord = (N_KNOTS_LOW - 1) * (logx - log10(C.minPoint.x)) / (log10(C.midPoint.x) - log10(C.minPoint.x));
        int j = knot_coord;
        float t = knot_coord - j;

        float3 cf = { C.coefsLow[j], C.coefsLow[j + 1], C.coefsLow[j + 2] };

        float3 monomials = { t * t, t, 1.0f };
        logy = dot(monomials, mul(cf, M));
    }
    else if ((logx >= log10(C.midPoint.x)) && (logx < log10(C.maxPoint.x)))
    {
        float knot_coord = (N_KNOTS_HIGH - 1) * (logx - log10(C.midPoint.x)) / (log10(C.maxPoint.x) - log10(C.midPoint.x));
        int j = knot_coord;
        float t = knot_coord - j;

        float3 cf = { C.coefsHigh[j], C.coefsHigh[j + 1], C.coefsHigh[j + 2]};

        float3 monomials = { t * t, t, 1.0f };
        logy = dot(monomials, mul(cf, M));
    }
    else //if (logIn >= log10(C.maxPoint.x))
    {
        logy = logx * C.slopeHigh + (log10(C.maxPoint.y) - C.slopeHigh * log10(C.maxPoint.x));
    }

    return pow(10.0f, logy);
}

float Y_2_linCV(float Y, float Ymax, float Ymin)
{
    return (Y - Ymin) / (Ymax - Ymin);
}

// Transformations between CIE XYZ tristimulus values and CIE x,y
// chromaticity coordinates
float3 XYZ_2_xyY(float3 XYZ)
{
    float divisor = (XYZ.r + XYZ.g + XYZ.b);
    if (divisor == 0.0f)
    {
        divisor = 1e-10;
    }

    return float3(XYZ.rg / divisor, XYZ.g);
}

float3 xyY_2_XYZ(float3 xyY)
{
    float3 XYZ;
    XYZ.r = xyY.r * xyY.b / max(xyY.g, 1e-10f);
    XYZ.g = xyY.b;
    XYZ.b = (1.0f - xyY.r - xyY.g) * xyY.b / max(xyY.g, 1e-10f);

    return XYZ;
}

float3 darkSurround_to_dimSurround(float3 linearCV)
{
  float3 XYZ = mul(AP1toXYZ, linearCV);

  float3 xyY = XYZ_2_xyY(XYZ);
  xyY.b = clamp(xyY.b, 0.0f, POS_INFINITY);
  xyY.b = pow(xyY.b, DIM_SURROUND_GAMMA);
  XYZ = xyY_2_XYZ(xyY);

  return mul(XYZtoAP1, XYZ);
}

float moncurve_r(float y, float gamma, float offs)
{
    // Reverse monitor curve
    float x;
    const float yb = pow(offs * gamma / ((gamma - 1.0f) * (1.0f + offs)), gamma);
    const float rs = pow((gamma - 1.0f) / offs, gamma - 1.0f) * pow((1.0f + offs) / gamma, gamma);
    if (y >= yb)
    {
        return (1.0f + offs) * pow(y, 1.0f / gamma) - offs;
    }
    else
    {
        return y * rs;
    }
}

PixOut MainPS(VertToPix IN)
{
    PixOut OUT = (PixOut)0;

    IN.TC = FromDirectXCoordSystem(IN.TC);

    float3 bloomValue = drawData.bloomTexture.Sample2D<float4>(drawData.bloomSampler.Get(), IN.TC).rgb;
    float3 hdrValue = drawData.hdrTexture.Sample2D<float4>(drawData.hdrSampler.Get(), IN.TC).rgb;

    hdrValue += 0.1f * bloomValue;

#define TONEMAP 1

#if TONEMAP == 0
    float3 sdrValue = ACESFittedApproximated(hdrValue);
    sdrValue = GammaCorrection(sdrValue);
#elif TONEMAP == 1
    float3 ap1 = mul(SRGBtoAP1, hdrValue);

    // --- Glow module --- //
    float saturation = rgb_2_saturation(ap1);
    float ycIn = rgb_2_yc(ap1);
    float s = sigmoid_shaper((saturation - 0.4f) / 0.2f);
    float addedGlow = 1.0f + glow_fwd(ycIn, RRT_GLOW_GAIN * s, RRT_GLOW_MID);

    ap1 *= addedGlow;

    // --- Red modifier --- //
    float hue = rgb_2_hue(ap1);
    float centeredHue = center_hue(hue, RRT_RED_HUE);
    float hueWeight = cubic_basis_shaper(centeredHue, RRT_RED_WIDTH);

    ap1.r = ap1.r + hueWeight * saturation * (RRT_RED_PIVOT - ap1.r) * (1.0f - RRT_RED_SCALE);

    // --- ACES to RGB rendering space --- //
    ap1 = clamp(ap1, 0.0f, POS_INFINITY);  // avoids saturated negative colors from becoming positive in the matrix

    float3 rgbPre = ap1;

    // --- Global desaturation --- //
    rgbPre = mul(RRT_SAT_MAT, rgbPre);

    // --- Apply the tonescale independently in rendering-space RGB --- //
    float3 rgbPost = rgbPre;
    //rgbPost = RRTAndODTFit(rgbPre);
    rgbPost.r = segmented_spline_c5_fwd(rgbPre.r);
    rgbPost.g = segmented_spline_c5_fwd(rgbPre.g);
    rgbPost.b = segmented_spline_c5_fwd(rgbPre.b);

    // OCES to RGB rendering space
    rgbPre = rgbPost;

    // Apply the tonescale independently in rendering-space RGB
    rgbPost = rgbPre;
    rgbPost.r = segmented_spline_c9_fwd(rgbPre.r);
    rgbPost.g = segmented_spline_c9_fwd(rgbPre.g);
    rgbPost.b = segmented_spline_c9_fwd(rgbPre.b);

    // Scale luminance to linear code value
    float3 linearCV;
    linearCV.r = Y_2_linCV(rgbPost.r, CINEMA_WHITE, CINEMA_BLACK);
    linearCV.g = Y_2_linCV(rgbPost.g, CINEMA_WHITE, CINEMA_BLACK);
    linearCV.b = Y_2_linCV(rgbPost.b, CINEMA_WHITE, CINEMA_BLACK);

    // Apply gamma adjustment to compensate for dim surround
    linearCV = darkSurround_to_dimSurround(linearCV);

    // Apply desaturation to compensate for luminance difference
    linearCV = mul(ODT_SAT_MAT, linearCV);

    linearCV = mul(AP1toSRGB, linearCV);
    linearCV = saturate(linearCV);

    // Encode linear code values with transfer function
    float3 outputCV = linearCV;
    // moncurve_r with gamma of 2.4 and offset of 0.055 matches the EOTF found in IEC 61966-2-1:1999 (sRGB)
    outputCV.r = moncurve_r(linearCV.r, 2.4f, 0.055f);
    outputCV.g = moncurve_r(linearCV.g, 2.4f, 0.055f);
    outputCV.b = moncurve_r(linearCV.b, 2.4f, 0.055f);

    float3 sdrValue = outputCV;
#elif TONEMAP == 2
    float3 ap1 = mul(blueCorrect, mul(SRGBtoAP1, hdrValue));
    ap1 = RRTAndODTFit(ap1);
    float3 sdrValue = mul(AP1toSRGB, ap1);
#endif

    sdrValue = saturate(sdrValue);

    OUT.Color = float4(sdrValue, 1.0f);

    return OUT;
}