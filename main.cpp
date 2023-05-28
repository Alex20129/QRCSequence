#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
//#include <opencv2/imgcodecs.hpp>
//#include <opencv2/imgproc.hpp>

using namespace std;
using namespace cv;

uint32_t g_CodesDetectedTotal=0, g_FrameID=1000;
string *g_QRCodeContent;
Mat *g_SlidingWindow;

void SimpleAutoContrast(cv::Mat *src, cv::Mat *dst)
{
	cv::Mat channels[3];
	// Split the image into separate channels
	cv::split(*src, channels);
	// Normalize the brightness and increase the contrast of each channel
	cv::equalizeHist(channels[0], channels[0]);
	cv::equalizeHist(channels[1], channels[1]);
	cv::equalizeHist(channels[2], channels[2]);
	// Merge all channels back into a solid image
	cv::merge(channels, 3, *dst);
}

bool PerformDetection(Mat *image, QRCodeDetector *detector, uint32_t window_size=100)
{
	uint32_t wX, wY, step=window_size/4;
	bool detected=false;
	vector<Point2d> regionPoints;
//	uint32_t windowPositionID=0;

//	vector<int> img_compression_params;
//	img_compression_params.push_back(IMWRITE_PNG_COMPRESSION);
//	img_compression_params.push_back(9);

	for(wX=0; wX<image->cols-window_size && !detected; wX+=step)
	{
		for(wY=0; wY<image->rows-window_size && !detected; wY+=step)
		{
			*g_SlidingWindow=(*image)(Range(wY, wY+window_size), Range(wX, wX+window_size));
//			imwrite(string("./debug/")+to_string(g_FrameID)+string("_")+to_string(windowPositionID)+string(".png"), *g_SlidingWindow, img_compression_params);
//			windowPositionID++;
			detected=detector->detect(*g_SlidingWindow, regionPoints);
			if(!detected)
			{
				continue;
			}
			*g_QRCodeContent=detector->decode(*g_SlidingWindow, regionPoints);
			if(g_QRCodeContent->empty())
			{
				detected=false;
			}
		}
	}
	return(detected);
}

int main(int argc, char *argv[])
{
	uint WindowSize=200, threshold_shift;
	double scale_factor=0.5;

	g_SlidingWindow=new(Mat);

	Mat *NewFrame=new(Mat);
	Mat *GrayImage=new(Mat);
	Mat *BnWImage=new(Mat);

	QRCodeDetector *QRCDetector=new(QRCodeDetector);

	g_QRCodeContent=new(string);

	String QRcodeSequencePath("./qr_photo/png/img_1000.png");

	VideoCapture QRcodeSequence(QRcodeSequencePath, CAP_IMAGES);

	vector<int> img_compression_params;
	img_compression_params.push_back(IMWRITE_PNG_COMPRESSION);
	img_compression_params.push_back(9);

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

		if(NewFrame->size().width>3999 || NewFrame->size().height>3999)
		{
			//downscale it reduce amount of work
			resize(*NewFrame, *NewFrame, Size(), scale_factor, scale_factor, INTER_LANCZOS4);
		}

		SimpleAutoContrast(NewFrame, NewFrame);
		imwrite(string("./debug/img_")+frame_id_with_leading_zeros+string("_contrast.png"), *NewFrame, img_compression_params);

		//make it grayscale, because the color is not important for the QR code reading process
		cvtColor(*NewFrame, *GrayImage, COLOR_BGR2GRAY);
		imwrite(string("./debug/img_")+frame_id_with_leading_zeros+string("_gray.png"), *GrayImage, img_compression_params);

		g_QRCodeContent->clear();
		fprintf(stdout, "Frame %04u : ", g_FrameID);
		for(threshold_shift=0; threshold_shift<64; threshold_shift++)
		{
			//make it pure black-and-white binary image to drop off all unnecessary information
//			if(threshold_shift%2)
//			{
//				threshold(*GrayImage, *BnWImage, 127+threshold_shift, 255, THRESH_BINARY);
//			}
//			else
//			{
//				threshold(*GrayImage, *BnWImage, 127-threshold_shift, 255, THRESH_BINARY);
//			}
//			imwrite(string("./debug/img_")+frame_id_with_leading_zeros+string("_threshold_")+to_string(threshold_shift)+string(".png"), *BnWImage, img_compression_params);

			//now we can scan the image using the sliding window technique
			if(PerformDetection(GrayImage, QRCDetector, WindowSize))
			{
				while(g_QRCodeContent->back()==' ' || g_QRCodeContent->back()=='\n' || g_QRCodeContent->back()=='\t')
				{
					g_QRCodeContent->pop_back();
				}

				imwrite(string("./qr_photo/decoded/")+*g_QRCodeContent+string(".png"), *NewFrame, img_compression_params);

				imshow("QR-Code detected", *g_SlidingWindow);
				fprintf(stdout, "%s", g_QRCodeContent->data());
				//save data to file
				detected_codes=fopen("decoded.csv", "a");
				fprintf(detected_codes, "%s\n", g_QRCodeContent->data());
				fclose(detected_codes);

				g_CodesDetectedTotal++;
				break;
			}
		}
		fprintf(stdout, "\n");

		if(g_QRCodeContent->empty())
		{
			imwrite(string("./qr_photo/bad/img_")+frame_id_with_leading_zeros+string(".png"), *NewFrame, img_compression_params);
		}

		sprintf(cmd_buf, "rm -f ./qr_photo/png/img_%04u.png", g_FrameID);
		if(system(cmd_buf));

		g_FrameID++;
    }

	fprintf(stdout, "Total files processed: %i\n", g_FrameID);
	fprintf(stdout, "Total codes decoded: %i\n", g_CodesDetectedTotal);
	destroyAllWindows();

    return(0);
}
