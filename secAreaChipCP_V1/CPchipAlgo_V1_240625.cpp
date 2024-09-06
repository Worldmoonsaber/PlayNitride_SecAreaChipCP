#include "secAreaChipCP_lib.h"


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
	Mat thresresult= Mat::zeros(thresimg.size(), CV_8UC1);
	threshold(EnHBGR, thres2, int(minVal+15), 255, THRESH_BINARY_INV);
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
					bxangle=abs(boxReqcont.angle - 90);
				}
				else
				{
					bxangle =(abs(boxReqcont.angle));
				}
				//cout << "check angle is :" << bxangle <<" / boxReqcont.angle" << boxReqcont.angle << endl;														
				if (bx.width * bx.height< (resizeTDwidth * resizeTDheight *1.4) &&
					bx.width * bx.height > (resizeTDwidth * resizeTDheight * 0.6)&&
					bxangle< theta) //theta=3
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
			potentialchip =Point2i((Point2f((M1.m10 / M1.m00), (M1.m01 / M1.m00))));
			//cout << "check tdpt(full img) is : " << potentialchip << endl;

			flag = 0;

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

	Kcomclose.release();
	return{ potentialchip,flag };
}

#pragma endregion STEP1_roughlysearch 



#pragma region STEP2_ROIfineDefine 


std::tuple<Point, int,Mat,Mat,Rect> FinechipDefine_V1(Mat rawimg, sizeTD target, thresP thresParm, int boolflag, Point Potchip, SettingP chipsetting)
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

	//Mat gauBGR, EnHBGR,FNDimg;
	Mat medimg, adptThres, comthresIMG;
	//cropedRImg.convertTo(cropedRImg, -1, 1.2, 0);
	//cv::GaussianBlur(cropedRImg, gauBGR, Size(0, 0), 13);
	//cv::addWeighted(cropedRImg, 1.5, gauBGR, -0.7, 0.0, EnHBGR); //(1.5, -0.7)
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

		//cv::medianBlur(adptThres, comthresIMG, 7);
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

	cv::findContours(adptThres, contH,cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point());
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

				cv::rectangle(markimg, Rect(drawrect.x+ IMGoffset.x, drawrect.y + IMGoffset.y, drawrect.width, drawrect.height), cv::Scalar(255, 0, 0), 8);
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