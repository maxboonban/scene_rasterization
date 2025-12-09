#include "imagereader.h"

/**
 * @brief Stores the image specified from the input file in this class's
 * `std::vector<RGBA> m_image`.
 * @param file: file path to an image
 * @return True if successfully loads image, False otherwise.
 */
Image* loadImageFromFile(std::string file) {
    QImage myQImage;

    int width; int height;
    if (!myQImage.load(QString::fromStdString(file))) {
        std::cout<<"Failed to load in image: " << file << std::endl;
        return nullptr;
    }
    myQImage = myQImage.convertToFormat(QImage::Format_RGBX8888);
    width = myQImage.width();
    height = myQImage.height();

    RGBA* data = new RGBA[width*height];
    QByteArray arr = QByteArray::fromRawData((const char*) myQImage.bits(), myQImage.sizeInBytes());

    for (int i = 0; i < arr.size() / 4.f; i++){
        data[i] = RGBA{(std::uint8_t) arr[4*i], (std::uint8_t) arr[4*i+1], (std::uint8_t) arr[4*i+2], (std::uint8_t) arr[4*i+3]};
    }

    Image* myImage = new Image{data, width, height};

    return myImage;
}

void saveImageToFile(std::string file, Image* source) {
    QImage image(source->width, source->height, QImage::Format_RGB32);
    QRgb value;
    for (int r = 0; r < source->height; r++) {
        for (int c = 0; c < source->width; c++) {
            int pix = r * source -> width + c;
            RGBA pixVal = source->data[pix];
            value = qRgb(pixVal.r, pixVal.g, pixVal.b);
            image.setPixel(c, r, value);
        }
    }
    QString filepath = QString::fromStdString(file);
    image.save(filepath, "PNG");
}
