


#include "secAreaChipCP_lib.h"



int main()
{
	SettingP_ chipsetting;
	thresP_ thresParm;
	ImgP_ imageParm;
	sizeTD_ target;



	imageParm.cols = 5320; //800 ;900-1600
	imageParm.rows = 4600;

	chipsetting.interval[0] = 0; 
	//chipsetting.interval[1] = 176; //490
	//chipsetting.interval[2] = 114; //273 


	imageParm.Outputmode = 0; //0:center coord ; 1: multiple mode
	imageParm.PICmode = 0;  // 0=B or L¡B1=G¡B2=R
	//chipsetting.interval[0] = 0; //2
	
	chipsetting.carx = 1000;
	chipsetting.cary = 3700;

	//Tell AOI how many angles should rotate : positive: counterclockwise   /negative:clockwise
	//imageParm.correctTheta = 2.6; //8280402
	imageParm.correctTheta = 0; // rotate angle=3
	//imageParm.correctTheta = -0.247; //L4_1020
	/////////////////////////////////////////
	Mat rawimg, cropedRImg;
	int picorder;
	Point piccenter;
	Point IMGoffset;
	Point2f creteriaPoint;




	//output parameters::
	Mat ReqIMG, marksize;
	Point simupt;
	int boolflag = 0;//11

	Point Finechip;
	Mat Grayimg, markimg;

	
	Mat markimg_simu;


	//operating mode
	int mode = 1;
	if (mode == 1)
	{
		/*
		// Image source input: IMG format:RGB
		try
		{

			std::tie(picorder, rawimg) = Inputfunction();
			
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
		*/
		/////
		rawimg = imread("C:\\Users\\Playuser\\Downloads\\20240930_154751.bmp");
		picorder = 704001;


		target.TDmaxW = 1.3;
		target.TDminW =0.7;


		target.TDmaxH = 1.3;
		target.TDminH = 0.7;



		if (picorder > 11800 && picorder < 11820)
		{
			
			target.TDwidth = 166;
			target.TDheight = 222;
			thresParm = { 3,{280,99999,99999},{99999,99999,99999} ,{9,99999,99999}, {99999,99999,99999} };//pic24052202
			chipsetting.xpitch[0] = 290;
			chipsetting.ypitch[0] = 290;
		}
		else if (picorder > 11900 && picorder < 11920)
		{
			
			target.TDwidth = 122;
			target.TDheight = 224;
			thresParm = { 3,{335,99999,99999},{99999,99999,99999} ,{9,99999,99999}, {99999,99999,99999} };//pic24052202
			chipsetting.xpitch[0] = 190;
			chipsetting.ypitch[0] = 290;
		}
		else if (picorder > 704000 && picorder < 704011)
		{

			target.TDwidth = 122;
			target.TDheight = 224;
			thresParm = { 3,{335,99999,99999},{99999,99999,99999} ,{9,99999,99999}, {99999,99999,99999} };//pic24052202
			chipsetting.xpitch[0] = 190;
			chipsetting.ypitch[0] = 290;
		}
		else if (picorder > 704100 && picorder < 704111)
		{

			target.TDwidth = 166;
			target.TDheight = 222;
			thresParm = { 3,{280,99999,99999},{99999,99999,99999} ,{9,99999,99999}, {99999,99999,99999} };//pic24052202
			chipsetting.xpitch[0] = 290;
			chipsetting.ypitch[0] = 290;
		}
		
		target.TDwidth = 250;
		target.TDheight = 280;

		chipsetting.xpitch[0] = 300;
		chipsetting.ypitch[0] = 310;
		//{mode,bgmax,bgmin,fgmax,fgmin}
		//thresParm = { 0,{90,99999,99999},{0,99999,99999} ,{255,9,9}, {120,0,0} };//2040




		/*create image::::*/
		//CreateRotImg(rawimg, 8280402,-1*imageParm.correctTheta); //negative:counter-clockwise // positive:clockwise

		if (imageParm.cols!= rawimg.cols || imageParm.rows!= rawimg.rows)
			boolflag = 7;

		if (boolflag == 0)	
			CheckCropImgIsReasonable(rawimg, chipsetting, target, imageParm,boolflag, creteriaPoint);


		//if (boolflag != 0)
		//{
			rawimg.copyTo(markimg_simu);
			//Grayimg = Mat::zeros(Size(600, 500), CV_8UC1);
		//}


		if (boolflag == 0)
		{
			/*Rotate picture::: */
			//if (imageParm.correctTheta != 0)
			//{
			//	rawimg = RotatecorrectImg(imageParm.correctTheta, rawimg);

			//}
			///*rotate end----------------*/

			//creteriaPoint = find_piccenter(rawimg);

			/*****Step.1 roughly search chip:::*/
			/*Resize image to speed up::start*/
			float ratio = 2;

			double resizeTDwidth= target.TDwidth / ratio;
			double resizeTDheight = target.TDheight / ratio;
			std::cout << "calculate resize TD dimension is:: " << resizeTDwidth << " / " << resizeTDheight << endl;
			cv::resize(rawimg, cropedRImg, Size(int(rawimg.cols / ratio), int(rawimg.rows / ratio)), INTER_NEAREST);

			auto t_start2 = std::chrono::high_resolution_clock::now();

			Point Potchip;
			std::tie(Potchip, boolflag) = potentialchipSearch_V1(cropedRImg, resizeTDwidth, resizeTDheight, target, thresParm.thresmode,boolflag, 3, creteriaPoint);
		
			auto t_end2 = std::chrono::high_resolution_clock::now();
			double elapsed_time_ms2 = std::chrono::duration<double, std::milli>(t_end2 - t_start2).count();
			std::cout << "calculate roughly-search-op time is:: " << elapsed_time_ms2 << endl;

			/*Resize image to speed up:: end*/

			if ((Potchip.y< chipsetting.ypitch[0] || Potchip.y>rawimg.rows - chipsetting.ypitch[0]||
				Potchip.x< chipsetting.xpitch[0] || Potchip.x>rawimg.cols - chipsetting.xpitch[0] )&& boolflag == 0)
			{
				rawimg.copyTo(markimg_simu);
				circle(markimg_simu, Potchip, 20, Scalar(0, 0, 255), -1);
				cv::resize(markimg_simu, markimg_simu, Size(1100, 800), INTER_LINEAR);
				simupt = Point(0, 0);
				boolflag = 3;
				//Grayimg = Mat::zeros(Size(600, 500), CV_8UC1);//<---
				/*check pitch or move antother area....*/
			}


			/*****Step.2 automatically crop ROI and fine define chip:::*/
			//intput= Potchip
			//output: chipcenter
			Rect finerect;
		
			if (boolflag == 0)
			{
				std::tie(Finechip, boolflag, Grayimg, markimg, finerect) = FinechipDefine_V1(rawimg, target, thresParm, boolflag, Potchip, chipsetting);

				if (boolflag == 0)
				{
					std::tie(simupt, markimg_simu, boolflag) = SimulateCoord_V1(rawimg, creteriaPoint, Finechip, boolflag, chipsetting, finerect);
				}
			}
			
			else
			{
				//rawimg.copyTo(markimg_simu);
				DrawNG(markimg_simu, thresParm, chipsetting, Grayimg); //Ã¸»sNG ¼v¹³
			}
			

		}




		cv::resize(Grayimg, Grayimg, Size(500, 600), INTER_LINEAR);
		cv::resize(markimg_simu, markimg_simu, Size(1100, 800), INTER_LINEAR);

		std::cout << "check img state:: " << boolflag << endl;
		std::cout << "check center is ::" << simupt << endl;
	}

	

	return 0;
}

























