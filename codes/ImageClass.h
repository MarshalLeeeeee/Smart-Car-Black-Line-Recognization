#include <opencv\\highgui.h>

template<class T>    //定义不同图像类型，方便访问图像元素
class Image
{
private:
	IplImage* imgp;
public:
	Image(IplImage* img = 0) { imgp = img; }
	~Image(){ imgp = 0; }
	inline T* operator[](const int rowIndx)
	{
		return ((T *)(imgp->imageData + rowIndx*imgp->widthStep));
	}
};


typedef struct{
	unsigned char b, g, r;
} RgbPixel;

typedef struct{
	float b, g, r;
} RgbPixelFloat;

typedef struct{
	unsigned char h, s, v;
} HsvPixel;

typedef struct{
	float h, s, v;
} HsvPixelFloat;


typedef Image<RgbPixelFloat>  RgbFloatImage;
typedef Image<RgbPixel>  RgbImage;
typedef Image<HsvPixelFloat>  HsvFloatImage;
typedef Image<HsvPixel>  HsvImage;
typedef Image<unsigned char>  BwImage;