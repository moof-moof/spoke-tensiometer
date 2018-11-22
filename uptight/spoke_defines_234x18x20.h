/**
 * Definition file for this spoke type: DT Alpine III
 * ------------------------------------ =============
 * 
 * Dimension:   Triple butted 2.34 x 1.8 x 2.0
 *
 * Length:      Approx. 300
 *
 ***************************************************************************************** */


#define  ELBOW              "2.34"  // Two of the spoke's critical gauges are displayed for reference
#define  TRUNK              "1.8"   // (Threaded end is assumed to be 2.0 mm)
#define  Y_OFFSET           649     // Results from calibration graph fitting formula
//#define  GRADE              118     // (1.18) Using a percentage ratio like this avoids floats
#define  GRADE              0.848  // Reciprocal of angle ratio of fitting x/y graph (i.e. 1/1.18)
