#pragma once
#pragma once

#include<opencv2/opencv.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp> //mophorlogical operation
#include<opencv2/core.hpp>
#include <numeric>
#include <chrono>

using namespace cv;
using namespace std;

#pragma region structure_declare
typedef struct
{
	int interval[4]; //x interval, y interval
	int xpitch[3]; //in unit of pixel 
	int ypitch[3];
	int carx;
	int cary;
}SettingP_;

typedef struct
{

	double TDwidth;
	double TDmaxW;
	double TDminW;
	double TDheight;
	double TDmaxH;
	double TDminH;

}sizeTD_;

typedef struct
{
	int thresmode; //0:gray ; 1:RGB ; 2:HSV
	int bgmax[3];
	int bgmin[3];
	int fgmax[3];
	int fgmin[3];

}thresP_;

typedef struct
{
	int PICmode; //0: read pic from path ; 1: read pic from pointer address
	int Outputmode;
	int cols; //
	int rows; //
	double correctTheta;
}ImgP_;

#pragma endregion



#pragma region function_declare
/*Input function*/
std::tuple<int, Mat> Inputfunction(); //picture order, bg coodinate, picture
std::tuple < int, int> dict_masksize(int picorder);
std::tuple<Mat, Mat, Mat>Histplotting(Mat src, int hist_w, int hist_h, int histsize);

std::tuple < int, int> dict_thresval(int picorder);
void CreateRotImg(Mat src, int picsavenumber, double theta);

/*general operation*/
Point find_piccenter(Mat src);
Mat CropIMG(Mat img, Rect size);
Mat RotatecorrectImg(double Rtheta, Mat src);

void gammaCorrection(const Mat& src, Mat& dst, const float gamma);

std::tuple<int, Point_<int>> FindMF_pixel(Mat histImg);
Mat KmeanOP(int knum, Mat src);


int findBoundary(Mat creteriaIMG, Rect inirect, char direction);
std::tuple<Rect, Point>FindMaxInnerRect(Mat src, Mat colorSRC, sizeTD_ target, Point TDcenter);

std::tuple<Point, int> potentialchipSearch_V1(Mat cropedRImg, double resizeTDwidth, double resizeTDheight, sizeTD_ target,int thresmode,int flag, double theta, Point2f creteriaPoint);

std::tuple<Point, int, Mat, Mat,Rect> FinechipDefine_V1(Mat rawimg, sizeTD_ target, thresP_ thresParm, int boolflag,Point Potchip, SettingP_ chipsetting);

void DrawNG(Mat& markimg_simu, thresP_ thresParm, SettingP_ chipsetting, Mat& Grayimg);

std::tuple<Point, Mat, int>SimulateCoord_V1(Mat rawimg, Point piccenter, Point Finechip, int boolflag, SettingP_ chipsetting, Rect fineRect);
#pragma endregion

void funcThreshold(Mat ImgInput, Mat& ImgThres, thresP_ thresParm, int PicMode, sizeTD_ target);


void CheckCropImgIsReasonable(Mat img, SettingP_ chipsetting, sizeTD_ target, ImgP_ imageParm, int& flag, Point2f& PicCenterOut);
