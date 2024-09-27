#include "pch.h"
#include "AOILib_SecAreaChipCP_V1.h"


#include<opencv2/opencv.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp> //mophorlogical operation
#include<opencv2/core.hpp>
#include "../secAreaChipCP_V1/secAreaChipCP_lib.h"
//#include <numeric>
//#include <chrono>


using namespace cv;
using namespace std;


void CPchips_SecArea(thresP thresParm, ImgP imageParm, SettingP chipsetting, sizeTD target, unsigned int* imageIN,
					unsigned int* imageOUT, unsigned char* imageGray, float boolResult[], float outputLEDX[], float outputLEDY[])
{
	#pragma region 格式轉換
	thresP_ _thresParm;

	_thresParm.thresmode = thresParm.thresmode;

	for (int i = 0; i < 3; i++)
	{
		_thresParm.bgmax[i] = thresParm.bgmax[i];
		_thresParm.bgmin[i] = thresParm.bgmin[i];
		_thresParm.fgmax[i] = thresParm.fgmax[i];
		_thresParm.fgmin[i] = thresParm.fgmin[i];
	}

	ImgP_ _imageParm;

	_imageParm.correctTheta = imageParm.correctTheta;
	_imageParm.cols = imageParm.cols;
	_imageParm.rows = imageParm.rows;
	_imageParm.Outputmode = imageParm.Outputmode;
	_imageParm.PICmode = imageParm.PICmode;

	SettingP_ _chipsetting;

	_chipsetting.carx = chipsetting.carx;
	_chipsetting.cary = chipsetting.cary;

	for (int i = 0; i < 4; i++)
		_chipsetting.interval[i] = chipsetting.interval[i];

	for (int i = 0; i < 3; i++)
	{
		_chipsetting.xpitch[i] = chipsetting.xpitch[i];
		_chipsetting.ypitch[i] = chipsetting.ypitch[i];
	}

	sizeTD_ _target;

	_target.TDheight = target.TDheight;
	_target.TDmaxH = target.TDmaxH;
	_target.TDmaxW = target.TDmaxW;
	_target.TDminH = target.TDminH;
	_target.TDminW = target.TDminW;
	_target.TDwidth = target.TDwidth;
#pragma endregion

	Mat rawimg, cropedRImg, gauBGR;
	Mat Gimg, drawF2;
	vector<Point> Fourchipspt;

	Point piccenter;
	Point2f creteriaPoint;
	Point IMGoffset=Point(0,0);

	//output parameters::
	Mat ReqIMG, marksize;
	Point simupt;
	int boolflag = 0;//11
	Point Finechip;
	Mat  markimg;

	Mat image_input(imageParm.rows, imageParm.cols, CV_8UC4, &imageIN[0]); // THIS IS THE INPUT IMAGE, POINTER TO DATA			
	image_input.copyTo(rawimg);

	Mat image_output(800, 1100, CV_8UC4, &imageOUT[0]);
	Mat thres_output(600, 500, CV_8UC1, &imageGray[0]);

	try
	{
		if (rawimg.empty())
		{
			boolflag = 8;
			throw "something wrong::input image failure";
		} //check if image is empty

	} //try loop
	catch (const char* message)
	{

		std::cout << "check catch state:: " << boolflag << endl;


	}//catch loop

	if (imageParm.cols != rawimg.cols || imageParm.rows != rawimg.rows)
	{
		boolflag == 7;
		rawimg.copyTo(drawF2);
		Gimg = Mat::zeros(Size(600, 500), CV_8UC1);
	}

	//----防呆 如果 carX carY 是(0,0) 自動帶入 (2660,2300)
	//if (_chipsetting.carx == 0 && _chipsetting.cary == 0)
	//{
	//	_chipsetting.carx = 2660;
	//	_chipsetting.cary = 2300;
	//}


	if (imageParm.cols != rawimg.cols || imageParm.rows != rawimg.rows)
		boolflag = 7;

	if (boolflag == 0)
		CheckCropImgIsReasonable(rawimg, _chipsetting, _target, _imageParm, boolflag, creteriaPoint);

	if (boolflag == 7)
	{
		rawimg.copyTo(drawF2);
		Gimg = Mat::zeros(Size(600, 500), CV_8UC1);
	}


	if (boolflag == 0) //&& imageParm.Outputmode == 0
	{
		
		//creteriaPoint = find_piccenter(rawimg);

		/*****Step.1 roughly search chip:::*/
			/*Resize image to speed up::start*/
		double resizeTDwidth = _target.TDwidth / 10;
		double resizeTDheight = _target.TDheight / 10;
		std::cout << "calculate resize TD dimension is:: " << resizeTDwidth << " / " << resizeTDheight << endl;
		cv::resize(rawimg, cropedRImg, Size(int(rawimg.cols / 10), int(rawimg.rows / 10)), INTER_NEAREST);

		//auto t_start2 = std::chrono::high_resolution_clock::now();

		Point Potchip;
		std::tie(Potchip, boolflag) = potentialchipSearch_V1(cropedRImg, resizeTDwidth, resizeTDheight, _target, thresParm.thresmode, boolflag, 3, creteriaPoint);

		/*auto t_end2 = std::chrono::high_resolution_clock::now();
		double elapsed_time_ms2 = std::chrono::duration<double, std::milli>(t_end2 - t_start2).count();
		std::cout << "calculate roughly-search-op time is:: " << elapsed_time_ms2 << endl;*/

		/*Resize image to speed up:: end*/

		if (Potchip.y< chipsetting.ypitch[0] || Potchip.y>rawimg.rows - chipsetting.ypitch[0] ||
			Potchip.x< chipsetting.xpitch[0] || Potchip.x>rawimg.cols - chipsetting.xpitch[0])
		{
			rawimg.copyTo(drawF2);
			circle(drawF2, Potchip, 20, Scalar(0, 0, 255), -1);
			//cv::resize(drawF2, drawF2, Size(1100, 800), INTER_LINEAR);
			simupt = Point(0, 0);
			boolflag = 3;
			Gimg = Mat::zeros(Size(600, 500), CV_8UC1);
			/*check pitch or move antother area....*/
		}
		 

		
		/*****Step.2 automatically crop ROI and fine define chip:::*/
			//intput= Potchip
			//output: chipcenter
		Rect finerect;

		if (boolflag == 0)
		{
			/*Grayimg isze= (500,500)*/
			std::tie(Finechip, boolflag, Gimg, markimg, finerect) = FinechipDefine_V1(rawimg, _target, _thresParm, boolflag, Potchip, _chipsetting);

			if (boolflag == 0)
			{
				/*****Step.3 Simulate coordinate:::*/
				//intput= chipcenter,rawpic_center(2660,2300)
				//output: simulated center

				std::tie(simupt, drawF2, boolflag) = SimulateCoord_V1(rawimg, creteriaPoint, Finechip, boolflag, _chipsetting, finerect);

			}
		}
		else
		{
			rawimg.copyTo(drawF2);
			Gimg = Mat::zeros(Size(600, 500), CV_8UC1);
		}

		
	}

	std::cout << "check img state:: " << boolflag << endl;
	std::cout << "check center is ::" << simupt << endl;

	cv::resize(Gimg, Gimg, Size(500, 600), INTER_LINEAR);
	cv::resize(drawF2, drawF2, Size(1100, 800), INTER_LINEAR);

	/*  :::::::OUTPUT area:::::::  */
	outputLEDX[0] = simupt.x ;
	outputLEDY[0] = simupt.y ;
	Gimg.copyTo(thres_output);
	drawF2.copyTo(image_output);
	boolResult[0] = boolflag;

	/*for (int i = 1; i < Fourchipspt.size() + 1; i++) //save corner coordinates
	{
		outputLEDX[i] = Fourchipspt[i - 1].x;
		outputLEDY[i] = Fourchipspt[i - 1].y;
	}*/
}




/*
0=初始預設值
1=閥值設定有問題 (調整fgmax[0] & bgmax[0])
2= 找不到符合大小的目標物
3=XT pitch設定有誤或圖面上有效晶片太接近邊緣(調整xpitch[0] & ypitch[0],或人工確認有效晶片)
4=FOV內沒有有效晶片(請人工確認)
7=圖片大小設定有誤 (調整rows & cols)  V
8=輸入無影像  V
9=辨識成功且位於正確加工區域

*/