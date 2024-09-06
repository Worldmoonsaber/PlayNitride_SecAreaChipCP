#include "pch.h"
#include "AOILib_SecAreaChipCP_V1.h"


#include<opencv2/opencv.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp> //mophorlogical operation
#include<opencv2/core.hpp>
//#include <numeric>
//#include <chrono>


using namespace cv;
using namespace std;


/*general operation*/
#pragma region generalfunction_declare
	Point find_piccenter(Mat src);
	Mat CropIMG(Mat img, Rect size);
	std::tuple<Rect, Point>FindMaxInnerRect(Mat src, Mat colorSRC, sizeTD target, Point TDcenter);
	int findBoundary(Mat creteriaIMG, Rect inirect, char direction);
#pragma endregion generalfunction_declare

/******Single - phase chip:::*******/
#pragma region chipalgorithm_declare
	std::tuple<Point, int> potentialchipSearch_V1(Mat cropedRImg, double resizeTDwidth, double resizeTDheight, sizeTD target, int thresmode, int flag, double theta);

	std::tuple<Point, int, Mat, Mat, Rect> FinechipDefine_V1(Mat rawimg, sizeTD target, thresP thresParm, int boolflag, Point Potchip, SettingP chipsetting);

	std::tuple<Point, Mat, int>SimulateCoord_V1(Mat rawimg, Point piccenter, Point Finechip, int boolflag, SettingP chipsetting, Rect fineRect);
#pragma endregion chipalgorithm_declare


void CPchips_SecArea(thresP thresParm, ImgP imageParm, SettingP chipsetting, sizeTD target, unsigned int* imageIN,
					unsigned int* imageOUT, unsigned char* imageGray, float boolResult[], float outputLEDX[], float outputLEDY[])
{

	
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
	

	if (boolflag == 0) //&& imageParm.Outputmode == 0
	{
		
		creteriaPoint = find_piccenter(rawimg);

		/*****Step.1 roughly search chip:::*/
			/*Resize image to speed up::start*/
		double resizeTDwidth = target.TDwidth / 12;
		double resizeTDheight = target.TDheight / 12;
		std::cout << "calculate resize TD dimension is:: " << resizeTDwidth << " / " << resizeTDheight << endl;
		cv::resize(rawimg, cropedRImg, Size(int(rawimg.cols / 12), int(rawimg.rows / 12)), INTER_NEAREST);

		//auto t_start2 = std::chrono::high_resolution_clock::now();

		Point Potchip;
		std::tie(Potchip, boolflag) = potentialchipSearch_V1(cropedRImg, resizeTDwidth, resizeTDheight, target, thresParm.thresmode, boolflag, 3);

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
			std::tie(Finechip, boolflag, Gimg, markimg, finerect) = FinechipDefine_V1(rawimg, target, thresParm, boolflag, Potchip, chipsetting);

			if (boolflag == 0)
			{
				/*****Step.3 Simulate coordinate:::*/
				//intput= chipcenter,rawpic_center(2660,2300)
				//output: simulated center

				std::tie(simupt, drawF2, boolflag) = SimulateCoord_V1(rawimg, creteriaPoint, Finechip, boolflag, chipsetting, finerect);

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



#pragma region General_function


Point find_piccenter(Mat src) {
	int piccenterx = int(src.size().width * 0.5);
	int piccentery = int(src.size().height * 0.5);
	Point piccenter = Point(piccenterx, piccentery);
	return piccenter;
}

Mat CropIMG(Mat img, Rect size)
{
	Mat croppedIMG;
	img(size).copyTo(croppedIMG);
	return croppedIMG;

}


std::tuple<Rect, Point>FindMaxInnerRect(Mat src, Mat colorSRC, sizeTD target, Point TDcenter)
{
	//output:::
	Rect innerboundary;
	Point center = TDcenter;
	Mat markcolor = Mat::zeros(colorSRC.size(), CV_8UC4);
	colorSRC.copyTo(markcolor);
	cv::circle(markcolor, center, 2, Scalar(180, 180, 180), 5);
	cv::circle(src, center, 2, Scalar(180, 180, 180), 5);
	//
	//find inner rect:
	Size ksize;
	Mat src2;
	ksize = Size(15, 15);
	Mat Kcomclose = Mat::ones(ksize, CV_8UC1);
	cv::morphologyEx(src, src2, cv::MORPH_CLOSE, Kcomclose, Point(-1, -1), 4);//4
	threshold(src2, src2, 175, 255, THRESH_BINARY);
	vector<Vec4i> hierarchy;
	vector<vector<Point>> contours, reqCon;
	vector<Point> approx;
	Rect retCOMP;
	vector<Point> reqCenter;

	Rect fineRect;

	cv::findContours(src2, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	double areasrc = cv::contourArea(contours[0]);

	for (int i = 0; i < contours.size(); i++)
	{
		areasrc = cv::contourArea(contours[i]);
		if (areasrc < target.TDwidth * target.TDminH * target.TDheight)
		{
			Rect bdrect = cv::boundingRect(contours[i]);
			cv::rectangle(src2, bdrect, Scalar(255, 255, 255), -1);
		}
	}
	cv::findContours(src2, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	cv::drawContours(src2, contours, -1, Scalar(255, 255, 255), -1);
	//cv::approxPolyDP(contours[0], approx, arcLength(contours[0], true) * 0.02, true); //

	//std::cout << "check approx size : " << approx.size() << endl;

	//Step.NEW7-find inner rect (via tiny scanning mechanism) ::
	cv::Mat inverted_mask;
	cv::bitwise_not(src2, inverted_mask);
	cv::Mat pointsmsk = Mat::zeros(src.size(), CV_8UC1);;
	cv::findNonZero(src2, pointsmsk);
	const cv::Rect outside_rect = cv::boundingRect(pointsmsk);
	Mat TDrect = Mat::zeros(src.size(), CV_8UC1);
	src.copyTo(TDrect);
	cv::rectangle(TDrect, outside_rect, Scalar(180, 180, 180), 1); //Scalar(0, 0, 0)

	/*std::cout << "check area : " << outside_rect.width * outside_rect.height << " // " << target.TDwidth * target.TDheight << endl;
	std::cout << "calculate area : " << outside_rect.width * outside_rect.height << " // " << target.TDwidth * target.TDheight << endl;*/
	int step_w, step_h;


	/*if ((outside_rect.width * outside_rect.height) / (target.TDwidth * target.TDheight) > 0.8
		&& (outside_rect.width * outside_rect.height) / (target.TDwidth * target.TDheight) < 1.15 )*/


	std::cout << "[select inner rect...]" << endl;
	if (outside_rect.width > outside_rect.height)
	{
		step_w = 1;//2
		step_h = 1;//1
	}
	else
	{
		step_w = 1;//1
		step_h = 1;//2
	}


	auto inside_rect = outside_rect;


	while (true)
	{
		//const auto count = cv::countNonZero(inverted_mask(inside_rect));
		const auto count = cv::countNonZero(inverted_mask(inside_rect));


		if (count == 0)
		{
			// we found a rectangle we can use!
			break;
		}

		inside_rect.x += step_w;
		inside_rect.y += step_h;
		inside_rect.width -= (step_w * 2);
		inside_rect.height -= (step_h * 2);
	}



	//cv::rectangle(TDrect, inside_rect, Scalar(100, 100, 100), 1); //Scalar(0, 0, 0)

	/*std::cout << "check inside rect:: " << inside_rect << endl;
	std::cout << "check outside_rect:: " << outside_rect << endl;*/

	//Step.NEW8-find inner rect boundary ::

	cv::rectangle(inverted_mask, Rect(0, 0, inverted_mask.size().width, inverted_mask.size().height), Scalar(255, 255, 255), 1);
	Rect line = Rect(inside_rect.x, inside_rect.y, inside_rect.width, 1);
	//cv::rectangle(gamimg, line, Scalar(0, 0, 0), 1); //Scalar(0, 0, 0)
	const auto count = cv::countNonZero(inverted_mask(line));
	//std::cout << "999999999999999999999999999999999: " << count << endl;
	int leftBound;
	//Rect Leftline = Rect(inside_rect.x, inside_rect.y, 1, inside_rect.height); //360,355 
	Rect Leftline = Rect(int(inside_rect.x + 1), inside_rect.y + int(inside_rect.height * 0.15), 1, int(0.7 * inside_rect.height)); //360,355 
	//cv::rectangle(colorSRC, Leftline, Scalar(88, 50, 155), 2);
	leftBound = findBoundary(inverted_mask, Leftline, 'L');
	//std::cout << "check left boundary " << leftBound << endl;


	int topBound;
	//Rect Topline = Rect(inside_rect.x, inside_rect.y, inside_rect.width, 1);
	Rect Topline = Rect(inside_rect.x + int(0.15 * inside_rect.width), int(inside_rect.y + 1), int(0.7 * inside_rect.width), 1);
	topBound = findBoundary(inverted_mask, Topline, 'T');
	//std::cout << "check Top boundary " << topBound << endl;

	int RightBound;
	//Rect Rightline = Rect(inside_rect.x + inside_rect.width, inside_rect.y, 1, inside_rect.height);
	Rect Rightline = Rect(inside_rect.x + int(inside_rect.width - 1), inside_rect.y + int(inside_rect.height * 0.15), 1, int(0.7 * inside_rect.height));
	//cv::rectangle(colorSRC, Rightline, Scalar(88, 50, 155), 2);
	RightBound = findBoundary(inverted_mask, Rightline, 'R');
	//std::cout << "check right boundary " << RightBound << endl;

	int BottomBound;
	//Rect bottomline = Rect(inside_rect.x, inside_rect.y + inside_rect.height, inside_rect.width, 1);
	Rect bottomline = Rect(inside_rect.x + int(0.15 * inside_rect.width), inside_rect.y + int(inside_rect.height - 1), int(0.7 * inside_rect.width), 1);
	BottomBound = findBoundary(inverted_mask, bottomline, 'B');
	//std::cout << "check bottom boundary " << BottomBound << endl;

	//innerboundary = Rect(leftBound, topBound, (RightBound - leftBound), (BottomBound - topBound));

	innerboundary = Rect(leftBound, topBound, (RightBound - leftBound), (BottomBound - topBound));

	center = Point(int(innerboundary.x + innerboundary.width * 0.5), int(innerboundary.y + innerboundary.height * 0.5));
	//std::cout << "previous center is :: " << center << endl;

	cv::rectangle(markcolor, innerboundary, Scalar(0, 255, 255), 1);

	//std::cout << "innerboundary.width " << innerboundary.width << "/*/* " << target.TDwidth << endl;





	//Step.NEW9-Mark inner rect::
	cv::rectangle(markcolor, innerboundary, Scalar(0, 0, 255), 1);
	cv::rectangle(markcolor, fineRect, Scalar(255, 0, 0), 1);
	cv::rectangle(TDrect, innerboundary, Scalar(50, 50, 50), 2);
	cv::circle(markcolor, center, 2, Scalar(20, 20, 20), 5);
	//check area::
	/*std::cout << "check dimension-width::: " << innerboundary.width << " ||| " << target.TDwidth << endl;
	std::cout << "check dimension-height::: " << innerboundary.height << " ||| " << target.TDheight << endl;
	std::cout << "check center::: " << center << endl;*/

	std::cout << "fini" << endl;

	//
	return { innerboundary,center };

}


int findBoundary(Mat creteriaIMG, Rect inirect, char direction)
{
	int step = 1;
	auto findRecr = inirect;
	int BoundaryVal;

	switch (direction)
	{
	case 'L':
		while (true)
		{
			//const auto count = cv::countNonZero(inverted_mask(inside_rect));
			const auto count = cv::countNonZero(creteriaIMG(findRecr));
			if (count > 0)
			{
				BoundaryVal = findRecr.x;
				break;
			}
			findRecr.x -= step;
		}
		break;
	case 'T':
		while (true)
		{
			const auto count = cv::countNonZero(creteriaIMG(findRecr));
			if (count > 0)
			{
				BoundaryVal = findRecr.y;
				break;
			}
			findRecr.y -= step;
		}
		break;
	case 'R':
		while (true)
		{
			const auto count = cv::countNonZero(creteriaIMG(findRecr));
			if (count > 0)
			{
				BoundaryVal = findRecr.x;
				break;
			}
			findRecr.x += step;
		}
		break;

		break;
	case 'B':
		while (true)
		{
			const auto count = cv::countNonZero(creteriaIMG(findRecr));
			if (count > 0)
			{
				BoundaryVal = findRecr.y;
				break;
			}
			findRecr.y += step;
		}
		break;

	default:
		std::cout << "****** Error case mode ******" << endl;
		break;

	}


	std::cout << "finish findboundary~" << endl;
	std::cout << "fi";
	return BoundaryVal;
}

# pragma endregion

#pragma region chipalgorithm


#pragma region STEP1_roughlysearch 

std::tuple<Point, int> potentialchipSearch_V1(Mat cropedRImg, double resizeTDwidth, double resizeTDheight, sizeTD target, int thresmode, int flag, double theta)
{
	Point potentialchip = Point(0, 0);


	/*sub-function start*/
			//Input: cropedRImg

	Mat gauBGR, EnHBGR;
	cv::cvtColor(cropedRImg, cropedRImg, cv::COLOR_BGR2GRAY);
	cropedRImg.convertTo(cropedRImg, -1, 1.2, 0);
	cv::GaussianBlur(cropedRImg, gauBGR, Size(0, 0), 13);
	cv::addWeighted(cropedRImg, 1.5, gauBGR, -0.7, 0.0, EnHBGR); //(1.5, -0.7)

	double minVal, maxVal; //maxVal: frequency
	Point minLoc, maxLoc; //maxLoc.y: pixel value
	minMaxLoc(EnHBGR, &minVal, &maxVal, &minLoc, &maxLoc);
	//std::cout << "calculate min Loc is:: " << minLoc.y << " / " << maxLoc.y << " / " << minVal << " / " << maxVal << endl;

	Mat thresimg;
	if (thresmode == 3)
	{
		threshold(EnHBGR, thresimg, int(0.4 * (maxVal - minVal) + minVal), 255, THRESH_BINARY_INV);
	}
	else if (thresmode == 4)
	{
		threshold(EnHBGR, thresimg, int(0.4 * (maxVal - minVal) + minVal), 255, THRESH_BINARY);
	}
	else //default =3
	{
		threshold(EnHBGR, thresimg, int(0.4 * (maxVal - minVal) + minVal), 255, THRESH_BINARY_INV);
	}


	vector<vector<Point>> contthres;
	vector<vector<Point>> potCNT;
	vector<Point2f> potPT;
	vector<double>potDist;

	Mat Kcomclose = Mat::ones(Size(3, 3), CV_8UC1);
	Mat upscaleImg = Mat::zeros(thresimg.size(), CV_8UC1);
	Mat potentialIMG = Mat::zeros(thresimg.size(), CV_8UC1);
	cv::morphologyEx(thresimg, thresimg, cv::MORPH_CLOSE, Kcomclose, Point(-1, -1), 1);
	cv::morphologyEx(thresimg, thresimg, cv::MORPH_OPEN, Kcomclose, Point(-1, -1), 3);
	//cv::medianBlur(thresimg, thresimg, 7);




	Mat thres2;
	Mat thresresult = Mat::zeros(thresimg.size(), CV_8UC1);
	threshold(EnHBGR, thres2, int(minVal + 15), 255, THRESH_BINARY_INV);
	vector<vector<Point>> thres2cnt;
	cv::medianBlur(thres2, thres2, 3);
	cv::morphologyEx(thres2, thres2, cv::MORPH_DILATE, Kcomclose, Point(-1, -1), 1);
	thres2.copyTo(thresresult);
	cv::findContours(thres2, thres2cnt, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point());
	for (int i = 0; i < thres2cnt.size(); i++)
	{
		RotatedRect bangle = cv::minAreaRect(thres2cnt[i]);
		double bxangle = 0;
		if (abs(bangle.angle) > 45)
		{
			bxangle = abs(bangle.angle - 90);
		}
		else
		{
			bxangle = (abs(bangle.angle));
		}
		if (bxangle > theta) //theta=3
		{
			cv::drawContours(thresresult, thres2cnt, i, Scalar(0, 0, 0), -1);
			cv::drawContours(thresimg, thres2cnt, i, Scalar(0, 0, 0), -1);
		}


	}




	cv::findContours(thresimg, contthres, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point());
	Point2f piccenter = find_piccenter(thresimg);

	try
	{
		if (contthres.size() == 0)
		{
			flag = 1;
			throw "something wrong::threshold value issue";
		}
		else
		{


			for (int i = 0; i < contthres.size(); i++)
			{
				Rect bx = boundingRect(contthres[i]);

				RotatedRect boxReqcont = cv::minAreaRect(contthres[i]);
				double bxangle = 0;
				if (abs(boxReqcont.angle) > 45)
				{
					bxangle = abs(boxReqcont.angle - 90);
				}
				else
				{
					bxangle = (abs(boxReqcont.angle));
				}
				//cout << "check angle is :" << bxangle <<" / boxReqcont.angle" << boxReqcont.angle << endl;														
				if (bx.width * bx.height< (resizeTDwidth * resizeTDheight * 1.4) &&
					bx.width * bx.height >(resizeTDwidth * resizeTDheight * 0.6) &&
					bxangle < theta) //theta=3
				{
					//cout << "check angle is :" << bxangle << endl;					

					Moments M = (moments(contthres[i], false));
					Point2f Mpt = (Point2f((M.m10 / M.m00), (M.m01 / M.m00)));



					potCNT.push_back(contthres[i]);
					potPT.push_back(Mpt);
					potDist.push_back((norm(piccenter - potPT[potPT.size() - 1])));
					cv::drawContours(potentialIMG, contthres, i, Scalar::all(255), -1);




					//cout << "check potential center"<<endl;
				}
				/*else if (bxangle > 3)
				{
					cv::drawContours(cropedRImg, contthres, i, Scalar::all(180), 2);
				}*/
			}

			auto chipiter = std::min_element(potDist.begin(), potDist.end());
			int chipIndex = std::distance(potDist.begin(), chipiter);

			//cout << "check potential center is : " << potPT[chipIndex]  << "[ "<< piccenter<<" ]" << endl;

			circle(upscaleImg, potPT[chipIndex], 3, Scalar::all(255), -1);
			circle(thresimg, piccenter, 1, Scalar::all(150), -1);
			cv::resize(upscaleImg, upscaleImg, Size(int(thresimg.cols * 12), int(thresimg.rows * 12)), INTER_NEAREST);

			cv::findContours(upscaleImg, contthres, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point());

			Moments M1 = (moments(contthres[0], false));
			potentialchip = Point2i((Point2f((M1.m10 / M1.m00), (M1.m01 / M1.m00))));
			//cout << "check tdpt(full img) is : " << potentialchip << endl;

			flag = 0;


		}

	}

	catch (const char* message)
	{
		std::cout << message << std::endl;

	}

	Kcomclose.release();
	return{ potentialchip,flag };
}

#pragma endregion STEP1_roughlysearch 



#pragma region STEP2_ROIfineDefine 


std::tuple<Point, int, Mat, Mat, Rect> FinechipDefine_V1(Mat rawimg, sizeTD target, thresP thresParm, int boolflag, Point Potchip, SettingP chipsetting)
{
	Point finechip = Point(0, 0);
	Point IMGoffset = Point(0, 0);
	Mat Grayimg, markimg;
	rawimg.copyTo(markimg);
	/*Automatically crop image via pitch setting---> use chip dimension*/



	IMGoffset.x = Potchip.x - int(chipsetting.xpitch[0] * 0.8);
	IMGoffset.y = Potchip.y - int(chipsetting.ypitch[0] * 0.8);
	Rect Cregion(IMGoffset.x, IMGoffset.y, int(chipsetting.xpitch[0] * 0.8) * 2, int(chipsetting.ypitch[0] * 0.8 * 2));


	Mat cropedrawimg = CropIMG(rawimg, Cregion);
	Mat cropedRImg;

	cvtColor(cropedrawimg, cropedRImg, COLOR_BGR2GRAY);

cv:rectangle(markimg, Cregion, Scalar(0, 0, 255), 2);


	Mat medimg, adptThres, comthresIMG;

	cv::medianBlur(cropedRImg, medimg, 5);
	int adaptWsize = 3;
	int adaptKsize = 2;
	if (thresParm.thresmode == 3)
	{
		if (thresParm.bgmax[0] & 1)
		{
			adaptWsize = thresParm.bgmax[0];
			adaptKsize = thresParm.fgmax[0];
		}
		else
		{
			adaptWsize = thresParm.bgmax[0] + 1;
			adaptKsize = thresParm.fgmax[0];
		}
		adaptiveThreshold(medimg, adptThres, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, adaptWsize, adaptKsize);//55,1 //ADAPTIVE_THRESH_MEAN_C


		Mat Kcomclose = Mat::ones(Size(5, 5), CV_8UC1);  //Size(10,5)
		cv::morphologyEx(adptThres, comthresIMG, cv::MORPH_CLOSE, Kcomclose, Point(-1, -1), 1);//1 //2
	}

	else if (thresParm.thresmode == 4)
	{
		if (thresParm.bgmax[0] & 1)
		{
			adaptWsize = thresParm.bgmax[0];
			adaptKsize = thresParm.fgmax[0];
		}
		else
		{
			adaptWsize = thresParm.bgmax[0] + 1;
			adaptKsize = thresParm.fgmax[0];
		}
		adaptiveThreshold(medimg, adptThres, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, adaptWsize, adaptKsize);//55,1 //ADAPTIVE_THRESH_MEAN_C
		//cv::medianBlur(adptThres, comthresIMG, 7);
		//Mat Kcomclose = Mat::ones(Size(5, 5), CV_8UC1);  //Size(10,5)
		//cv::morphologyEx(adptThres, comthresIMG, cv::MORPH_CLOSE, Kcomclose, Point(-1, -1), 1);//1 //2
	}

	vector<vector<Point>>  contH, REQcont;
	vector<Rect> Rectlist;
	vector<Point2f> center;
	vector<double> distance;
	vector<double> approxList;
	Point2f piccenter;
	int minIndex;
	vector<Point> approx;
	Point crossCenter, centerTD;
	Rect drawrect;
	Rect fineRect;
	Mat Reqcomthres = Mat::zeros(medimg.size(), CV_8UC1);

	cv::findContours(adptThres, contH, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point());
	try
	{

		if (contH.size() == 0)
		{
			boolflag = 1;
			throw "something wrong::threshold value issue";
		}
		else

		{
			cv::resize(adptThres, Grayimg, Size(500, 600), INTER_NEAREST);

			for (int i = 0; i < contH.size(); i++)
			{

				Rect retCOMP = cv::boundingRect(contH[i]);

				/* add angle rotation limitation code::::::::::::::::::::::*/



				/*cout << "check retCOMP: " << retCOMP << " / " << target.TDwidth * target.TDminW << "/ / / / " << target.TDheight * target.TDminH << endl;
				cout << target.TDwidth * target.TDmaxW << "/ / / / " << target.TDheight * target.TDmaxH << endl;*/
				cv::approxPolyDP(contH[i], approx, 15, true); //30,15
				if (retCOMP.width > target.TDwidth * target.TDminW
					&& retCOMP.height > target.TDheight * target.TDminH
					&& retCOMP.width < target.TDwidth * target.TDmaxW
					&& retCOMP.height < target.TDheight * target.TDmaxH
					)

				{
					Moments M = (moments(contH[i], false));
					center.push_back((Point2f((M.m10 / M.m00), (M.m01 / M.m00))));
					piccenter = find_piccenter(comthresIMG);
					distance.push_back(norm(Point2f(Potchip) - center[center.size() - 1])); // get Euclidian distance
					Rectlist.push_back(retCOMP);
					approxList.push_back(approx.size());
					REQcont.push_back(contH[i]);
					cv::rectangle(medimg, retCOMP, Scalar::all(180), 4);



				}

			} //for-loop: contours



			if (center.size() == 0)
			{
				boolflag = 2;
				throw "something wrong::potential object doesn't fit suitable dimension";
			}
			else
			{


				//Find a LED coordinate with the shortest distance to the pic center
				auto it = std::min_element(distance.begin(), distance.end());
				minIndex = std::distance(distance.begin(), it);
				//minvalue = *it;

				cv::drawContours(Reqcomthres, REQcont, minIndex, Scalar(255, 255, 255), -1);

				//std::wcout << "check approx  size main: " << approxList[minIndex] << endl;


				std::cout << "start fine define...." << endl;
				cv::drawContours(cropedrawimg, REQcont, minIndex, Scalar(180, 180, 180), 2);

				std::tie(fineRect, centerTD) = FindMaxInnerRect(Reqcomthres, cropedrawimg, target, center[minIndex]);

				crossCenter = Point2f(centerTD) + Point2f(chipsetting.carx, chipsetting.cary);
				drawrect = fineRect;




				cv::circle(cropedrawimg,
					(Point2i(crossCenter)), //coordinate
					6, //radius
					Scalar(255, 0, 255),  //color
					FILLED,
					LINE_AA);

				cv::rectangle(markimg, Rect(drawrect.x + IMGoffset.x, drawrect.y + IMGoffset.y, drawrect.width, drawrect.height), cv::Scalar(255, 0, 0), 8);
				/*cv::line(marksize, Point(0, crossCenter.y - chipsetting.cary), Point(marksize.size[1], crossCenter.y - chipsetting.cary), Scalar(255, 255, 255), 1, 8);
				cv::line(marksize, Point(crossCenter.x - chipsetting.carx, 0), Point(crossCenter.x - chipsetting.carx, marksize.size[0]), Scalar(255, 255, 255), 1, 8);*/



				cv::line(cropedrawimg, Point(0, crossCenter.y), Point(cropedrawimg.size[1], crossCenter.y), Scalar(0, 0, 255), 1, 8);
				cv::line(cropedrawimg, Point(crossCenter.x, 0), Point(crossCenter.x, cropedrawimg.size[0]), Scalar(0, 0, 255), 1, 8);

				boolflag = 0;
				finechip = crossCenter + IMGoffset;

				cv::line(markimg, Point(0, finechip.y), Point(rawimg.size[1], finechip.y), Scalar(0, 0, 255), 1, 8);
				cv::line(markimg, Point(finechip.x, 0), Point(finechip.x, rawimg.size[0]), Scalar(0, 0, 255), 1, 8);


				std::cout << "check chip crossCenternew is: [ " << finechip << " ]" << endl;
				std::cout << "-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*" << endl;


			}
		}
	}
	catch (const char* message)
	{
		std::cout << message << std::endl;

	}




	/*Uchip:fine_search:AOI*/

	//cout << "step 2 finish..." << endl;

	return{ finechip,boolflag ,Grayimg,markimg,Rect(drawrect.x + IMGoffset.x, drawrect.y + IMGoffset.y, drawrect.width, drawrect.height) };
}


#pragma endregion STEP2_ROIfineDefine 



#pragma region STEP3_Simulatecoord 




std::tuple<Point, Mat, int>SimulateCoord_V1(Mat rawimg, Point piccenter, Point Finechip, int boolflag, SettingP chipsetting, Rect fineRect)
{
	Point simucoord = Point(0, 0);
	Mat markimg, Resized_markimg;

	rawimg.copyTo(markimg);


	cv::circle(markimg, piccenter, 5, Scalar(255, 0, 0), -1); //blue=piccenter
	cv::circle(markimg, Finechip, 5, Scalar(255, 0, 255), -1);
	cv::rectangle(markimg, fineRect, Scalar(0, 0, 255), 5);

	cv::line(markimg, Point(0, Finechip.y), Point(markimg.size[1], Finechip.y), Scalar(255, 0, 0), 5, 8);
	cv::line(markimg, Point(Finechip.x, 0), Point(Finechip.x, markimg.size[0]), Scalar(255, 0, 0), 5, 8);

	Point searchdirection = Point(1, 1);
	Point iniPt = Finechip;
	Point FinalPt = Finechip;

	double Xdist = abs(piccenter.x - Finechip.x);
	double Ydist = abs(piccenter.y - Finechip.y);
	double bdratio = 0.5;
	/*std::cout << "Ini....current X and Y distance are::  " << Xdist << " , " << Ydist<<" , pitchX: "<< chipsetting.xpitch[0] << " , pitchY: " << chipsetting.ypitch[0] << endl;
	std::cout << "Ini....fineRect::  " << fineRect << endl;*/

	if (piccenter.x > Finechip.x && Xdist > fineRect.width * bdratio)
	{
		searchdirection.x = 1;
	}
	else if (piccenter.x < Finechip.x && Xdist > fineRect.width * bdratio)
	{
		searchdirection.x = -1;
	}
	else { searchdirection.x = 0; }


	if (piccenter.y > Finechip.y && Ydist > fineRect.height * bdratio)
	{

		searchdirection.y = 1;
	}
	else if (piccenter.y < Finechip.y && Ydist>fineRect.height * bdratio)
	{
		searchdirection.y = -1;
	}
	else { searchdirection.y = 0; }

	int currentXpos = Finechip.x;
	int currentYpos = Finechip.y;

	//std::cout << "sear direction is::  " << searchdirection << endl << endl;




	while (true)
	{
		if (Xdist < chipsetting.xpitch[0] * 0.7) //0.7=lower than one pitch
		{
			FinalPt.x = iniPt.x;
			break;
		}
		else
		{
			iniPt.x = iniPt.x + searchdirection.x * chipsetting.xpitch[0];
			Xdist = abs(piccenter.x - iniPt.x);
			//cout << "new Xdist= " << Xdist << " x pos= " << iniPt.x << endl;
		}

		if (iniPt.x< chipsetting.xpitch[0] || iniPt.x>rawimg.cols - chipsetting.xpitch[0])
		{
			boolflag = 4;
			break;
		}



	}

	while (true)
	{
		if (Ydist < chipsetting.ypitch[0] * 0.7) //0.7=lower than one pitch
		{
			FinalPt.y = iniPt.y;
			break;
		}
		else
		{
			iniPt.y = iniPt.y + searchdirection.y * chipsetting.ypitch[0];
			Ydist = abs(piccenter.y - iniPt.y);
			//cout << "new Ydist= " << Ydist << " y pos= " << iniPt.y << endl;
		}

		if (iniPt.y< chipsetting.ypitch[0] || iniPt.y>rawimg.rows - chipsetting.ypitch[0])
		{
			boolflag = 4;
			break;
		}
	}



	if (boolflag == 0)
	{
		cv::circle(markimg, FinalPt, 5, Scalar(0, 0, 255), -1); //blue=piccenter


		cv::rectangle(markimg, Rect(FinalPt.x - 0.5 * fineRect.width, FinalPt.y - 0.5 * fineRect.height, fineRect.width, fineRect.height),
			Scalar(255, 133, 170), 5);
		std::cout << "Fin.. chip pos is: " << FinalPt << endl;

		cv::line(markimg, Point(0, FinalPt.y), Point(markimg.size[1], FinalPt.y), Scalar(255, 133, 170), 5, 8);
		cv::line(markimg, Point(FinalPt.x, 0), Point(FinalPt.x, markimg.size[0]), Scalar(255, 133, 170), 5, 8);

		boolflag = 9;


	}
	else
	{
		FinalPt = Point(0, 0);
	}

	cv::resize(markimg, Resized_markimg, Size(1100, 800), INTER_LINEAR);


	cout << "step 3 finish..." << endl;

	return{ FinalPt,Resized_markimg,boolflag };

}



#pragma endregion STEP3_Simulatecoord 














#pragma endregion chipalgorithm






