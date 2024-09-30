#include "secAreaChipCP_lib.h"
#include "OpenCV_Extension_Tool.h"


#pragma region STEP1_roughlysearch 

std::tuple<Point, int> potentialchipSearch_V1(Mat cropedRImg, double resizeTDwidth, double resizeTDheight, sizeTD_ target, int thresmode, int flag, double theta,Point2f creteriaPoint)
{
	Point potentialchip = Point(0, 0);
	float ratio = 2;

	Mat gauBGR, EnHBGR;
	cv::cvtColor(cropedRImg, cropedRImg, cv::COLOR_BGR2GRAY);
	cropedRImg.convertTo(cropedRImg, -1, 1.2, 0);
	cv::GaussianBlur(cropedRImg, gauBGR, Size(0, 0), 13);
	cv::addWeighted(cropedRImg, 1.5, gauBGR, -0.7, 0.0, EnHBGR); //(1.5, -0.7)

	double minVal, maxVal; //maxVal: frequency
	Point minLoc, maxLoc; //maxLoc.y: pixel value
	minMaxLoc(EnHBGR, &minVal, &maxVal, &minLoc, &maxLoc);

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

	//Mat Kcomclose = Mat::ones(Size(3, 3), CV_8UC1);
	//cv::morphologyEx(thresimg, thresimg, cv::MORPH_CLOSE, Kcomclose, Point(-1, -1), 3);
	//cv::morphologyEx(thresimg, thresimg, cv::MORPH_OPEN, Kcomclose, Point(-1, -1), 3);
	
	vector<BlobInfo> vRegions = RegionPartitionTopology(thresimg);

	Point2f piccenter = Point2f(creteriaPoint.x/ ratio, creteriaPoint.y / ratio);

	try
	{
		if (vRegions.size() == 0)
		{
			flag = 1;
			throw "something wrong::threshold value issue";
		}
		else
		{
			vector<BlobInfo> vChipPossible;
			vector<vector<Point>> vContour;

			for (int i = 0; i < vRegions.size(); i++)
			{	
				double bxangle = 0;
			
				if (vRegions[i].Rectangularity() < 0.7)
					continue;

				if (vRegions[i].Angle() > 45)
					bxangle = abs(vRegions[i].Angle() - 90);
				else
					bxangle = (abs(vRegions[i].Angle()));

				if (vRegions[i].Width()* vRegions[i].Height() < (resizeTDwidth * resizeTDheight *1.4) &&
					vRegions[i].Width() * vRegions[i].Height() > (resizeTDwidth * resizeTDheight * 0.6) &&
					bxangle <theta) //theta=3
				{
					vChipPossible.push_back(vRegions[i]);
					vContour.push_back(vRegions[i].contourMain());
				}

			}

			//Mat debug = Mat(cropedRImg.size(), CV_8UC1);

			//drawContours(debug, vContour, -1, Scalar(255, 255, 255), -1);


			std::sort(vChipPossible.begin(), vChipPossible.end(), [&, piccenter](BlobInfo& a, BlobInfo& b)
				{
					norm(a.Center() - piccenter);
					return norm(a.Center() - piccenter) < norm(b.Center() - piccenter);
				});


			if (vChipPossible.size() > 0)
			{
				potentialchip = Point2i((Point2f(vChipPossible[0].Center().x* ratio, vChipPossible[0].Center().y * ratio)));

				flag = 0;
			}
			else
			{
				flag = 2;
			}
			/*
			如果是要縮小圖片的話，通常 INTER_AREA 使用效果較佳。
			如果是要放大圖片的話，通常 INTER_CUBIC 使用效果較佳，次等則是 INTER_LINEAR。
			如果要追求速度的話，通常使用 INTER_NEAREST。
			*/
		}

	}
	catch (const char* message)
	{
		std::cout << message << std::endl;
	}

	cropedRImg.release();
	EnHBGR.release();
	gauBGR.release();
	//Kcomclose.release();
	thresimg.release();
	return{ potentialchip,flag };
}

#pragma endregion STEP1_roughlysearch 



#pragma region STEP2_ROIfineDefine 


std::tuple<Point, int,Mat,Mat,Rect> FinechipDefine_V1(Mat rawimg, sizeTD_ target, thresP_ thresParm, int boolflag, Point Potchip, SettingP_ chipsetting)
{
	Point finechip = Point(0, 0);
	Point IMGoffset = Point(0, 0);
	Mat Grayimg, markimg;
	rawimg.copyTo(markimg);
	/*Automatically crop image via pitch setting---> use chip dimension*/

	IMGoffset.x = Potchip.x - int(chipsetting.xpitch[0]*0.8);
	IMGoffset.y = Potchip.y - int(chipsetting.ypitch[0] * 0.8);
	Rect Cregion(IMGoffset.x, IMGoffset.y, int(chipsetting.xpitch[0] * 0.8) * 2, int(chipsetting.ypitch[0] * 0.8 * 2));
	
	Mat cropedrawimg = CropIMG(rawimg, Cregion);
	Mat cropedRImg;

	cvtColor(cropedrawimg, cropedRImg, COLOR_BGR2GRAY);
	cv:rectangle(markimg, Cregion, Scalar(0, 0, 255), 2);
	Mat medimg, adptThres, comthresIMG;

	Mat gauBGR, EnHBGR,FNDimg;
	cropedRImg.convertTo(cropedRImg, -1, 1.2, 0);
	cv::GaussianBlur(cropedRImg, gauBGR, Size(0, 0), 13);
	cv::addWeighted(cropedRImg, 1.5, gauBGR, -0.7, 0.0, EnHBGR); //(1.5, -0.7)
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
		Mat Kcomclose = Mat::ones(Size(5, 5), CV_8UC1);  //Size(10,5)
		cv::morphologyEx(adptThres, comthresIMG, cv::MORPH_CLOSE, Kcomclose, Point(-1, -1), 1);//1 //2
	}

	adptThres.release();

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

	int max = target.TDwidth * target.TDheight * target.TDmaxH * target.TDmaxW;
	int min = target.TDwidth * target.TDheight * target.TDminH * target.TDminW;
	vector<BlobInfo> vRegions = RegionPartition(comthresIMG,max,min);

	try
	{
		if (vRegions.size() == 0)
		{
			boolflag = 1;
			throw "something wrong::threshold value issue";
		}
		else			
		{
			cv::resize(comthresIMG, Grayimg, Size(500, 600), INTER_NEAREST);
			vector<BlobInfo> vChipsPossible;
			piccenter = find_piccenter(comthresIMG);

			for (int i = 0; i < vRegions.size(); i++)
			{
				if (vRegions[i].Width() > target.TDwidth * target.TDminW
					&& vRegions[i].Height() > target.TDheight * target.TDminH
					&& vRegions[i].Width() < target.TDwidth * target.TDmaxW
					&& vRegions[i].Height() < target.TDheight * target.TDmaxH
					)
				{
					vChipsPossible.push_back(vRegions[i]);
					cv::rectangle(medimg, Rect(vRegions[i].Xmin(), vRegions[i].Ymin(), vRegions[i].Width(), vRegions[i].Height()), Scalar::all(180), 4);
				}

			} //for-loop: contours

			std::sort(vChipsPossible.begin(), vChipsPossible.end(), [&, piccenter](BlobInfo& a, BlobInfo& b)
				{
					norm(a.Center() - piccenter);
					return norm(a.Center() - piccenter) < norm(b.Center() - piccenter);
				});


			if (vChipsPossible.size() == 0)
			{
				boolflag = 2;
				throw "something wrong::potential object doesn't fit suitable dimension";
			}
			else
			{
				crossCenter = vChipsPossible[0].Center();// +Point2f(chipsetting.carx, chipsetting.cary);
				drawrect = Rect(vChipsPossible[0].Xmin(), vChipsPossible[0].Ymin(), vChipsPossible[0].Width(), vChipsPossible[0].Height());

				cv::circle(cropedrawimg,
					(Point2i(crossCenter)), //coordinate
					6, //radius
					Scalar(255, 0, 255),  //color
					FILLED,
					LINE_AA);

				cv::rectangle(markimg, Rect(drawrect.x+ IMGoffset.x, drawrect.y + IMGoffset.y, drawrect.width, drawrect.height), cv::Scalar(255, 0, 0), 8);			
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
	
	medimg.release();
	comthresIMG.release();
	cropedrawimg.release();
	cropedRImg.release();
	EnHBGR.release();
	gauBGR.release();
	rawimg.release();
	return{ finechip,boolflag ,Grayimg,markimg,Rect(drawrect.x + IMGoffset.x, drawrect.y + IMGoffset.y, drawrect.width, drawrect.height) };
}


#pragma endregion STEP2_ROIfineDefine 



#pragma region STEP3_Simulatecoord 

std::tuple<Point, Mat, int>SimulateCoord_V1(Mat rawimg, Point piccenter, Point Finechip, int boolflag, SettingP_ chipsetting, Rect fineRect)
{
	Point simucoord = Point(0, 0);
	Mat markimg, Resized_markimg;
	
	rawimg.copyTo(markimg);
	//std::cout << "Ini.. chip pos is: " << Finechip << endl;
	//piccenter.x = piccenter.x + 4 * chipsetting.xpitch[0];

	cv::circle(markimg, piccenter, 5, Scalar(255, 0, 0), -1); //blue=piccenter
	cv::circle(markimg, Finechip, 5, Scalar(255, 0, 255), -1);
	cv::rectangle(markimg, fineRect, Scalar(0, 0, 255), 5);

	cv::line(markimg, Point(0, Finechip.y), Point(markimg.size[1], Finechip.y), Scalar(255, 0, 0), 5, 8);
	cv::line(markimg, Point(Finechip.x, 0), Point(Finechip.x, markimg.size[0]), Scalar(255, 0, 0), 5, 8);

	Point searchdirection = Point(1, 1);
	Point iniPt = Finechip;
	Point FinalPt = Finechip;

	double Xdist = abs(piccenter.x - Finechip.x);
	double Ydist = abs(piccenter.y-Finechip.y);
	double bdratio = 0.5;
	/*std::cout << "Ini....current X and Y distance are::  " << Xdist << " , " << Ydist<<" , pitchX: "<< chipsetting.xpitch[0] << " , pitchY: " << chipsetting.ypitch[0] << endl;
	std::cout << "Ini....fineRect::  " << fineRect << endl;*/
	
	if (piccenter.x > Finechip.x && Xdist > fineRect.width* bdratio)
	{
		searchdirection.x = 1;
	}
	else if (piccenter.x < Finechip.x && Xdist > fineRect.width* bdratio)
	{
		searchdirection.x = -1;
	}
	else{ searchdirection.x = 0; }


	if (piccenter.y > Finechip.y && Ydist > fineRect.height* bdratio) 
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



	/*while 找太久: bool result=error code!!*/
	while (true)
	{
		if(Xdist < chipsetting.xpitch[0]*0.7) //0.7=lower than one pitch
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
			boolflag =4 ;
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
	
	/*chipsetting.xpitch[0] = 199;
	chipsetting.ypitch[0] = 293;*/

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