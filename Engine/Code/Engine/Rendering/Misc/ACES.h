#pragma once

#if 1

#include <glm/glm.hpp>

// ----------------- CONSTANTS ------------------

static const glm::dmat3 CONE_RESP_MAT_BRADFORD = glm::dmat3(
     0.89510, -0.75020,  0.03890,
     0.26640,  1.71350, -0.06850,
    -0.16140,  0.03670,  1.02960
);

// LMT for desaturating blue hues in order to reduce the artifact where bright
// blue colors (e.g. sirens, headlights, LED lighting, etc.) may become
// oversaturated or exhibit hue shifts as a result of clipping.
//
static const glm::dmat3 blueCorrectionMatrix = glm::dmat3(
     0.9404372683, 0.0083786969,  0.0005471261,
    -0.0183068787, 0.8286599939, -0.0008833746,
     0.0778696104, 0.1629613092,  1.0003362486
);

// ----------------------------------------------



// -------------- HELPER FUNCTIONS --------------

glm::dvec3 xyY_2_XYZ(glm::dvec3 xyY)
{
    glm::dvec3 XYZ;
    XYZ[0] = xyY[0] * xyY[2] / std::max( xyY[1], 1e-10);
    XYZ[1] = xyY[2];  
    XYZ[2] = (1.0 - xyY[0] - xyY[1]) * xyY[2] / std::max( xyY[1], 1e-10);

    return XYZ;
}

// ----------------------------------------------

struct Chromaticities
{
    glm::dvec2 red;  // CIE xy coordinates of red primary
    glm::dvec2 green;  // CIE xy coordinates of green primary
    glm::dvec2 blue;  // CIE xy coordinates of blue primary
    glm::dvec2 white;  // CIE xy coordinates of white point
};

// returns column-major matrix
glm::dmat3 RGBtoXYZ(Chromaticities chroma, double Y)
{
    //
    // X and Z values of RGB value (1, 1, 1), or "white"
    //

    double X = chroma.white.x * Y / chroma.white.y;
    double Z = (1.0f - chroma.white.x - chroma.white.y) * Y / chroma.white.y;

    //
    // Scale factors for matrix rows
    //

    double d = chroma.red.x * (chroma.blue.y - chroma.green.y) +
        chroma.blue.x * (chroma.green.y - chroma.red.y) +
        chroma.green.x * (chroma.red.y - chroma.blue.y);

    double Sr = (X * (chroma.blue.y - chroma.green.y) -
        chroma.green.x * (Y * (chroma.blue.y - 1.0f) +
            chroma.blue.y * (X + Z)) +
        chroma.blue.x * (Y * (chroma.green.y - 1.0f) +
            chroma.green.y * (X + Z))) / d;

    double Sg = (X * (chroma.red.y - chroma.blue.y) +
        chroma.red.x * (Y * (chroma.blue.y - 1.0f) +
            chroma.blue.y * (X + Z)) -
        chroma.blue.x * (Y * (chroma.red.y - 1.0f) +
            chroma.red.y * (X + Z))) / d;

    double Sb = (X * (chroma.green.y - chroma.red.y) -
        chroma.red.x * (Y * (chroma.green.y - 1.0f) +
            chroma.green.y * (X + Z)) +
        chroma.green.x * (Y * (chroma.red.y - 1.0f) +
            chroma.red.y * (X + Z))) / d;

    //
    // Assemble the matrix
    //

    glm::dmat3 M;

    M[0][0] = Sr * chroma.red.x;
    M[1][0] = Sr * chroma.red.y;
    M[2][0] = Sr * (1 - chroma.red.x - chroma.red.y);

    M[0][1] = Sg * chroma.green.x;
    M[1][1] = Sg * chroma.green.y;
    M[2][1] = Sg * (1 - chroma.green.x - chroma.green.y);

    M[0][2] = Sb * chroma.blue.x;
    M[1][2] = Sb * chroma.blue.y;
    M[2][2] = Sb * (1 - chroma.blue.x - chroma.blue.y);

    return M;
}


// src_xy - x,y chromaticity of source white
// des_xy - x,y chromaticity of destination white
glm::dmat3 calculate_cat_matrix(glm::dvec2 src_xy, glm::dvec2 des_xy, glm::dmat3 coneRespMat = CONE_RESP_MAT_BRADFORD)
{
    //
    // Calculates and returns a 3x3 Von Kries chromatic adaptation transform 
    // from src_xy to des_xy using the cone response primaries defined 
    // by coneRespMat. By default, coneRespMat is set to CONE_RESP_MAT_BRADFORD. 
    // The default coneRespMat can be overridden at runtime. 
    //

    const glm::dvec3 src_xyY = { src_xy[0], src_xy[1], 1.0 };
    const glm::dvec3 des_xyY = { des_xy[0], des_xy[1], 1.0 };

    glm::dvec3 src_XYZ = xyY_2_XYZ(src_xyY);
    glm::dvec3 des_XYZ = xyY_2_XYZ(des_xyY);

    glm::dvec3 src_coneResp = coneRespMat * src_XYZ;
    glm::dvec3 des_coneResp = coneRespMat * des_XYZ;

    glm::dmat3 vkMat = glm::dmat3(
        des_coneResp[0] / src_coneResp[0], 0.0, 0.0,
        0.0, des_coneResp[1] / src_coneResp[1], 0.0,
        0.0, 0.0, des_coneResp[2] / src_coneResp[2]
    );

    glm::dmat3 cat_matrix = glm::inverse(coneRespMat) * (vkMat * coneRespMat);

    return glm::transpose(cat_matrix);
}

const Chromaticities AP0Chromaticities = // ACES Primaries from SMPTE ST2065-1
{
    { 0.73470f,  0.26530f },
    { 0.00000f,  1.00000f },
    { 0.00010f, -0.07700f },
    { 0.32168f,  0.33767f }
};

const Chromaticities AP1Chromaticities = // Working space and rendering primaries for ACES 1.0
{
    { 0.71300f,  0.29300f },
    { 0.16500f,  0.83000f },
    { 0.12800f,  0.04400f },
    { 0.32168f,  0.33767f }
};

static const Chromaticities REC709Chromaticities =
{
    { 0.6400f, 0.3300f },
    { 0.3000f, 0.6000f },
    { 0.1500f, 0.0600f },
    { 0.3127f, 0.3290f }
};

// this matrix is stored in column-major order, transpose before using with glm
static const glm::dmat3 SRGBtoXYZ = RGBtoXYZ(REC709Chromaticities, 1.0);
/*
    0.412390799265959560, 0.35758433938387801, 0.180480788401834210
    0.212639005871510410, 0.71516867876775603, 0.072192315360733686
    0.019330818715591835, 0.11919477979462598, 0.950532152249660260
*/

// this matrix is stored in column-major order, transpose before using with glm
static const glm::dmat3 XYZtoSRGB = glm::inverse(SRGBtoXYZ);
/*
     3.240969941904520400, -1.53738317757009280, -0.498610760293003110
    -0.969243636280879730,  1.87596750150772040,  0.041555057407175591
     0.055630079696993670, -0.20397695888897660,  1.056971514242878800
*/

// this matrix is stored in column-major order, transpose before using with glm
static const glm::dmat3 AP0toXYZ = RGBtoXYZ(AP0Chromaticities, 1.0);
/*
     0.95255242816624375,    0.00000000000000000,  9.3678631222451214e-05
     0.34396645746348570,    0.72816609000916566, -0.072132547472651146
    -3.8639272659732602e-08, 0.00000000000000000,  1.008825204731154100
*/

// this matrix is stored in column-major order, transpose before using with glm
static const glm::dmat3 XYZtoAP0 = glm::inverse(AP0toXYZ);
/*
     1.04981098197537630,    0.0000000000000000, -9.7484534855529282e-05
    -0.49590301791093411,    1.3733130582713799,  0.098240036452023866
     4.0209079415832239e-08, 0.0000000000000000,  0.991251998172198220
*/

// this matrix is stored in column-major order, transpose before using with glm
static const glm::dmat3 AP1toXYZ = RGBtoXYZ(AP1Chromaticities, 1.0);
/*
     0.66245420313861858,   0.13400421044655456,   0.15618769321229287
     0.27222873775640632,   0.67408174554290545,   0.053689516700688222
    -0.0055746609768594652, 0.0040607417178481625, 1.0103390853508929
*/

// this matrix is stored in column-major order, transpose before using with glm
static const glm::dmat3 XYZtoAP1 = glm::inverse(AP1toXYZ);
/*
     1.6410233343579648,   -0.32480330174139527,   -0.23642470163833421
    -0.66366291334624439,   1.6153316547472050,     0.016756358233453519
     0.011721918507054247, -0.0082844591992188316, 0.98839487027473794
*/

// this matrix is stored in column-major order, transpose before using with glm
static const glm::dmat3 D60toD65 = calculate_cat_matrix(AP0Chromaticities.white, REC709Chromaticities.white);
/*
     0.98722399655431903,   -0.0061132352638656861, 0.015953296282401858
    -0.0075983811619341930,  1.0018614910122581,    0.0053300387424436643
     0.0030725781766883925, -0.0050959630984631910, 1.0816806368735590
*/

// this matrix is stored in column-major order, transpose before using with glm
static const glm::dmat3 D65toD60 = calculate_cat_matrix(REC709Chromaticities.white, AP0Chromaticities.white);
/*
     1.0130349272651751,    0.0061052645328172842, -0.014970950836636326
     0.0076982396408062630, 0.99816334595616962,   -0.0050320412675673859
    -0.0028413183705392059, 0.0046851579864760906,  0.92450610857806415
*/

// this matrix is stored in column-major order, transpose before using with glm
static const glm::dmat3 SRGBtoAP1_BlueCorrect = glm::transpose(glm::transpose(blueCorrectionMatrix) * glm::transpose(XYZtoAP1) * glm::transpose(D65toD60) * glm::transpose(SRGBtoXYZ));
/*
    0.57717906121649643,  0.32703801295684265, 0.045146017126661581
    0.046924630206983384, 0.75303338971673572, 0.0095117206762808798
    0.079803051920395413, 0.28537532421905765, 0.87598879206054714
*/

// this matrix is stored in column-major order, transpose before using with glm
static const glm::dmat3 SRGBtoAP0_BlueCorrect = glm::transpose(glm::transpose(blueCorrectionMatrix) * glm::transpose(XYZtoAP0) * glm::transpose(D65toD60) * glm::transpose(SRGBtoXYZ));
/*
    0.41420905289197057, 0.36705342236474414, 0.16810061604328555
    0.066330365032269356, 0.66695482548766416, 0.076184550080066041
    0.066411226478631652, 0.27396635261484203, 0.90078958910652707
*/

// this matrix is stored in column-major order, transpose before using with glm
static const glm::dmat3 AP1toSRGB = glm::transpose(glm::transpose(XYZtoSRGB) * glm::transpose(D60toD65) * glm::transpose(AP1toXYZ));
/*
     1.7050509978497401,   -0.62179212130162886, -0.083258876548112426
    -0.13025644572905334,   1.1408047599651361,  -0.010548314236082178
    -0.024003375005876577, -0.12896889906550266,  1.1529722740713793
*/

// -------------------- RRT ---------------------

glm::dmat3 calc_sat_adjust_matrix(double sat, glm::dvec3 rgb2Y)
{
    //
    // This function determines the terms for a 3x3 saturation matrix that is
    // based on the luminance of the input.
    //
    glm::dmat3 M;
    M[0][0] = (1.0 - sat) * rgb2Y[0] + sat;
    M[1][0] = (1.0 - sat) * rgb2Y[0];
    M[2][0] = (1.0 - sat) * rgb2Y[0];

    M[0][1] = (1.0 - sat) * rgb2Y[1];
    M[1][1] = (1.0 - sat) * rgb2Y[1] + sat;
    M[2][1] = (1.0 - sat) * rgb2Y[1];

    M[0][2] = (1.0 - sat) * rgb2Y[2];
    M[1][2] = (1.0 - sat) * rgb2Y[2];
    M[2][2] = (1.0 - sat) * rgb2Y[2] + sat;

    M = glm::transpose(M);
    return M;
}

static const double RRT_SAT_FACTOR = 0.96;
static const glm::dvec3 AP1_RGB2Y = { glm::transpose(AP1toXYZ)[0][1], glm::transpose(AP1toXYZ)[1][1], glm::transpose(AP1toXYZ)[2][1] };

static const glm::dmat3 RRT_SAT_MAT = calc_sat_adjust_matrix(RRT_SAT_FACTOR, AP1_RGB2Y);
/*
    0.97088914951025618,   0.010889149510256263,  0.010889149510256263
    0.026963269821716242,  0.98696326982171623,   0.026963269821716242
    0.0021475806680275309, 0.0021475806680275309, 0.96214758066802752
*/

// Saturation compensation factor
static const double ODT_SAT_FACTOR = 0.93;
static const glm::dmat3 ODT_SAT_MAT = calc_sat_adjust_matrix(ODT_SAT_FACTOR, AP1_RGB2Y);
/*
    0.94905601164294850,   0.019056011642948428,  0.019056011642948428
    0.047185722188003348,  0.97718572218800337,   0.047185722188003348
    0.0037582661690481732, 0.0037582661690481732, 0.93375826616904822
*/

// ----------------------------------------------

#endif