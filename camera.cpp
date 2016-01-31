#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <stdio.h>
#include <stdlib.h>

int n_boards = 0;
const int board_dt = 20;
int board_w;
int board_h;

void get_calib(CvCapture* capture);
void distort_image(CvCapture* capture);
void get_bird_eye(CvCapture* capture);
void bird_eye();

int main(int argc, char* argv[]) {
	if(argc != 5) {
		printf("ERROR: Wrong number of input parameters\n");
		return -1;
	}
	board_w = atoi(argv[1]);
	board_h = atoi(argv[2]);
	n_boards = atoi(argv[3]);
	CvCapture* capture = cvCreateCameraCapture(0);
	assert(capture);
	
	//get_calib(capture);
	//distort_image(capture);
	//get_bird_eye(capture);
	bird_eye();
	return 0;
}

void bird_eye() {
	int board_n = board_w * board_h;
	CvSize board_sz = cvSize(board_w, board_h);
	CvMat *intrinsic = (CvMat*) cvLoad("Intrinsics.xml");
	CvMat *distortion = (CvMat*) cvLoad("Distortion.xml");

	IplImage* image = cvLoadImage("./Resource/bird-eye.jpg", 1);
	IplImage* gray_image = cvCreateImage(cvGetSize(image), 8, 1);
	cvCvtColor(image, gray_image, CV_BGR2GRAY);

	IplImage* mapx = cvCreateImage(cvGetSize(image), IPL_DEPTH_32F, 1);
	IplImage* mapy = cvCreateImage(cvGetSize(image), IPL_DEPTH_32F, 1);
	
	cvInitUndistortMap(
			intrinsic, 
			distortion, 
			mapx, 
			mapy
	);
	
	IplImage* t = cvCloneImage(image);

	cvRemap(t, image, mapx, mapy);
	
	cvNamedWindow("Chessboard");
	cvShowImage("Chessboard", image);
	int c = cvWaitKey(-1);
	CvPoint2D32f* corners = new CvPoint2D32f[board_n];
	int corner_count = 0;
	
	int found = cvFindChessboardCorners(
			image, 
			board_sz, 
			corners, 
			&corner_count, 
			CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS
	);
	
	if(!found){
		printf("couldn't aquire chessboard!\n");
		return;
	}
	
	cvFindCornerSubPix(
			gray_image, 
			corners, 
			corner_count, 
			cvSize(11, 11), 
			cvSize(-1, -1), 
			cvTermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 30, 0.1)
	);

	CvPoint2D32f objPts[4], imgPts[4];
	objPts[0].x = 0;			objPts[0].y = 0;
	objPts[1].x = board_w - 1;	objPts[1].y = 0;
	objPts[2].x = 0;			objPts[2].y = board_h - 1;
	objPts[3].x = board_w - 1;	objPts[3].y = board_h - 1;
	imgPts[0]   = corners[0];
	imgPts[1]	= corners[board_w - 1];
	imgPts[2]	= corners[(board_h - 1) * board_w];
	imgPts[3]	= corners[(board_h - 1) * board_w + board_w - 1];

	cvCircle(image, cvPointFrom32f(imgPts[0]), 9, CV_RGB(0, 0, 255), 3);
	cvCircle(image, cvPointFrom32f(imgPts[1]), 9, CV_RGB(0, 255, 0), 3);
	cvCircle(image, cvPointFrom32f(imgPts[2]), 9, CV_RGB(255, 0, 0), 3);
	cvCircle(image, cvPointFrom32f(imgPts[3]), 9, CV_RGB(255, 255, 0), 3);

	cvDrawChessboardCorners(
		image,
		board_sz,
		corners,
		corner_count,
		found
	);

	cvShowImage("Chessboard", image);

	CvMat *H = cvCreateMat(3, 3, CV_32F);
	cvGetPerspectiveTransform(objPts, imgPts, H);

	float z = 25;
	int key = 0;
	IplImage * birds_image = cvCloneImage(image);
	cvNamedWindow("Birds_Eye");

	while(key != 27) {
		CV_MAT_ELEM(*H, float, 2, 2) = z;

		cvWarpPerspective(
			image,
			birds_image,
			H,
			CV_INTER_LINEAR| CV_WARP_INVERSE_MAP | CV_WARP_FILL_OUTLIERS
		);

		cvShowImage("Birds_Eye", birds_image);

		key = cvWaitKey();
		if(key == 'u') z += 0.5;
		if(key == 'd') z -= 0.5;
	}

	cvSave("H.xml", H);
}

void get_bird_eye(CvCapture* capture) {
	printf("haha\n");
	//get bird_eye picture
	cvNamedWindow("Get_birdeye");
	
	CvMat *intrinsic = (CvMat*) cvLoad("Intrinsics.xml");
	CvMat *distortion = (CvMat*) cvLoad("Distortion.xml");
	
	IplImage *image = cvQueryFrame(capture);
	IplImage *gray_image = cvCreateImage(cvGetSize(image), 8, 1);
	
	int board_n = board_w * board_h;
	
	IplImage* mapx = cvCreateImage(cvGetSize(image), IPL_DEPTH_32F, 1);
	IplImage* mapy = cvCreateImage(cvGetSize(image), IPL_DEPTH_32F, 1);
	
	cvInitUndistortMap(
			intrinsic, 
			distortion, 
			mapx, 
			mapy
	);
	
	CvSize board_sz = cvSize(board_w, board_h);
	CvPoint2D32f* corners = new CvPoint2D32f[board_n];
	
	int frame = 0;
	int corner_count;
	bool catch_bird = false;


	while(!catch_bird) {
		IplImage *t = cvCloneImage(image);
		if(frame++ % board_dt == 0) {
			IplImage* t = cvCloneImage(image);
			cvRemap(t, image, mapx, mapy);
			
			int found = cvFindChessboardCorners(
					image, 
					board_sz, 
					corners, 
					&corner_count,
					CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS
			);

			cvCvtColor(
					image, 
					gray_image, 
					CV_RGB2GRAY
			);
			
			cvFindCornerSubPix(
					gray_image, 
					corners, 
					corner_count, 
					cvSize(11,11), 
					cvSize(-1, -1), 
					cvTermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1)
			);
			
			cvDrawChessboardCorners(
					image, 
					board_sz, 
					corners, 
					corner_count, 
					found
			);
			
			cvShowImage("Get_birdeye", image);
			//get a good board, add to data
			if(corner_count == board_n) {
				catch_bird = true;
				printf("That's it!\n");
			}
		}
		int c;
		if(catch_bird) c = cvWaitKey(-1);
		else c = cvWaitKey(15);
		if(catch_bird && c == 's') {
			cvSaveImage("./Resource/bird-eye.jpg", t, 0);
			printf("save at ./Resource/bird-eye.jpg\n");
		}
		else catch_bird = false;
		if(c == 'p') {
			c = 0;
			while(c!='p' && c!= 27){
				c = cvWaitKey(250);
			}
		}
		image = cvQueryFrame(capture);
	}
}

void get_calib(CvCapture* capture) {
	int board_n = board_w * board_h;
	CvSize board_sz = cvSize(board_w, board_h);
	cvNamedWindow("Calibration");
	CvMat* image_points = cvCreateMat(n_boards * board_n, 2, CV_32FC1);
	CvMat* object_points = cvCreateMat(n_boards * board_n, 3, CV_32FC1);
	CvMat* point_counts = cvCreateMat(n_boards, 1, CV_32FC1);
	CvMat* intrinsic_matrix = cvCreateMat(3, 3, CV_32FC1);
	CvMat* distortion_coeffs = cvCreateMat(5, 1, CV_32FC1);

	CvPoint2D32f* corners = new CvPoint2D32f[board_n];
	int corner_count;
	int successes = 0;
	int step, frame = 0;
	IplImage *image = cvQueryFrame(capture);
	IplImage *gray_image = cvCreateImage(cvGetSize(image), 8, 1);


	while(successes < n_boards) {
		if(frame++ % board_dt == 0) {
			int found = cvFindChessboardCorners(
					image, board_sz, corners, &corner_count,
					CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS);
			cvCvtColor(image, gray_image, CV_RGB2GRAY);
			cvFindCornerSubPix(gray_image, corners, corner_count, 
					cvSize(11,11), cvSize(-1, -1), 
					cvTermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
			cvDrawChessboardCorners(image, board_sz, corners, corner_count, found);
			cvShowImage("Calibration", image);
			//get a good board, add to data
			if(corner_count == board_n) {
				printf("here\n");
				step = successes * board_n;
				for(int i = step, j = 0; j < board_n; ++i, ++j) {
					CV_MAT_ELEM(*image_points, float, i, 0) = corners[j].x;
					CV_MAT_ELEM(*image_points, float, i, 1) = corners[j].y;
					CV_MAT_ELEM(*object_points, float, i, 0) = j / board_w;
					CV_MAT_ELEM(*object_points, float, i, 1) = j % board_w;
					CV_MAT_ELEM(*object_points, float, i, 2) = 0.0f;
				}
				
				CV_MAT_ELEM(*point_counts, int, successes, 0) = board_n;
				successes++;
				printf("Nice shot!\n");
				int k = cvWaitKey(-1);
				while(k != 'n') k = cvWaitKey(-1);
			}
		}
		//Handle pause/unpause
		int c = cvWaitKey(15);
		if(c == 'p') {
			c = 0;
			while(c!='p' && c!= 27){
				c = cvWaitKey(250);
			}
		}

		if(c == 27) return;
		image = cvQueryFrame(capture);
	}

	printf("Finish samples!");

	CvMat *object_points2 = cvCreateMat(successes * board_n, 3, CV_32FC1);
	CvMat *image_points2 = cvCreateMat(successes * board_n, 2, CV_32FC1);
	CvMat *point_counts2 = cvCreateMat(successes, 1, CV_32SC1);

	for(int i = 0; i < successes * board_n; ++i) {
		CV_MAT_ELEM(*image_points2, float, i, 0) = CV_MAT_ELEM(*image_points, float, i, 0);
		CV_MAT_ELEM(*image_points2, float, i, 1) = CV_MAT_ELEM(*image_points, float, i, 1);
		CV_MAT_ELEM(*object_points2, float, i, 0) = CV_MAT_ELEM(*object_points, float, i, 0);
		CV_MAT_ELEM(*object_points2, float, i, 1) = CV_MAT_ELEM(*object_points, float, i, 1);
		CV_MAT_ELEM(*object_points2, float, i, 2) = CV_MAT_ELEM(*object_points, float, i, 2);
	}

	for(int i = 0; i < successes; ++i) {
		CV_MAT_ELEM(*point_counts2, int, i, 0) = CV_MAT_ELEM(*point_counts, int, i, 0);
	}

	cvReleaseMat(&object_points);
	cvReleaseMat(&image_points);
	cvReleaseMat(&point_counts);

	CV_MAT_ELEM(*intrinsic_matrix, float, 0, 0) = 1.0f;
	CV_MAT_ELEM(*intrinsic_matrix, float, 1, 1) = 1.0f;

	printf("Ready for calib\n");

	cvCalibrateCamera2(
			object_points2, image_points2, 
			point_counts2, cvGetSize(image), 
			intrinsic_matrix, distortion_coeffs, 
			NULL, NULL, 0
	);

	printf("Ready for save\n");
	cvSave("Intrinsics.xml", intrinsic_matrix);
	cvSave("Distortion.xml", distortion_coeffs);
}


void distort_image(CvCapture* capture){
	IplImage *image = cvQueryFrame(capture);
	CvMat *intrinsic = (CvMat*) cvLoad("Intrinsics.xml");
	CvMat *distortion = (CvMat*) cvLoad("Distortion.xml");

	IplImage* mapx = cvCreateImage(cvGetSize(image), IPL_DEPTH_32F, 1);
	IplImage* mapy = cvCreateImage(cvGetSize(image), IPL_DEPTH_32F, 1);

	printf("Ready for undistort\n");
	cvInitUndistortMap(intrinsic, distortion, mapx, mapy);

	printf("After init\n");
	cvNamedWindow("Undistort");
	while(image) {
		IplImage* t = cvCloneImage(image);
		cvShowImage("Calibration", image);
		cvRemap(t, image, mapx, mapy);
		cvReleaseImage(&t);
		cvShowImage("Undistort", image);

		int c = cvWaitKey(15);
		if(c=='p') {
			c = 0;
			while(c!='p' && c!=27) {
				c = cvWaitKey(250);
			}
		}

		if(c == 27) break;
		image = cvQueryFrame(capture);
	}
}
