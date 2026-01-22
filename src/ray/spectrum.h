#pragma once

// Spectrum classes for physically-based spectral rendering
// Based on PBRT-v3 and pbr-book 4th edition

#include "vec.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>

// ============================================================================
// Spectrum Type - distinguishes reflectance from illuminant spectra
// ============================================================================

enum class SpectrumType { Reflectance, Illuminant };

// ============================================================================
// Spectral Data (CIE XYZ and RGB->Spectrum conversion)
// ============================================================================

namespace spectrum_data {

// CIE XYZ matching functions (380-780nm, 5nm intervals)
inline constexpr int nCIESamples = 81;

inline constexpr double CIE_lambda[nCIESamples] = {
    380, 385, 390, 395, 400, 405, 410, 415, 420, 425, 430, 435, 440, 445, 450, 455, 460,
    465, 470, 475, 480, 485, 490, 495, 500, 505, 510, 515, 520, 525, 530, 535, 540, 545,
    550, 555, 560, 565, 570, 575, 580, 585, 590, 595, 600, 605, 610, 615, 620, 625, 630,
    635, 640, 645, 650, 655, 660, 665, 670, 675, 680, 685, 690, 695, 700, 705, 710, 715,
    720, 725, 730, 735, 740, 745, 750, 755, 760, 765, 770, 775, 780};

inline constexpr double CIE_X[nCIESamples] = {
    0.0001299, 0.0002321, 0.0004149, 0.0007416, 0.001368, 0.002236, 0.004243, 0.00765,  0.01431,
    0.02319,   0.04351,   0.07763,   0.13438,   0.21477,  0.2839,   0.3285,   0.34828,  0.34806,
    0.3362,    0.3187,    0.2908,    0.2511,    0.19536,  0.1421,   0.09564,  0.05795,  0.03201,
    0.0147,    0.0049,    0.0024,    0.0093,    0.0291,   0.06327,  0.1096,   0.1655,   0.2257,
    0.2904,    0.3597,    0.4334,    0.5121,    0.5945,   0.6784,   0.7621,   0.8425,   0.9163,
    0.9786,    1.0263,    1.0567,    1.0622,    1.0456,   1.0026,   0.9384,   0.8544,   0.7514,
    0.6424,    0.5419,    0.4479,    0.3608,    0.2835,   0.2187,   0.1649,   0.1212,   0.0874,
    0.0636,    0.04677,   0.0329,    0.0227,    0.01584,  0.01136,  0.008111, 0.00579,  0.004109,
    0.002899,  0.002049,  0.00144,   0.001,     0.00069,  0.000476, 0.000332, 0.000235, 0.000166};

inline constexpr double CIE_Y[nCIESamples] = {
    0.000003917, 0.000006965, 0.00001239, 0.00002202, 0.000039,  0.000064,  0.00012,  0.000217,
    0.000396,    0.00064,     0.00121,    0.00218,    0.004,     0.0073,    0.0116,   0.01684,
    0.023,       0.0298,      0.038,      0.048,      0.06,      0.0739,    0.09098,  0.1126,
    0.13902,     0.1693,      0.208,      0.2586,     0.323,     0.4073,    0.503,    0.6082,
    0.71,        0.7932,      0.862,      0.9149,     0.954,     0.9803,    0.995,    1.0,
    0.995,       0.9786,      0.952,      0.9154,     0.87,      0.8163,    0.757,    0.6949,
    0.631,       0.5668,      0.503,      0.4412,     0.381,     0.321,     0.265,    0.217,
    0.175,       0.1382,      0.107,      0.0816,     0.061,     0.04458,   0.032,    0.0232,
    0.017,       0.01192,     0.00821,    0.005723,   0.004102,  0.002929,  0.002091, 0.001484,
    0.001047,    0.00074,     0.00052,    0.0003611,  0.0002492, 0.0001719, 0.00012,  0.0000848,
    0.00006};

inline constexpr double CIE_Z[nCIESamples] = {
    0.0006061,  0.001086,   0.001946,   0.003486,   0.006450001, 0.01055,    0.02077,    0.03808,
    0.07149,    0.1172,     0.2237,     0.4073,     0.734,       1.175,      1.612,      1.905,
    2.064,      2.077,      2.037,      1.944,      1.795,       1.595,      1.317,      1.03,
    0.7721,     0.5701,     0.4153,     0.3024,     0.2185,      0.1592,     0.1117,     0.07825,
    0.05725,    0.04216,    0.02984,    0.0203,     0.0134,      0.008749,   0.005749,   0.0039,
    0.002749,   0.0021,     0.0018,     0.001714,   0.001304,    0.000977,   0.000741,   0.000563,
    0.000447,   0.0003689,  0.000296,   0.000231,   0.000178,    0.000139,   0.000109,   0.0000848,
    0.0000656,  0.0000506,  0.0000393,  0.000030,   0.0000231,   0.0000178,  0.0000137,  0.0000105,
    0.0000081,  0.0000062,  0.00000478, 0.0000037,  0.0000029,   0.00000227, 0.00000178, 0.00000139,
    0.00000110, 0.00000087, 0.00000069, 0.00000055, 0.00000044,  0.00000035, 0.00000028, 0.00000022,
    0.00000018};

inline constexpr double CIE_Y_integral = 106.856895;

// RGB to Spectrum conversion (Smits 1999)
inline constexpr int nRGB2SpectSamples = 32;

inline constexpr double RGB2SpectLambda[nRGB2SpectSamples] = {
    380.0,      390.967743, 401.935486, 412.903229, 423.870972, 434.838715, 445.806458, 456.774200,
    467.741943, 478.709686, 489.677429, 500.645172, 511.612915, 522.580627, 533.548340, 544.516052,
    555.483765, 566.451477, 577.419189, 588.386902, 599.354614, 610.322327, 621.290039, 632.257751,
    643.225464, 654.193176, 665.160889, 676.128601, 687.096313, 698.064026, 709.031738, 720.0};

// Reflectance spectra
inline constexpr double RGBRefl2SpectWhite[nRGB2SpectSamples] = {
    1.0618958571272863, 1.0615019980348779, 1.0614335379927147, 1.0622711654692485,
    1.0622036218416742, 1.0625059965187085, 1.0623938486985884, 1.0624706448043137,
    1.0625048144827762, 1.0624366131308856, 1.0620694238892607, 1.0613167586932164,
    1.0610334029377020, 1.0613868564828413, 1.0614215366116762, 1.0620336151299086,
    1.0625497454805051, 1.0624317487992085, 1.0625249140554480, 1.0624277664486914,
    1.0624749854090769, 1.0625538581025402, 1.0625326910104864, 1.0623922312225325,
    1.0623650980354129, 1.0625256476715284, 1.0612277619533155, 1.0594262608698046,
    1.0599810758292072, 1.0602547314449409, 1.0601263046243634, 1.0606565756823634};

inline constexpr double RGBRefl2SpectCyan[nRGB2SpectSamples] = {
    1.0414628021426751,  1.0328661533771188,  1.0126146228964314,  1.0350460524836209,
    1.0078661447098567,  1.0422280385081280,  1.0442596738499825,  1.0535238290294409,
    1.0180776226938120,  1.0442729908727713,  1.0529362541920750,  1.0537034271160244,
    1.0533901869215969,  1.0537782700979574,  1.0527093770467102,  1.0530449040446797,
    1.0550554640191208,  1.0553673610724821,  1.0454306634683976,  0.6234895063923081,
    0.1803807161318898,  -0.0076303759201985, -0.0001521784703578, -0.0075102257347258,
    -0.0021708639328491, 0.0006591946660237,  0.0122788153185398,  -0.0044669775637208,
    0.0171197990828651,  0.0049211089759760,  0.0058762925143335,  0.0252593994155501};

inline constexpr double RGBRefl2SpectMagenta[nRGB2SpectSamples] = {
    0.9942213815123685,  0.9898693712297568, 0.9829365828611696,
    0.9962786839985931,  1.0198955019000133, 1.0166395501210359,
    1.0220913178757398,  0.9965166604068244, 1.0097766178917882,
    1.0215422470827016,  0.6403195338779096, 0.0025012379477078,
    0.0065339939555770,  0.0028334080462676, 0.0,
    -0.0090592291646646, 0.0033936718323331, -0.0030638741121828,
    0.2220393616828629,  0.6314114002481197, 0.9748098557650096,
    0.9720956233359057,  1.0173770302868150, 0.9987519432273413,
    0.9470172573960224,  0.8525862315435480, 0.9489779858166084,
    0.9475187609652149,  0.9959894419105979, 0.8630135150380908,
    0.8915098785352315,  0.8486649265284508};

inline constexpr double RGBRefl2SpectYellow[nRGB2SpectSamples] = {
    0.0055740622924921,  -0.0047982831631447, -0.0052536564298614, -0.0064571480044500,
    -0.0059693514658007, -0.0021836716037687, 0.0167811206010553,  0.0960963554290626,
    0.2121735708198645,  0.3616913329068507,  0.5396101154323253,  0.7440881049217151,
    0.9220957114839405,  1.0460304298411225,  1.0513824989063714,  1.0511991822135085,
    1.0510530911991052,  1.0517397230360510,  1.0516043086790485,  1.0511944032061460,
    1.0511590325868068,  1.0516612465483031,  1.0514038526836869,  1.0515941029228475,
    1.0511460436960840,  1.0515123758830476,  1.0508871369510702,  1.0508923708102380,
    1.0477492815668303,  1.0493272144017338,  1.0435963333422726,  1.0392280772051465};

inline constexpr double RGBRefl2SpectRed[nRGB2SpectSamples] = {
    0.1657560486708618,  0.1184644280274780,  0.1240829332963745,  0.1137127205834992,
    0.0789924345188991,  0.0322056035931065,  -0.0107983654078779, 0.0180519755167304,
    0.0053407196598731,  0.0136549187295013,  -0.0059564213545643, -0.0018444365067353,
    -0.0105718843615295, -0.0029375521078000, -0.0107904762718359, -0.0080224306697504,
    -0.0022669167702496, 0.0070200240494707,  -0.0081528469000299, 0.6077286696925279,
    0.9883156086543240,  0.9939169104407882,  1.0039338994753197,  0.9923449986116713,
    0.9992653085885552,  1.0084621557617270,  0.9835829682744122,  1.0085023660099048,
    0.9745113832656870,  0.9854326957005994,  0.9349576398096204,  0.9871390779231940};

inline constexpr double RGBRefl2SpectGreen[nRGB2SpectSamples] = {
    0.0026494153587602,  -0.0050175013429732, -0.0125472362724896, -0.0094554964308389,
    -0.0125260861816005, -0.0079170697760438, -0.0079955735204176, -0.0093559433444469,
    0.0654686119829993,  0.3957287551763414,  0.7524402229988666,  0.9637647869021856,
    0.9985443385516233,  0.9999297702528792,  0.9993908675114045,  0.9999437226707140,
    0.9993912181341867,  0.9991123731042448,  0.9601958487827158,  0.6318627933843244,
    0.2579740102876347,  0.0094014888527336,  -0.0030798345608650, -0.0045230367033685,
    -0.0068933410388274, -0.0090352195539015, -0.0085913667165340, -0.0083690869120289,
    -0.0078685832338754, -0.0000083657578711, 0.0054301225442817,  -0.0027745589759259};

inline constexpr double RGBRefl2SpectBlue[nRGB2SpectSamples] = {
    0.9920977146972068,
    0.9887642605936913,
    0.9953904074450564,
    0.9952931735300822,
    0.9918144741163395,
    1.0002584039673432,
    0.9996847843734251,
    0.9998812076665717,
    0.9850401214637043,
    0.7902984905303128,
    0.5608219861746397,
    0.3313345851399653,
    0.1369241084083918,
    0.0189149065596642,
    0.0,
    0.0,
    0.0,
    0.0017473028136487,
    0.0037999160177631,
    0.0,
    0.0,
    0.0075874501748733,
    0.0257956507805540,
    0.0381683765325005,
    0.0494895864080308,
    0.0495959922901029,
    0.0498148195058122,
    0.0398409110649780,
    0.0305010249372339,
    0.0212430547652411,
    0.0069596532104356,
    0.0041733649330981};

// Illuminant spectra
inline constexpr double RGBIllum2SpectWhite[nRGB2SpectSamples] = {
    1.1565232050369776, 1.1567225000119139, 1.1566203150243823, 1.1555782088080084,
    1.1562175509215700, 1.1567674012207332, 1.1568023194808630, 1.1567677445485520,
    1.1563563182952830, 1.1567054702510189, 1.1565134139372772, 1.1564336176499312,
    1.1568023181530034, 1.1473147688514642, 1.1339317140561065, 1.1293876083653714,
    1.1290515328639648, 1.0504864823782283, 1.0459696042230884, 0.9225854480428110,
    0.8454276526114720, 0.7995211256879550, 0.7494146805654590, 0.6991509447115905,
    0.6497440609498020, 0.6505459617977653, 0.6681914889334765, 0.6373140859498823,
    0.5765116810412302, 0.6066428403905630, 0.6455196861498680, 0.6766517043447875};

inline constexpr double RGBIllum2SpectCyan[nRGB2SpectSamples] = {
    1.1334479663682135, 1.1266762330194116, 1.1346827504710164, 1.1357395805744794,
    1.1356371830149636, 1.1361152989346193, 1.1362179057706772, 1.1364819652587022,
    1.1355107110714324, 1.1364060812667711, 1.1360363621722465, 1.1360122641141395,
    1.1354569229820140, 1.0985755469236140, 0.9671327581311273, 0.8258195916437977,
    0.7468262102710455, 0.5765245226833480, 0.5765245226833480, 0.4333824504263195,
    0.3210176830614118, 0.2667669786525810, 0.2108522809648640, 0.1677973524667530,
    0.1510697467004360, 0.1525864457953965, 0.1731625339091650, 0.1757247996543320,
    0.1646177260294845, 0.1806497862882020, 0.1975190800096230, 0.2093645854900410};

inline constexpr double RGBIllum2SpectMagenta[nRGB2SpectSamples] = {
    1.0371892935878366,
    1.0587542155320550,
    1.0767271213905705,
    1.0762642409278290,
    1.0795954846836230,
    1.0802757021429915,
    1.0789652839441770,
    1.0791510202395145,
    1.0797397797288730,
    1.0795597705698960,
    0.7426238355637110,
    0.0636892277275805,
    0.0,
    -0.0399348785728636,
    0.0,
    0.0,
    0.1070136285562195,
    0.1253570690498660,
    0.3773927035984615,
    0.6297193426353315,
    0.9090349699624250,
    0.9591006926820490,
    1.0098351967972700,
    1.0025261046879095,
    0.9813039995447365,
    0.9552489653592085,
    0.9713662576111800,
    0.9715830984115540,
    0.9717753756328095,
    0.9777773839262370,
    0.9892072577437455,
    1.0001416202956060};

inline constexpr double RGBIllum2SpectYellow[nRGB2SpectSamples] = {
    0.0001,
    0.0,
    0.0,
    0.0,
    0.0,
    0.0,
    0.0,
    0.0,
    0.0817266587280000,
    0.2820513950240990,
    0.5261244700726620,
    0.7631815830008660,
    0.9142218250413660,
    1.0049311531593166,
    1.0072971193687170,
    1.0061585903558660,
    1.0064893748110310,
    1.0140256285697780,
    1.0133527423539130,
    0.9565256227992760,
    0.9272172065266420,
    0.9116913148588000,
    0.9038697814600820,
    0.8960406609549260,
    0.8883666661455885,
    0.8821843766757775,
    0.8893170393040590,
    0.8890839538077900,
    0.8737093312060155,
    0.8798098442037855,
    0.8870497953295100,
    0.8968347628440955};

inline constexpr double RGBIllum2SpectRed[nRGB2SpectSamples] = {
    0.0568913815185970, 0.0590618192844440, 0.0623629227688875, 0.0631133392616785,
    0.0621432839498565, 0.0615070696653530, 0.0601831019461820, 0.0607969019544730,
    0.0591165009258485, 0.0574627892295925, 0.0507970959813320, 0.0491312945998305,
    0.0463591843556535, 0.0449098608139405, 0.0447625498684640, 0.0452174774548735,
    0.0444055261837160, 0.0455312098916080, 0.0476556116426195, 0.5885015937858510,
    0.9977328279251040, 1.0037305096893420, 1.0040260694827070, 1.0039497929902340,
    1.0036540267085400, 1.0023915178257140, 0.9975730485679995, 0.9981200478662735,
    0.9843815070009235, 0.9893967302903780, 0.9931667369768635, 0.9968245096844755};

inline constexpr double RGBIllum2SpectGreen[nRGB2SpectSamples] = {
    0.0,
    0.0,
    0.0,
    0.0,
    0.0,
    0.0,
    0.0,
    0.0,
    0.1047808871295200,
    0.3772773385117530,
    0.7360867328578955,
    0.9619245156575470,
    0.9989571276379060,
    0.9999587755898299,
    0.9994167663934370,
    0.9999567127137495,
    0.9994187562433960,
    0.9990933499960885,
    0.9561313756301960,
    0.6204139879015715,
    0.2454065708642885,
    0.0039183917135615,
    0.0,
    0.0,
    0.0,
    0.0,
    0.0,
    0.0,
    0.0,
    0.0,
    0.0,
    0.0};

inline constexpr double RGBIllum2SpectBlue[nRGB2SpectSamples] = {
    1.0570490759753140,
    1.0538466912851301,
    1.0550494258140670,
    1.0530407754701598,
    1.0579930596460185,
    1.0578439494812455,
    1.0583132387180239,
    1.0579712943137320,
    1.0561884233578465,
    0.7867071393078710,
    0.5765445346057415,
    0.3546119779615915,
    0.1521265253664475,
    0.0250819024608695,
    0.0,
    0.0,
    0.0,
    0.0006754149834505,
    0.0029933106258745,
    0.0,
    0.0,
    0.0042212014793765,
    0.0180952568741500,
    0.0268098079952720,
    0.0348203082656960,
    0.0348870913498680,
    0.0351245710765800,
    0.0280054628346820,
    0.0214915133838965,
    0.0149523649113090,
    0.0048977893873885,
    0.0029341067577825};

}  // namespace spectrum_data

// ============================================================================
// Color Space Conversion
// ============================================================================

inline void XYZToRGB(const double xyz[3], double rgb[3]) {
    rgb[0] = 3.240479 * xyz[0] - 1.537150 * xyz[1] - 0.498535 * xyz[2];
    rgb[1] = -0.969256 * xyz[0] + 1.875991 * xyz[1] + 0.041556 * xyz[2];
    rgb[2] = 0.055648 * xyz[0] - 0.204043 * xyz[1] + 1.057311 * xyz[2];
}

inline void RGBToXYZ(const double rgb[3], double xyz[3]) {
    xyz[0] = 0.412453 * rgb[0] + 0.357580 * rgb[1] + 0.180423 * rgb[2];
    xyz[1] = 0.212671 * rgb[0] + 0.715160 * rgb[1] + 0.072169 * rgb[2];
    xyz[2] = 0.019334 * rgb[0] + 0.119193 * rgb[1] + 0.950227 * rgb[2];
}

// ============================================================================
// Utility Functions
// ============================================================================

inline double
interpolateSpectrumSamples(const double* lambda, const double* vals, int n, double l) {
    if (l <= lambda[0]) return vals[0];
    if (l >= lambda[n - 1]) return vals[n - 1];
    int i = 0;
    while (i < n - 1 && lambda[i + 1] < l) ++i;
    double t = (l - lambda[i]) / (lambda[i + 1] - lambda[i]);
    return (1 - t) * vals[i] + t * vals[i + 1];
}

inline double averageSpectrumSamples(
    const double* lambda, const double* vals, int n, double lambdaStart, double lambdaEnd) {
    if (lambdaEnd <= lambda[0]) return vals[0];
    if (lambdaStart >= lambda[n - 1]) return vals[n - 1];
    if (n == 1) return vals[0];

    double sum = 0;
    if (lambdaStart < lambda[0]) sum += vals[0] * (lambda[0] - lambdaStart);
    if (lambdaEnd > lambda[n - 1]) sum += vals[n - 1] * (lambdaEnd - lambda[n - 1]);

    int i = 0;
    while (i + 1 < n && lambdaStart > lambda[i + 1]) ++i;

    auto interp = [lambda, vals](double w, int i) {
        return (w - lambda[i]) / (lambda[i + 1] - lambda[i]) * (vals[i + 1] - vals[i]) + vals[i];
    };

    for (; i + 1 < n && lambdaEnd >= lambda[i]; ++i) {
        double segStart = std::max(lambdaStart, lambda[i]);
        double segEnd = std::min(lambdaEnd, lambda[i + 1]);
        sum += 0.5 * (interp(segStart, i) + interp(segEnd, i)) * (segEnd - segStart);
    }
    return sum / (lambdaEnd - lambdaStart);
}

// Blackbody spectral radiance
inline double blackbody(double lambda, double T) {
    if (T <= 0) return 0;
    const double c = 299792458.0, h = 6.62607015e-34, kb = 1.380649e-23;
    double l = lambda * 1e-9;
    return (2 * h * c * c) / (l * l * l * l * l * (std::exp((h * c) / (l * kb * T)) - 1));
}

// ============================================================================
// Spectrum Class (Sampled Spectrum Implementation)
// ============================================================================

class Spectrum {
public:
    static constexpr int nSamples = 60;
    static constexpr double lambdaStart = 400;
    static constexpr double lambdaEnd = 700;

private:
    std::array<double, nSamples> c = {};

    // Static conversion data stored as arrays
    struct StaticData {
        std::array<double, nSamples> sX, sY, sZ;
        std::array<double, nSamples> rgbRefl2SpectWhite, rgbRefl2SpectCyan, rgbRefl2SpectMagenta;
        std::array<double, nSamples> rgbRefl2SpectYellow, rgbRefl2SpectRed, rgbRefl2SpectGreen,
            rgbRefl2SpectBlue;
        std::array<double, nSamples> rgbIllum2SpectWhite, rgbIllum2SpectCyan, rgbIllum2SpectMagenta;
        std::array<double, nSamples> rgbIllum2SpectYellow, rgbIllum2SpectRed, rgbIllum2SpectGreen,
            rgbIllum2SpectBlue;
        bool initialized = false;
    };
    static StaticData& data() {
        static StaticData d;
        return d;
    }

    static double lerp(double t, double a, double b) { return (1 - t) * a + t * b; }

public:
    Spectrum(double v = 0) { c.fill(v); }

    // Initialize static data
    static void init() {
        auto& d = data();
        if (d.initialized) return;
        using namespace spectrum_data;

        for (int i = 0; i < nSamples; ++i) {
            double wl0 = lerp(double(i) / nSamples, lambdaStart, lambdaEnd);
            double wl1 = lerp(double(i + 1) / nSamples, lambdaStart, lambdaEnd);

            d.sX[i] = averageSpectrumSamples(CIE_lambda, CIE_X, nCIESamples, wl0, wl1);
            d.sY[i] = averageSpectrumSamples(CIE_lambda, CIE_Y, nCIESamples, wl0, wl1);
            d.sZ[i] = averageSpectrumSamples(CIE_lambda, CIE_Z, nCIESamples, wl0, wl1);

            d.rgbRefl2SpectWhite[i] = averageSpectrumSamples(
                RGB2SpectLambda, RGBRefl2SpectWhite, nRGB2SpectSamples, wl0, wl1);
            d.rgbRefl2SpectCyan[i] = averageSpectrumSamples(
                RGB2SpectLambda, RGBRefl2SpectCyan, nRGB2SpectSamples, wl0, wl1);
            d.rgbRefl2SpectMagenta[i] = averageSpectrumSamples(
                RGB2SpectLambda, RGBRefl2SpectMagenta, nRGB2SpectSamples, wl0, wl1);
            d.rgbRefl2SpectYellow[i] = averageSpectrumSamples(
                RGB2SpectLambda, RGBRefl2SpectYellow, nRGB2SpectSamples, wl0, wl1);
            d.rgbRefl2SpectRed[i] = averageSpectrumSamples(
                RGB2SpectLambda, RGBRefl2SpectRed, nRGB2SpectSamples, wl0, wl1);
            d.rgbRefl2SpectGreen[i] = averageSpectrumSamples(
                RGB2SpectLambda, RGBRefl2SpectGreen, nRGB2SpectSamples, wl0, wl1);
            d.rgbRefl2SpectBlue[i] = averageSpectrumSamples(
                RGB2SpectLambda, RGBRefl2SpectBlue, nRGB2SpectSamples, wl0, wl1);

            d.rgbIllum2SpectWhite[i] = averageSpectrumSamples(
                RGB2SpectLambda, RGBIllum2SpectWhite, nRGB2SpectSamples, wl0, wl1);
            d.rgbIllum2SpectCyan[i] = averageSpectrumSamples(
                RGB2SpectLambda, RGBIllum2SpectCyan, nRGB2SpectSamples, wl0, wl1);
            d.rgbIllum2SpectMagenta[i] = averageSpectrumSamples(
                RGB2SpectLambda, RGBIllum2SpectMagenta, nRGB2SpectSamples, wl0, wl1);
            d.rgbIllum2SpectYellow[i] = averageSpectrumSamples(
                RGB2SpectLambda, RGBIllum2SpectYellow, nRGB2SpectSamples, wl0, wl1);
            d.rgbIllum2SpectRed[i] = averageSpectrumSamples(
                RGB2SpectLambda, RGBIllum2SpectRed, nRGB2SpectSamples, wl0, wl1);
            d.rgbIllum2SpectGreen[i] = averageSpectrumSamples(
                RGB2SpectLambda, RGBIllum2SpectGreen, nRGB2SpectSamples, wl0, wl1);
            d.rgbIllum2SpectBlue[i] = averageSpectrumSamples(
                RGB2SpectLambda, RGBIllum2SpectBlue, nRGB2SpectSamples, wl0, wl1);
        }
        d.initialized = true;
    }

    // Factory: from RGB
    static Spectrum fromRGB(const double rgb[3], SpectrumType type = SpectrumType::Reflectance) {
        auto& d = data();
        if (!d.initialized) init();
        Spectrum r;

        auto& white =
            (type == SpectrumType::Reflectance) ? d.rgbRefl2SpectWhite : d.rgbIllum2SpectWhite;
        auto& cyan =
            (type == SpectrumType::Reflectance) ? d.rgbRefl2SpectCyan : d.rgbIllum2SpectCyan;
        auto& magenta =
            (type == SpectrumType::Reflectance) ? d.rgbRefl2SpectMagenta : d.rgbIllum2SpectMagenta;
        auto& yellow =
            (type == SpectrumType::Reflectance) ? d.rgbRefl2SpectYellow : d.rgbIllum2SpectYellow;
        auto& red = (type == SpectrumType::Reflectance) ? d.rgbRefl2SpectRed : d.rgbIllum2SpectRed;
        auto& green =
            (type == SpectrumType::Reflectance) ? d.rgbRefl2SpectGreen : d.rgbIllum2SpectGreen;
        auto& blue =
            (type == SpectrumType::Reflectance) ? d.rgbRefl2SpectBlue : d.rgbIllum2SpectBlue;

        auto addWeighted = [&r](double weight, const std::array<double, nSamples>& arr) {
            for (int i = 0; i < nSamples; ++i) r.c[i] += weight * arr[i];
        };

        if (rgb[0] <= rgb[1] && rgb[0] <= rgb[2]) {
            addWeighted(rgb[0], white);
            if (rgb[1] <= rgb[2]) {
                addWeighted(rgb[1] - rgb[0], cyan);
                addWeighted(rgb[2] - rgb[1], blue);
            } else {
                addWeighted(rgb[2] - rgb[0], cyan);
                addWeighted(rgb[1] - rgb[2], green);
            }
        } else if (rgb[1] <= rgb[0] && rgb[1] <= rgb[2]) {
            addWeighted(rgb[1], white);
            if (rgb[0] <= rgb[2]) {
                addWeighted(rgb[0] - rgb[1], magenta);
                addWeighted(rgb[2] - rgb[0], blue);
            } else {
                addWeighted(rgb[2] - rgb[1], magenta);
                addWeighted(rgb[0] - rgb[2], red);
            }
        } else {
            addWeighted(rgb[2], white);
            if (rgb[0] <= rgb[1]) {
                addWeighted(rgb[0] - rgb[2], yellow);
                addWeighted(rgb[1] - rgb[0], green);
            } else {
                addWeighted(rgb[1] - rgb[2], yellow);
                addWeighted(rgb[0] - rgb[1], red);
            }
        }
        return r.clamp();
    }

    static Spectrum fromRGB(const Color& color, SpectrumType type = SpectrumType::Reflectance) {
        double rgb[3] = {color.r(), color.g(), color.b()};
        return fromRGB(rgb, type);
    }

    // For backward compatibility
    explicit Spectrum(const Color& color) : Spectrum(fromRGB(color)) {}
    explicit Spectrum(const Vec3& rgb) : Spectrum(Color(rgb)) {}

    // Sample at wavelength
    double operator()(double lambda) const {
        if (lambda < lambdaStart || lambda > lambdaEnd) return 0;
        double t = (lambda - lambdaStart) / (lambdaEnd - lambdaStart) * nSamples;
        int i = std::min(int(t), nSamples - 1);
        double dt = t - i;
        if (i >= nSamples - 1) return c[nSamples - 1];
        return (1 - dt) * c[i] + dt * c[i + 1];
    }

    // Alias for backward compatibility
    double value(double lambda) const { return (*this)(lambda); }

    // Convert to XYZ
    void toXYZ(double xyz[3]) const {
        auto& d = data();
        if (!d.initialized) init();
        xyz[0] = xyz[1] = xyz[2] = 0;
        for (int i = 0; i < nSamples; ++i) {
            xyz[0] += d.sX[i] * c[i];
            xyz[1] += d.sY[i] * c[i];
            xyz[2] += d.sZ[i] * c[i];
        }
        double scale = (lambdaEnd - lambdaStart) / (spectrum_data::CIE_Y_integral * nSamples);
        xyz[0] *= scale;
        xyz[1] *= scale;
        xyz[2] *= scale;
    }

    // Convert to RGB
    void toRGB(double rgb[3]) const {
        double xyz[3];
        toXYZ(xyz);
        XYZToRGB(xyz, rgb);
    }

    Color toColor() const {
        double rgb[3];
        toRGB(rgb);
        return Color(std::max(0.0, rgb[0]), std::max(0.0, rgb[1]), std::max(0.0, rgb[2]));
    }

    // Luminance
    double y() const {
        auto& d = data();
        if (!d.initialized) init();
        double yy = 0;
        for (int i = 0; i < nSamples; ++i) yy += d.sY[i] * c[i];
        return yy * (lambdaEnd - lambdaStart) / (spectrum_data::CIE_Y_integral * nSamples);
    }

    bool isBlack() const {
        for (int i = 0; i < nSamples; ++i)
            if (c[i] != 0) return false;
        return true;
    }

    double maxComponent() const {
        double m = c[0];
        for (int i = 1; i < nSamples; ++i) m = std::max(m, c[i]);
        return m;
    }

    // Array access
    double& operator[](int i) { return c[i]; }
    double operator[](int i) const { return c[i]; }

    // Arithmetic
    Spectrum& operator+=(const Spectrum& s) {
        for (int i = 0; i < nSamples; ++i) c[i] += s.c[i];
        return *this;
    }
    Spectrum& operator-=(const Spectrum& s) {
        for (int i = 0; i < nSamples; ++i) c[i] -= s.c[i];
        return *this;
    }
    Spectrum& operator*=(const Spectrum& s) {
        for (int i = 0; i < nSamples; ++i) c[i] *= s.c[i];
        return *this;
    }
    Spectrum& operator*=(double a) {
        for (int i = 0; i < nSamples; ++i) c[i] *= a;
        return *this;
    }
    Spectrum& operator/=(double a) { return *this *= (1.0 / a); }

    Spectrum operator+(const Spectrum& s) const {
        Spectrum r = *this;
        return r += s;
    }
    Spectrum operator-(const Spectrum& s) const {
        Spectrum r = *this;
        return r -= s;
    }
    Spectrum operator*(const Spectrum& s) const {
        Spectrum r = *this;
        return r *= s;
    }
    Spectrum operator*(double a) const {
        Spectrum r = *this;
        return r *= a;
    }
    Spectrum operator/(double a) const {
        Spectrum r = *this;
        return r /= a;
    }
    friend Spectrum operator*(double a, const Spectrum& s) { return s * a; }
    Spectrum operator-() const {
        Spectrum r;
        for (int i = 0; i < nSamples; ++i) r.c[i] = -c[i];
        return r;
    }

    bool operator==(const Spectrum& s) const {
        for (int i = 0; i < nSamples; ++i)
            if (c[i] != s.c[i]) return false;
        return true;
    }
    bool operator!=(const Spectrum& s) const { return !(*this == s); }

    Spectrum clamp(double low = 0, double high = 1e30) const {
        Spectrum r;
        for (int i = 0; i < nSamples; ++i) r.c[i] = std::clamp(c[i], low, high);
        return r;
    }

    friend Spectrum sqrt(const Spectrum& s) {
        Spectrum r;
        for (int i = 0; i < nSamples; ++i) r.c[i] = std::sqrt(s.c[i]);
        return r;
    }
    friend Spectrum exp(const Spectrum& s) {
        Spectrum r;
        for (int i = 0; i < nSamples; ++i) r.c[i] = std::exp(s.c[i]);
        return r;
    }
    friend Spectrum pow(const Spectrum& s, double e) {
        Spectrum r;
        for (int i = 0; i < nSamples; ++i) r.c[i] = std::pow(s.c[i], e);
        return r;
    }
    friend Spectrum lerp(double t, const Spectrum& s1, const Spectrum& s2) {
        return (1 - t) * s1 + t * s2;
    }

    // Backward compatibility aliases
    static constexpr double visible_min = lambdaStart;
    static constexpr double visible_max = lambdaEnd;
    static Spectrum constant(double v) { return Spectrum(v); }
};

// ============================================================================
// Wavelength Utilities
// ============================================================================

inline Vec3 wavelengthToXYZ(double lambda) {
    using namespace spectrum_data;
    return Vec3(
        interpolateSpectrumSamples(CIE_lambda, CIE_X, nCIESamples, lambda),
        interpolateSpectrumSamples(CIE_lambda, CIE_Y, nCIESamples, lambda),
        interpolateSpectrumSamples(CIE_lambda, CIE_Z, nCIESamples, lambda));
}

inline Color wavelengthToColor(double lambda) {
    Vec3 xyz = wavelengthToXYZ(lambda);
    double rgb[3], xyzArr[3] = {xyz.x(), xyz.y(), xyz.z()};
    XYZToRGB(xyzArr, rgb);
    return Color(std::max(0.0, rgb[0]), std::max(0.0, rgb[1]), std::max(0.0, rgb[2]));
}

// D65 white point in XYZ (standard daylight illuminant)
inline constexpr double D65_X = 0.95047;
inline constexpr double D65_Y = 1.0;
inline constexpr double D65_Z = 1.08883;

// Convert spectral radiance to RGB color
// wavelength: wavelength in nm
// radiance: spectral radiance at that wavelength  
// wavelength_pdf: PDF of wavelength sampling (typically 1/(lambda_max - lambda_min))
inline Color spectralRadianceToRGB(double radiance, double wavelength, double wavelength_pdf) {
    if (radiance <= 0.0 || wavelength_pdf <= 0.0) {
        return Color(0, 0, 0);
    }
    Vec3 xyz = wavelengthToXYZ(wavelength);
    // Scale by radiance and normalize by wavelength PDF and CIE Y integral
    double scale = radiance / (wavelength_pdf * spectrum_data::CIE_Y_integral);
    xyz *= scale;
    // Convert XYZ to RGB
    double rgb[3], xyzArr[3] = {xyz.x(), xyz.y(), xyz.z()};
    XYZToRGB(xyzArr, rgb);
    return Color(std::max(0.0, rgb[0]), std::max(0.0, rgb[1]), std::max(0.0, rgb[2]));
}
