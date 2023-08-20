
#include "stdafx.h"

//#include <vector>
//#include <atltypes.h>
//#include <thread>
//#include <opencv2\opencv.hpp>

#include "framework.h"
#include "StreamVierwer.h"
#include "yolov5_engine.h"
#include "logfile.h"

#define DRAW_ALL 0
#define DRAW_UPLEFT 1
#define DRAW_UPRIGHT 2
#define DRAW_DOWNLEFT 3
#define DRAW_DOWNRIGHT 4

//���t���b�V�����[�g�Ɖ�ʐؑ֎���
#define INIT_FPS_MSEC 33 //1500
#define INIT_INTERVAL_TIME_SEC 30 //8

//drawing_position 0:�S�� 1:���� 2:�E�� 3:�E�� 4:����
//volatile int cvw_status = 0;
volatile bool cvw_stop = true;
volatile bool cvw_set_stop = 0;
volatile int display_time_seconds = INIT_INTERVAL_TIME_SEC;// 8;
volatile int sleep = INIT_FPS_MSEC;// 100;
extern bool _ai_text_output=false;
volatile bool _next_source = false; //���̉�ʂɍs��
volatile bool _drawing_flag = false; //AI�̉��Z���d�Ȃ�̂�h��
std::vector<std::vector<std::string>> cam_urls; //[0]�ɂ̓^�C�g���A[1]�ɂ�url�������Ă���

int DrawPicToHDC(cv::Mat cvImg, HWND hWnd, HDC hDC, bool bMaintainAspectRatio, int drawing_position); //bMaintainAspectRatio=true

int set_display_time_seconds(int _display_time_seconds)
{
	display_time_seconds = _display_time_seconds;
	return display_time_seconds;
}

int set_frame_between_time(int _sleep)
{
	sleep = _sleep;
	return sleep;
}

bool get_cvw_stop()
{
	return cvw_stop;
}

bool svw_wait_stop()
{
	while (!get_cvw_stop()) //����������doevents�Ŗ���
	{
		Sleep(100);
	}
	return get_cvw_stop();
}

int set_score_threshold(float _st, void* __pt_yod)
{
	//�����I�ɃL���X�g 
	YoloObjectDetection* _pt_yod = (YoloObjectDetection*)__pt_yod;
	if (_pt_yod == nullptr)
		return -1;
	else
		_pt_yod->_YP._score_threshold = _st;
	return 0;
}
int set_nms_threshold(float _nms, void* __pt_yod)
{
	//�����I�ɃL���X�g 
	YoloObjectDetection* _pt_yod = (YoloObjectDetection*)__pt_yod;
	if (_pt_yod == nullptr)
		return -1;
	else
		_pt_yod->_YP._nms_threshold = _nms;
	return 0;
}

int set_conf_threshold(float _conf, void* __pt_yod)
{
	//�����I�ɃL���X�g 
	YoloObjectDetection* _pt_yod = (YoloObjectDetection*)__pt_yod;
	if (_pt_yod == nullptr)
		return -1;
	else
		_pt_yod->_YP._confidence_thresgold = _conf;
	return 0;
}


void DoEvents(int n)
{
	MSG msg;
	for (int i = 0; i < n; i++)
	{
		while (::PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

//true�ŃX�g�b�v
bool set_cvw_stop(bool _cvw_set_stop)
{
	cvw_set_stop = _cvw_set_stop;
	return cvw_set_stop;
}

std::wstring stringToWstring(const std::string& s) 
{
	int requiredSize = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
	std::wstring result(requiredSize, 0);
	//MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &result[0], requiredSize);
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &result[0], requiredSize);
	return result;
}

//HDC���₽��Q�b�g����Ƃ��������Ȃ�̂Ő������Ȃ�
int AllPaintBlack(HWND hWnd, HDC hDC)
{
	//��ʂ������h��Ԃ�
	CRect rect;
	GetClientRect(hWnd, rect);
	HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
	FillRect(hDC, &rect, hBrush);
	DeleteObject(hBrush);

	return 0;
}

int MyDrawText(HWND hWnd, HDC hDC, std::string& _text,int x0=0, int y0=0, bool _add = false)
{
	//HDC hDC = ::GetDC(hWnd);
	RECT rect;
	GetClientRect(hWnd, &rect);
	rect.left = x0;
	rect.top = y0;

	std::string _text_tmp;

	//�s�����J�E���g
	int lineCount = std::count(_text.begin(), _text.end(), '\n') + 1;
	int n = (rect.bottom - rect.top) / 8 /2;
	//if (lineCount * 8 > (rect.bottom - rect.top))
	//��ʂ��͂ݏo��悤��������A��������폜
	if (lineCount > n)
	{
		size_t pos = 0;
		for (int i = 0; i < n; ++i) {
			pos = _text.find('\n', pos);
			if (pos == std::string::npos) {
				break;
			}
			++pos;
		}
		// �ŏ���n�s���폜
		_text_tmp = _text.substr(pos);

		AllPaintBlack(hWnd, hDC);
	}
	else
		_text_tmp = _text;

	//���Ǒ������Ȃ�_text_tmp�v��Ȃ������ˁB
	_text = _text_tmp;

	HFONT hFont = CreateFont(
		-MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72),    // �t�H���g�̍����i�s�N�Z���P�ʁj
		0,                                                  // ���ϕ�����
		0,                                                  // ��]�p�x
		0,                                                  // �x�[�X���C����x���Ƃ̊p�x
		FW_BOLD,                                             // �t�H���g�̏d���i�����j
		FALSE,                                               // �C�^���b�N
		FALSE,                                               // ����
		FALSE,                                               // ��������
		DEFAULT_CHARSET,                                     // �����Z�b�g
		OUT_DEFAULT_PRECIS,                                  // �o�͐��x
		CLIP_DEFAULT_PRECIS,                                 // �N���b�s���O���x
		DEFAULT_QUALITY,                                     // �o�͕i��
		DEFAULT_PITCH | FF_SWISS,                            // �s�b�`�ƃt�@�~���[
		_T("Meiryo UI")                                      // �t�H���g��
	);
	HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

	SetBkMode(hDC, TRANSPARENT); // �e�L�X�g�̔w�i�𓧖��ɂ��܂��B
	SetTextColor(hDC, RGB(255, 255, 255)); // �e�L�X�g�̐F�𔒂ɂ��܂��B

	std::wstring _ostW = stringToWstring(_text_tmp);
	DrawText(hDC, _ostW.c_str(), -1, &rect, DT_LEFT | DT_TOP | DT_WORDBREAK);

	// �g�p���I�������A�I�u�W�F�N�g��I���������A�폜���܂�
	SelectObject(hDC, hOldFont);
	DeleteObject(hFont);

	return 0;
}

/*
void DoEvents()
{
	// ���b�Z�[�W���[�v�����s���ăC�x���g������
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
*/

void wait_drawing()
{
	while (_drawing_flag)
	{
		DoEvents();
	}
}

//������������������������������������������������������������������������������������������������������������������
int DrawCV2Window(
	HWND hWnd,		//Window�n���h��
	void* __pt_yod,	//yolov5�����N���X�̃C���X�^���X�ւ̃|�C���^ null���Ɩ���
	std::vector<std::vector<std::string>> urls,
	int draw_area,
	std::ofstream* pAICSV
)
{
	bool _cam_on = false;
	cvw_stop = 0;
	cv::VideoCapture capture;
	cv::Mat image0;

	std::string _ai_out_put_string;
	cv::Mat resized_image;
	cv::Mat post_proccesed_image;


	//�����I�ɃL���X�g 
	YoloObjectDetection* _pt_yod = (YoloObjectDetection*)__pt_yod;

	int current_url_index = 0;
	//int display_time_seconds = 3; // Change display time in seconds

	HDC hDC = ::GetDC(hWnd);

	while (true)
	{
		auto start = std::chrono::steady_clock::now();
		while (_cam_on == false)
		{
			//�E�B���h�E�L���v�V�����̕�����
			std::string _ipurl = urls[current_url_index][1];
			//std::string _title = urls[current_url_index][0];
			std::ostringstream window_caption;
			window_caption << urls[current_url_index][0] << " " << ((YoloObjectDetection*)__pt_yod)->_YP._onnx_file_name;
			//std::wstring newTitleW = stringToWstring(urls[current_url_index][0]);
			std::wstring newTitleW = stringToWstring(window_caption.str().c_str());
			SetWindowTextW(hWnd, newTitleW.c_str());

			int num_usb;
			std::istringstream iss(_ipurl);
			if (iss >> num_usb)
			{
				capture.open(num_usb);
			}
			else
			{
				capture.open(_ipurl);
			}

			//��ʂ������h��Ԃ�
			AllPaintBlack(hWnd, hDC);
			//�e�L�X�g���N���A
			_ai_out_put_string = "";
			DoEvents();
			if (capture.isOpened())
			{
				_cam_on = true;
			}
			else
			{
				std::cerr << "Failed to open the stream: " << _ipurl << std::endl;
				_cam_on = false;
			}
		}

		while (true)
		{
			std::string _ost;
			//std::string _dummy("test ");
			capture >> image0;

			//AI����
			float NMS_INIT = _pt_yod->_YP._nms_threshold;
			float CNF_INIT = _pt_yod->_YP._confidence_thresgold;
			float SCR_INIT = _pt_yod->_YP._score_threshold;

			//CSV�o�͂̐���
			std::ostringstream _header;

			//cv::Mat resized_image;
			if (image0.empty() || image0.rows == 0 || image0.cols == 0)
			{
				cvw_set_stop = true;
				goto GOTO_LOOPEND;
			}
			resize(
				image0,
				resized_image,
				cv::Size(),
				(double)_pt_yod->_YP._input_width / image0.cols,
				(double)_pt_yod->_YP._input_height / image0.rows);

			wait_drawing();
			_drawing_flag = true;

			//AI�̏���
			_pt_yod->_pre_process(resized_image);

			//AI�̌㏈��
			_header <<"AI,\""<< urls[current_url_index][0] <<"\","<< urls[current_url_index][1]<<"," << _TimeStumpStr();
			post_proccesed_image = _pt_yod->_post_process(true, image0.clone(), _header.str().c_str(), _ost);
			_drawing_flag = false;

			//AllPaintBlack(hWnd, hDC); //�����
			//�`��
			DrawPicToHDC(post_proccesed_image, hWnd, hDC, true, draw_area);//�A�X�y�N�g����ێ�����B

			_ai_out_put_string = _ai_out_put_string + "\n" + _ost;
			if (_ai_text_output)
			{
				//ai�̏o�͂��e�L�X�g�ŉ�ʂɃo�[���Əo���B�^�[�~�l�[�^�[�A�̂悤�ɂ͂Ȃ�Ȃ������B
				MyDrawText(hWnd, hDC, _ai_out_put_string, 5, 50);
			}

			//AI�̏o�͂��e�L�X�g�t�@�C���ɕۑ�
			if (pAICSV != nullptr)
			{
				*pAICSV << _ost;
				pAICSV->flush();
			}

			MSG msg;
			if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
			DoEvents();
			Sleep(sleep);

			GOTO_LOOPEND:
			auto now = std::chrono::steady_clock::now();
			//�^�C�}�[
			if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >= display_time_seconds)
				break;
			//���̉��
			if (_next_source)
			{
				_next_source = false;
				break;
			}
			//�`�惋�[�v���o��
			if (cvw_set_stop == true)
				break;
		}
		capture.release();
		_cam_on = false;

		//�J�����؂�ւ����[�v���o��
		if (cvw_set_stop == true)
			break;

		current_url_index = (current_url_index + 1) % (int)urls.size();
	}

	ReleaseDC(hWnd, hDC);
	cvw_stop = true;
	return 0;
}


//������������������������������������������������������������������������������������������������������������������
void AdjustAspectImageSize(const cv::Size& imageSize, const cv::Size& destSize, cv::Size& newSize)
{
	double destAspectRatio = float(destSize.width) / float(destSize.height);
	double imageAspectRatio = float(imageSize.width) / float(imageSize.height);

	if (imageAspectRatio > destAspectRatio)
	{
		// Margins on top/bottom
		newSize.width = destSize.width;
		newSize.height = int(imageSize.height *
			(double(destSize.width) / double(imageSize.width)));
	}
	else
	{
		// Margins on left/right
		newSize.height = destSize.height;
		newSize.width = int(imageSize.width *
			(double(destSize.height) / double(imageSize.height)));
	}
}

//������������������������������������������������������������������������������������������������������������������
//Windows�̃R���g���[���ɏ������ފ֐�
//int DrawPicToHDC(cv::Mat cvImg, HWND nDlgID, int nIDDlgItem, bool bMaintainAspectRatio) //bMaintainAspectRatio=true
int DrawPicToHDC(cv::Mat cvImg, HWND hWnd, HDC hDC, bool bMaintainAspectRatio, int drawing_position) //bMaintainAspectRatio=true
{
	//�������[�v�ɂȂ邱�Ƃ�����̂ŁA�V�X�e���ɏ�����n���B
	//DoEvents();
	if (cvImg.rows == 0 || cvImg.cols == 0) //�摜�̑傫����0�������牽�����Ȃ�
	{
		return 0;
	}
	//�I���W�i���ł�HDC�𐶐����Ă������O����
	//HDC hDC = ::GetDC(hWnd);

	CRect rect;

	//Window�̃T�C�Y
	GetClientRect(hWnd, rect);
	cv::Size winSize(rect.right, rect.bottom);

	//�\������摜�̃T�C�Y
	cv::Size origImageSize(cvImg.cols, cvImg.rows);
	cv::Size imageSize;
	int  offsetX;
	int  offsetY;

	if (!bMaintainAspectRatio)//�A�X�y�N�g��𖳎�
	{
		// Image should be the same size as the control's rectangle
		imageSize = winSize;
	}
	else //�A�X�y�N�g����ێ�
	{
		cv::Size newSize;
		AdjustAspectImageSize(origImageSize, winSize, imageSize);
	}
	if (drawing_position == DRAW_ALL)
	{
		offsetX = (winSize.width - imageSize.width) / 2;
		offsetY = (winSize.height - imageSize.height) / 2;
	}
	//4����
	else if (drawing_position == DRAW_UPLEFT)
	{
		//�`��̌��_
		offsetX = (winSize.width - imageSize.width) / 2;
		offsetY = 0;

		imageSize = imageSize / 2;
	}
	else if (drawing_position == DRAW_UPRIGHT)
	{
		//�`��̌��_
		offsetX = winSize.width / 2;
		offsetY = 0;

		imageSize = imageSize / 2;
	}
	else if (drawing_position == 3)
	{
		//�`��̌��_
		offsetX = (winSize.width - imageSize.width) / 2;
		offsetY = winSize.height / 2;

		imageSize = imageSize / 2;
	}
	else if (drawing_position == 4)
	{
		//�`��̌��_
		offsetX = winSize.width / 2;
		offsetY = winSize.height / 2;

		imageSize = imageSize / 2;
	}


	// Resize the source to the size of the destination image if necessary

	cv::Mat cvImgTmp;
	cv::resize(cvImg, cvImgTmp, imageSize, 0, 0, cv::INTER_AREA);
	//cv::Mat* pt_cvImgTmp;
	//pt_cvImgTmp = new cv::Mat();
	//cv::resize(cvImg, *pt_cvImgTmp, imageSize, 0, 0, cv::INTER_AREA);


	// To handle our Mat object of this width, the source rows must
	// be even multiples of a DWORD in length to be compatible with 
	// SetDIBits().  Calculate what the correct byte width of the 
	// row should be to be compatible with SetDIBits() below.
	int stride = ((((imageSize.width * 24) + 31) & ~31) >> 3);

	// Allocate a buffer for our DIB bits
	uchar* pcDibBits = (uchar*)malloc(imageSize.height * stride);

	if (pcDibBits != NULL)
	{
		// Copy the raw pixel data over to our dibBits buffer.
		// NOTE: Can setup cvImgTmp to add the padding to skip this.
		for (int row = 0; row < cvImgTmp.rows; ++row)
			//for (int row = 0; row < pt_cvImgTmp->rows; ++row)
		{
			// Get pointers to the beginning of the row on both buffers
			uchar* pcSrcPixel = cvImgTmp.ptr<uchar>(row);
			//uchar* pcSrcPixel = pt_cvImgTmp->ptr<uchar>(row);
			uchar* pcDstPixel = pcDibBits + (row * stride);

			// We can just use memcpy
			memcpy(pcDstPixel,
				pcSrcPixel,
				stride);
		}
		// Initialize the BITMAPINFO structure
		BITMAPINFO bitInfo;
		bitInfo.bmiHeader.biBitCount = 24;
		bitInfo.bmiHeader.biWidth = cvImgTmp.cols;
		bitInfo.bmiHeader.biHeight = -cvImgTmp.rows;
		//bitInfo.bmiHeader.biWidth = pt_cvImgTmp->cols;
		//bitInfo.bmiHeader.biHeight = -pt_cvImgTmp->rows;
		bitInfo.bmiHeader.biPlanes = 1;
		bitInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitInfo.bmiHeader.biCompression = BI_RGB;
		bitInfo.bmiHeader.biClrImportant = 0;
		bitInfo.bmiHeader.biClrUsed = 0;
		bitInfo.bmiHeader.biSizeImage = 0;      //winSize.height * winSize.width * * 3;
		bitInfo.bmiHeader.biXPelsPerMeter = 0;
		bitInfo.bmiHeader.biYPelsPerMeter = 0;

		// Add header and OPENCV image's data to the HDC
		StretchDIBits(hDC,
			offsetX,
			offsetY,
			cvImgTmp.cols,
			cvImgTmp.rows,
			//pt_cvImgTmp->cols,
			//pt_cvImgTmp->rows,
			0,
			0,
			cvImgTmp.cols,
			cvImgTmp.rows,
			//pt_cvImgTmp->cols,
			//pt_cvImgTmp->rows,
			pcDibBits,
			&bitInfo,
			DIB_RGB_COLORS,
			SRCCOPY);

		free(pcDibBits);
	}
	//ReleaseDC(hWnd, hDC);
	//delete pt_cvImgTmp;
	return 0;
}

