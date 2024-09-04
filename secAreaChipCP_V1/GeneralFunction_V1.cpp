
#include "secAreaChipCP_lib.h"

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




#pragma region General_Unused_function


Mat RotatecorrectImg(double Rtheta, Mat src)
{
	Point2f center((src.cols - 1) / 2.0, (src.rows - 1) / 2.0);

	// using getRotationMatrix2D() to get the rotation matrix
	Mat rotation_matix = getRotationMatrix2D(center, Rtheta, 1.0);
	//angle:: ++ mean counter-clockwise rotation (the coordinate origin is assumed to be the top-left corner)

	// save the resulting image in rotated_image matrix
	Mat rotated_image;
	// rotate the image using warpAffine
	warpAffine(src, rotated_image, rotation_matix, src.size());
	Mat patchrotaIMG;
	rotated_image.copyTo(patchrotaIMG);

	if (src.channels() == 1)
	{
		for (int y = 0; y < rotated_image.rows; y++)
		{
			for (int x = 0; x < rotated_image.cols; x++)
			{
				if (rotated_image.at<uchar>(Point(x, y)) == float(0))
				{
					patchrotaIMG.at<uchar>(Point(x, y)) = float(255);
				}
			}
		}
	}
	else
	{
		for (int y = 0; y < rotated_image.rows; y++)
		{
			for (int x = 0; x < rotated_image.cols; x++)
			{
				if (rotated_image.at<Vec3b>(Point(x, y)) == Vec3b(0, 0, 0))
				{
					patchrotaIMG.at<Vec3b>(Point(x, y)) = Vec3b(255, 255, 255);
				}
			}
		}
	}




	return patchrotaIMG;
}

std::tuple<Mat, Mat, Mat>Histplotting(Mat src, int hist_w, int hist_h, int histsize)
{
	int imgnumber = 1;
	int imgdimension = 1;
	//int histsize = 256;
	float grayrange[2] = { 0,256 };
	imgdimension = 1;
	/*int hist_w = 256;
	int hist_h = 300;*/
	int bin_w = hist_w / histsize;
	Mat histplot(hist_h, hist_w, CV_8UC3, Scalar(0, 0, 0));
	const float* grayhistrange = { grayrange };
	Mat histImg;
	vector<int> nonzerolist;
	vector<int> slopelist;
	Mat maskthres, histmsk, histmskplot;
	Point maxLocgau, maxLoc;
	Mat threslow2;
	//cv::calcHist(&sobelbitor, 1, 0, cv::Mat(), histImg, imgdimension, &histsize, &grayhistrange, true, false);
	cv::calcHist(&src, 1, 0, cv::Mat(), histImg, imgdimension, &histsize, &grayhistrange, true, false);

	histImg.copyTo(histmskplot);
	cv::normalize(histmskplot, histmskplot, 0, hist_h, NORM_MINMAX, -1, Mat());


	//Mat sorthistmak = std::sort(histmskplot.begin(), histmskplot.end());


	for (int i = 1; i < histsize; i++)
	{
		cv::line(histplot, Point((i - 1) * bin_w, hist_h - cvRound(histmskplot.at<float>(i - 1))),
			Point(i * bin_w, hist_h - -cvRound(histmskplot.at<float>(i))), Scalar(255, 0, 0), 2, 8, 0);
	}

	return{ histImg,histmskplot,histplot };
}

void gammaCorrection(const Mat& src, Mat& dst, const float gamma)
{
	float invGamma = 1 / gamma;

	Mat table(1, 256, CV_8U);
	uchar* p = table.ptr();
	for (int i = 0; i < 256; ++i) {
		p[i] = (uchar)(pow(i / 255.0, invGamma) * 255);
	}

	LUT(src, table, dst);
}

Mat KmeanOP(int knum, Mat src)
{
	TermCriteria criteria;
	Mat labels, centeres;
	Mat pixelVal, segmentedIMG;

	//define stopping criteria
	criteria = TermCriteria(cv::TermCriteria::EPS + TermCriteria::MAX_ITER, 100, 0.2);
	int attempts = 10;
	int KmeanFlag = cv::KMEANS_RANDOM_CENTERS;



	Mat samples(src.rows * src.cols, src.channels(), CV_32F); //change to float

	Mat clonesample;


	for (int y = 0; y < src.rows; y++)
	{
		for (int x = 0; x < src.cols; x++)
		{
			for (int z = 0; z < src.channels(); z++)
			{
				if (src.channels() == 3)
				{
					samples.at<float>(y + x * src.rows, z) = src.at<Vec3b>(y, x)[z];
				}

				else
				{
					samples.at<float>(y + x * src.rows, z) = src.at<uchar>(y, x); //for gray img
				}

			}
		}
	}



	cv::kmeans(samples, knum, labels, criteria, attempts, KmeanFlag, centeres);

	Mat NewIMG(src.size(), src.type()); //CV8U3

	for (int y = 0; y < src.rows; y++)
	{
		for (int x = 0; x < src.cols; x++)
		{
			int cluster_idx = labels.at<int>(y + x * src.rows, 0);
			if (src.channels() == 3)
			{
				for (int z = 0; z < src.channels(); z++) //3 channels
				{
					NewIMG.at<Vec3b>(y, x)[z] = (centeres.at<float>(cluster_idx, z)); //CV32F


				}
			}
			else
			{
				NewIMG.at<uchar>(y, x) = centeres.at<float>(cluster_idx, 0); //for gray img
			}


		}
	}



	//std::cout << "finish Kop" << endl;

	return NewIMG;
}

std::tuple<int, Point_<int>> FindMF_pixel(Mat histImg)
{
	double minVal, maxVal; //maxVal: frequency
	Point minLoc, maxLoc; //maxLoc.y: pixel value
	minMaxLoc(histImg, &minVal, &maxVal, &minLoc, &maxLoc);
	return { maxVal,maxLoc };
}




# pragma endregion General_Unused_function