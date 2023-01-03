#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
//#include <opencv2/imgcodecs.hpp>
//#include <opencv2/imgproc.hpp>

using namespace std;
using namespace cv;

uint32_t g_CodesDetectedTotal=0, g_FrameID=1000;
string *g_QRCodeContent, *g_QRCodeContentLastTime;
Mat *g_SlidingWindow;

static const double g_ThresholdLadder[]=
{
	127,
	125,
	129,
	123,
	131,
	121,
	133,
	119,
	135,
	117,
	137,
	115,
	139,
	113,
	141,
	111,
	143,
	109,
	145,
	107,
	147,
	105,
	149,
	103,
	151,
	101,
	153,
	99,
	155,
	97,
	157,
	95,
	159,
	93,
	161,
	91,
	0,
};

uint PerformDetection(Mat *image, QRCodeDetector *detector, uint32_t window_size=100)
{
	uint32_t wX, wY, step=window_size/4;
	bool something_detected;
	vector<Point2d> regionPoints;
	//uint32_t windowPositionID=0;

//	vector<int> img_compression_params;
//	img_compression_params.push_back(IMWRITE_PNG_COMPRESSION);
//	img_compression_params.push_back(7);

	for(wX=0; wX<image->cols-window_size; wX+=step)
	{
		for(wY=0; wY<image->rows-window_size; wY+=step)
		{
			*g_SlidingWindow=(*image)(Range(wY, wY+window_size), Range(wX, wX+window_size));
			//imwrite(string("./debug/")+to_string(g_FrameID)+string("_")+to_string(windowPositionID)+string(".png"), *g_SlidingWindow, img_compression_params);
			//windowPositionID++;
			something_detected=detector->detect(*g_SlidingWindow, regionPoints);
			if(!something_detected)
			{
				continue;
			}
			if(regionPoints.size()!=4)
			{
				continue;
			}
			if(contourArea(regionPoints)<4.0)
			{
				continue;
			}
			*g_QRCodeContent=detector->decode(*g_SlidingWindow, regionPoints);
			if(!g_QRCodeContent->empty())
			{
				if(0==g_QRCodeContent->compare(*g_QRCodeContentLastTime))
				{
					return(1);
				}
				else
				{
					g_QRCodeContentLastTime=g_QRCodeContent;
				}
			}
		}
	}
	return(0);
}

int main(int argc, char *argv[])
{
	uint WindowSize=200, threshold_level_id;
	double scale=0.5;

	g_SlidingWindow=new(Mat);

	Mat *NewFrame=new(Mat);
	Mat *GrayImage=new(Mat);
	Mat *ScaledImage=new(Mat);
	//Mat *CorrectedImage=new(Mat);
	Mat *BnWImage=new(Mat);

	QRCodeDetector *QRCDetector=new(QRCodeDetector);

	g_QRCodeContent=new(string);
	g_QRCodeContentLastTime=new(string);

	String QRcodeSequencePath("./qr_photo/png/img_1000.png");

	VideoCapture QRcodeSequence(QRcodeSequencePath, CAP_IMAGES);

	vector<int> img_compression_params;
	img_compression_params.push_back(IMWRITE_PNG_COMPRESSION);
	img_compression_params.push_back(7);

	if(!QRcodeSequence.isOpened())
    {
		fprintf(stderr, "Cannot open the sequence. Exit now.\n");
        return 11;
    }
	string frame_id_with_leading_zeros;
	FILE *detected_codes;
	char cmd_buf[256];

	while(42)
    {
		// Load frame-by-frame
		QRcodeSequence.read(*NewFrame);

		if(NewFrame->empty())
        {
			fprintf(stdout, "Frame is empty. Exit now.\n");
			break;
        }
		frame_id_with_leading_zeros = std::string(4-to_string(g_FrameID).length(), '0') + to_string(g_FrameID);

		//make it grayscale, because the color is not important for the QR code reading process
		cvtColor(*NewFrame, *GrayImage, COLOR_BGR2GRAY);

		//downscale it reduce amount of work
		resize(*GrayImage, *ScaledImage, Size(), scale, scale, INTER_LANCZOS4);
		//imwrite(string("./debug/img_")+frame_id_with_leading_zeros+string("_gray.png"), *ScaledImage, img_compression_params);

		fprintf(stdout, "Frame %04u : ", g_FrameID);
		g_QRCodeContent->clear();
		for(threshold_level_id=0; g_ThresholdLadder[threshold_level_id]; threshold_level_id++)
		{
			//make it pure black-and-white binary image to drop off all unnecessary information
			threshold(*ScaledImage, *BnWImage, g_ThresholdLadder[threshold_level_id], 255, THRESH_BINARY);
			//imwrite(string("./debug/img_")+frame_id_with_leading_zeros+string("_threshold_")+to_string(threshold_level_id)+string(".png"), *BnWImage, img_compression_params);

			//now we can scan the image using the sliding window technique
			if(PerformDetection(BnWImage, QRCDetector, WindowSize))
			{
				while(g_QRCodeContent->back()==' ' || g_QRCodeContent->back()=='\n' || g_QRCodeContent->back()=='\t')
				{
					g_QRCodeContent->pop_back();
				}
				//imwrite(string("./detected_codes/")+frame_id_with_leading_zeros+string("_")+*g_QRCodeContent+string(".png"), *g_SlidingWindow, img_compression_params);

				sprintf(cmd_buf, "mv ./qr_photo/png/img_%04u.png ./qr_photo/decoded/%s.png", g_FrameID, g_QRCodeContent->data());
				if(system(cmd_buf));

				imshow("QR-Code detected", *g_SlidingWindow);
				fprintf(stdout, "%s", g_QRCodeContent->data());
				detected_codes=fopen("decoded.csv", "a");
				fprintf(detected_codes, "%04u;%s\n", g_FrameID, g_QRCodeContent->data());
				fclose(detected_codes);
				g_CodesDetectedTotal++;
				break;
			}
		}
		fprintf(stdout, "\n");

		if(g_QRCodeContent->empty())
		{
			sprintf(cmd_buf, "mv ./qr_photo/png/img_%04u.png ./qr_photo/bad/img_%04u.png", g_FrameID, g_FrameID);
			if(system(cmd_buf));
		}

		g_FrameID++;
    }

	fprintf(stdout, "Total files processed: %i\n", g_FrameID);
	fprintf(stdout, "Total codes decoded: %i\n", g_CodesDetectedTotal);
	destroyAllWindows();

    return(0);
}
